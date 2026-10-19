#pragma once
#include <QVariantMap>
#include <QStringList>
class KeyboardPack {
public:
    static QVariantMap load(const QString &path);
    static QVariantMap audit(const QString &directory);
};
