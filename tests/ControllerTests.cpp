#include <QtTest>
#include <QTemporaryDir>
#include "Controller.h"
#include "KeyboardPack.h"
#include "KeyboardLayout.h"
#include <QDir>
#include <QStandardPaths>
#include <QFile>
#include <QJsonDocument>
class ControllerTests:public QObject {
 Q_OBJECT
private slots:
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
 void nativePanelRejectsUnsupportedPlacementWithoutChangingLayout(){
  Controller controller;controller.configureInputMethod([](QString){return true;},[](QString,bool){return true;});
  controller.openLayout("keyboard-alphabet-14x4-top",false);
  QCOMPARE(controller.property("layoutName").toString(),QString("keyboard-alphabet-14x4-bottom"));
  QVERIFY(controller.property("actionStatus").toString().contains("standalone mode"));
  controller.openLayout("main-14x1-bottom",false);
  const auto keys=controller.property("layout").toMap()["keys"].toList();
  const auto reason=keys.front().toMap()["unavailableReason"].toString();
  QVERIFY(reason.contains("top"));
  controller.clickCommand("key:0");
  QCOMPARE(controller.property("actionStatus").toString(),reason);
  QCOMPARE(controller.property("layoutName").toString(),QString("main-14x1-bottom"));
  QVERIFY(!controller.executingCommands());
  Controller standalone;standalone.openLayout("main-14x1-bottom",false);
  QVERIFY(standalone.property("layout").toMap()["keys"].toList().front().toMap()["unavailableReason"].toString().isEmpty());
  standalone.clickCommand("key:0");
  QTRY_COMPARE(standalone.property("layoutName").toString(),QString("main-14x1-top"));
 }
 void corruptMappingsUseDefaultsAndPreserveSavedFile(){
  const auto path=QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);QVERIFY(QDir().mkpath(path));
  QFile file(QDir(path).filePath("linux-mappings.json"));QVERIFY(file.open(QIODevice::WriteOnly));file.write("{broken");file.close();
  {Controller controller;QVERIFY(controller.property("profileStatus").toString().contains("Using default mappings"));
   QVERIFY(!QJsonDocument::fromJson(controller.mappingJson().toUtf8()).object().isEmpty());}
  QVERIFY(file.open(QIODevice::ReadOnly));QCOMPARE(file.readAll(),QByteArray("{broken"));file.close();QVERIFY(file.remove());
 }
 void visualPreferencesPersistAndReset(){
  {Controller controller;controller.setProperty("keyboardScale",125);controller.setProperty("highContrast",true);QVERIFY(controller.savePreferences());}
  {Controller controller;QCOMPARE(controller.keyboardScale(),125);QVERIFY(controller.property("highContrast").toBool());
   controller.resetPreferences();QCOMPARE(controller.keyboardScale(),100);QVERIFY(!controller.property("highContrast").toBool());}
 }
 void settingsByDwellPauseAndCancelBusyOutput(){
  Controller controller;controller.setProperty("dwellMs",250);QSignalSpy requested(&controller,&Controller::settingsRequested);
  QVERIFY(controller.executeCommands({QVariantMap{{"tag","Wait"},{"text","10000"}}}));
  for(int i=0;i<8&&requested.isEmpty();++i){controller.hover("Settings");QTest::qWait(50);}
  QCOMPARE(requested.count(),1);QVERIFY(controller.property("paused").toBool());QVERIFY(!controller.executingCommands());
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
 QCoreApplication app(argc,argv);app.setOrganizationName("OptiKeyTests");app.setApplicationName("Controller");
 ControllerTests tests;return QTest::qExec(&tests,argc,argv);
}
#include "ControllerTests.moc"
