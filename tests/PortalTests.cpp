#include <QtTest>
#include <QDBusConnection>
#include <QDBusContext>
#include <QDBusAbstractAdaptor>
#include <QDBusObjectPath>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QDir>
#include <QProcess>
#include <QFile>
#include "PortalInput.h"
#include "PortalPermissionStore.h"
#include "ProfileStore.h"
class FakeRegistry:public QDBusAbstractAdaptor {
 Q_OBJECT
 Q_CLASSINFO("D-Bus Interface","org.freedesktop.host.portal.Registry")
public:
 explicit FakeRegistry(QObject *parent):QDBusAbstractAdaptor(parent){}
 QString appId;
public slots:
 void Register(QString id,QVariantMap){appId=id;}
};
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
 FakeRegistry registry{&fake};
 PortalPermissionStore store(){return PortalPermissionStore(QDir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)).filePath("desktop-permission.json"));}
private slots:
 void initTestCase(){auto bus=QDBusConnection::sessionBus();QVERIFY(bus.registerService("org.freedesktop.portal.Desktop"));QVERIFY(bus.registerObject("/org/freedesktop/portal/desktop",&fake,QDBusConnection::ExportAllSlots|QDBusConnection::ExportAllProperties|QDBusConnection::ExportAdaptors));}
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
 void appAutomaticallyRestoresRememberedPermission(){
  const auto config=QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
  QString error;auto preferences=ProfileStore::defaults();preferences["rememberDesktop"]=true;
  QVERIFY(ProfileStore(QDir(config).filePath("profile.json")).save(preferences,error,true));
  QVERIFY(store().save("startup-token",error));
  QProcess app;auto environment=QProcessEnvironment::systemEnvironment();
  environment.insert("GAZEGRID_CONFIG_DIR",config);
  environment.insert("QT_QPA_PLATFORM","offscreen");environment.insert("QT_QUICK_BACKEND","software");
  environment.insert("XDG_CACHE_HOME",QDir(config).filePath("cache"));
  app.setProcessEnvironment(environment);
  app.start(QDir(QCoreApplication::applicationDirPath()).filePath("gazegrid"),QStringList{});
  QVERIFY(app.waitForStarted());
  QTRY_COMPARE(fake.selected.value("restore_token").toString(),QString("startup-token"));
  QCOMPARE(registry.appId,QString("org.gazegrid.GazeGrid"));
  QCOMPARE(fake.selected.value("persist_mode").toUInt(),2u);
  QTRY_VERIFY(QFile::exists(QDir(config).filePath("desktop-permission.json")));
  app.terminate();QVERIFY(app.waitForFinished());
  QCOMPARE(store().take(error),QString("replacement"));
 }
};
int main(int argc,char **argv){QTemporaryDir dir;if(!dir.isValid())return 1;qputenv("XDG_CONFIG_HOME",dir.path().toUtf8());QCoreApplication app(argc,argv);app.setOrganizationName("GazeGridTests");app.setApplicationName("Portal");PortalTests tests;return QTest::qExec(&tests,argc,argv);}
#include "PortalTests.moc"
