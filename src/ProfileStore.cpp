#include "ProfileStore.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QJsonDocument>
#include <QSaveFile>
#include <QRegularExpression>

namespace {
QJsonObject read(const QString &path) {
    QFile file(path);
    if(!file.open(QIODevice::ReadOnly))return {};
    return QJsonDocument::fromJson(file.readAll()).object();
}
bool write(const QString &path, const QJsonObject &values, QString &error) {
    QSaveFile file(path);
    const auto bytes=QJsonDocument(values).toJson();
    if(!file.open(QIODevice::WriteOnly)||file.write(bytes)!=bytes.size()||!file.commit()){
        error="Could not save preferences: "+file.errorString();return false;
    }
    return true;
}
}
ProfileStore::ProfileStore(QString path):path(std::move(path)){}
QJsonObject ProfileStore::defaults(){
    return {{"version",1},{"dwellMs",750},{"literal",false},{"learning",false},
            {"startLayout","keyboard-alphabet-14x4-bottom"},{"keyboardScale",100},{"highContrast",false},{"rememberDesktop",false}};
}
bool ProfileStore::valid(const QJsonObject &v){
    static const QRegularExpression layout("^[A-Za-z0-9_-]+$");
    return v["version"].isDouble()&&v["version"].toDouble()==1
        &&v["dwellMs"].isDouble()&&v["dwellMs"].toDouble()==v["dwellMs"].toInt()
        &&v["dwellMs"].toInt()>=250&&v["dwellMs"].toInt()<=3000
        &&v["literal"].isBool()&&v["learning"].isBool()
        &&(!v.contains("keyboardScale")||(v["keyboardScale"].isDouble()&&v["keyboardScale"].toDouble()==v["keyboardScale"].toInt()&&v["keyboardScale"].toInt()>=50&&v["keyboardScale"].toInt()<=150))
        &&(!v.contains("highContrast")||v["highContrast"].isBool())
        &&(!v.contains("rememberDesktop")||v["rememberDesktop"].isBool())
        &&v["startLayout"].isString()&&layout.match(v["startLayout"].toString()).hasMatch();
}
ProfileStore::Loaded ProfileStore::load()const{
    auto current=read(path);
    if(valid(current))return {current,{}};
    if(current["version"].toDouble()>1)
        return {defaults(),"Preferences use a newer version. Using defaults; reset preferences to enable saving."};
    auto backup=read(path+".bak");
    if(valid(backup))return {backup,"Recovered preferences from the last known good copy."};
    return {defaults(),QFile::exists(path)||QFile::exists(path+".bak")
        ?"Preferences could not be read. Using defaults.":QString()};
}
bool ProfileStore::save(const QJsonObject &values,QString &error,bool reset)const{
    error.clear();
    if(!valid(values)){error="Invalid preferences";return false;}
    auto current=read(path);
    if(!reset&&current["version"].toDouble()>1){error="Newer preferences were preserved. Reset preferences to enable saving.";return false;}
    if(!QDir().mkpath(QFileInfo(path).absolutePath())){error="Could not create preferences directory";return false;}
    // Never replace a valid backup with corrupt input. On first save, seed recovery.
    auto backup=reset?values:valid(current)?current:read(path+".bak");
    if(!valid(backup))backup=values;
    if(!write(path+".bak",backup,error))return false;
    return write(path,values,error);
}
