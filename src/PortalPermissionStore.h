#pragma once
#include <QString>
class PortalPermissionStore {
public:
 explicit PortalPermissionStore(QString path):path(std::move(path)){}
 QString take(QString &error)const;
 bool save(const QString &token,QString &error)const;
 bool forget(QString &error)const;
private:
 QString path;
};
