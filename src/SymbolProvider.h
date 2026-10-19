#pragma once
#include <QQuickImageProvider>
#include <QJsonObject>
class SymbolProvider:public QQuickImageProvider{
public:
    SymbolProvider();
    QImage requestImage(const QString &id,QSize *size,const QSize &requestedSize)override;
private:
    QJsonObject paths;
};
