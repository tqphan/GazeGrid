#include <QtTest>
#include <QTemporaryDir>
#include "Controller.h"
#include "AppPaths.h"
#include "ProfileStore.h"
#include "GazeClock.h"
#include "MagnifierGeometry.h"
#include "KeyboardPack.h"
#include "KeyboardLayout.h"
#include <QDir>
#include <QStandardPaths>
#include <QFile>
#include <QJsonDocument>
#include <QGuiApplication>
#include <QScreen>
#include <QWindow>
class ControllerTests:public QObject {
 Q_OBJECT
 static QVariantMap overlapKey(QString tag,QString text,int duration=10){
  return {{"kind","DynamicKey"},{"dwell",duration},{"commands",QVariantList{QVariantMap{{"tag",tag},{"text",text}}}}};
 }
 static void addScratchpad(Controller &c){
  auto layout=c.property("layout").toMap();auto keys=layout["keys"].toList();
  keys<<QVariantMap{{"kind","Scratchpad"}};layout["keys"]=keys;c.setProperty("layout",layout);
 }
 static void overlapLayout(Controller &c,const QVariantList &keys){
  c.openLayout("keyboard-conversation-fullscreen",false);c.setProperty("paused",false);
  auto content=keys;content<<QVariantMap{{"kind","Scratchpad"}};
  c.setProperty("layout",QVariantMap{{"layoutMode","canvas"},{"activateoverlappingkeys",true},{"keys",content}});
 }
private slots:
 void overlapDwellOrdersModifiersAndDoesNotRetrigger(){
  Controller c;overlapLayout(c,{overlapKey("Text","a"),overlapKey("Text","b"),overlapKey("Action","LeftShift")});
  const QStringList targets{"key:1","key:0","key:2","key:0"};
  c.hoverKeys(targets);QTest::qWait(20);c.hoverKeys(targets);
  QTRY_COMPARE(c.property("scratchpad").toString(),QString("Ab"));
  for(int i=0;i<6;++i){c.hoverKeys(targets);QTest::qWait(20);}
  QCOMPARE(c.property("scratchpad").toString(),QString("Ab"));
  c.hoverKeys({});c.hoverKeys(targets);QTest::qWait(20);c.hoverKeys(targets);
  QTRY_COMPARE(c.property("scratchpad").toString(),QString("AbAb"));
 }
 void overlapDwellKeepsIndependentTimers(){
  Controller c;overlapLayout(c,{overlapKey("Text","a",10),overlapKey("Text","b",100)});
  c.hoverKeys({"key:0","key:1"});QTest::qWait(20);c.hoverKeys({"key:0","key:1"});
  QTRY_COMPARE(c.property("scratchpad").toString(),QString("a"));
  // Leaving A does not restart B's dwell timer.
  c.hoverKeys({"key:1"});
  for(int i=0;i<8;++i){QTest::qWait(20);c.hoverKeys({"key:1"});}
  QTRY_COMPARE(c.property("scratchpad").toString(),QString("ab"));
 }
 void overlapPendingCancelsOnLeavePauseAndNavigation(){
  for(const QString event:{"leave","pause","navigate","failure"}){
   Controller c;auto slow=overlapKey("Wait","80");
   if(event=="navigate")slow=overlapKey("ChangeKeyboard","keyboard-alphabet-14x4-bottom");
   if(event=="failure")slow=overlapKey("Action","UnsupportedAction");
   overlapLayout(c,{slow,overlapKey("Text","b")});
   c.hoverKeys({"key:0","key:1"});QTest::qWait(20);c.hoverKeys({"key:0","key:1"});
   if(event=="leave")c.hoverKeys({"key:0"});
   if(event=="pause")c.togglePause();
   QTest::qWait(120);QVERIFY(c.property("scratchpad").toString().isEmpty());
   if(event=="navigate")QCOMPARE(c.property("layoutName").toString(),QString("keyboard-alphabet-14x4-bottom"));
  }
 }
 void overlapOptOutAndGridUseOneTarget(){
  for(const QString mode:{"canvas","grid"}){
   Controller c;overlapLayout(c,{overlapKey("Text","a"),overlapKey("Text","b")});
   auto layout=c.property("layout").toMap();layout["layoutMode"]=mode;layout["activateoverlappingkeys"]=mode=="grid";
   c.setProperty("layout",layout);
   c.hoverKeys({"key:1","key:0"});QTest::qWait(20);c.hoverKeys({"key:1","key:0"});
   QTRY_COMPARE(c.property("scratchpad").toString(),QString("b"));
  }
 }
 void overlapRepeatAndClickConsumption(){
  Controller c;c.configureInputMethod([](QString){return true;},[](QString,bool){return true;});c.inputContextChanged(true,false);
  auto a=overlapKey("Text","a"),b=overlapKey("Text","b");a["mode"]="Repeat";b["mode"]="Repeat";
  overlapLayout(c,{a,b});c.setProperty("repeatIntervalMs",40);
  for(int i=0;i<12;++i){c.hoverKeys({"key:0","key:1"});QTest::qWait(20);}
  c.hoverKeys({});QTRY_VERIFY(!c.executingCommands());
  const auto text=c.property("scratchpad").toString();QVERIFY(text.size()>=4);
  QCOMPARE(text,QString("ab").repeated(text.size()/2));
  overlapLayout(c,{overlapKey("Text","a"),overlapKey("Text","b")});c.setProperty("scratchpad",QString());
  c.clickCommand("key:1");QTRY_COMPARE(c.property("scratchpad").toString(),QString("b"));
  for(int i=0;i<6;++i){c.hoverKeys({"key:0","key:1"});QTest::qWait(20);}
  QCOMPARE(c.property("scratchpad").toString(),QString("ba"));
 }
 void overlapPauseAndEditControlsAreExclusive(){
  Controller c;overlapLayout(c,{overlapKey("Text","a"),overlapKey("Action","Sleep"),overlapKey("Action","Sleep")});
  const QStringList targets{"key:0","key:1","key:2"};
  for(int i=0;i<6;++i){c.hoverKeys(targets);QTest::qWait(20);}
  QVERIFY(c.property("paused").toBool());QVERIFY(c.property("scratchpad").toString().isEmpty());
  c.hoverKeys({});for(int i=0;i<6;++i){c.hoverKeys(targets);QTest::qWait(20);}
  QVERIFY(!c.property("paused").toBool());QVERIFY(c.property("scratchpad").toString().isEmpty());
  overlapLayout(c,{overlapKey("Action","MoveModeToggle"),overlapKey("Text","a")});
  for(int i=0;i<6;++i){c.hoverKeys({"key:0","key:1"});QTest::qWait(20);}
  QVERIFY(c.property("sizeMoveMode").toBool());QVERIFY(c.property("scratchpad").toString().isEmpty());
 }
 void keyboardBackgroundIsPerLayout(){
  Controller c;c.openLayout("size-move-demo",false);c.setProperty("saveCanvasChanges",false);
  auto layout=c.property("layout").toMap();
  QCOMPARE(layout.value("transparentbackground").toString(),QString("true"));
  QCOMPARE(layout.value("backgroundclickthrough").toString(),QString("true"));
  c.openLayout("main-14x1-bottom",false);layout=c.property("layout").toMap();
  QVERIFY(!layout.contains("transparentbackground"));
  QVERIFY(!layout.contains("backgroundclickthrough"));
  QVERIFY(c.metaObject()->indexOfProperty("transparentKeyboardBackground")<0);
  QVERIFY(c.metaObject()->indexOfProperty("keyboardBackgroundClickThrough")<0);
 }
 void keyboardInputMask(){
  Controller c;QWindow window;
  c.updateKeyboardMask(&window,{QRectF(0,0,40,40),QRectF(60,0,40,40)},true);
  QVERIFY(window.mask().contains(QPoint(20,20)));
  QVERIFY(!window.mask().contains(QPoint(50,20)));
  QVERIFY(window.mask().contains(QPoint(80,20)));
  c.updateKeyboardMask(&window,{QRectF(10,50,20,20)},true);
  QVERIFY(!window.mask().contains(QPoint(20,20)));
  QVERIFY(window.mask().contains(QPoint(20,60)));
  c.updateKeyboardMask(&window,{},true);
  QVERIFY(!window.mask().isEmpty());QVERIFY(!window.mask().contains(QPoint(0,0)));
  c.updateKeyboardMask(&window,{},false);QVERIFY(window.mask().isEmpty());
 }

