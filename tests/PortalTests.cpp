#include <QtTest>
#include <QDBusConnection>
#include <QDBusContext>
#include <QDBusObjectPath>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QDir>
#include "PortalInput.h"
#include "PortalPermissionStore.h"
class FakeDesktop:public QObject,protected QDBusContext {
 Q_OBJECT
 Q_CLASSINFO("D-Bus Interface","org.freedesktop.portal.RemoteDesktop")
 Q_PROPERTY(uint version MEMBER version)
public:
 uint version=2,code=0;QVariantMap selected;QString token="replacement";
 QDBusObjectPath respond(QVariantMap options,QVariantMap result,uint responseCode=0){
  QString sender=message().service().mid(1);sender.replace('.','_');
  QString path="/org/freedesktop/portal/desktop/request/"+sender+"/"+options["handle_token"].toString();
  QTimer::singleShot(10,this,[path,result,responseCode]{auto signal=QDBusMessage::createSignal(path,"org.freedesktop.portal.Request","Response");signal.setArguments({responseCode,result});QDBusConnection::sessionBus().send(signal);});
  return QDBusObjectPath(path);
 }
public slots:
 QDBusObjectPath CreateSession(QVariantMap options){return respond(options,{{"session_handle","/org/freedesktop/portal/desktop/session/test"}});}
 QDBusObjectPath SelectDevices(QDBusObjectPath,QVariantMap options){selected=options;return respond(options,{},code);}
 QDBusObjectPath Start(QDBusObjectPath,QString,QVariantMap options){return respond(options,{{"devices",uint(3)},{"restore_token",token}});}
};
class PortalTests:public QObject {
 Q_OBJECT
 FakeDesktop fake;
 PortalPermissionStore store(){return PortalPermissionStore(QDir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)).filePath("desktop-permission.json"));}
private slots:
 void initTestCase(){auto bus=QDBusConnection::sessionBus();QVERIFY(bus.registerService("org.freedesktop.portal.Desktop"));QVERIFY(bus.registerObject("/org/freedesktop/portal/desktop",&fake,QDBusConnection::ExportAllSlots|QDBusConnection::ExportAllProperties));}
 void init(){QString error;QVERIFY(store().forget(error));fake.version=2;fake.code=0;fake.selected.clear();fake.token="replacement";}
 void defaultsDoNotRequestPersistence(){PortalInput portal;portal.connectPortal();QTRY_VERIFY(portal.ready());QVERIFY(!fake.selected.contains("persist_mode"));QVERIFY(!fake.selected.contains("restore_token"));QString error;QVERIFY(store().take(error).isEmpty());}
 void remembersAndRotatesSingleUseToken(){
  QString error;QVERIFY(store().save("previous",error));PortalInput portal;portal.setRememberPermission(true);portal.connectPortal();QTRY_VERIFY(portal.ready());
  QCOMPARE(fake.selected["persist_mode"].toUInt(),2u);QCOMPARE(fake.selected["restore_token"].toString(),QString("previous"));portal.close();
  fake.token="rotated";portal.connectPortal();QTRY_VERIFY(portal.ready());QCOMPARE(fake.selected["restore_token"].toString(),QString("replacement"));QCOMPARE(store().take(error),QString("rotated"));
 }
 void oldPortalOmitsUnsupportedOptions(){fake.version=1;PortalInput portal;portal.setRememberPermission(true);portal.connectPortal();QTRY_VERIFY(portal.ready());QVERIFY(!fake.selected.contains("persist_mode"));QString error;QVERIFY(store().take(error).isEmpty());}
 void denialConsumesTokenWithoutConnecting(){
  QString error;QVERIFY(store().save("previous",error));fake.code=1;PortalInput portal;QSignalSpy status(&portal,&PortalInput::status);portal.setRememberPermission(true);portal.connectPortal();
  QTRY_VERIFY(status.count()>=2);QCOMPARE(status.last()[0].toString(),QString("Desktop permission cancelled"));QVERIFY(!portal.ready());QVERIFY(store().take(error).isEmpty());
 }
 void forgetDisconnectsAndRemovesToken(){PortalInput portal;portal.setRememberPermission(true);portal.connectPortal();QTRY_VERIFY(portal.ready());portal.forgetPermission();QVERIFY(!portal.ready());QString error;QVERIFY(store().take(error).isEmpty());}
};
int main(int argc,char **argv){QTemporaryDir dir;if(!dir.isValid())return 1;qputenv("XDG_CONFIG_HOME",dir.path().toUtf8());QCoreApplication app(argc,argv);app.setOrganizationName("OptiKeyTests");app.setApplicationName("Portal");PortalTests tests;return QTest::qExec(&tests,argc,argv);}
#include "PortalTests.moc"
