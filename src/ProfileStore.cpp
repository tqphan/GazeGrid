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
    return {{"clampGazeForKeyboard",false},{"clampGazeForPointSelection",true},{"invalidGazeToleranceMs",250},{"scrollSpeed",100},{"scrollDeadzone",50},{"scrollAxis",0},{"gazeEnabled",false},{"repeatIntervalMs",750},{"preserveTogglesBetweenLayouts",false},{"cycleModifiersOnHover",false},{"hideKeyboardDuringMouseActions",false},{"gazeIndicatorSize",18},{"gazeIndicatorOpacity",100},{"gazeIndicatorColor","#00bfff"},{"gazeIndicatorMorph",false},{"version",1},{"restoreCorner",2},{"restoreWidth",64},{"restoreHeight",64},{"restoreOpacity",100},{"gazeSmoothingLevel",1},{"smoothWhenChangingGazeTarget",false},{"dwellMs",750},{"dwellIndicatorStyle","bar"},{"literal",false},{"learning",false},{"predictionMode","context"},{"predictNextWord",true},{"addSpaceAfterCompletion",true},{"suggestSentenceStarters",true},
            {"startLayout","keyboard-alphabet-14x4-bottom"},{"keyboardScale",100},{"keyboardTheme","system"},{"shortLabelScale",150},{"longLabelScale",100},{"magnifierZoom",4},{"magnifierEnabled",false},{"progressiveZoom",false},{"zoomDurationMs",1500},{"targetDwellEnabled",true},{"targetStartDwellMs",750},{"targetConfirmDwellMs",1000},{"targetDwellRadius",40},{"rememberDesktop",false}};
}
bool ProfileStore::valid(const QJsonObject &v){
    static const QRegularExpression layout("^[A-Za-z0-9_-]+$");
    return v["version"].isDouble()&&v["version"].toDouble()==1
        &&(!v.contains("gazeEnabled")||v["gazeEnabled"].isBool())
        &&(!v.contains("clampGazeForKeyboard")||v["clampGazeForKeyboard"].isBool())
        &&(!v.contains("clampGazeForPointSelection")||v["clampGazeForPointSelection"].isBool())
        &&(!v.contains("invalidGazeToleranceMs")||(v["invalidGazeToleranceMs"].isDouble()&&v["invalidGazeToleranceMs"].toDouble()==v["invalidGazeToleranceMs"].toInt()&&v["invalidGazeToleranceMs"].toInt()>=0&&v["invalidGazeToleranceMs"].toInt()<=1000))
        &&(!v.contains("gazeIndicatorSize")||(v["gazeIndicatorSize"].isDouble()&&v["gazeIndicatorSize"].toDouble()==v["gazeIndicatorSize"].toInt()&&v["gazeIndicatorSize"].toInt()>=4&&v["gazeIndicatorSize"].toInt()<=120))
        &&(!v.contains("gazeIndicatorOpacity")||(v["gazeIndicatorOpacity"].isDouble()&&v["gazeIndicatorOpacity"].toDouble()==v["gazeIndicatorOpacity"].toInt()&&v["gazeIndicatorOpacity"].toInt()>=5&&v["gazeIndicatorOpacity"].toInt()<=100))
        &&(!v.contains("repeatIntervalMs")||(v["repeatIntervalMs"].isDouble()&&v["repeatIntervalMs"].toDouble()==v["repeatIntervalMs"].toInt()&&v["repeatIntervalMs"].toInt()>=100&&v["repeatIntervalMs"].toInt()<=2000))
        &&(!v.contains("preserveTogglesBetweenLayouts")||v["preserveTogglesBetweenLayouts"].isBool())
        &&(!v.contains("cycleModifiersOnHover")||v["cycleModifiersOnHover"].isBool())
        &&(!v.contains("hideKeyboardDuringMouseActions")||v["hideKeyboardDuringMouseActions"].isBool())
        &&(!v.contains("gazeIndicatorMorph")||v["gazeIndicatorMorph"].isBool())
        &&(!v.contains("gazeIndicatorColor")||(v["gazeIndicatorColor"].isString()&&QRegularExpression("^#[0-9a-fA-F]{6}$").match(v["gazeIndicatorColor"].toString()).hasMatch()))
        &&(!v.contains("dwellIndicatorStyle")||(v["dwellIndicatorStyle"].isString()&&QStringList{"bar","top-bar","fill","outline","ring","pie","collapse"}.contains(v["dwellIndicatorStyle"].toString())))
        &&v["dwellMs"].isDouble()&&v["dwellMs"].toDouble()==v["dwellMs"].toInt()
        &&v["dwellMs"].toInt()>=250&&v["dwellMs"].toInt()<=3000
        &&(!v.contains("gazeSmoothingLevel")||(v["gazeSmoothingLevel"].isDouble()&&v["gazeSmoothingLevel"].toDouble()==v["gazeSmoothingLevel"].toInt()&&v["gazeSmoothingLevel"].toInt()>=0&&v["gazeSmoothingLevel"].toInt()<=4))
        &&(!v.contains("smoothWhenChangingGazeTarget")||v["smoothWhenChangingGazeTarget"].isBool())
        &&(!v.contains("restoreCorner")||(v["restoreCorner"].isDouble()&&v["restoreCorner"].toDouble()==v["restoreCorner"].toInt()&&v["restoreCorner"].toInt()>=0&&v["restoreCorner"].toInt()<=3))
        &&(!v.contains("restoreOpacity")||(v["restoreOpacity"].isDouble()&&v["restoreOpacity"].toDouble()==v["restoreOpacity"].toInt()&&v["restoreOpacity"].toInt()>=10&&v["restoreOpacity"].toInt()<=100))
        &&(!v.contains("restoreWidth")||(v["restoreWidth"].isDouble()&&v["restoreWidth"].toDouble()==v["restoreWidth"].toInt()&&v["restoreWidth"].toInt()>=40&&v["restoreWidth"].toInt()<=128))
        &&(!v.contains("restoreHeight")||(v["restoreHeight"].isDouble()&&v["restoreHeight"].toDouble()==v["restoreHeight"].toInt()&&v["restoreHeight"].toInt()>=40&&v["restoreHeight"].toInt()<=128))
        &&(!v.contains("restoreSize")||(v["restoreSize"].isDouble()&&v["restoreSize"].toDouble()==v["restoreSize"].toInt()&&v["restoreSize"].toInt()>=40&&v["restoreSize"].toInt()<=128))
        &&v["literal"].isBool()&&v["learning"].isBool()
        &&(!v.contains("predictionMode")||(v["predictionMode"].isString()&&QStringList{"context","frequency","off"}.contains(v["predictionMode"].toString())))
        &&(!v.contains("suggestSentenceStarters")||v["suggestSentenceStarters"].isBool())
        &&(!v.contains("addSpaceAfterCompletion")||v["addSpaceAfterCompletion"].isBool())
        &&(!v.contains("predictNextWord")||v["predictNextWord"].isBool())
        &&(!v.contains("keyboardScale")||(v["keyboardScale"].isDouble()&&v["keyboardScale"].toDouble()==v["keyboardScale"].toInt()&&v["keyboardScale"].toInt()>=50&&v["keyboardScale"].toInt()<=150))
        &&(!v.contains("keyboardTheme")||(v["keyboardTheme"].isString()&&QStringList{"system","midnight","ocean","forest","paper","high-contrast"}.contains(v["keyboardTheme"].toString())))
        &&(!v.contains("highContrast")||v["highContrast"].isBool())
        &&(!v.contains("liveZoom")||v["liveZoom"].isBool())
        &&(!v.contains("progressiveZoom")||v["progressiveZoom"].isBool())
        &&(!v.contains("targetDwellEnabled")||v["targetDwellEnabled"].isBool())
        &&(!v.contains("targetStartDwellMs")||(v["targetStartDwellMs"].isDouble()&&v["targetStartDwellMs"].toDouble()==v["targetStartDwellMs"].toInt()&&v["targetStartDwellMs"].toInt()>=250&&v["targetStartDwellMs"].toInt()<=3000))
        &&(!v.contains("targetConfirmDwellMs")||(v["targetConfirmDwellMs"].isDouble()&&v["targetConfirmDwellMs"].toDouble()==v["targetConfirmDwellMs"].toInt()&&v["targetConfirmDwellMs"].toInt()>=250&&v["targetConfirmDwellMs"].toInt()<=3000))
        &&(!v.contains("targetDwellRadius")||(v["targetDwellRadius"].isDouble()&&v["targetDwellRadius"].toDouble()==v["targetDwellRadius"].toInt()&&v["targetDwellRadius"].toInt()>=10&&v["targetDwellRadius"].toInt()<=100))
        &&(!v.contains("zoomDurationMs")||(v["zoomDurationMs"].isDouble()&&v["zoomDurationMs"].toDouble()==v["zoomDurationMs"].toInt()&&v["zoomDurationMs"].toInt()>=500&&v["zoomDurationMs"].toInt()<=5000))
        &&(!v.contains("enlargeShortLabels")||v["enlargeShortLabels"].isBool())
        &&(!v.contains("magnifierEnabled")||v["magnifierEnabled"].isBool())
        &&(!v.contains("scrollSpeed")||(v["scrollSpeed"].isDouble()&&v["scrollSpeed"].toDouble()==v["scrollSpeed"].toInt()&&v["scrollSpeed"].toInt()>=10&&v["scrollSpeed"].toInt()<=300))
        &&(!v.contains("scrollDeadzone")||(v["scrollDeadzone"].isDouble()&&v["scrollDeadzone"].toDouble()==v["scrollDeadzone"].toInt()&&v["scrollDeadzone"].toInt()>=10&&v["scrollDeadzone"].toInt()<=200))
        &&(!v.contains("scrollAxis")||(v["scrollAxis"].isDouble()&&v["scrollAxis"].toDouble()==v["scrollAxis"].toInt()&&v["scrollAxis"].toInt()>=0&&v["scrollAxis"].toInt()<=2))
        &&(!v.contains("magnifierZoom")||(v["magnifierZoom"].isDouble()&&v["magnifierZoom"].toDouble()==v["magnifierZoom"].toInt()&&v["magnifierZoom"].toInt()>=2&&v["magnifierZoom"].toInt()<=8))
        &&(!v.contains("shortLabelScale")||(v["shortLabelScale"].isDouble()&&v["shortLabelScale"].toDouble()==v["shortLabelScale"].toInt()&&v["shortLabelScale"].toInt()>=50&&v["shortLabelScale"].toInt()<=200))
        &&(!v.contains("longLabelScale")||(v["longLabelScale"].isDouble()&&v["longLabelScale"].toDouble()==v["longLabelScale"].toInt()&&v["longLabelScale"].toInt()>=50&&v["longLabelScale"].toInt()<=200))
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
