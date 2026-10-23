#include <QtTest>
#include "GameController.h"
#include "ActionCatalog.h"
#include "KeyboardPack.h"
#include "KeyboardLayout.h"
#include <linux/input.h>
class GameControllerTests:public QObject {
 Q_OBJECT
private slots:
 void buttonModes(){
  for(const QString source:{"GamepadATap","GamepadDpadUpToggle","GamepadButton32Press"}){
   const auto press=GameController::buttonAction(source,"Press");
   QVERIFY(!press.isEmpty());
   GameController pad;QList<int> events;
   pad.output=[&](int,int,int value){events<<value;return true;};
   QVERIFY(pad.perform(GameController::buttonAction(source,"Tap")));
   QTest::qWait(130);QCOMPARE(events,QList<int>({1,0}));
   events.clear();QVERIFY(pad.perform(press));QTest::qWait(130);
   QVERIFY(pad.held(source));QCOMPARE(events,QList<int>({1}));
   QVERIFY(pad.perform(GameController::buttonAction(source,"Release")));
   QVERIFY(!pad.held(source));
   QVERIFY(pad.perform(GameController::buttonAction(source,"Toggle")));
   QVERIFY(pad.held(source));
   QVERIFY(pad.perform(GameController::buttonAction(source,"Toggle")));
   QVERIFY(!pad.held(source));
  }
  for(const QString excluded:{"GamepadARelease","GamepadReset","GamepadLeftTrigger:1","GamepadLeftX:1","Sleep"})
   QVERIFY(GameController::buttonAction(excluded,"Press").isEmpty());
  QVERIFY(ActionCatalog::supported("CycleButtonMode"));
 }

 void catalog(){
  for(const auto &action:GameController::actions())QVERIFY(ActionCatalog::supported(action));
  for(const auto &bad:{"GamepadLeftX:nan","GamepadLeftX:1.1","GamepadLeftTrigger:-1","GamepadButton33Tap","GamepadATypo"})QVERIFY(!GameController::supports(bad));
 }
 void buttonsAndReset(){
  GameController pad;QMap<int,int> state;pad.output=[&](int,int code,int value){state[code]=value;return true;};
  QVERIFY(pad.perform("GamepadAPress"));QCOMPARE(state[BTN_SOUTH],1);
  QVERIFY(pad.perform("GamepadAToggle"));QCOMPARE(state[BTN_SOUTH],0);
  QVERIFY(pad.perform("GamepadBTap"));QCOMPARE(state[BTN_EAST],1);
  QTest::qWait(130);QCOMPARE(state[BTN_EAST],0);
  pad.perform("GamepadBTap");pad.reset();pad.perform("GamepadBPress");
  QTest::qWait(130);QCOMPARE(state[BTN_EAST],1);
  pad.perform("GamepadLeftTrigger:1");QCOMPARE(state[ABS_Z],32767);
  pad.reset();QCOMPARE(state[ABS_Z],0);QCOMPARE(state[BTN_EAST],0);
 }
 void analog(){
  GameController pad;QMap<int,int> state;pad.output=[&](int,int code,int value){state[code]=value;return true;};
  QVERIFY(pad.stick("Left",1,0));QCOMPARE(state[ABS_X],32767);
  QVERIFY(pad.stick("Left",.1,.1));QCOMPARE(state[ABS_X],0);QCOMPARE(state[ABS_Y],0);
  QVERIFY(pad.stick("Right",1,1));QVERIFY(state[ABS_RX]>23000&&state[ABS_RX]<23200);
  QVERIFY(!pad.stick("Left",qQNaN(),0));QVERIFY(!pad.stick("invalid",1,1));
  pad.reset();QCOMPARE(state[ABS_RX],0);QCOMPARE(state[ABS_RY],0);
 }
 void failure(){GameController pad;pad.output=[](int,int,int){return false;};QVERIFY(!pad.perform("GamepadAPress"));QVERIFY(!pad.error().isEmpty());}
 void layout(){auto layout=KeyboardLayout::fromXml(KeyboardPack::load(QStringLiteral(PACK_PATH "/game-controller.xml")));QVERIFY2(!layout.contains("error"),qPrintable(layout["error"].toString()));QCOMPARE(layout["keys"].toList().first().toMap()["kind"].toString(),QString("Joystick"));}
};
QTEST_GUILESS_MAIN(GameControllerTests)
#include "GameControllerTests.moc"
