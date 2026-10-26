#include <QtTest>
#include "WindowsGamepad.h"
#include "GamepadCodes.h"
class WindowsGamepadTests:public QObject {
 Q_OBJECT
private slots:
 void buttonsAndAxes(){
  WindowsGamepad::Report r;int opens=0;
  WindowsGamepad pad({[&](QString &){++opens;return true;},[&](const auto &value,QString &){r=value;return true;},[]{}});
  const QList<QPair<int,uint16_t>> expected{{BTN_SOUTH,0x1000},{BTN_EAST,0x2000},{BTN_WEST,0x4000},{BTN_NORTH,0x8000},
   {BTN_TL,0x100},{BTN_TR,0x200},{BTN_SELECT,0x20},{BTN_START,0x10},{BTN_MODE,0x400},{BTN_THUMBL,0x40},{BTN_THUMBR,0x80},
   {BTN_DPAD_UP,1},{BTN_DPAD_DOWN,2},{BTN_DPAD_LEFT,4},{BTN_DPAD_RIGHT,8}};
  for(const auto &[code,mask]:expected){QVERIFY(pad.send(EV_KEY,code,1));QCOMPARE(r.buttons,mask);QVERIFY(pad.send(EV_KEY,code,0));QCOMPARE(r.buttons,0);}
  QVERIFY(pad.send(EV_ABS,ABS_X,-32767));QCOMPARE(r.leftX,-32767);
  QVERIFY(pad.send(EV_ABS,ABS_Y,-32767));QCOMPARE(r.leftY,32767);
  QVERIFY(pad.send(EV_ABS,ABS_RX,32767));QCOMPARE(r.rightX,32767);
  QVERIFY(pad.send(EV_ABS,ABS_RY,32767));QCOMPARE(r.rightY,-32767);
  QVERIFY(pad.send(EV_ABS,ABS_Z,16384));QCOMPARE(r.leftTrigger,128);
  QVERIFY(pad.send(EV_ABS,ABS_RZ,32767));QCOMPARE(r.rightTrigger,255);
  QCOMPARE(opens,1);
 }
 void overlappingTriggerAndDpadSources(){
  WindowsGamepad::Report r;
  WindowsGamepad pad({[](QString &){return true;},[&](const auto &value,QString &){r=value;return true;},[]{}});
  QVERIFY(pad.send(EV_ABS,ABS_Z,16384));QVERIFY(pad.send(EV_KEY,BTN_TL2,1));QCOMPARE(r.leftTrigger,255);
  QVERIFY(pad.send(EV_KEY,BTN_TL2,0));QCOMPARE(r.leftTrigger,128);
  QVERIFY(pad.send(EV_ABS,ABS_HAT0X,-1));QVERIFY(pad.send(EV_KEY,BTN_DPAD_LEFT,1));
  QVERIFY(pad.send(EV_ABS,ABS_HAT0X,0));QCOMPARE(r.buttons,4);
  QVERIFY(pad.send(EV_KEY,BTN_DPAD_LEFT,0));QCOMPARE(r.buttons,0);
  QVERIFY(pad.send(EV_ABS,ABS_HAT0Y,-1));QCOMPARE(r.buttons,1);
 }
 void invalidActionsDoNotConnect(){
  int opens=0;
  WindowsGamepad pad({[&](QString &){++opens;return true;},[](const auto &,QString &){return true;},[]{}});
  for(int code:{BTN_C,BTN_Z,BTN_TRIGGER_HAPPY,BTN_TRIGGER_HAPPY+31})QVERIFY(!pad.send(EV_KEY,code,1));
  for(int code:{ABS_THROTTLE,ABS_RUDDER,ABS_WHEEL})QVERIFY(!pad.send(EV_ABS,code,1));
  QVERIFY(!pad.send(EV_ABS,ABS_Z,-1));QVERIFY(!pad.send(EV_ABS,ABS_Y,32768));
  QVERIFY(!pad.send(EV_ABS,ABS_HAT0X,2));QVERIFY(!pad.send(EV_KEY,BTN_SOUTH,2));
  QCOMPARE(opens,0);QVERIFY(!pad.error().isEmpty());
 }
 void failureClearsStateAndReconnects(){
  QList<WindowsGamepad::Report> reports;int opens=0,closes=0;bool reject=false;
  WindowsGamepad pad({[&](QString &){++opens;return true;},[&](const auto &r,QString &error){reports<<r;if(reject){error="bus disconnected";return false;}return true;},[&]{++closes;}});
  QVERIFY(pad.send(EV_KEY,BTN_SOUTH,1));reject=true;
  QVERIFY(!pad.send(EV_ABS,ABS_X,123));QCOMPARE(pad.error(),QString("bus disconnected"));
  QCOMPARE(closes,1);QVERIFY(reports.last()==WindowsGamepad::Report{});
  reject=false;QVERIFY(pad.send(EV_KEY,BTN_EAST,1));QCOMPARE(opens,2);
  QCOMPARE(reports.last().buttons,0x2000);QCOMPARE(reports.last().leftX,0);
  pad.close();QCOMPARE(closes,2);QVERIFY(reports.last()==WindowsGamepad::Report{});
  pad.close();QCOMPARE(closes,2);
 }
 void failedOpenRetriesAndDestructorNeutralizes(){
  int opens=0,closes=0;WindowsGamepad::Report last;
  {
   WindowsGamepad pad({[&](QString &error){if(++opens==1){error="Install ViGEmBus";return false;}return true;},
    [&](const auto &r,QString &){last=r;return true;},[&]{++closes;}});
   QVERIFY(!pad.send(EV_KEY,BTN_SOUTH,1));QCOMPARE(closes,1);QVERIFY(pad.error().contains("ViGEmBus"));
   QVERIFY(pad.send(EV_KEY,BTN_SOUTH,1));QVERIFY(pad.error().isEmpty());QCOMPARE(last.buttons,0x1000);
  }
  QCOMPARE(closes,2);QVERIFY(last==WindowsGamepad::Report{});
 }
};
QTEST_APPLESS_MAIN(WindowsGamepadTests)
#include "WindowsGamepadTests.moc"