 void canvasModeDwellDoesNotRetrigger(){
  Controller c;c.openLayout("size-move-demo",false);c.setProperty("saveCanvasChanges",false);c.setProperty("dwellMs",20);
  c.hover("key:0");QTest::qWait(30);c.hover("key:0");
  QTRY_VERIFY(!c.executingCommands());QVERIFY(c.property("sizeMoveMode").toBool());
  for(int i=0;i<8;++i){
   c.canvasEditAt(.15,.10,true);QTest::qWait(25);
   QCOMPARE(c.property("canvasEditMode").toString(),QString("move"));
  }
  c.canvasEditAt(.50,.10,true);QTest::qWait(30);c.canvasEditAt(.50,.10,true);
  QTRY_VERIFY(!c.executingCommands());QCOMPARE(c.property("canvasEditMode").toString(),QString("size"));
  for(int i=0;i<8;++i){c.canvasEditAt(.50,.10,true);QTest::qWait(25);QCOMPARE(c.property("canvasEditMode").toString(),QString("size"));}
  c.canvasEditAt(.99,.99,true);c.canvasEditAt(.50,.10,true);QTest::qWait(30);c.canvasEditAt(.50,.10,true);
  QTRY_VERIFY(!c.executingCommands());QVERIFY(!c.property("sizeMoveMode").toBool());
  for(int i=0;i<8;++i){c.hover("key:8");QTest::qWait(25);QVERIFY(!c.property("sizeMoveMode").toBool());}
 }
 void canvasPlacementDwell(){
  Controller c;c.openLayout("size-move-demo",false);c.setProperty("saveCanvasChanges",false);c.setProperty("dwellMs",10);
  auto dwellAt=[&](double x,double y){c.canvasEditAt(x,y,true);QTest::qWait(20);c.canvasEditAt(x,y,true);};
  c.activate("MoveModeToggle");QCOMPARE(c.property("canvasEditMode").toString(),QString("move"));
  dwellAt(.15,.60);QCOMPARE(c.property("sizeMoveSelection").toInt(),3);
  dwellAt(.15,.60);QCOMPARE(c.property("sizeMoveSelection").toInt(),3);
  const auto scratch=c.property("scratchpad");
  dwellAt(.40,.60);QCOMPARE(c.property("sizeMoveSelection").toInt(),-1);
  auto rect=[&]{return c.property("layout").toMap()["keys"].toList()[3].toMap();};
  QCOMPARE(rect()["col"].toDouble(),30.0);QCOMPARE(rect()["row"].toDouble(),46.0);
  dwellAt(.40,.60);QCOMPARE(c.property("sizeMoveSelection").toInt(),-1);
  c.activate("SizeModeToggle");QCOMPARE(c.property("canvasEditMode").toString(),QString("size"));
  c.clickCommand("key:3");c.canvasEditAt(.40,.60,true);
  dwellAt(.65,.90);QCOMPARE(rect()["width"].toDouble(),35.0);QCOMPARE(rect()["height"].toDouble(),44.0);
  c.activate("MoveModeToggle");c.clickCommand("key:3");c.canvasEditAt(.4,.6,true);
  c.canvasEditAt(.9,.9,true);QTest::qWait(20);c.canvasEditAt(.9,.9,false);
  QCOMPARE(c.property("sizeMoveSelection").toInt(),3);
  c.canvasEditAt(.9,.9,true);dwellAt(1,1);
  QCOMPARE(rect()["col"].toDouble(),65.0);QCOMPARE(rect()["row"].toDouble(),56.0);
  QCOMPARE(c.property("scratchpad"),scratch);
  c.activate("MoveModeToggle");QVERIFY(!c.property("sizeMoveMode").toBool());
  QCOMPARE(c.property("sizeMoveSelection").toInt(),-1);
 }
 void sizeMoveMode(){
  Controller c;c.openLayout("size-move-demo",false);c.setProperty("saveCanvasChanges",false);
  QVERIFY(!c.property("layout").toMap().contains("error"));
  QVERIFY(!c.setCanvasKeyRect(3,1,2,20,20));
  c.clickCommand("key:0");QTRY_VERIFY(!c.executingCommands());
  QVERIFY(c.property("sizeMoveMode").toBool());
  const auto original=c.property("layout").toMap();
  QVERIFY(!c.setCanvasKeyRect(-1,1,2,20,20));
  QVERIFY(!c.setCanvasKeyRect(3,-1,2,20,20));
  QVERIFY(!c.setCanvasKeyRect(3,90,2,20,20));
  QVERIFY(!c.setCanvasKeyRect(3,1,2,0,20));
  QCOMPARE(c.property("layout").toMap(),original);
  const auto scratch=c.property("scratchpad");c.clickCommand("key:3");
  QCOMPARE(c.property("scratchpad"),scratch);
  QVERIFY(c.setCanvasKeyRect(3,10.5,40.5,25.5,30.5));
  auto key=c.property("layout").toMap()["keys"].toList()[3].toMap();
  QCOMPARE(key["col"].toDouble(),10.5);QCOMPARE(key["height"].toDouble(),30.5);
  c.clickCommand("key:0");QTRY_VERIFY(!c.executingCommands());
  QVERIFY(!c.property("sizeMoveMode").toBool());
  QCOMPARE(c.property("layout").toMap()["keys"].toList()[3].toMap(),key);
  c.activate("MoveModeToggle");QVERIFY(c.property("sizeMoveMode").toBool());
  c.openLayout("size-move-demo",false);c.setProperty("saveCanvasChanges",false);QVERIFY(!c.property("sizeMoveMode").toBool());
  QCOMPARE(c.property("layout").toMap(),original);
  c.openLayout("game-controller",false);c.activate("MoveModeToggle");
  QVERIFY(!c.property("sizeMoveMode").toBool());
 }

