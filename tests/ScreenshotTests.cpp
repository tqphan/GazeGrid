#include <QtTest>
#include <QDBusConnection>
#include <QDBusContext>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QTemporaryDir>
#include <QUrl>
#include "ScreenshotPortal.h"
class FakeScreenshot:public QObject,protected QDBusContext {
 Q_OBJECT
 Q_CLASSINFO("D-Bus Interface","org.freedesktop.portal.Screenshot")
public:
 QString uri;uint code=0;int delay=20;bool wrongHandle=false;
public slots:
 QDBusObjectPath Screenshot(QString,QVariantMap options){
  QString sender=message().service().mid(1);sender.replace('.','_');
  QString path="/org/freedesktop/portal/desktop/request/"+sender+"/"+options["handle_token"].toString();
  const auto result=QVariantMap{{"uri",uri}};const auto responseCode=code;
  QTimer::singleShot(delay,this,[path,result,responseCode]{
   auto signal=QDBusMessage::createSignal(path,"org.freedesktop.portal.Request","Response");
   signal.setArguments({responseCode,result});QDBusConnection::sessionBus().send(signal);
  });
  return QDBusObjectPath(wrongHandle?path+"_unexpected":path);
 }
};
class ScreenshotTests:public QObject {
 Q_OBJECT
 QTemporaryDir dir;FakeScreenshot fake;
private slots:
 void initTestCase(){
  auto bus=QDBusConnection::sessionBus();QVERIFY(bus.registerService("org.freedesktop.portal.Desktop"));
  QVERIFY(bus.registerObject("/org/freedesktop/portal/desktop",&fake,QDBusConnection::ExportAllSlots));
  QImage image(200,100,QImage::Format_RGB32);image.fill(Qt::blue);QVERIFY(image.save(dir.filePath("capture.png")));
 }
 void init(){fake.uri=QUrl::fromLocalFile(dir.filePath("capture.png")).toString();fake.code=0;fake.delay=20;fake.wrongHandle=false;}
 void loadsPortalImage(){
  ScreenshotPortal portal;QSignalSpy images(&portal,&ScreenshotPortal::captured),errors(&portal,&ScreenshotPortal::failed);
  portal.capture();QTRY_COMPARE(images.count(),1);QCOMPARE(errors.count(),0);
  auto image=qvariant_cast<QImage>(images[0][0]);QCOMPARE(image.size(),QSize(200,100));QCOMPARE(image.pixelColor(0,0),QColor(Qt::blue));
 }
 void denialDoesNotReturnImage(){
  fake.code=1;ScreenshotPortal portal;QSignalSpy images(&portal,&ScreenshotPortal::captured),errors(&portal,&ScreenshotPortal::failed);
  portal.capture();QTRY_COMPARE(errors.count(),1);QCOMPARE(images.count(),0);
 }
 void cancellationIgnoresLateResponse(){
  fake.delay=150;ScreenshotPortal portal;QSignalSpy images(&portal,&ScreenshotPortal::captured),errors(&portal,&ScreenshotPortal::failed);
  portal.capture();QTest::qWait(30);portal.cancel();QTest::qWait(200);QCOMPARE(images.count(),0);QCOMPARE(errors.count(),0);
 }
 void rejectsNonlocalUri(){
  fake.uri="https://example.invalid/capture.png";ScreenshotPortal portal;QSignalSpy errors(&portal,&ScreenshotPortal::failed);
  portal.capture();QTRY_COMPARE(errors.count(),1);
 }
 void rejectsUnexpectedHandle(){
  fake.wrongHandle=true;ScreenshotPortal portal;QSignalSpy errors(&portal,&ScreenshotPortal::failed),images(&portal,&ScreenshotPortal::captured);
  portal.capture();QTRY_COMPARE(errors.count(),1);QTest::qWait(50);QCOMPARE(images.count(),0);
 }
};
QTEST_GUILESS_MAIN(ScreenshotTests)
#include "ScreenshotTests.moc"
