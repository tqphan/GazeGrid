#include <QtTest>
#include <QTemporaryDir>
#include <QSemaphore>
#include <QScopeGuard>
#include <atomic>
#include <thread>
#include "TobiiRuntimePaths.h"
#include "TobiiCallbacks.h"
#ifdef GAZEGRID_WINDOWS
#include "WindowsTobiiLibrary.h"
#endif
namespace {
enum class CallbackError { Success,TimedOut,ConnectionFailed,Internal };
}
class TobiiRuntimeTests:public QObject {
 Q_OBJECT
private slots:
 void callbacksFollowEachSubscribedStreamWake(){
  const QList<QList<int>> batches{{1,2},{3},{4,5,6}};
  QStringList events{"subscribe"};QList<int> pending,received;int cycle=0;
  auto wait=[&]{events<<QString("wait:%1").arg(cycle);pending=batches.at(cycle);return CallbackError::Success;};
  auto process=[&]{
   events<<QString("process:%1").arg(cycle++);
   for(const int sample:pending){received<<sample;events<<QString("sample:%1").arg(sample);}
   pending.clear();return CallbackError::Success;
  };
  for(int i=0;i<batches.size();++i){
   const auto result=tobiiWaitAndProcess(wait,process,[]{return false;},CallbackError::Success,CallbackError::TimedOut);
   QVERIFY(result.has_value());QCOMPARE(*result,CallbackError::Success);
  }
  QCOMPARE(received,QList<int>({1,2,3,4,5,6}));
  QCOMPARE(events,QStringList({"subscribe","wait:0","process:0","sample:1","sample:2",
   "wait:1","process:1","sample:3","wait:2","process:2","sample:4","sample:5","sample:6"}));
 }
 void repeatedWaitTimeoutsDoNotDispatchCallbacks(){
  int waits=0,callbacks=0;
  for(int i=0;i<1000;++i){
   const auto result=tobiiWaitAndProcess([&]{++waits;return CallbackError::TimedOut;},
    [&]{++callbacks;return CallbackError::ConnectionFailed;},[]{return false;},CallbackError::Success,CallbackError::TimedOut);
   QVERIFY(result.has_value());QCOMPARE(*result,CallbackError::Success);
  }
  QCOMPARE(waits,1000);QCOMPARE(callbacks,0);
 }
 void interruptedBeforeWaitDoesNotTouchTheSdk(){
  int waits=0,callbacks=0,reconnects=0;
  const auto result=tobiiWaitAndProcess([&]{++waits;return CallbackError::ConnectionFailed;},
   [&]{++callbacks;return CallbackError::Success;},[]{return true;},CallbackError::Success,CallbackError::TimedOut);
  if(result&&*result==CallbackError::ConnectionFailed)++reconnects;
  QVERIFY(!result.has_value());QCOMPARE(waits,0);QCOMPARE(callbacks,0);QCOMPARE(reconnects,0);
 }
 void callbackProcessingWaitsForTheSdkWake(){
  QSemaphore waiting,ready;std::atomic<bool> processed=false;
  std::optional<CallbackError> result;
  std::thread worker([&]{
   result=tobiiWaitAndProcess([&]{waiting.release();ready.acquire();return CallbackError::Success;},
    [&]{processed=true;return CallbackError::Success;},[]{return false;},CallbackError::Success,CallbackError::TimedOut);
  });
  // Every assertion exit releases the mock SDK wait and joins the worker.
  const auto cleanup=qScopeGuard([&]{ready.release();if(worker.joinable())worker.join();});
  QVERIFY(waiting.tryAcquire(1,1000));QVERIFY(!processed.load());
  ready.release();worker.join();
  QVERIFY(processed.load());QVERIFY(result.has_value());QCOMPARE(*result,CallbackError::Success);
 }
 void interruptedDuringWaitDoesNotProcessOrReconnect(){
  QSemaphore waiting,ready;std::atomic<bool> interrupted=false;int callbacks=0,reconnects=0;
  std::optional<CallbackError> result;
  std::thread worker([&]{
   result=tobiiWaitAndProcess([&]{waiting.release();ready.acquire();return CallbackError::ConnectionFailed;},
    [&]{++callbacks;return CallbackError::Success;},[&]{return interrupted.load();},CallbackError::Success,CallbackError::TimedOut);
   if(result&&*result==CallbackError::ConnectionFailed)++reconnects;
  });
  const auto cleanup=qScopeGuard([&]{ready.release();if(worker.joinable())worker.join();});
  QVERIFY(waiting.tryAcquire(1,1000));
  interrupted=true;ready.release();worker.join();
  QVERIFY(!result.has_value());QCOMPARE(callbacks,0);QCOMPARE(reconnects,0);
 }
 void waitAndProcessingErrorsPropagate_data(){
  QTest::addColumn<int>("waitError");QTest::addColumn<int>("processError");
  QTest::addColumn<int>("expectedError");QTest::addColumn<int>("expectedCallbacks");
  const int ok=int(CallbackError::Success),connection=int(CallbackError::ConnectionFailed),internal=int(CallbackError::Internal);
  QTest::newRow("wait connection failure")<<connection<<ok<<connection<<0;
  QTest::newRow("wait internal failure")<<internal<<ok<<internal<<0;
  QTest::newRow("process connection failure")<<ok<<connection<<connection<<1;
  QTest::newRow("process internal failure")<<ok<<internal<<internal<<1;
  QTest::newRow("success")<<ok<<ok<<ok<<1;
 }
 void waitAndProcessingErrorsPropagate(){
  QFETCH(int,waitError);QFETCH(int,processError);QFETCH(int,expectedError);QFETCH(int,expectedCallbacks);
  int waits=0,callbacks=0;
  const auto result=tobiiWaitAndProcess([&]{++waits;return CallbackError(waitError);},
   [&]{++callbacks;return CallbackError(processError);},[]{return false;},CallbackError::Success,CallbackError::TimedOut);
  QVERIFY(result.has_value());QCOMPARE(int(*result),expectedError);QCOMPARE(waits,1);QCOMPARE(callbacks,expectedCallbacks);
 }
 void discoveryIsConfinedToVendorTrees(){
  QTemporaryDir dir;QVERIFY(dir.isValid());
  const auto vendor=dir.path()+"/Tobii";
  QVERIFY(QDir().mkpath(vendor+"/Runtime/x64"));
  QFile inside(vendor+"/Runtime/x64/tobii_stream_engine.dll");QVERIFY(inside.open(QIODevice::WriteOnly));inside.close();
  QFile outside(dir.path()+"/tobii_stream_engine.dll");QVERIFY(outside.open(QIODevice::WriteOnly));outside.close();
  QCOMPARE(tobiiRuntimePaths({vendor,vendor}),QStringList{QFileInfo(inside).canonicalFilePath()});
  QVERIFY(tobiiRuntimePaths({dir.path()+"/absent"}).isEmpty());
#ifndef GAZEGRID_WINDOWS
  QVERIFY(QFile::link(dir.path(),vendor+"/outside"));
  QCOMPARE(tobiiRuntimePaths({vendor}),QStringList{QFileInfo(inside).canonicalFilePath()});
#endif
 }
#ifdef GAZEGRID_WINDOWS
 void missingRuntimeHasSetupGuidance(){
  QTemporaryDir dir;QVERIFY(dir.isValid());WindowsTobiiLibrary library;
  QVERIFY(!library.loadFromRoots({dir.path()}));
  QVERIFY(library.errorString().contains("Tobii Experience"));
  QVERIFY(library.errorString().contains("Mouse dwell"));
 }
 void invalidDllDoesNotLoad(){
  QTemporaryDir dir;QVERIFY(dir.isValid());
  QFile fake(dir.path()+"/tobii_stream_engine.dll");QVERIFY(fake.open(QIODevice::WriteOnly));fake.write("invalid");fake.close();
  WindowsTobiiLibrary library;QVERIFY(!library.loadFromRoots({dir.path()}));
  QVERIFY(library.errorString().contains("Windows error"));
 }
#endif
};
QTEST_GUILESS_MAIN(TobiiRuntimeTests)
#include "TobiiRuntimeTests.moc"