 void preserveTogglePreference(){
  auto values=ProfileStore::defaults();
  QCOMPARE(values["preserveTogglesBetweenLayouts"].toBool(),false);
  values["preserveTogglesBetweenLayouts"]=1;QVERIFY(!ProfileStore::valid(values));
  {Controller c;c.resetPreferences();QVERIFY(!c.property("preserveTogglesBetweenLayouts").toBool());
   c.setProperty("preserveTogglesBetweenLayouts",true);QVERIFY(c.savePreferences());}
  {Controller c;QVERIFY(c.property("preserveTogglesBetweenLayouts").toBool());
   c.resetPreferences();QVERIFY(!c.property("preserveTogglesBetweenLayouts").toBool());}
 }

 void cycleButtonModeAction(){
  Controller c;c.openLayout("game-controller",false);
  QString modeKey;
  const auto keys=c.property("layout").toMap()["keys"].toList();
  for(int i=0;i<keys.size();++i){
   const auto commands=keys[i].toMap()["commands"].toList();
   if(commands.size()==1&&commands.front().toMap()["text"]=="CycleButtonMode")modeKey=QString("key:%1").arg(i);
  }
  QVERIFY(!modeKey.isEmpty());
  for(const QString expected:{"Repeat","Hold","Toggle","Tap"}){
   c.clickCommand(modeKey);QTRY_VERIFY(!c.executingCommands());
   QCOMPARE(c.buttonMode(),expected);QCOMPARE(c.keyMode(),QString("Tap"));
  }
  c.setProperty("dwellMs",10);
  c.hover("");c.hover(modeKey);QTest::qWait(20);c.hover(modeKey);
  QTRY_VERIFY(!c.executingCommands());QCOMPARE(c.buttonMode(),QString("Repeat"));
  for(int i=0;i<4;++i){QTest::qWait(20);c.hover(modeKey);}
  QCOMPARE(c.buttonMode(),QString("Repeat"));
  c.clickCommand("CycleButtonMode");QCOMPARE(c.buttonMode(),QString("Hold"));
 }
 void cycleKeyModeAction(){
  Controller c;
  QCOMPARE(c.keyMode(),QString("Tap"));
  c.clickCommand("CycleKeyMode");QCOMPARE(c.keyMode(),QString("Repeat"));
  c.clickCommand("CycleKeyMode");QCOMPARE(c.keyMode(),QString("Hold"));
  c.clickCommand("CycleKeyMode");QCOMPARE(c.keyMode(),QString("Toggle"));
  c.clickCommand("CycleKeyMode");QCOMPARE(c.keyMode(),QString("Tap"));
  QVERIFY(c.executeCommands({QVariantMap{{"tag","Action"},{"text","CycleKeyMode"}}}));
  QTRY_VERIFY(!c.executingCommands());QCOMPARE(c.keyMode(),QString("Repeat"));
 }

