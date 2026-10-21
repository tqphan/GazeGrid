#include <QtTest>
#include <QTemporaryDir>
#include "Controller.h"
#include "AppPaths.h"
#include "ProfileStore.h"
#include "GazeClock.h"
#include "KeyboardPack.h"
#include "KeyboardLayout.h"
#include <QDir>
#include <QStandardPaths>
#include <QFile>
#include <QJsonDocument>
#include <QGuiApplication>
#include <QScreen>
class ControllerTests:public QObject {
 Q_OBJECT
private slots:
 void repeatRatePersistsAndResets(){
  {Controller c;c.setProperty("repeatIntervalMs",150);QVERIFY(c.savePreferences());}
  {Controller c;QCOMPARE(c.property("repeatIntervalMs").toInt(),150);c.resetPreferences();QCOMPARE(c.property("repeatIntervalMs").toInt(),750);}
 }

 void repeatPreferencesAreIndependent(){
  const char *names[]={"repeatLettersOnHover","repeatBackspaceOnHover","repeatArrowsOnHover"};
  for(int selected=0;selected<3;++selected){
   {Controller c;c.resetPreferences();c.setProperty(names[selected],true);QVERIFY(c.savePreferences());}
   {Controller c;for(int i=0;i<3;++i)QCOMPARE(c.property(names[i]).toBool(),i==selected);c.resetPreferences();for(auto n:names)QVERIFY(!c.property(n).toBool());}
  }
 }
 void repeatKeyCategories(){
  const char *options[]={"repeatLettersOnHover","repeatBackspaceOnHover","repeatArrowsOnHover"};
  const QStringList values{"a","BackOne","Left","Right","Up","Down","1"," ","Return"};
  for(int selected=0;selected<3;++selected){
   for(int k=0;k<values.size();++k){
    Controller c;c.configureInputMethod([](QString){return true;},[](QString,bool){return true;});c.inputContextChanged(true,false);
    c.setProperty(options[selected],true);
    const bool text=k==0||k==6||k==7;
    const QVariantList commands{QVariantMap{{"tag",text?"Text":"Action"},{"text",values[k]}}};
    const QVariantList keys{QVariantMap{{"commands",commands}}};
    c.setProperty("layout",QVariantMap{{"keys",keys}});
    c.clickCommand("key:0");QTRY_VERIFY(!c.executingCommands());
    QCOMPARE(c.hover("key:0"),0.0);QTest::qWait(30);
    const bool expected=(selected==0&&k==0)||(selected==1&&k==1)||(selected==2&&k>=2&&k<=5);
    QCOMPARE(c.hover("key:0")>0,expected);
   }
  }
 }

 void modifierCyclingPreference(){
  {Controller c;QVERIFY(!c.property("cycleModifiersOnHover").toBool());c.setProperty("cycleModifiersOnHover",true);QVERIFY(c.savePreferences());}
  {Controller c;QVERIFY(c.property("cycleModifiersOnHover").toBool());c.resetPreferences();QVERIFY(!c.property("cycleModifiersOnHover").toBool());}
 }

 void busyActionDoesNotRearmHoveredKey(){
  Controller c;c.configureInputMethod([](QString){return true;},[](QString,bool){return true;});c.inputContextChanged(true,false);
  c.setProperty("dwellMs",250);c.setProperty("cycleModifiersOnHover",true);
  // Keep the runner busy across multiple hover ticks without external input.
  const QVariantList commands{QVariantMap{{"tag","Wait"},{"text","150"}}};
  c.setProperty("layout",QVariantMap{{"keys",QVariantList{QVariantMap{{"commands",commands}}}}});
  c.clickCommand("key:0");QVERIFY(c.executingCommands());
  for(int i=0;i<24;++i){QCOMPARE(c.hover("key:0"),0.0);QTest::qWait(25);}
  QVERIFY(!c.executingCommands());
  c.hover("");QCOMPARE(c.hover("key:0"),0.0);QTest::qWait(50);QVERIFY(c.hover("key:0")>0);
 }

