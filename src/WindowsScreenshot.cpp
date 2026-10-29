#include "ScreenshotPortal.h"
#include <QGuiApplication>
#include <QScreen>
#include <QPixmap>
#include <QWindow>
#include <qt_windows.h>
ScreenshotPortal::ScreenshotPortal(QObject *parent):QObject(parent){
 liveTimer.setTimerType(Qt::PreciseTimer);
 connect(&liveTimer,&QTimer::timeout,this,[this]{
  const auto screen=QGuiApplication::primaryScreen();
  const auto image=screen?screen->grabWindow(0).toImage():QImage();
  if(image.isNull()){cancel();emit failed("Windows desktop capture failed");}
  else emit liveFrame(image);
 });
}
ScreenshotPortal::~ScreenshotPortal(){cancel();}
void ScreenshotPortal::cancel(){++generation;liveTimer.stop();}
void ScreenshotPortal::startLive(int hz){
 if(hz!=15&&hz!=30&&hz!=45&&hz!=60)return;
 if(QGuiApplication::platformName()!="windows")return;
 for(auto window:QGuiApplication::allWindows()){
  if(window->objectName()!="targetWindow")continue;
  DWORD affinity=0;
  if(!GetWindowDisplayAffinity(reinterpret_cast<HWND>(window->winId()),&affinity)||affinity!=0x00000011){
   emit failed("Cannot exclude magnifier from live desktop capture");return;
  }
 }
 liveTimer.start(qRound(1000.0/hz));
}
void ScreenshotPortal::capture(){
 cancel();const auto epoch=generation;
 QTimer::singleShot(0,this,[this,epoch]{
  if(epoch!=generation)return;
  const auto screen=QGuiApplication::primaryScreen();
  const auto image=screen?screen->grabWindow(0).toImage():QImage();
  if(image.isNull())emit failed("Windows desktop capture failed");else emit captured(image);
 });
}
void ScreenshotPortal::response(uint,const QVariantMap &){}
