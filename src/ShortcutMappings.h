#pragma once
#include <QJsonObject>
#include <QVariantList>
namespace ShortcutMappings {
struct Result {QVariantList commands;QString message;bool allowed=false;};
QJsonObject parse(const QByteArray &json,QString &error);
Result resolve(const QVariantList &commands,const QJsonObject &profile,bool literal);
}