 void emptyPredictionsDoNotDwell(){
  Controller c;c.setProperty("suggestions",QStringList{"word",""," "});
  QCOMPARE(c.hover("suggest:0"),0.0);QTest::qWait(50);QVERIFY(c.hover("suggest:0")>0);
  for(const auto &key:{"suggest:1","suggest:2","suggest:3","suggest:-1"}){
   QCOMPARE(c.hover(key),0.0);QTest::qWait(50);QCOMPARE(c.hover(key),0.0);
  }
  c.setProperty("suggestions",QStringList{});QCOMPARE(c.hover("suggest:0"),0.0);
 }

 void hideKeyboardMouseActionLifecycle(){
  Controller c;c.configureInputMethod([](QString){return true;},[](QString,bool){return true;});c.inputContextChanged(true,false);
  c.setProperty("hideKeyboardDuringMouseActions",true);QVERIFY(c.savePreferences());
  {Controller restored;QVERIFY(restored.property("hideKeyboardDuringMouseActions").toBool());}
  const QVariantList action{QVariantMap{{"tag","Action"},{"text","MouseMoveToLeft"}}};
  QVERIFY(c.executeCommands(action));QTRY_VERIFY(c.keyboardTemporarilyHidden());
  c.clickCommand("CancelTarget");QVERIFY(!c.keyboardTemporarilyHidden());
  QVERIFY(c.executeCommands(action));QTRY_VERIFY(c.keyboardTemporarilyHidden());
  QTRY_VERIFY(!c.keyboardTemporarilyHidden()); // Unavailable portal fails safely and restores the keyboard.
  c.resetPreferences();QVERIFY(!c.property("hideKeyboardDuringMouseActions").toBool());
 }

 void gazeIndicatorPreferences(){
  {Controller c;c.setProperty("gazeIndicatorSize",42);c.setProperty("gazeIndicatorOpacity",65);c.setProperty("gazeIndicatorColor","#ff1234");c.setProperty("gazeIndicatorMorph",true);QVERIFY(c.savePreferences());}
  {Controller c;QCOMPARE(c.property("gazeIndicatorSize").toInt(),42);QCOMPARE(c.property("gazeIndicatorOpacity").toInt(),65);QCOMPARE(c.property("gazeIndicatorColor").toString(),QString("#ff1234"));QVERIFY(c.property("gazeIndicatorMorph").toBool());c.resetPreferences();QCOMPARE(c.property("gazeIndicatorSize").toInt(),18);QCOMPARE(c.property("gazeIndicatorOpacity").toInt(),100);QCOMPARE(c.property("gazeIndicatorColor").toString(),QString("#00bfff"));QVERIFY(!c.property("gazeIndicatorMorph").toBool());}
 }

 void restoreDimensionsPersistAndMigrate(){
  {Controller c;c.setRestoreCorner(1);c.setRestoreWidth(96);c.setRestoreHeight(48);QVERIFY(c.savePreferences());}
  {Controller c;QCOMPARE(c.restoreCorner(),1);QCOMPARE(c.restoreWidth(),96);QCOMPARE(c.restoreHeight(),48);
   c.setRestoreWidth(1);QCOMPARE(c.restoreWidth(),40);QCOMPARE(c.restoreHeight(),48);
   c.setRestoreHeight(999);QCOMPARE(c.restoreHeight(),128);QCOMPARE(c.restoreWidth(),40);
   c.resetPreferences();QCOMPARE(c.restoreWidth(),64);QCOMPARE(c.restoreHeight(),64);}
  auto legacy=ProfileStore::defaults();legacy.remove("restoreWidth");legacy.remove("restoreHeight");legacy["restoreSize"]=96;
  QString error;QVERIFY(ProfileStore(QDir(appConfigDirectory()).filePath("profile.json")).save(legacy,error));
  {Controller c;QCOMPARE(c.restoreWidth(),96);QCOMPARE(c.restoreHeight(),96);c.resetPreferences();}
 }

