#include <QtTest>
#include <QTemporaryDir>
#include "TobiiRuntimePaths.h"
#ifdef _WIN32
#include "WindowsTobiiLibrary.h"
#endif
class TobiiRuntimeTests:public QObject {
 Q_OBJECT
private slots:
 void discoveryIsConfinedToVendorTrees(){
  QTemporaryDir dir;QVERIFY(dir.isValid());
  const auto vendor=dir.path()+"/Tobii";
  QVERIFY(QDir().mkpath(vendor+"/Runtime/x64"));
  QFile inside(vendor+"/Runtime/x64/tobii_stream_engine.dll");QVERIFY(inside.open(QIODevice::WriteOnly));inside.close();
  QFile outside(dir.path()+"/tobii_stream_engine.dll");QVERIFY(outside.open(QIODevice::WriteOnly));outside.close();
  QCOMPARE(tobiiRuntimePaths({vendor,vendor}),QStringList{QFileInfo(inside).canonicalFilePath()});
  QVERIFY(tobiiRuntimePaths({dir.path()+"/absent"}).isEmpty());
#ifndef _WIN32
  QVERIFY(QFile::link(dir.path(),vendor+"/outside"));
  QCOMPARE(tobiiRuntimePaths({vendor}),QStringList{QFileInfo(inside).canonicalFilePath()});
#endif
 }
#ifdef _WIN32
 void missingRuntimeHasSetupGuidance(){
  QTemporaryDir dir;QVERIFY(dir.isValid());WindowsTobiiLibrary library;
  QVERIFY(!library.loadFromRoots({dir.path()}));
  QVERIFY(library.errorString().contains("Tobii Gaming"));
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
