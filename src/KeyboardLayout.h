#pragma once
#include <QVariantMap>
class KeyboardLayout {
public:
    static QVariantMap fromXml(const QVariantMap &root);
};
