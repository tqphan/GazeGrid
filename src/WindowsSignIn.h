#pragma once
#include <QJsonObject>
#include <QStringList>
namespace WindowsSignIn {
bool secureDesktop();
QJsonObject profile();
QJsonObject validatedProfile(const QJsonObject &source);
bool configure(bool enabled,const QJsonObject &profile);
int setup(const QStringList &arguments);
}