 void restoreOpacityPersistsAndResets(){
  {Controller controller;controller.setProperty("restoreOpacity",35);QVERIFY(controller.savePreferences());}
  {Controller controller;QCOMPARE(controller.property("restoreOpacity").toInt(),35);controller.resetPreferences();QCOMPARE(controller.property("restoreOpacity").toInt(),100);}
 }
 void gazeSmoothingPipelineAndRecovery(){
  Controller controller;controller.setDisplaySize(1000,1000);controller.setGazeSmoothingLevel(2);controller.setSmoothWhenChangingGazeTarget(false);
  QSignalSpy points(&controller,&Controller::gazePoint);
  auto sample=[&](double x,double y,bool valid=true,qint64 timestamp=0){
   QTest::qWait(1);if(!timestamp)timestamp=GazeClock::now();
   QVERIFY(QMetaObject::invokeMethod(&controller,"receiveGaze",Qt::DirectConnection,Q_ARG(double,x),Q_ARG(double,y),Q_ARG(bool,valid),Q_ARG(qint64,timestamp)));
  };
  sample(.1,.1);QCOMPARE(points.last()[0].toDouble(),.1);
  sample(.11,.105);QVERIFY(qAbs(points.last()[0].toDouble()-.10000074915421219)<1e-12);
  sample(.8,.8,false);QVERIFY(!points.last()[2].toBool());
  sample(.8,.8);QCOMPARE(points.last()[0].toDouble(),.8);
  sample(.9,.9,true,GazeClock::now()-300000);QVERIFY(!points.last()[2].toBool());
  sample(.2,.2);QCOMPARE(points.last()[0].toDouble(),.2);
  sample(1.1,.4);QVERIFY(!points.last()[2].toBool());
  sample(.3,.3);QCOMPARE(points.last()[0].toDouble(),.3);
  QTest::qWait(230);QVERIFY(!points.last()[2].toBool());
  sample(.7,.7);QCOMPARE(points.last()[0].toDouble(),.7);
  controller.setDisplaySize(2000,1000);sample(.4,.4);QCOMPARE(points.last()[0].toDouble(),.4);
  controller.setGazeSmoothingLevel(0);sample(.41,.42);QCOMPARE(points.last()[0].toDouble(),.41);
  controller.setGazeSmoothingLevel(2);sample(.6,.6);QCOMPARE(points.last()[0].toDouble(),.6);
  controller.setSmoothWhenChangingGazeTarget(true);sample(.2,.2);QCOMPARE(points.last()[0].toDouble(),.2);
 }
 void smoothingPreferencesPersistAndReset(){
  {Controller controller;controller.setGazeSmoothingLevel(4);controller.setSmoothWhenChangingGazeTarget(true);QVERIFY(controller.savePreferences());}
  {Controller controller;QCOMPARE(controller.gazeSmoothingLevel(),4);QVERIFY(controller.smoothWhenChangingGazeTarget());
   controller.resetPreferences();QCOMPARE(controller.gazeSmoothingLevel(),1);QVERIFY(!controller.smoothWhenChangingGazeTarget());}
  {Controller controller;QCOMPARE(controller.gazeSmoothingLevel(),1);QVERIFY(!controller.smoothWhenChangingGazeTarget());}
 }

