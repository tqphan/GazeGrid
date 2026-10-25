#pragma once
#include <QVariantMap>
class KeyboardLayout {
public:
    static bool saveCanvasGeometry(const QString &path,QByteArray &source,const QVariantList &keys,QString &error);
    static QVariantMap fromXml(const QVariantMap &root);
};