 void controllerLayoutAndConversationSafety(){
  Controller c;c.openLayout("game-controller",false);
  QCOMPARE(c.property("layoutName").toString(),QString("game-controller"));
  const auto keys=c.property("layout").toMap()["keys"].toList();
  QCOMPARE(keys.first().toMap()["kind"].toString(),QString("Joystick"));
  c.joystick(-1,0,0,false);c.togglePause();c.joystick(0,1,1,true);
  c.togglePause();c.openLayout("keyboard-conversation-fullscreen",false);
  QVERIFY(c.executeCommands({QVariantMap{{"tag","Action"},{"text","GamepadAPress"}}}));
  QTRY_VERIFY(!c.executingCommands());QVERIFY(!c.gamepadHeld("GamepadAPress"));
 }
 void experimentalConversationEditing(){
  Controller c;c.openLayout("keyboard-conversation-fullscreen",false);
  QCOMPARE(c.property("layoutName").toString(),QString("keyboard-conversation-fullscreen"));
  auto keys=c.property("layout").toMap()["keys"].toList();
  auto findKey=[&](QString tag,QString text){for(int i=0;i<keys.size();++i)for(const auto &v:keys[i].toMap()["commands"].toList()){
   auto cmd=v.toMap();if(cmd["tag"]==tag&&cmd["text"].toString()==text)return QString("key:%1").arg(i);
  }return QString();};
  const auto letter=findKey("Text","a"),back=findKey("Action","BackMany");QVERIFY(!letter.isEmpty());QVERIFY(!back.isEmpty());
  c.clickCommand(letter);QTRY_VERIFY(!c.executingCommands());QCOMPARE(c.property("scratchpad").toString(),QString("a"));
  c.setProperty("scratchpad",QString("hello world "));
  c.clickCommand(back);QTRY_VERIFY(!c.executingCommands());QCOMPARE(c.property("scratchpad").toString(),QString("hello "));
  c.clickCommand(findKey("Action","ClearScratchpad"));QTRY_VERIFY(!c.executingCommands());QVERIFY(c.property("scratchpad").toString().isEmpty());
  c.clickCommand(letter);QTRY_VERIFY(!c.executingCommands());
  const auto first=c.property("suggestions").toStringList();QVERIFY(!first.isEmpty());
  c.clickCommand(findKey("Action","NextSuggestions"));QTRY_VERIFY(!c.executingCommands());
  QVERIFY(c.property("suggestions").toStringList()!=first);
  c.clickCommand(findKey("Action","PreviousSuggestions"));QTRY_VERIFY(!c.executingCommands());
  QCOMPARE(c.property("suggestions").toStringList(),first.mid(0,6));
 }
 void ordinaryKeyboardDoesNotCaptureOrEditScratchpad(){
  Controller c;QString output;QStringList keys;
  c.configureInputMethod([&](QString text){output+=text;return true;},[&](QString key,bool down){if(down)keys<<key;return true;});
  c.inputContextChanged(true,false);c.setProperty("paused",false);
  c.setProperty("scratchpad",QString("saved conversation"));
  auto type=[&](QString text){QVERIFY(c.executeCommands({QVariantMap{{"tag","Text"},{"text",text}}}));QTRY_VERIFY(!c.executingCommands());};
  type("thank y");
  QCOMPARE(output,QString("thank y"));
  QCOMPARE(c.property("scratchpad").toString(),QString("saved conversation"));
  QCOMPARE(c.property("suggestions").toStringList().first(),QString("you"));
  c.clickCommand("suggest:0");QTRY_VERIFY(!c.executingCommands());
  QCOMPARE(output,QString("thank you "));
  QVERIFY(c.executeCommands({QVariantMap{{"tag","Action"},{"text","Backspace"}}}));QTRY_VERIFY(!c.executingCommands());
  QVERIFY(!keys.isEmpty());
  QCOMPARE(c.property("scratchpad").toString(),QString("saved conversation"));
  c.openLayout("keyboard-conversation-fullscreen",false);
  QCOMPARE(c.property("scratchpad").toString(),QString("saved conversation"));
  type("!");QCOMPARE(c.property("scratchpad").toString(),QString("saved conversation!"));
  c.openLayout("keyboard-alphabet-14x4-bottom",false);type("other");
  c.openLayout("keyboard-conversation-fullscreen",false);
  QCOMPARE(c.property("scratchpad").toString(),QString("saved conversation!"));
 }
 void predictionsInsertCompletionsAndNextWordsWithoutLearning(){
  Controller c;c.openLayout("keyboard-conversation-fullscreen",false);
  c.setProperty("learning",false);
  QVERIFY(c.executeCommands({QVariantMap{{"tag","Text"},{"text","thank "}}}));QTRY_VERIFY(!c.executingCommands());
  QCOMPARE(c.property("suggestions").toStringList().first(),QString("you"));
  c.clickCommand("suggest:0");QTRY_VERIFY(!c.executingCommands());
  QCOMPARE(c.property("scratchpad").toString(),QString("thank you "));
  QVERIFY(c.executeCommands({QVariantMap{{"tag","Action"},{"text","ClearScratchpad"}}}));QTRY_VERIFY(!c.executingCommands());
  QVERIFY(c.executeCommands({QVariantMap{{"tag","Text"},{"text","thank y"}}}));QTRY_VERIFY(!c.executingCommands());
  QCOMPARE(c.property("suggestions").toStringList().first(),QString("you"));
  c.clickCommand("suggest:0");QTRY_VERIFY(!c.executingCommands());
  QCOMPARE(c.property("scratchpad").toString(),QString("thank you "));
  c.inputContextChanged(true,true);
  c.openLayout("keyboard-conversation-fullscreen",false);
  QVERIFY(c.executeCommands({QVariantMap{{"tag","Action"},{"text","NextSuggestions"}}}));QTRY_VERIFY(!c.executingCommands());
  QVERIFY(c.property("suggestions").toStringList().isEmpty());
 }
 void completionSpaceCanBeDisabled(){
  for(bool addSpace:{false,true}){
   for(const QString prefix:{QString("thank "),QString("thank y")}){
    Controller c;c.openLayout("keyboard-conversation-fullscreen",false);
    c.setAddSpaceAfterCompletion(addSpace);
    QString output;
    c.configureInputMethod([&](QString text){output+=text;return true;},[](QString,bool){return true;});
    c.inputContextChanged(true,false);addScratchpad(c);
    QVERIFY(c.executeCommands({QVariantMap{{"tag","Text"},{"text",prefix}}}));QTRY_VERIFY(!c.executingCommands());
    QCOMPARE(c.property("suggestions").toStringList().first(),QString("you"));
    c.clickCommand("suggest:0");QTRY_VERIFY(!c.executingCommands());
    const QString expected=addSpace?"thank you ":"thank you";
    QCOMPARE(c.property("scratchpad").toString(),expected);
    QCOMPARE(output,expected);
   }
  }
 }
 void sentenceStartersAppearAndInsertWhenEnabled(){
  Controller c;c.openLayout("keyboard-conversation-fullscreen",false);
  QVERIFY(!c.property("suggestions").toStringList().isEmpty());
  c.setSuggestSentenceStarters(false);
  QVERIFY(c.property("suggestions").toStringList().isEmpty());
  c.setPredictNextWord(false);c.setSuggestSentenceStarters(true);
  const auto starters=c.property("suggestions").toStringList();QVERIFY(!starters.isEmpty());
  c.setSuggestSentenceStarters(false);QVERIFY(c.property("suggestions").toStringList().isEmpty());
  c.setSuggestSentenceStarters(true);
  c.setPredictionMode("off");QVERIFY(c.property("suggestions").toStringList().isEmpty());
  c.setPredictionMode("context");
  QString output;
  c.configureInputMethod([&](QString text){output+=text;return true;},[](QString,bool){return true;});
  c.inputContextChanged(true,false);addScratchpad(c);
  c.clickCommand("suggest:0");QTRY_VERIFY(!c.executingCommands());
  QCOMPARE(output,starters.first()+" ");
  QCOMPARE(c.property("scratchpad").toString(),output);
  QVERIFY(c.executeCommands({QVariantMap{{"tag","Action"},{"text","ClearScratchpad"}}}));QTRY_VERIFY(!c.executingCommands());
  QCOMPARE(c.property("suggestions").toStringList(),starters);
  c.inputContextChanged(true,true);
  QVERIFY(c.property("suggestions").toStringList().isEmpty());
 }
 void predictionOptionsApplyImmediatelyAndPersist(){
  ProfileStore store(QDir(appConfigDirectory()).filePath("profile.json"));QString error;
  auto legacy=ProfileStore::defaults();legacy.remove("predictionMode");legacy.remove("predictNextWord");legacy.remove("addSpaceAfterCompletion");legacy.remove("suggestSentenceStarters");QVERIFY(store.save(legacy,error));
  {
   Controller c;c.openLayout("keyboard-conversation-fullscreen",false);
   QCOMPARE(c.predictionMode(),QString("context"));QVERIFY(c.predictNextWord());QVERIFY(c.addSpaceAfterCompletion());QVERIFY(c.suggestSentenceStarters());
   auto type=[&](QString text){QVERIFY(c.executeCommands({QVariantMap{{"tag","Text"},{"text",text}}}));QTRY_VERIFY(!c.executingCommands());};
   type("thank ");QCOMPARE(c.property("suggestions").toStringList().first(),QString("you"));
   c.setPredictNextWord(false);QVERIFY(c.property("suggestions").toStringList().isEmpty());
   type("y");QVERIFY(!c.property("suggestions").toStringList().isEmpty());
   c.setPredictionMode("off");QVERIFY(c.property("suggestions").toStringList().isEmpty());
   QVERIFY(c.executeCommands({QVariantMap{{"tag","Action"},{"text","NextSuggestions"}}}));QTRY_VERIFY(!c.executingCommands());
   QVERIFY(c.property("suggestions").toStringList().isEmpty());
   c.setPredictionMode("invalid");QCOMPARE(c.predictionMode(),QString("off"));
   c.setPredictionMode("frequency");QVERIFY(!c.property("suggestions").toStringList().isEmpty());
   c.setAddSpaceAfterCompletion(false);c.setSuggestSentenceStarters(false);
   c.setLearning(true);QVERIFY(c.savePreferences());
  }
  {
   Controller c;QCOMPARE(c.predictionMode(),QString("frequency"));QVERIFY(!c.predictNextWord());QVERIFY(!c.addSpaceAfterCompletion());QVERIFY(!c.suggestSentenceStarters());QVERIFY(c.learningEnabled());
   c.openLayout("keyboard-conversation-fullscreen",false);
   QVERIFY(c.executeCommands({QVariantMap{{"tag","Text"},{"text","zorbella "}}}));QTRY_VERIFY(!c.executingCommands());
   QVERIFY(c.executeCommands({QVariantMap{{"tag","Action"},{"text","ClearScratchpad"}},QVariantMap{{"tag","Text"},{"text","zorb"}}}));QTRY_VERIFY(!c.executingCommands());
   QVERIFY(c.property("suggestions").toStringList().contains("zorbella"));
   c.setLearning(false);QVERIFY(!c.property("suggestions").toStringList().contains("zorbella"));
   c.setLearning(true);QVERIFY(c.property("suggestions").toStringList().contains("zorbella"));
   c.resetLearning();QVERIFY(!c.property("suggestions").toStringList().contains("zorbella"));
   c.resetPreferences();QCOMPARE(c.predictionMode(),QString("context"));QVERIFY(c.predictNextWord());QVERIFY(c.addSpaceAfterCompletion());QVERIFY(c.suggestSentenceStarters());QVERIFY(!c.learningEnabled());
  }
 }
 void gazePreferenceLoadsSavesAndResets(){
  ProfileStore store(QDir(appConfigDirectory()).filePath("profile.json"));QString error;
  auto values=ProfileStore::defaults();QVERIFY(!values["gazeEnabled"].toBool());
  values.remove("gazeEnabled");QVERIFY(ProfileStore::valid(values));
  values["gazeEnabled"]="true";QVERIFY(!ProfileStore::valid(values));
  values["gazeEnabled"]=true;QVERIFY(store.save(values,error));
  {Controller c;QVERIFY(c.isGazeEnabled());QVERIFY(c.property("gazeEnabled").toBool());QVERIFY(c.savePreferences());}
  {Controller c;QVERIFY(c.isGazeEnabled());c.setProperty("dwellMs",1250);c.setGazeEnabled(false);}
  {Controller c;QVERIFY(!c.isGazeEnabled());QCOMPARE(c.property("dwellMs").toInt(),750);}
  values["gazeEnabled"]=true;QVERIFY(store.save(values,error));
  {Controller c;QVERIFY(c.isGazeEnabled());c.resetPreferences();QVERIFY(!c.isGazeEnabled());}
  {Controller c;QVERIFY(!c.isGazeEnabled());}
 }
 void scrollSettingsPersistAndReset(){
  {Controller c;c.setProperty("scrollSpeed",170);c.setProperty("scrollDeadzone",90);c.setProperty("scrollAxis",1);QVERIFY(c.savePreferences());}
  {Controller c;QCOMPARE(c.property("scrollSpeed").toInt(),170);QCOMPARE(c.property("scrollDeadzone").toInt(),90);QCOMPARE(c.property("scrollAxis").toInt(),1);c.resetPreferences();}
  {Controller c;QCOMPARE(c.property("scrollSpeed").toInt(),100);QCOMPARE(c.property("scrollDeadzone").toInt(),50);QCOMPARE(c.property("scrollAxis").toInt(),0);}
  for(const auto key:{"scrollSpeed","scrollDeadzone","scrollAxis"}){
   auto values=ProfileStore::defaults();values.remove(key);QVERIFY(ProfileStore::valid(values));
   values[key]=-1;QVERIFY(!ProfileStore::valid(values));values[key]=1000;QVERIFY(!ProfileStore::valid(values));
   values[key]=1.5;QVERIFY(!ProfileStore::valid(values));values[key]="50";QVERIFY(!ProfileStore::valid(values));
  }
 }
 void repeatRatePersistsAndResets(){
  {Controller c;c.setProperty("repeatIntervalMs",150);QVERIFY(c.savePreferences());}
  {Controller c;QCOMPARE(c.property("repeatIntervalMs").toInt(),150);c.resetPreferences();QCOMPARE(c.property("repeatIntervalMs").toInt(),750);}
 }

