#include "PortalPermissionStore.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QSaveFile>
#include <QLockFile>
#include <QJsonDocument>
#include <QJsonObject>
QString PortalPermissionStore::take(QString &error)const{
 error.clear();if(!QFile::exists(path))return {};
 QLockFile lock(path+".lock");if(!lock.tryLock()){error="Saved desktop permission is in use";return {};}
 QFile file(path);if(!file.open(QIODevice::ReadOnly)||file.size()>8192){error="Could not read saved desktop permission";return {};}
 auto object=QJsonDocument::fromJson(file.readAll()).object();file.close();
 if(!QFile::remove(path)){error="Could not consume saved desktop permission";return {};}
 if(object["version"].toDouble()!=1||!object["token"].isString()||object["token"].toString().size()>4096){error="Saved desktop permission was invalid";return {};}
 return object["token"].toString();
}
bool PortalPermissionStore::save(const QString &token,QString &error)const{
 error.clear();if(token.isEmpty()||token.size()>4096){error="Invalid desktop permission token";return false;}
 if(!QDir().mkpath(QFileInfo(path).absolutePath())){error="Could not create desktop permission directory";return false;}
 QLockFile lock(path+".lock");if(!lock.tryLock()){error="Saved desktop permission is in use";return false;}
 QSaveFile file(path);auto bytes=QJsonDocument(QJsonObject{{"version",1},{"token",token}}).toJson(QJsonDocument::Compact);
 if(!file.open(QIODevice::WriteOnly)||!file.setPermissions(QFileDevice::ReadOwner|QFileDevice::WriteOwner)||file.write(bytes)!=bytes.size()||!file.commit()){
  error="Could not privately save desktop permission";return false;
 }
 return true;
}
bool PortalPermissionStore::forget(QString &error)const{
 error.clear();if(!QFile::exists(path))return true;
 QLockFile lock(path+".lock");if(!lock.tryLock()||!QFile::remove(path)){error="Could not remove saved desktop permission";return false;}return true;
}
