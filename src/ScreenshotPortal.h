#pragma once
#include <QObject>
#ifndef GAZE_GRID_WINDOWS
#include <QDBusContext>
#endif
#include <QImage>
#include <QRectF>
#include <QList>
#include <QTimer>
#include <QVariantMap>
class ScreenshotPortal:public QObject
#ifndef GAZE_GRID_WINDOWS
,protected QDBusContext
#endif
{
 Q_OBJECT
public:
 explicit ScreenshotPortal(QObject *parent=nullptr);
 ~ScreenshotPortal()override;
 void capture();
 void cancel();
 void startLive(int hz);
signals:
 void captured(QImage image);
 void failed(QString message);
 void liveFrame(QImage image);
private slots:
 void response(uint code,const QVariantMap &results);
private:
 QString requestPath;
 quint64 generation=0;
 QTimer timeout,liveTimer;
 QList<QRectF> captureDisplays;
 int captureDisplay=-1;
};
