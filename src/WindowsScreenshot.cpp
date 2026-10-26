#include "ScreenshotPortal.h"
#include <QGuiApplication>
#include <QScreen>
#include <QPixmap>
ScreenshotPortal::ScreenshotPortal(QObject *parent):QObject(parent){}
ScreenshotPortal::~ScreenshotPortal(){cancel();}
void ScreenshotPortal::cancel(){++generation;}
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