 void repeatRequiresModeOrXmlOverride(){
  Controller c;c.configureInputMethod([](QString){return true;},[](QString,bool){return true;});c.inputContextChanged(true,false);
  c.setProperty("repeatIntervalMs",100);
  for(const QString global:{"Tap","Repeat"}){
   if(global=="Repeat")c.clickCommand("CycleKeyMode");
   for(const QString fixed:{"","Tap","Repeat"}){
    for(const QString value:{"a","1"," ",".","BackOne","Left","Return","F1"}){
     const bool text=value.size()==1;
     const QVariantList commands{QVariantMap{{"tag",text?"Text":"Action"},{"text",value}}};
     c.setProperty("layout",QVariantMap{{"keys",QVariantList{QVariantMap{{"mode",fixed},{"commands",commands}}}}});
     c.clickCommand("key:0");QTRY_VERIFY(!c.executingCommands());
     QCOMPARE(c.hover("key:0"),0.0);QTest::qWait(20);
     QCOMPARE(c.hover("key:0")>0,(fixed.isEmpty()?global:fixed)=="Repeat");
    }
   }
  }
 }
 void obsoleteRepeatPreferencesIgnored(){
  const QStringList obsolete{"repeatLettersOnHover","repeatPunctuationOnHover","repeatBackspaceOnHover","repeatArrowsOnHover"};
  auto values=ProfileStore::defaults();
  for(const auto &name:obsolete){QVERIFY(!values.contains(name));values[name]=true;}
  QString error;ProfileStore store(QDir(appConfigDirectory()).filePath("profile.json"));QVERIFY(store.save(values,error));
  Controller c;QCOMPARE(c.keyMode(),QString("Tap"));
  for(const auto &name:obsolete)QVERIFY(c.metaObject()->indexOfProperty(name.toUtf8().constData())<0);
  QVERIFY(c.savePreferences());
  const auto saved=store.load().values;
  for(const auto &name:obsolete)QVERIFY(!saved.contains(name));
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

 void dwellIndicatorPreferences(){
  const QStringList styles{"bar","top-bar","fill","outline","ring","pie","collapse"};
  for(const auto &style:styles){
   {Controller c;c.setProperty("dwellIndicatorStyle",style);QVERIFY(c.savePreferences());}
   {Controller c;QCOMPARE(c.property("dwellIndicatorStyle").toString(),style);}
  }
  {Controller c;c.resetPreferences();QCOMPARE(c.property("dwellIndicatorStyle").toString(),QString("bar"));}
  auto values=ProfileStore::defaults();values.remove("dwellIndicatorStyle");
  QString error;ProfileStore store(QDir(appConfigDirectory()).filePath("profile.json"));
  QVERIFY(store.save(values,error));
  {Controller c;QCOMPARE(c.property("dwellIndicatorStyle").toString(),QString("bar"));}
  values["dwellIndicatorStyle"]="unknown";QVERIFY(!store.save(values,error));
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
 void unavailableGazeDoesNotInterruptMouseFallback(){
  Controller c;QString output;
  c.configureInputMethod([&](QString text){output+=text;return true;},[](QString,bool){return true;});c.inputContextChanged(true,false);
  c.setProperty("dwellMs",100);
  c.setProperty("layout",QVariantMap{{"keys",QVariantList{QVariantMap{{"commands",QVariantList{QVariantMap{{"tag","Text"},{"text","a"}}}}}}}});
  auto sample=[&](bool valid){QVERIFY(QMetaObject::invokeMethod(&c,"receiveGaze",Qt::DirectConnection,Q_ARG(double,.5),Q_ARG(double,.5),Q_ARG(bool,valid),Q_ARG(qint64,valid?GazeClock::now():0)));};
  sample(true);sample(false);
  c.hover("key:0");
  for(int i=0;i<6;++i){QTest::qWait(25);sample(false);c.hover("key:0");}
  QTRY_COMPARE(output,QString("a"));
  QVERIFY(c.executeCommands({QVariantMap{{"tag","Wait"},{"text","50"}},QVariantMap{{"tag","Text"},{"text","b"}}}));
  sample(false);QTRY_COMPARE(output,QString("ab"));
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
  sample(qQNaN(),.4);QVERIFY(!points.last()[2].toBool());
  sample(.3,.3);QCOMPARE(points.last()[0].toDouble(),.3);
  // Timer delivery can be delayed on busy Windows runners. Check the event,
  // with a bounded wait, rather than assuming it arrived at exactly 230 ms.
  QTRY_VERIFY_WITH_TIMEOUT(!points.last()[2].toBool(),1000);
  sample(.7,.7);QCOMPARE(points.last()[0].toDouble(),.7);
  controller.setDisplaySize(2000,1000);sample(.4,.4);QCOMPARE(points.last()[0].toDouble(),.4);
  controller.setGazeSmoothingLevel(0);sample(.41,.42);QCOMPARE(points.last()[0].toDouble(),.41);
  controller.setGazeSmoothingLevel(2);sample(.6,.6);QCOMPARE(points.last()[0].toDouble(),.6);
  controller.setSmoothWhenChangingGazeTarget(true);sample(.2,.2);QCOMPARE(points.last()[0].toDouble(),.2);
 }
 void offscreenGazeClampsWithoutCanceling(){
  Controller c;c.setGazeSmoothingLevel(0);
  QSignalSpy points(&c,&Controller::gazePoint);
  QVERIFY(c.executeCommands({QVariantMap{{"tag","Wait"},{"text","10000"}}}));
  const QList<QPointF> samples{{0,.5},{1,.5},{.5,0},{.5,1},{-.01,.5},{1.01,.5},{.5,-.1},{.5,1.1},{-.5,-.5},{1.5,1.5}};
  for(const auto &p:samples){
   QTest::qWait(1);
   QVERIFY(QMetaObject::invokeMethod(&c,"receiveGaze",Qt::DirectConnection,Q_ARG(double,p.x()),Q_ARG(double,p.y()),Q_ARG(bool,true),Q_ARG(qint64,GazeClock::now())));
   QVERIFY(points.last()[2].toBool());
   QCOMPARE(points.last()[0].toDouble(),qBound(0.,p.x(),1.));
   QCOMPARE(points.last()[1].toDouble(),qBound(0.,p.y(),1.));
   QVERIFY(c.executingCommands());
  }
  QVERIFY(QMetaObject::invokeMethod(&c,"receiveGaze",Qt::DirectConnection,Q_ARG(double,0.),Q_ARG(double,0.),Q_ARG(bool,false),Q_ARG(qint64,0)));
  QVERIFY(!points.last()[2].toBool());QVERIFY(!c.executingCommands());
 }
 void smoothingPreferencesPersistAndReset(){
  {Controller controller;controller.setGazeSmoothingLevel(4);controller.setSmoothWhenChangingGazeTarget(true);QVERIFY(controller.savePreferences());}
  {Controller controller;QCOMPARE(controller.gazeSmoothingLevel(),4);QVERIFY(controller.smoothWhenChangingGazeTarget());
   controller.resetPreferences();QCOMPARE(controller.gazeSmoothingLevel(),1);QVERIFY(!controller.smoothWhenChangingGazeTarget());}
  {Controller controller;QCOMPARE(controller.gazeSmoothingLevel(),1);QVERIFY(!controller.smoothWhenChangingGazeTarget());}
 }

 void unicodeScratchpadBackspaceRemovesWholeGrapheme(){
  Controller controller;controller.configureInputMethod([](QString){return true;},[](QString,bool){return true;});controller.inputContextChanged(true,false);addScratchpad(controller);
  QVERIFY(controller.executeCommands({QVariantMap{{"tag","Text"},{"text",QString::fromUtf8("a😀é")}}}));QTRY_VERIFY(!controller.executingCommands());
  const QVariantMap back{{"tag","Action"},{"text","Backspace"}};
  QVERIFY(controller.executeCommands({back}));QTRY_VERIFY(!controller.executingCommands());QCOMPARE(controller.property("scratchpad").toString(),QString::fromUtf8("a😀"));
  QVERIFY(controller.executeCommands({back}));QTRY_VERIFY(!controller.executingCommands());QCOMPARE(controller.property("scratchpad").toString(),QString("a"));
 }
 void nativeContextCancellationAndSensitiveText(){
  Controller controller;QString output;
  controller.configureInputMethod([&](QString text){output+=text;return true;},[](QString,bool){return true;});
  controller.inputContextChanged(true,false);addScratchpad(controller);QVERIFY(controller.ready());
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
  const auto path=appConfigDirectory();QVERIFY(QDir().mkpath(path));
  QFile file(QDir(path).filePath("linux-mappings.json"));QVERIFY(file.open(QIODevice::WriteOnly));file.write("{broken");file.close();
#ifndef _WIN32
  {Controller controller;QVERIFY(controller.property("profileStatus").toString().contains("Using default mappings"));
   QVERIFY(!QJsonDocument::fromJson(controller.mappingJson().toUtf8()).object().isEmpty());}
#else
  {Controller controller;QVERIFY(!controller.property("profileStatus").toString().contains("Using default mappings"));}
#endif
  QVERIFY(file.open(QIODevice::ReadOnly));QCOMPARE(file.readAll(),QByteArray("{broken"));file.close();QVERIFY(file.remove());
 }
 void visualPreferencesPersistAndReset(){
  {Controller controller;controller.setProperty("keyboardTheme","ocean");controller.setProperty("keyboardScale",125);controller.setProperty("shortLabelScale",125);controller.setProperty("longLabelScale",150);QVERIFY(controller.savePreferences());}
  {Controller controller;QCOMPARE(controller.property("keyboardTheme").toString(),QString("ocean"));QCOMPARE(controller.keyboardScale(),125);QCOMPARE(controller.property("shortLabelScale").toInt(),125);QCOMPARE(controller.property("longLabelScale").toInt(),150);
   controller.resetPreferences();QCOMPARE(controller.property("keyboardTheme").toString(),QString("system"));QCOMPARE(controller.keyboardScale(),100);QCOMPARE(controller.property("shortLabelScale").toInt(),150);QCOMPARE(controller.property("longLabelScale").toInt(),100);}
 }
 void legacyHighContrastMigratesToTheme(){
  const auto path=QDir(appConfigDirectory()).filePath("profile.json");
  QVERIFY(QDir().mkpath(QFileInfo(path).absolutePath()));
  QFile file(path);auto values=ProfileStore::defaults();
  values["highContrast"]=true;values["keyboardTheme"]="ocean";
  QVERIFY(file.open(QIODevice::WriteOnly));file.write(QJsonDocument(values).toJson());file.close();
  {Controller controller;QCOMPARE(controller.property("keyboardTheme").toString(),QString("high-contrast"));QVERIFY(controller.savePreferences());}
  QVERIFY(file.open(QIODevice::ReadOnly));values=QJsonDocument::fromJson(file.readAll()).object();file.close();
  QVERIFY(!values.contains("highContrast"));QCOMPARE(values["keyboardTheme"].toString(),QString("high-contrast"));
  {Controller controller;QCOMPARE(controller.property("keyboardTheme").toString(),QString("high-contrast"));
   controller.setProperty("keyboardTheme","paper");QVERIFY(controller.savePreferences());}
  {Controller controller;QCOMPARE(controller.property("keyboardTheme").toString(),QString("paper"));controller.resetPreferences();}
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
  QSignalSpy generalChanges(&controller,&Controller::changed);
  QSignalSpy frames(&controller,&Controller::magnifierFrameChanged);
  QTest::qWait(200);const auto middle=controller.magnifierImage().size();
  QVERIFY(frames.count()>0);QCOMPARE(generalChanges.count(),0);
  QVERIFY(middle.width()<initial.width());QVERIFY(middle.width()>frame.width()/4);
  QTRY_VERIFY(!controller.property("zoomAnimating").toBool());
  QCOMPARE(controller.magnifierImage().size(),QSize(frame.width()/4,frame.height()/4));
  const int revision=controller.property("magnifierRevision").toInt();QTest::qWait(100);
  QCOMPARE(controller.property("magnifierRevision").toInt(),revision);
 }
 void progressiveZoomIgnoresPointerMotion_data(){
  QTest::addColumn<bool>("dwellEnabled");
  QTest::newRow("dwell enabled")<<true;
  QTest::newRow("dwell disabled")<<false;
 }
 void progressiveZoomIgnoresPointerMotion(){
  QFETCH(bool,dwellEnabled);
  Controller controller;controller.inputContextChanged(true,false);
  controller.setProperty("targeting",true);controller.setProperty("magnifierEnabled",true);
  controller.setProperty("progressiveZoom",true);controller.setProperty("zoomDurationMs",500);
  controller.setProperty("magnifierZoom",4);controller.setProperty("targetDwellEnabled",dwellEnabled);
  controller.targetAt(.65,.6);QVERIFY(controller.property("capturePending").toBool());
  QImage frame(QGuiApplication::primaryScreen()->size(),QImage::Format_RGB32);
  for(int y=0;y<frame.height();++y)for(int x=0;x<frame.width();++x)frame.setPixelColor(x,y,QColor(x*255/frame.width(),y*255/frame.height(),0));
  QVERIFY(QMetaObject::invokeMethod(&controller,"receiveMagnifierFrame",Qt::DirectConnection,Q_ARG(QImage,frame)));
  QCOMPARE(controller.dwellAt(1,1,true),0.);
  QTRY_VERIFY(!controller.property("zoomAnimating").toBool());
  auto result=controller.magnifierImage();
  QCOMPARE(result,frame.copy(MagnifierGeometry::crop(frame.size(),{.65,.6},4)));
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
  QCOMPARE(controller.property("menuLayoutNames").toStringList(),QStringList({"game-controller","main-14x1-top","size-move-demo"}));
  QCOMPARE(controller.property("layoutNames").toStringList().size(),31);
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
  for(const auto &name:{"keyboard-conversation-fullscreen"}){
   controller.recover();controller.openLayout(name,false);
   const auto keys=controller.property("layout").toMap()["keys"].toList();
   QString sleep;
   for(int i=0;i<keys.size();++i){
    const auto commands=keys[i].toMap()["commands"].toList();
    if(commands.size()==1&&commands.front().toMap()["text"]=="Sleep")sleep="key:"+QString::number(i);
   }
   QVERIFY(!sleep.isEmpty());
   controller.clickCommand(sleep);QVERIFY(controller.property("paused").toBool());
   controller.clickCommand("key:1");QCOMPARE(controller.property("layoutName").toString(),QString(name));
   controller.clickCommand(sleep);QVERIFY(!controller.property("paused").toBool());
  }
 }
 void sleepKeyDwellResumesOnlyAfterLookingAway(){
  Controller controller;controller.recover();controller.openLayout("keyboard-conversation-fullscreen",false);controller.setProperty("dwellMs",250);
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
 qputenv("GAZEGRID_CONFIG_DIR",dir.path().toUtf8());
 qputenv("QT_QPA_PLATFORM","offscreen");
 QGuiApplication app(argc,argv);app.setOrganizationName("GazeGridTests");app.setApplicationName("Controller");
 ControllerTests tests;return QTest::qExec(&tests,argc,argv);
}
#include "ControllerTests.moc"
