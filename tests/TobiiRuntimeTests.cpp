#include <QtTest>
#include <QTemporaryDir>
#include "TobiiRuntimePaths.h"
#include "TobiiTimeSync.h"
#ifdef GAZEGRID_WINDOWS
#include "WindowsTobiiLibrary.h"
#endif
class TobiiRuntimeTests:public QObject {
 Q_OBJECT
private slots:
 void synchronizationRunsPeriodicallyDuringLongSessions(){
  // Cross the 32-bit microsecond and millisecond boundaries without real waits.
  // Simulate +/-100ppm drift between synchronization attempts.
  for(const int direction:{-1,1})for(const qint64 start:{qint64(0),qint64(1)<<31,(qint64(1)<<31)*1000}){
   TobiiTimeSync sync;
   qint64 lastSync=start;int calls=0;
   for(qint64 elapsed=0;elapsed<=12LL*60*60*1000000;elapsed+=100000){
    const auto now=start+elapsed+1000000;
    sync.update(now,[&]{lastSync=now;++calls;return true;});
    const qint64 deviceDrift=direction*(now-lastSync)/10000;
    QVERIFY(qAbs(deviceDrift)<=3000);
   }
   QCOMPARE(calls,1441);
  }
 }
 void synchronizationRetriesPacketLossWithoutFloodingCallbacks(){
  TobiiTimeSync sync;int calls=0;int polls=0;
  auto poll=[&](qint64 now){
   ++polls;sync.update(now,[&]{++calls;return calls>=3;});
  };
  // Regular 8ms polling must continue while failed sync operations are retried.
  for(qint64 now=0;now<2000000;now+=8000)poll(now);
  QCOMPARE(calls,2);QCOMPARE(polls,250);
  poll(2000000);QCOMPARE(calls,3);
  poll(31999999);QCOMPARE(calls,3);
  poll(32000000);QCOMPARE(calls,4);
 }
 void reconnectionSynchronizesBeforeTheNextSample(){
  TobiiTimeSync sync;int calls=0;
  auto synchronize=[&]{++calls;return true;};
  sync.update(1000000,synchronize);QCOMPARE(calls,1);
  sync.update(2000000,synchronize);QCOMPARE(calls,1);
  sync.reset();
  sync.update(2000001,synchronize);QCOMPARE(calls,2);
  sync.update(32000000,synchronize);QCOMPARE(calls,2);
  sync.update(32000001,synchronize);QCOMPARE(calls,3);
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
