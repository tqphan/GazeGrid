#pragma once
#include <QString>
#include <QPair>
namespace KeyNames {
QString canonical(QString name);
QString fromLinuxCode(int code);
int linuxCode(const QString &name);
QPair<QString,bool> usCharacter(QChar c);
QChar usTypedCharacter(QChar c, bool shifted);
QString functionKey(const QString &action);
}
