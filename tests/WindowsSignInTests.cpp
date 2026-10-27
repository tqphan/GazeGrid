#include <QtTest>
#include "SignInRegistry.h"
#include "WindowsSignIn.h"
#include "ProfileStore.h"
class WindowsSignInTests:public QObject {
 Q_OBJECT
private slots:
 void retainsOtherAccessibilityTools(){
  using namespace SignInRegistry;
  const std::wstring before=L"osk,narrator";
  auto enabled=updated(before,true);QVERIFY(contains(enabled));
  QVERIFY(updated(enabled,true)==enabled);QVERIFY(updated(enabled,false)==before);
  QVERIFY(updated(L"osk,GazeGrid_GazeGrid_v1,narrator,GAZEGRID_GAZEGRID_V1",false)==before);
  QVERIFY(!contains(L"osk,,narrator"));QVERIFY(!contains(L"other_GazeGrid_GazeGrid_v1"));
  QVERIFY(contains(L"osk, GazeGrid_GazeGrid_v1 ,narrator"));
 }
 void sharedProfileOnlyContainsValidatedKeyboardPreferences(){
  auto values=ProfileStore::defaults();values["learning"]=true;values["predictionMode"]="context";values["magnifierEnabled"]=true;
  values["keyboardScale"]=120;values["startLayout"]="main-14x1-top";
  auto result=WindowsSignIn::validatedProfile(values);
  QCOMPARE(result["startLayout"].toString(),QString("main-14x1-top"));QCOMPARE(result["keyboardScale"].toInt(),120);
  QCOMPARE(result["predictionMode"].toString(),QString("off"));QVERIFY(!result["learning"].toBool());QVERIFY(!result["magnifierEnabled"].toBool());
  values["startLayout"]="../user-keyboard";QVERIFY(WindowsSignIn::validatedProfile(values).isEmpty());
  values["startLayout"]="minimized-off-screen-bottom";QVERIFY(WindowsSignIn::validatedProfile(values).isEmpty());
  values["startLayout"]="main-14x1-bottom";values["dwellMs"]=-1;QVERIFY(WindowsSignIn::validatedProfile(values).isEmpty());
 }
};
QTEST_GUILESS_MAIN(WindowsSignInTests)
#include "WindowsSignInTests.moc"
