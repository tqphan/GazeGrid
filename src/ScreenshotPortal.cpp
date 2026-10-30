#include "ScreenshotPortal.h"
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QImageReader>
#include <QUrl>
#include <QUuid>
#include <QGuiApplication>
#include <QScreen>
#include "DisplayMapping.h"
namespace {
const QString service="org.freedesktop.portal.Desktop",requestInterface="org.freedesktop.portal.Request";
}
ScreenshotPortal::ScreenshotPortal(QObject *parent):QObject(parent){
 timeout.setSingleShot(true);timeout.setInterval(60000);
 connect(&timeout,&QTimer::timeout,this,[this]{cancel();emit failed("Desktop capture timed out");});
}
ScreenshotPortal::~ScreenshotPortal(){cancel();}
void ScreenshotPortal::cancel(){
 ++generation;timeout.stop();if(requestPath.isEmpty())return;
 auto path=requestPath;requestPath.clear();
 auto bus=QDBusConnection::sessionBus();bus.disconnect(service,path,requestInterface,"Response",this,SLOT(response(uint,QVariantMap)));
 bus.asyncCall(QDBusMessage::createMethodCall(service,path,requestInterface,"Close"));
}
void ScreenshotPortal::capture(){
 cancel();captureDisplays.clear();captureDisplay=-1;
 if(qobject_cast<QGuiApplication*>(QCoreApplication::instance()))for(auto screen:QGuiApplication::screens()){
  if(screen==QGuiApplication::primaryScreen())captureDisplay=captureDisplays.size();
  captureDisplays<<QRectF(screen->geometry());
 }
 const auto epoch=generation;auto bus=QDBusConnection::sessionBus();
 if(!bus.isConnected()){emit failed("Desktop screenshot portal is unavailable");return;}
 QString token="gaze_grid_magnifier_"+QUuid::createUuid().toString(QUuid::Id128);
 QString sender=bus.baseService().mid(1);sender.replace('.','_');
 requestPath="/org/freedesktop/portal/desktop/request/"+sender+"/"+token;
 if(!bus.connect(service,requestPath,requestInterface,"Response",this,SLOT(response(uint,QVariantMap)))){
  cancel();emit failed("Could not subscribe to the screenshot portal");return;
 }
 auto call=QDBusMessage::createMethodCall(service,"/org/freedesktop/portal/desktop","org.freedesktop.portal.Screenshot","Screenshot");
 call.setArguments({QString(),QVariantMap{{"handle_token",token},{"interactive",false},{"modal",false}}});
 auto watcher=new QDBusPendingCallWatcher(bus.asyncCall(call),this);timeout.start();
 connect(watcher,&QDBusPendingCallWatcher::finished,this,[this,epoch](QDBusPendingCallWatcher *watcher){
  QDBusPendingReply<QDBusObjectPath> reply=*watcher;watcher->deleteLater();if(epoch!=generation)return;
  if(reply.isError()){auto error=reply.error().message();cancel();emit failed("Desktop capture: "+error);return;}
  if(reply.value().path()!=requestPath){cancel();emit failed("Unexpected screenshot portal response handle");}
 });
}
void ScreenshotPortal::response(uint code,const QVariantMap &results){
 if(!calledFromDBus()||message().path()!=requestPath)return;
 auto bus=QDBusConnection::sessionBus();bus.disconnect(service,requestPath,requestInterface,"Response",this,SLOT(response(uint,QVariantMap)));
 requestPath.clear();++generation;timeout.stop();
 if(code!=0){emit failed(code==1?"Desktop capture cancelled":"Desktop capture failed");return;}
 QUrl uri(results["uri"].toString());
 if(!uri.isLocalFile()){emit failed("Screenshot portal returned an unsupported image location");return;}
 QImageReader reader(uri.toLocalFile());auto size=reader.size();
 if(!size.isValid()||qint64(size.width())*size.height()>64000000){emit failed("Desktop capture has unsupported dimensions");return;}
 auto image=reader.read();
 if(image.isNull()){emit failed("Could not read desktop capture");return;}
 if(!captureDisplays.isEmpty()){
  const auto crop=DisplayMapping::screenshot(captureDisplays,captureDisplay,image.size());
  if(!crop.isValid()){emit failed("Desktop capture cannot be mapped to the target display; target selection cancelled");return;}
  image=image.copy(crop);
 }
 emit captured(image);
}

// The screenshot portal is a single-shot API; live capture requires a screencast session.
void ScreenshotPortal::startLive(int){}
