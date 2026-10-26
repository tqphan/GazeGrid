#pragma once
#include <QObject>
#ifndef _WIN32
#include <QDBusContext>
#endif
#include <QImage>
#include <QTimer>
#include <QVariantMap>
class ScreenshotPortal:public QObject
#ifndef _WIN32
,protected QDBusContext
#endif
{
 Q_OBJECT
public:
 explicit ScreenshotPortal(QObject *parent=nullptr);
 ~ScreenshotPortal()override;
 void capture();
 void cancel();
signals:
 void captured(QImage image);
 void failed(QString message);
private slots:
 void response(uint code,const QVariantMap &results);
private:
 QString requestPath;
 quint64 generation=0;
 QTimer timeout;
};
