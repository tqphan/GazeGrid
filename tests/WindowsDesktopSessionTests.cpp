#include "WindowsDesktopSession.h"
#include <QtTest>
#include <QCoreApplication>
#include <QProcess>
#include <QScopeGuard>
#include <QTextStream>
#include <QUuid>
#include <thread>

namespace {
std::wstring prefix(){return (QStringLiteral("gaze-grid.Tests.")+QUuid::createUuid().toString(QUuid::WithoutBraces)).toStdWString();}
struct Desktop {
 HDESK handle=nullptr;
 std::wstring name;
 Desktop(){
  name=(QStringLiteral("gaze-gridTest")+QUuid::createUuid().toString(QUuid::WithoutBraces).remove('-')).toStdWString();
  handle=CreateDesktopW(name.c_str(),nullptr,nullptr,0,DESKTOP_CREATEWINDOW|DESKTOP_READOBJECTS|DESKTOP_WRITEOBJECTS,nullptr);
 }
 ~Desktop(){if(handle)CloseDesktop(handle);}
};
void hidden(QProcess &process,std::wstring *desktop=nullptr){
 process.setCreateProcessArgumentsModifier([desktop](QProcess::CreateProcessArguments *arguments){
  arguments->flags|=CREATE_NO_WINDOW;
  arguments->startupInfo->dwFlags|=STARTF_USESHOWWINDOW;
  arguments->startupInfo->wShowWindow=SW_HIDE;
  if(desktop)arguments->startupInfo->lpDesktop=desktop->data();
 });
}
int probe(const std::wstring &name,QByteArray &output,std::wstring *desktop=nullptr){
 QProcess process;hidden(process,desktop);
 process.start(QCoreApplication::applicationFilePath(),{"--instance-child",QString::fromStdWString(name),"--accessibility"});
 if(!process.waitForStarted(5000)||!process.waitForFinished(5000))return -1;
 output=process.readAllStandardOutput().trimmed();
 return process.exitStatus()==QProcess::NormalExit?process.exitCode():-2;
}
int child(const QStringList &arguments,bool hold){
 WindowsDesktop::Instance instance(arguments.value(2).toStdWString());
 QTextStream(stdout)<<(instance.owner()?"owner":instance.duplicate()?"duplicate":"error")<<Qt::endl;
 if(!instance.owner())return instance.duplicate()?10:20;
 if(hold){
  // QProcess supplies a pipe; no physical keyboard input is read. Closing the
  // parent pipe also releases the owner if a test exits early.
  char stop=0;DWORD received=0;
  ReadFile(GetStdHandle(STD_INPUT_HANDLE),&stop,1,&received,nullptr);
 }
 return 0;
}
}

class WindowsDesktopSessionTests:public QObject {
 Q_OBJECT
private slots:
 void keyboardInvocationsExcludeHelpersAndDiagnostics(){
  using WindowsDesktop::keyboardInvocation;
  QVERIFY(keyboardInvocation({"gaze-grid.exe"}));
  for(const QString argument:{"--accessibility","--settings","--gaze","--layout"})
   QVERIFY(keyboardInvocation({"gaze-grid.exe",argument,"keyboard-check"}));
  QVERIFY(keyboardInvocation({"gaze-grid.exe","--layout","my-target"}));
  for(const QString argument:{"--configure-sign-in","--editor","--diagnose","--validate-keyboards","--smoke",
      "--sign-in-ui-check","--input-check","--pointer-check","--input-target","--pointer-target"})
   QVERIFY2(!keyboardInvocation({"gaze-grid.exe",argument}),qPrintable(argument));
 }
 void brokerDuplicateExitsWhileOwnerRetainsItsLock(){
  const auto name=prefix();WindowsDesktop::Instance owner(name);QVERIFY(owner.owner());
  QByteArray output;
  QCOMPARE(probe(name,output),10);QCOMPARE(output,QByteArray("duplicate"));
  QCOMPARE(probe(name,output),10);QCOMPARE(output,QByteArray("duplicate"));
  QVERIFY(owner.owner());
 }
 void releaseAllowsAnotherProcessToOwnTheDesktop(){
  const auto name=prefix();
  {WindowsDesktop::Instance owner(name);QVERIFY(owner.owner());}
  QByteArray output;QCOMPARE(probe(name,output),0);QCOMPARE(output,QByteArray("owner"));
 }
 void processExitReleasesAnOwnedInstance(){
  const auto name=prefix();QProcess holder;hidden(holder);
  const auto cleanup=qScopeGuard([&]{if(holder.state()!=QProcess::NotRunning){holder.closeWriteChannel();if(!holder.waitForFinished(1000)){holder.kill();holder.waitForFinished(1000);}}});
  holder.start(QCoreApplication::applicationFilePath(),{"--instance-holder",QString::fromStdWString(name)});
  QVERIFY(holder.waitForStarted(5000));QVERIFY(holder.waitForReadyRead(5000));
  QCOMPARE(holder.readAllStandardOutput().trimmed(),QByteArray("owner"));
  QByteArray output;QCOMPARE(probe(name,output),10);
  holder.kill();QVERIFY(holder.waitForFinished(5000));
  QCOMPARE(probe(name,output),0);QCOMPARE(output,QByteArray("owner"));
 }
 void distinctDesktopsCanOwnIndependentInstances(){
  Desktop other;if(!other.handle)QSKIP(qPrintable(QString("Cannot create isolated test desktop: %1").arg(GetLastError())));
  const auto name=prefix();WindowsDesktop::Instance owner(name);QVERIFY(owner.owner());
  // A desktop in the current station can be named directly in STARTUPINFO.
  QByteArray output;QCOMPARE(probe(name,output,&other.name),0);QCOMPARE(output,QByteArray("owner"));
  QCOMPARE(probe(name,output),10);QCOMPARE(output,QByteArray("duplicate"));
 }
 void inactiveDesktopDoesNotAcceptInput(){
  Desktop other;if(!other.handle)QSKIP(qPrintable(QString("Cannot create isolated test desktop: %1").arg(GetLastError())));
  bool attached=false,active=true;
  std::thread worker([&]{attached=SetThreadDesktop(other.handle);if(attached)active=WindowsDesktop::inputDesktopActive();});
  worker.join();QVERIFY(attached);QVERIFY(!active);
 }
 void monitorSamplesInitiallyAndStartIsIdempotent(){
  WindowsDesktop::Monitor monitor;QSignalSpy changed(&monitor,&WindowsDesktop::Monitor::activeChanged);
  monitor.start();QCOMPARE(changed.size(),1);QCOMPARE(changed.front().front().toBool(),monitor.active());
  monitor.start();QCOMPARE(changed.size(),1);
 }
};
int main(int argc,char **argv){
 QCoreApplication application(argc,argv);const auto arguments=application.arguments();
 if(arguments.value(1)=="--instance-child")return child(arguments,false);
 if(arguments.value(1)=="--instance-holder")return child(arguments,true);
 WindowsDesktopSessionTests tests;return QTest::qExec(&tests,argc,argv);
}
#include "WindowsDesktopSessionTests.moc"