 void unicodeScratchpadBackspaceRemovesWholeGrapheme(){
  Controller controller;controller.configureInputMethod([](QString){return true;},[](QString,bool){return true;});controller.inputContextChanged(true,false);
  QVERIFY(controller.executeCommands({QVariantMap{{"tag","Text"},{"text",QString::fromUtf8("a😀é")}}}));QTRY_VERIFY(!controller.executingCommands());
  const QVariantMap back{{"tag","Action"},{"text","Backspace"}};
  QVERIFY(controller.executeCommands({back}));QTRY_VERIFY(!controller.executingCommands());QCOMPARE(controller.property("scratchpad").toString(),QString::fromUtf8("a😀"));
  QVERIFY(controller.executeCommands({back}));QTRY_VERIFY(!controller.executingCommands());QCOMPARE(controller.property("scratchpad").toString(),QString("a"));
 }
 void nativeContextCancellationAndSensitiveText(){
  Controller controller;QString output;
  controller.configureInputMethod([&](QString text){output+=text;return true;},[](QString,bool){return true;});
  controller.inputContextChanged(true,false);QVERIFY(controller.ready());
  QVERIFY(controller.executeCommands({QVariantMap{{"tag","Text"},{"text","public"}}}));QTRY_VERIFY(!controller.executingCommands());
  QCOMPARE(controller.property("scratchpad").toString(),QString("public"));
  QVERIFY(controller.executeCommands({QVariantMap{{"tag","Wait"},{"text","50"}},QVariantMap{{"tag","Text"},{"text","stale"}}}));
  controller.invalidateInputContext();controller.inputContextChanged(true,true);QTest::qWait(80);QCOMPARE(output,QString("public"));
  QVERIFY(controller.executeCommands({QVariantMap{{"tag","Text"},{"text","private"}}}));QTRY_VERIFY(!controller.executingCommands());
  QCOMPARE(output,QString("publicprivate"));QVERIFY(controller.property("scratchpad").toString().isEmpty());QVERIFY(controller.property("suggestions").toStringList().isEmpty());
  controller.inputContextChanged(false,true);QVERIFY(!controller.ready());
 }
 void nativePanelSwitchesTopAndBottomButRejectsOffsets(){
  Controller controller;controller.configureInputMethod([](QString){return true;},[](QString,bool){return true;});
  controller.openLayout("main-14x1-bottom",false);
  QVERIFY(controller.property("layout").toMap()["keys"].toList().front().toMap()["unavailableReason"].toString().isEmpty());
  controller.clickCommand("key:0");
  QTRY_COMPARE(controller.property("layoutName").toString(),QString("main-14x1-top"));
  controller.clickCommand("key:0");
  QTRY_COMPARE(controller.property("layoutName").toString(),QString("main-14x1-bottom"));
  controller.openLayout("keyboard-alphabet-14x4-top",false);
  QCOMPARE(controller.property("layoutName").toString(),QString("keyboard-alphabet-14x4-top"));
  controller.openLayout("minimized-off-screen-top",false);
  QCOMPARE(controller.property("layoutName").toString(),QString("keyboard-alphabet-14x4-top"));
  QVERIFY(controller.property("actionStatus").toString().contains("standalone mode"));
 }
 void corruptMappingsUseDefaultsAndPreserveSavedFile(){
  const auto path=QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);QVERIFY(QDir().mkpath(path));
  QFile file(QDir(path).filePath("linux-mappings.json"));QVERIFY(file.open(QIODevice::WriteOnly));file.write("{broken");file.close();
  {Controller controller;QVERIFY(controller.property("profileStatus").toString().contains("Using default mappings"));
   QVERIFY(!QJsonDocument::fromJson(controller.mappingJson().toUtf8()).object().isEmpty());}
  QVERIFY(file.open(QIODevice::ReadOnly));QCOMPARE(file.readAll(),QByteArray("{broken"));file.close();QVERIFY(file.remove());
 }
 void visualPreferencesPersistAndReset(){
  {Controller controller;controller.setProperty("keyboardScale",125);controller.setProperty("highContrast",true);controller.setProperty("shortLabelScale",125);controller.setProperty("longLabelScale",150);QVERIFY(controller.savePreferences());}
  {Controller controller;QCOMPARE(controller.keyboardScale(),125);QVERIFY(controller.property("highContrast").toBool());QCOMPARE(controller.property("shortLabelScale").toInt(),125);QCOMPARE(controller.property("longLabelScale").toInt(),150);
   controller.resetPreferences();QCOMPARE(controller.keyboardScale(),100);QVERIFY(!controller.property("highContrast").toBool());QCOMPARE(controller.property("shortLabelScale").toInt(),150);QCOMPARE(controller.property("longLabelScale").toInt(),100);}
 }
 void magnifierToggleAndZoomPreferences(){
  {Controller controller;QVERIFY(!controller.property("magnifierEnabled").toBool());
   QVERIFY(controller.executeCommands({QVariantMap{{"tag","Action"},{"text","MouseMagnifier"}}}));
   QTRY_VERIFY(!controller.executingCommands());QVERIFY(controller.property("magnifierEnabled").toBool());
   QVERIFY(controller.executeCommands({QVariantMap{{"tag","Action"},{"text","MouseMagnifier"}}}));
   QTRY_VERIFY(!controller.executingCommands());QVERIFY(!controller.property("magnifierEnabled").toBool());
   controller.setProperty("zoomDurationMs",2500);controller.setProperty("magnifierZoom",6);controller.setProperty("progressiveZoom",true);QVERIFY(controller.savePreferences());}
  {Controller controller;QCOMPARE(controller.property("zoomDurationMs").toInt(),2500);QCOMPARE(controller.property("magnifierZoom").toInt(),6);QVERIFY(controller.property("progressiveZoom").toBool());
   controller.resetPreferences();QCOMPARE(controller.property("zoomDurationMs").toInt(),1500);QCOMPARE(controller.property("magnifierZoom").toInt(),4);QVERIFY(!controller.property("progressiveZoom").toBool());}
 }
 void magnifierStateAutomaticallyPersists(){
  {Controller controller;controller.resetPreferences();controller.setProperty("magnifierZoom",7);controller.setMagnifierEnabled(true);}
  {Controller controller;QVERIFY(controller.isMagnifierEnabled());QCOMPARE(controller.property("magnifierZoom").toInt(),4);controller.setMagnifierEnabled(false);}
  {Controller controller;QVERIFY(!controller.isMagnifierEnabled());controller.setMagnifierEnabled(true);controller.resetPreferences();QVERIFY(!controller.isMagnifierEnabled());}
  {Controller controller;QVERIFY(!controller.isMagnifierEnabled());}
 }
 void progressiveZoomGrowsAndStopsAtChosenMagnification(){
  Controller controller;controller.inputContextChanged(true,false);
  controller.setProperty("targeting",true);controller.setProperty("capturePending",true);
  controller.setProperty("progressiveZoom",true);controller.setProperty("zoomDurationMs",500);controller.setProperty("magnifierZoom",4);
  QImage frame(QGuiApplication::primaryScreen()->size(),QImage::Format_RGB32);frame.fill(Qt::red);
  QVERIFY(QMetaObject::invokeMethod(&controller,"receiveMagnifierFrame",Qt::DirectConnection,Q_ARG(QImage,frame)));
  QVERIFY(controller.property("zoomAnimating").toBool());
  const auto initial=controller.magnifierImage().size();QVERIFY(initial.width()>frame.width()*.9);
  controller.targetAt(.5,.5);QVERIFY(controller.property("magnifierReady").toBool()); // No click while the image moves.
  QTest::qWait(200);const auto middle=controller.magnifierImage().size();
  QVERIFY(middle.width()<initial.width());QVERIFY(middle.width()>frame.width()/4);
  QTRY_VERIFY(!controller.property("zoomAnimating").toBool());
  QCOMPARE(controller.magnifierImage().size(),QSize(frame.width()/4,frame.height()/4));
  const int revision=controller.property("magnifierRevision").toInt();QTest::qWait(100);
  QCOMPARE(controller.property("magnifierRevision").toInt(),revision);
 }
 void progressiveZoomTracksPointerWithDwellDisabled(){
  Controller controller;controller.inputContextChanged(true,false);
  controller.setProperty("targeting",true);controller.setProperty("capturePending",true);
  controller.setProperty("progressiveZoom",true);controller.setProperty("zoomDurationMs",500);
  controller.setProperty("magnifierZoom",4);controller.setProperty("targetDwellEnabled",false);
  QImage frame(QGuiApplication::primaryScreen()->size(),QImage::Format_RGB32);
  for(int y=0;y<frame.height();++y)for(int x=0;x<frame.width();++x)frame.setPixelColor(x,y,QColor(x*255/frame.width(),y*255/frame.height(),0));
  QVERIFY(QMetaObject::invokeMethod(&controller,"receiveMagnifierFrame",Qt::DirectConnection,Q_ARG(QImage,frame)));
  QCOMPARE(controller.dwellAt(1,1,true),0.);
  QTRY_VERIFY(!controller.property("zoomAnimating").toBool());
  auto result=controller.magnifierImage();
  QVERIFY(result.pixelColor(0,0).red()>150);QVERIFY(result.pixelColor(0,0).green()>150);
  const int revision=controller.property("magnifierRevision").toInt();
  controller.dwellAt(0,0,true);QTest::qWait(60);
  QCOMPARE(controller.property("magnifierRevision").toInt(),revision);
  controller.resetPreferences();
 }
 void progressiveZoomCancellationStopsAnimation(){
  Controller controller;controller.setProperty("targeting",true);controller.setProperty("capturePending",true);
  controller.setProperty("progressiveZoom",true);
  QImage frame(QGuiApplication::primaryScreen()->size(),QImage::Format_RGB32);frame.fill(Qt::blue);
  QVERIFY(QMetaObject::invokeMethod(&controller,"receiveMagnifierFrame",Qt::DirectConnection,Q_ARG(QImage,frame)));
  QVERIFY(controller.property("zoomAnimating").toBool());
  controller.clickCommand("CancelTarget");QVERIFY(!controller.property("zoomAnimating").toBool());
  const int revision=controller.property("magnifierRevision").toInt();QTest::qWait(100);
  QVERIFY(controller.magnifierImage().isNull());QCOMPARE(controller.property("magnifierRevision").toInt(),revision);
 }
 void immediateZoomStillWorks(){
  Controller controller;controller.setProperty("targeting",true);controller.setProperty("capturePending",true);controller.setProperty("progressiveZoom",false);
  QImage frame(QGuiApplication::primaryScreen()->size(),QImage::Format_RGB32);frame.fill(Qt::blue);
  QVERIFY(QMetaObject::invokeMethod(&controller,"receiveMagnifierFrame",Qt::DirectConnection,Q_ARG(QImage,frame)));
  QVERIFY(!controller.property("zoomAnimating").toBool());QCOMPARE(controller.magnifierImage().size(),QSize(frame.width()/4,frame.height()/4));
 }
 void oldLiveZoomChoiceMigratesToProgressiveZoom(){
  ProfileStore store(QDir(appConfigDirectory()).filePath("profile.json"));QString error;
  auto values=ProfileStore::defaults();values.remove("progressiveZoom");values.remove("zoomDurationMs");values["liveZoom"]=true;
  QVERIFY(store.save(values,error,true));
  {Controller controller;QVERIFY(controller.property("progressiveZoom").toBool());
   QCOMPARE(controller.property("zoomDurationMs").toInt(),1500);QVERIFY(controller.savePreferences());}
  QVERIFY(!store.load().values.contains("liveZoom"));
  QVERIFY(store.load().values.value("progressiveZoom").toBool());
  QVERIFY(store.save(ProfileStore::defaults(),error,true));
 }
 void targetDwellSettingsPersistAndReset(){
  {Controller controller;controller.setProperty("targetDwellEnabled",false);controller.setProperty("targetStartDwellMs",500);
   controller.setProperty("targetConfirmDwellMs",1500);controller.setProperty("targetDwellRadius",65);QVERIFY(controller.savePreferences());}
  {Controller controller;QVERIFY(!controller.property("targetDwellEnabled").toBool());QCOMPARE(controller.property("targetStartDwellMs").toInt(),500);
   QCOMPARE(controller.property("targetConfirmDwellMs").toInt(),1500);QCOMPARE(controller.property("targetDwellRadius").toInt(),65);
   controller.resetPreferences();QVERIFY(controller.property("targetDwellEnabled").toBool());QCOMPARE(controller.property("targetStartDwellMs").toInt(),750);
   QCOMPARE(controller.property("targetConfirmDwellMs").toInt(),1000);QCOMPARE(controller.property("targetDwellRadius").toInt(),40);}
 }
 void shortLabelCheckboxPreferencesMigrate(){
  ProfileStore store(QDir(appConfigDirectory()).filePath("profile.json"));QString error;
  for(bool enlarged:{false,true}){
   auto values=ProfileStore::defaults();values.remove("shortLabelScale");values["enlargeShortLabels"]=enlarged;
   QVERIFY(store.save(values,error,true));
   {Controller controller;QCOMPARE(controller.property("shortLabelScale").toInt(),enlarged?150:100);
    QVERIFY(controller.savePreferences());}
   QCOMPARE(store.load().values["shortLabelScale"].toInt(),enlarged?150:100);
   QVERIFY(!store.load().values.contains("enlargeShortLabels"));
  }
  QVERIFY(store.save(ProfileStore::defaults(),error,true));
 }
 void settingsByDwellPreserveActiveInputAndBusyOutput(){
  Controller controller;controller.setProperty("dwellMs",250);QSignalSpy requested(&controller,&Controller::settingsRequested);
  QVERIFY(controller.executeCommands({QVariantMap{{"tag","Wait"},{"text","10000"}}}));
  for(int i=0;i<8&&requested.isEmpty();++i){controller.hover("Settings");QTest::qWait(50);}
  QCOMPARE(requested.count(),1);QVERIFY(!controller.property("paused").toBool());QVERIFY(controller.executingCommands());
  controller.togglePause();controller.clickCommand("Settings");
  QCOMPARE(requested.count(),2);QVERIFY(controller.property("paused").toBool());
 }
 void everyPackNavigationLinkAndBackFlagExecutes(){
  Controller controller;int links=0;
  const auto names=controller.property("layoutNames").toStringList();
  for(const auto &source:names){
   const auto layout=KeyboardLayout::fromXml(KeyboardPack::load(QDir(DEFAULT_PACK_PATH).filePath(source+".xml")));
   for(const auto &key:layout["keys"].toList())for(const auto &entry:key.toMap()["commands"].toList()){
    auto command=entry.toMap();if(command["tag"]!="ChangeKeyboard")continue;
    controller.recover();controller.openLayout(source,false);
    QVERIFY(controller.executeCommands({command}));auto target=command["text"].toString().trimmed();if(target.endsWith(".xml"))target.chop(4);
    QTRY_VERIFY(!controller.executingCommands());
    QTRY_COMPARE(controller.property("layoutName").toString(),target);
    const bool returns=command["attributes"].toMap().value("backreturnshere","true").toString().compare("false",Qt::CaseInsensitive)!=0;
    QVERIFY(controller.executeCommands({QVariantMap{{"tag","Action"},{"text","BackFromKeyboard"}}}));
    QTRY_VERIFY(!controller.executingCommands());
    const auto expected=returns&&source!=target?source:QString("main-14x1-bottom");
    QTRY_COMPARE(controller.property("layoutName").toString(),expected);++links;
   }
  }
  QVERIFY(links>0);
 }
 void keyboardMenuHonorsXmlVisibility(){
  Controller controller;
  QCOMPARE(controller.property("menuLayoutNames").toStringList(),QStringList({"main-14x1-top"}));
  QCOMPARE(controller.property("layoutNames").toStringList().size(),22);
  controller.openLayout("minimized-off-screen-bottom");
  QCOMPARE(controller.property("layoutName").toString(),QString("minimized-off-screen-bottom"));
  controller.recover();QCOMPARE(controller.property("layoutName").toString(),QString("main-14x1-bottom"));
 }
 void pauseByDwellCancelsBusyMacro(){
  Controller controller;controller.setProperty("dwellMs",250);
  QVERIFY(controller.executeCommands({QVariantMap{{"tag","Wait"},{"text","10000"}}}));
  for(int i=0;i<8&&!controller.property("paused").toBool();++i){controller.hover("Pause");QTest::qWait(50);}
  QVERIFY(controller.property("paused").toBool());
  QVERIFY(controller.executeCommands({})); // Previous macro no longer owns the runner.
 }
 void sleepKeyTogglesByClick(){
  Controller controller;
  for(const auto &name:{"main-14x1-bottom","main-14x1-top"}){
   controller.recover();controller.openLayout(name,false);
   const auto keys=controller.property("layout").toMap()["keys"].toList();
   QString sleep;
   for(int i=0;i<keys.size();++i){
    const auto commands=keys[i].toMap()["commands"].toList();
    if(commands.size()==1&&commands.front().toMap()["text"]=="Sleep")sleep="key:"+QString::number(i);
   }
   QVERIFY(!sleep.isEmpty());
   controller.clickCommand(sleep);QVERIFY(controller.property("paused").toBool());
   controller.clickCommand("key:0");QCOMPARE(controller.property("layoutName").toString(),QString(name));
   controller.clickCommand(sleep);QVERIFY(!controller.property("paused").toBool());
  }
 }
 void sleepKeyDwellResumesOnlyAfterLookingAway(){
  Controller controller;controller.recover();controller.setProperty("dwellMs",250);
  const auto keys=controller.property("layout").toMap()["keys"].toList();
  QString sleep;
  for(int i=0;i<keys.size();++i){
   const auto commands=keys[i].toMap()["commands"].toList();
   if(commands.size()==1&&commands.front().toMap()["text"]=="Sleep")sleep="key:"+QString::number(i);
  }
  QVERIFY(!sleep.isEmpty());
  QVERIFY(controller.executeCommands({QVariantMap{{"tag","Wait"},{"text","10000"}}}));
  for(int i=0;i<9;++i){controller.hover(sleep);QTest::qWait(50);}
  QVERIFY(controller.property("paused").toBool());QVERIFY(!controller.executingCommands());
  for(int i=0;i<9;++i){controller.hover(sleep);QTest::qWait(50);}
  QVERIFY(controller.property("paused").toBool());
  controller.hover("");
  for(int i=0;i<9;++i){controller.hover(sleep);QTest::qWait(50);}
  QVERIFY(!controller.property("paused").toBool());
 }
 void cancelTargetByDwellCancelsBusyMacro(){
  Controller controller;controller.setProperty("dwellMs",250);
  QVERIFY(controller.executeCommands({QVariantMap{{"tag","Wait"},{"text","10000"}}}));
  for(int i=0;i<8;++i){controller.hover("CancelTarget");QTest::qWait(50);}
  QVERIFY(controller.executeCommands({}));
 }
 void displayChangePausesAndCancelsOutput(){
  Controller controller;
  QVERIFY(controller.executeCommands({QVariantMap{{"tag","Wait"},{"text","10000"}}}));
  controller.displayChanged();QVERIFY(controller.property("paused").toBool());
  QVERIFY(controller.property("actionStatus").toString().contains("Display configuration changed"));
  QVERIFY(controller.executeCommands({}));
 }
 void resetStaysPausedAndPersists(){
  {Controller controller;controller.setProperty("dwellMs",1250);controller.setProperty("literal",true);
   QVERIFY(controller.savePreferences());}
  {Controller controller;QCOMPARE(controller.property("dwellMs").toInt(),1250);QVERIFY(controller.property("literal").toBool());
   controller.resetPreferences();QVERIFY(controller.property("paused").toBool());QCOMPARE(controller.property("dwellMs").toInt(),750);}
  {Controller controller;QCOMPARE(controller.property("dwellMs").toInt(),750);QVERIFY(!controller.property("literal").toBool());}
 }
};
int main(int argc,char **argv){
 QTemporaryDir dir;if(!dir.isValid())return 1;
 qputenv("XDG_CONFIG_HOME",dir.path().toUtf8());
 qputenv("QT_QPA_PLATFORM","offscreen");
 QGuiApplication app(argc,argv);app.setOrganizationName("OptiKeyTests");app.setApplicationName("Controller");
 ControllerTests tests;return QTest::qExec(&tests,argc,argv);
}
#include "ControllerTests.moc"
