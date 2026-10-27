#pragma once
#include <QJsonObject>
#include <QByteArray>
#include <QStringList>
namespace WindowsSignIn {
bool secureDesktop();
bool registered();
bool notify(bool running);
bool enabled();
QJsonObject profile();
QJsonObject profileFromData(const QByteArray &data);
QJsonObject validatedProfile(const QJsonObject &source);
bool configure(bool enabled,const QJsonObject &profile);
int setup(const QStringList &arguments);
}
