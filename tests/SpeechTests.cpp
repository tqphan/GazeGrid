#include <QtTest>
#include <QSemaphore>
#include <atomic>
#include "SpeechOutput.h"
class SpeechTests:public QObject {
 Q_OBJECT
private slots:
 void ownsConnectionAndStopsOnlyThroughThatBackend(){
  std::atomic<int> opens=0,says=0,cancels=0,closes=0;std::atomic<bool> offMain=false;
  {SpeechOutput speech({[&]{++opens;offMain=QThread::currentThread()!=QCoreApplication::instance()->thread();return true;},
    [&](const QString &text){if(text=="hello")++says;return true;},[&]{++cancels;return true;},[&]{++closes;}});
   QSignalSpy status(&speech,&SpeechOutput::status);speech.speak("hello");QTRY_COMPARE(says.load(),1);QVERIFY(offMain);
   speech.stop();QTRY_COMPARE(cancels.load(),1);QCOMPARE(opens.load(),1);QVERIFY(status.count()>=1);
  }
  QCOMPARE(closes.load(),1);QCOMPARE(cancels.load(),2);
 }
 void unavailableSpeechReportsFailureWithoutSending(){
  int says=0;SpeechOutput speech({[]{return false;},[&](const QString &){++says;return true;},[]{return true;},[]{}});
  QSignalSpy status(&speech,&SpeechOutput::status);speech.speak("hello");QTRY_COMPARE(status.count(),1);
  QVERIFY(status[0][0].toString().contains("unavailable"));QCOMPARE(says,0);
 }
 void cancelledPendingConnectionDoesNotSpeakLater(){
  QSemaphore entered,release;std::atomic<int> says=0,cancels=0;
  SpeechOutput speech({[&]{entered.release();release.acquire();return true;},[&](const QString &){++says;return true;},[&]{++cancels;return true;},[]{}});
  speech.speak("stale");const bool started=entered.tryAcquire(1,1000);speech.stop();release.release();QVERIFY(started);
  QTRY_COMPARE(cancels.load(),1);QCOMPARE(says.load(),0);
 }
 void failedSubmissionReconnectsOnNextRequest(){
  std::atomic<int> opens=0,says=0,closes=0;
  SpeechOutput speech({[&]{++opens;return true;},[&](const QString &){return ++says>1;},[]{return true;},[&]{++closes;}});
  QSignalSpy status(&speech,&SpeechOutput::status);speech.speak("first");QTRY_COMPARE(status.count(),1);QCOMPARE(closes.load(),1);
  speech.speak("second");QTRY_COMPARE(status.count(),2);QCOMPARE(opens.load(),2);QCOMPARE(status[1][0].toString(),QString("Speech queued"));
 }
};
QTEST_GUILESS_MAIN(SpeechTests)
#include "SpeechTests.moc"
