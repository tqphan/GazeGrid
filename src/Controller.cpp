#include <cmath>
#include <algorithm>
#include "Controller.h"
#include "KeyboardPack.h"
#include "KeyboardLayout.h"
#include "KeyNames.h"
#include "ActionCatalog.h"
#include "GazeClock.h"
#include "WordPrediction.h"
#include "ShortcutMappings.h"
#include "MagnifierGeometry.h"
#include <QGuiApplication>
#include <QScreen>
#include <QWindow>
#include <QRegion>
#include "ProfileStore.h"
#include <QTimer>
#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonArray>
#include "AppPaths.h"
#include <QTextBoundaryFinder>
namespace {
bool bypassPlatformMappings(bool literal){
#ifdef _WIN32
 return true;
#else
 return literal;
#endif
}
QVariantMap command(QString tag,QString value){return {{"tag",tag},{"text",value}};}
QString configFile(const QString &name){auto dir=appConfigDirectory();QDir().mkpath(dir);return QDir(dir).filePath(name);}
QString panelPlacementReason(const QVariantMap &layout){

 if(layout.value("verticaloffset","0").toString().remove('%').toDouble()!=0||layout.value("horizontaloffset","0").toString().remove('%').toDouble()!=0)
  return "Moving the keyboard off screen is unavailable in Plasma keyboard mode. Use standalone mode for this layout.";
 return {};
}
}
Controller::Controller(){
 zoomTimer.setInterval(33);connect(&zoomTimer,&QTimer::timeout,this,&Controller::updateMagnifierZoom);
 connect(&speech,&SpeechOutput::status,this,[this](QString message){actionStatus=message;emit changed();});
 clock.start();auto profile=ProfileStore(configFile("profile.json")).load();
 gazeEnabled=profile.values["gazeEnabled"].toBool(false);
 predictionModeValue=profile.values["predictionMode"].toString("context");
 nextWordEnabled=profile.values["predictNextWord"].toBool(true);
 completionSpaceEnabled=profile.values["addSpaceAfterCompletion"].toBool(true);
 sentenceStartersEnabled=profile.values["suggestSentenceStarters"].toBool(true);
 dwellIndicatorStyle=profile.values.value("dwellIndicatorStyle").toString("bar");
 dwellMs=profile.values["dwellMs"].toInt();literal=profile.values["literal"].toBool();learning=profile.values["learning"].toBool();profileStatus=profile.message;
 scalePercent=profile.values["keyboardScale"].toInt(100);keyboardTheme=profile.values["highContrast"].toBool()?QStringLiteral("high-contrast"):profile.values["keyboardTheme"].toString("system");
 shortLabelScale=profile.values.value("shortLabelScale").toInt(profile.values.value("enlargeShortLabels").toBool(true)?150:100);
 longLabelScale=profile.values["longLabelScale"].toInt(100);
 magnifierEnabled=profile.values.value("magnifierEnabled").toBool(false);
 scrollSpeed=profile.values["scrollSpeed"].toInt(100);scrollDeadzone=profile.values["scrollDeadzone"].toInt(50);scrollAxis=profile.values["scrollAxis"].toInt(0);
 magnifierZoom=profile.values["magnifierZoom"].toInt(4);
 progressiveZoom=profile.values.value("progressiveZoom").toBool(profile.values.value("liveZoom").toBool(false));
 zoomDurationMs=profile.values.value("zoomDurationMs").toInt(1500);
 targetDwellEnabled=profile.values.value("targetDwellEnabled").toBool(true);
 targetStartDwellMs=profile.values.value("targetStartDwellMs").toInt(750);
 targetConfirmDwellMs=profile.values.value("targetConfirmDwellMs").toInt(1000);
 targetDwellRadius=profile.values.value("targetDwellRadius").toInt(40);
 restoreOpacity=profile.values.value("restoreOpacity").toInt(100);
 recoveryCorner=profile.values["restoreCorner"].toInt(2);
 recoveryWidth=profile.values.value("restoreWidth").toInt(profile.values.value("restoreSize").toInt(64));
 recoveryHeight=profile.values.value("restoreHeight").toInt(profile.values.value("restoreSize").toInt(64));
 rememberDesktop=profile.values["rememberDesktop"].toBool();
 gazeIndicatorSize=profile.values["gazeIndicatorSize"].toInt(18);
 gazeIndicatorOpacity=profile.values["gazeIndicatorOpacity"].toInt(100);
 gazeIndicatorColor=profile.values["gazeIndicatorColor"].toString("#00bfff");
 repeatIntervalMs=profile.values["repeatIntervalMs"].toInt(750);
 preserveTogglesBetweenLayouts=profile.values["preserveTogglesBetweenLayouts"].toBool();
 cycleModifiersOnHover=profile.values["cycleModifiersOnHover"].toBool();
 hideKeyboardDuringMouseActions=profile.values["hideKeyboardDuringMouseActions"].toBool();
 gazeIndicatorMorph=profile.values["gazeIndicatorMorph"].toBool();
 smoothingLevel=profile.values["gazeSmoothingLevel"].toInt(1);smoothTargetChanges=profile.values["smoothWhenChangingGazeTarget"].toBool();
 layoutNames=QDir(keyboardPackDirectory()).entryList({"*.xml"},QDir::Files,QDir::Name);for(auto &name:layoutNames)name.chop(4);
 for(const auto &name:layoutNames){
  auto item=KeyboardLayout::fromXml(KeyboardPack::load(QDir(keyboardPackDirectory()).filePath(name+".xml")));
  if(!item.contains("error")&&item["hidefromkeyboardmenu"].toString().compare("true",Qt::CaseInsensitive)!=0)menuLayoutNames<<name;
 }
 QFile words(":/resources/EnglishUS.dic");if(words.open(QIODevice::ReadOnly)){dictionary=QString::fromUtf8(words.readAll()).remove(QChar(0xfeff)).split('\n',Qt::SkipEmptyParts);for(auto &word:dictionary)word=word.trimmed();dictionary.sort(Qt::CaseInsensitive);}
 WordPrediction::prepare(dictionary);
 QFile learned(configFile("learned.json"));if(learned.open(QIODevice::ReadOnly)){auto obj=QJsonDocument::fromJson(learned.readAll()).object();for(auto it=obj.begin();it!=obj.end();++it)frequencies[it.key()]=qBound(0,it.value().toInt(),1000000);}
#ifndef _WIN32
 QString mappingError;QFile mappings(configFile("linux-mappings.json"));
 if(mappings.exists()){
  if(mappings.open(QIODevice::ReadOnly))mappingProfile=ShortcutMappings::parse(mappings.readAll(),mappingError);
  else mappingError="Could not read saved mappings";
 }
 if(mappingProfile.isEmpty()){
  QFile defaults(":/resources/linux-mappings.json");QString defaultError;
  if(defaults.open(QIODevice::ReadOnly))mappingProfile=ShortcutMappings::parse(defaults.readAll(),defaultError);
  else defaultError="Could not read default mappings";
  if(!defaultError.isEmpty())actionStatus=defaultError;
 }
 if(!mappingError.isEmpty())profileStatus+=(profileStatus.isEmpty()?QString():QString(" "))+mappingError+". Using default mappings; saved file was preserved.";
#endif
 pointer.move=[this](double x,double y){return portal.moveAbsolute(x,y);};
 pointer.moveRelative=[this](double x,double y){return portal.move(x,y);};
 pointer.button=[this](int code,bool down){return portal.button(code,down);};
 pointer.scroll=[this](double amount){return portal.scroll(amount);};
 pointer.scrollAxes=[this](double x,double y){return portal.scrollAxes(x,y);};
 pointer.releaseAll=[this]{portal.releaseButtons();};
 connect(&pointer,&PointerActions::changed,this,[this]{targeting=pointer.targeting();if(!targeting)clearCapture();emit changed();});
 connect(&screenshot,&ScreenshotPortal::failed,this,[this](QString error){
  clearCapture();runner.cancel();endButtonHold();gameController.reset();activeJoystick=-1;pointer.cancel();actionStatus=error;emit changed();
 });
 connect(&screenshot,&ScreenshotPortal::captured,this,[this](QImage image){receiveMagnifierFrame(image);});
 connect(&pointer,&PointerActions::completed,this,[this](bool ok){actionStatus=pointer.prompt();runner.resume(ok);emit changed();});
 runner.sendKey=[this](QString name,bool down){return portal.key(KeyNames::linuxCode(name),down);};
 connect(&gameController,&GameController::changed,this,&Controller::changed);
 connect(&gameController,&GameController::failed,this,[this](QString error){activeJoystick=-1;actionStatus=error;emit changed();});
 runner.performAction=[this](QString action){return performAction(action);};
 runner.speakLabel=[this](QString label){if(!sensitiveInput)speech.speak(label);};
 runner.supportsAction=ActionCatalog::supported;
 runner.navigate=[this](QString name,bool back){openLayout(name,back);};
 connect(&runner,&ActionRunner::failed,this,[this](QString m){endButtonHold();gameController.reset();activeJoystick=-1;actionStatus=m.contains("Gamepad")&&!gameController.error().isEmpty()?gameController.error():m;emit changed();});
 connect(&runner,&ActionRunner::stateChanged,this,&Controller::changed);
 connect(&runner,&ActionRunner::cancelled,this,&Controller::clearOverlap);
 connect(&runner,&ActionRunner::failed,this,[this]{clearOverlap();});
 connect(&runner,&ActionRunner::finished,this,&Controller::dispatchOverlap,Qt::QueuedConnection);
 connect(&runner,&ActionRunner::textCommitted,this,&Controller::updateText);
 connect(&portal,&PortalInput::status,this,[this](QString m){portalStatus=m;emit changed();});
 connect(&portal,&PortalInput::readyChanged,this,[this]{dwell.reset();if(!ready()){runner.cancel();endButtonHold();gameController.reset();activeJoystick=-1;pointer.cancel();}emit changed();});
 connect(&tobii,&TobiiInput::status,this,[this](QString m){tobiiStatus=m;emit changed();});
 connect(&tobii,&TobiiInput::gaze,this,&Controller::receiveGaze);
 auto timer=new QTimer(this);timer->setInterval(25);connect(timer,&QTimer::timeout,this,[this]{
  if(!overlapDwells.isEmpty()&&clock.elapsed()-lastHoldHover>200)clearOverlap();
  if(!buttonHoldTarget.isEmpty()&&clock.elapsed()-lastHoldHover>200){endButtonHold();dwell.reset();}
  if(!heldDwellTarget.isEmpty()&&clock.elapsed()-lastHoldHover>200){runner.endDwellHold();heldDwellTarget.clear();dwell.reset();}
  if(gazeAge.isValid()&&GazeClock::now()-lastGazeSample>200000){gazeAge.invalidate();gazeSmoothing.reset();dwell.reset();runner.cancel();endButtonHold();gameController.reset();activeJoystick=-1;portal.releaseAll();pointer.trackingLost();emit gazePoint(0,0,false);}
 });timer->start();
 auto start=profile.values["startLayout"].toString();
 if(!layoutNames.contains(start)){start=ProfileStore::defaults()["startLayout"].toString();profileStatus="Saved keyboard is unavailable. Opened the default keyboard.";}
 openLayout(start,false);
}
Controller::~Controller(){runner.cancel();endButtonHold();gameController.reset();activeJoystick=-1;pointer.cancel();portal.releaseAll();portal.close();
 QJsonObject obj;for(auto it=frequencies.begin();it!=frequencies.end();++it)obj[it.key()]=it.value();QSaveFile file(configFile("learned.json"));if(file.open(QIODevice::WriteOnly)){file.write(QJsonDocument(obj).toJson());file.commit();}
}
void Controller::configureInputMethod(std::function<bool(QString)> text,std::function<bool(QString,bool)> key){
 nativeMode=true;sensitiveInput=true;
 runner.sendText=std::move(text);
 runner.canCommitText=[this]{return nativeActive;};
 runner.sendKey=[this,key=std::move(key)](QString name,bool down){return portal.ready()?portal.key(KeyNames::linuxCode(name),down):key(name,down);};
 runner.supportsPhysicalKey=[this](QString){return portal.ready();};
 openLayout("keyboard-alphabet-14x4-bottom",false);
 actionStatus="Native text input enabled; authorize desktop control for pointer actions and held shortcuts";emit changed();
}
void Controller::invalidateInputContext(){
 runner.cancel();endButtonHold();gameController.reset();activeJoystick=-1;pointer.cancel();portal.releaseAll();dwell.reset();speech.stop();scratchpad.clear();predictionContext.clear();suggestions.clear();emit changed();
}
void Controller::inputContextChanged(bool active,bool sensitive){
 nativeActive=active;sensitiveInput=sensitive;
 if(!active||sensitive)invalidateInputContext();
 else updateSuggestions();
 actionStatus=active?(sensitive?"Native input ready — private field":"Native text input ready"):"Select a text field to use native input";emit changed();
}
bool Controller::savePreferences(){
 QJsonObject values{{"scrollSpeed",scrollSpeed},{"scrollDeadzone",scrollDeadzone},{"scrollAxis",scrollAxis},{"gazeEnabled",gazeEnabled},{"repeatIntervalMs",repeatIntervalMs},{"preserveTogglesBetweenLayouts",preserveTogglesBetweenLayouts},{"cycleModifiersOnHover",cycleModifiersOnHover},{"hideKeyboardDuringMouseActions",hideKeyboardDuringMouseActions},{"gazeIndicatorSize",gazeIndicatorSize},{"gazeIndicatorOpacity",gazeIndicatorOpacity},{"gazeIndicatorColor",gazeIndicatorColor},{"gazeIndicatorMorph",gazeIndicatorMorph},{"version",1},{"dwellMs",dwellMs},{"dwellIndicatorStyle",dwellIndicatorStyle},{"literal",literal},{"learning",learning},{"predictionMode",predictionModeValue},{"predictNextWord",nextWordEnabled},{"addSpaceAfterCompletion",completionSpaceEnabled},{"suggestSentenceStarters",sentenceStartersEnabled},{"startLayout",layoutName},{"keyboardScale",scalePercent},{"keyboardTheme",keyboardTheme},{"shortLabelScale",shortLabelScale},{"longLabelScale",longLabelScale},{"magnifierZoom",magnifierZoom},{"magnifierEnabled",magnifierEnabled},{"progressiveZoom",progressiveZoom},{"zoomDurationMs",zoomDurationMs},{"targetDwellEnabled",targetDwellEnabled},{"targetStartDwellMs",targetStartDwellMs},{"targetConfirmDwellMs",targetConfirmDwellMs},{"targetDwellRadius",targetDwellRadius},{"rememberDesktop",rememberDesktop},{"gazeSmoothingLevel",smoothingLevel},{"smoothWhenChangingGazeTarget",smoothTargetChanges},{"restoreCorner",recoveryCorner},{"restoreWidth",recoveryWidth},{"restoreHeight",recoveryHeight},{"restoreOpacity",restoreOpacity}};
 QString error;bool ok=ProfileStore(configFile("profile.json")).save(values,error);
 profileStatus=ok?QString():error;emit changed();return ok;
}
void Controller::resetPreferences(){
 QString error;auto values=ProfileStore::defaults();
 if(!ProfileStore(configFile("profile.json")).save(values,error,true)){profileStatus=error;emit changed();return;}
 if(!paused)togglePause();
 setMagnifierEnabled(false);
 setGazeEnabled(false);
 dwellIndicatorStyle="bar";
 dwellMs=values["dwellMs"].toInt();literal=false;learning=false;predictionModeValue="context";nextWordEnabled=true;completionSpaceEnabled=true;sentenceStartersEnabled=true;backStack.clear();
 repeatIntervalMs=750;
 preserveTogglesBetweenLayouts=false;
 cycleModifiersOnHover=false;
 hideKeyboardDuringMouseActions=false;
 scrollSpeed=100;scrollDeadzone=50;scrollAxis=0;
 gazeIndicatorSize=18;gazeIndicatorOpacity=100;gazeIndicatorColor="#00bfff";gazeIndicatorMorph=false;
 setRestoreCorner(2);setRestoreWidth(64);setRestoreHeight(64);restoreOpacity=100;
 setKeyboardScale(100);keyboardTheme="system";shortLabelScale=values["shortLabelScale"].toInt();longLabelScale=values["longLabelScale"].toInt();magnifierZoom=values["magnifierZoom"].toInt();progressiveZoom=false;zoomDurationMs=1500;targetDwellEnabled=true;targetStartDwellMs=750;targetConfirmDwellMs=1000;targetDwellRadius=40;rememberDesktop=false;
 setGazeSmoothingLevel(values["gazeSmoothingLevel"].toInt());setSmoothWhenChangingGazeTarget(false);gazeSmoothing.reset();
 openLayout(values["startLayout"].toString(),false);
 profileStatus="Preferences reset. Input remains paused.";emit changed();
}
void Controller::setRestoreCorner(int corner){
 corner=qBound(0,corner,3);if(corner==recoveryCorner)return;
 recoveryCorner=corner;dwell.reset();emit geometryPreferencesChanged();
}
void Controller::setRestoreWidth(int width){
 width=qBound(40,width,128);if(width==recoveryWidth)return;
 recoveryWidth=width;dwell.reset();emit geometryPreferencesChanged();
}
void Controller::setRestoreHeight(int height){
 height=qBound(40,height,128);if(height==recoveryHeight)return;
 recoveryHeight=height;dwell.reset();emit geometryPreferencesChanged();
}
void Controller::setKeyboardScale(int scale){
 scale=qBound(50,scale,150);if(scale==scalePercent)return;
 runner.cancel();endButtonHold();gameController.reset();activeJoystick=-1;pointer.cancel();dwell.reset();scalePercent=scale;emit geometryPreferencesChanged();emit changed();
}
QString Controller::mappingJson()const{return QString::fromUtf8(QJsonDocument(mappingProfile).toJson(QJsonDocument::Indented));}
bool Controller::saveMappings(QString json){
 QString error;auto profile=ShortcutMappings::parse(json.toUtf8(),error);
 if(!error.isEmpty()){actionStatus=error;emit changed();return false;}
 const auto data=QJsonDocument(profile).toJson();QSaveFile file(configFile("linux-mappings.json"));
 if(!file.open(QIODevice::WriteOnly)||file.write(data)!=data.size()||!file.commit()){actionStatus="Could not save mappings";emit changed();return false;}
 mappingProfile=profile;actionStatus="Platform mappings saved";emit changed();return true;
}
QVariantList Controller::mapped(const QVariantList &commands,bool &ok){
 auto result=ShortcutMappings::resolve(commands,mappingProfile,bypassPlatformMappings(literal));ok=result.allowed;actionStatus=result.message;return result.commands;
}
void Controller::receiveGaze(double x,double y,bool valid,qint64 sampledAt){
 const auto now=GazeClock::now();
 valid=valid&&GazeClock::fresh(sampledAt,now,lastGazeSample)&&std::isfinite(x)&&std::isfinite(y);
 if(valid){
  // Valid tracker samples may lie beyond the display; keep edge actions active.
  x=qBound(0.,x,1.);y=qBound(0.,y,1.);
  if(!gazeAge.isValid()||now-lastGazeSample>200000)gazeSmoothing.reset();
  lastGazeSample=sampledAt;gazeAge.restart();
  const auto point=gazeSmoothing.update({x*gazeDisplaySize.width(),y*gazeDisplaySize.height()},smoothingLevel,smoothTargetChanges);
  x=point.x()/gazeDisplaySize.width();y=point.y()/gazeDisplaySize.height();
 }else{
  // Cancel gaze work once on tracking loss. Further invalid samples must not
  // continually reset mouse dwell or cancel actions started by the fallback.
  const bool wasValid=gazeAge.isValid();gazeAge.invalidate();gazeSmoothing.reset();
  if(wasValid){dwell.reset();runner.cancel();endButtonHold();gameController.reset();activeJoystick=-1;portal.releaseAll();pointer.trackingLost();}
 }
 emit gazePoint(x,y,valid);
}
void Controller::setDisplaySize(double width,double height){
 if(!std::isfinite(width)||!std::isfinite(height)||width<=0||height<=0)return;
 const QSizeF size(width,height);if(size!=gazeDisplaySize){gazeDisplaySize=size;gazeSmoothing.reset();}
 pointer.setDisplaySize(size);
}
void Controller::setGazeSmoothingLevel(int level){
 level=qBound(0,level,4);if(level==smoothingLevel)return;
 smoothingLevel=level;gazeSmoothing.reset();dwell.reset();runner.cancel();endButtonHold();gameController.reset();activeJoystick=-1;pointer.cancel();emit changed();
}
void Controller::setSmoothWhenChangingGazeTarget(bool enabled){
 if(enabled==smoothTargetChanges)return;
 smoothTargetChanges=enabled;gazeSmoothing.reset();dwell.reset();runner.cancel();endButtonHold();gameController.reset();activeJoystick=-1;pointer.cancel();emit changed();
}
void Controller::togglePause(){paused=!paused;dwell.reset();runner.cancel();endButtonHold();gameController.reset();activeJoystick=-1;portal.releaseAll();pointer.cancel();emit changed();}
void Controller::setGazeEnabled(bool enabled){
 const bool preferenceChanged=gazeEnabled!=enabled;
 gazeEnabled=enabled;dwell.reset();gazeSmoothing.reset();
 if(preferenceChanged){
  ProfileStore store(configFile("profile.json"));auto saved=store.load().values;
  saved["gazeEnabled"]=enabled;QString error;
  if(!store.save(saved,error))profileStatus=error;
 }
 if(enabled)connectTobii();
 if(!enabled){runner.cancel();endButtonHold();gameController.reset();activeJoystick=-1;pointer.cancel();}
 if(preferenceChanged)emit gazeEnabledChanged();
 emit changed();
}
void Controller::trackGaze(double x,double y,bool overDesktop){
 pointer.configureScroll(scrollSpeed,scrollDeadzone,scrollAxis);
 if(gazeEnabled&&!paused&&ready()&&gazeAge.isValid()&&GazeClock::now()-lastGazeSample<=200000)pointer.follow({x,y},overDesktop);
}
void Controller::displayChanged(){
 if(!paused)togglePause();else{runner.cancel();endButtonHold();gameController.reset();activeJoystick=-1;pointer.cancel();dwell.reset();}
 gazeAge.invalidate();gazeSmoothing.reset();emit gazePoint(0,0,false);
 actionStatus="Display configuration changed. Check tracker calibration and resume when ready.";emit changed();
}
bool Controller::isSleepKey(const QString &key) const{
 if(!key.startsWith("key:"))return false;
 bool valid=false;const int index=key.mid(4).toInt(&valid);
 const auto keys=layout["keys"].toList();
 if(!valid||index<0||index>=keys.size())return false;
 const auto entry=keys[index].toMap();
 if(!entry["unavailableReason"].toString().isEmpty())return false;
 const auto commands=entry["commands"].toList();
 if(commands.size()!=1)return false;
 const auto action=commands.front().toMap();
 return action["tag"]=="Action"&&action["text"].toString().trimmed()=="Sleep";
}
void Controller::endButtonHold(){
 const auto action=buttonHoldAction;buttonHoldAction.clear();buttonHoldTarget.clear();
 if(!action.isEmpty())gameController.perform(action);
}
double Controller::hover(QString key){
 if(!collectingOverlap)clearOverlap();
 if(sizeMoveMode){
  if(paused||runner.busy()){dwell.inhibit(key,clock.elapsed());return 0;}
  // Editing has its own dwell path: never execute hold, repeat or typing behavior.
  if(dwell.update(key,clock.elapsed(),dwellMs)){
   activate(key);dwell.consume(key,clock.elapsed());
  }
  return dwell.progress(clock.elapsed(),dwellMs);
 }
 if(!collectingOverlap){
  if(key!=buttonHoldTarget||paused)endButtonHold();
  if(key!=heldDwellTarget||paused){runner.endDwellHold();heldDwellTarget.clear();}
 }
 lastHoldHover=clock.elapsed();
 if(key.startsWith("suggest:")){
  bool valid=false;const int i=key.mid(8).toInt(&valid);
  if(!valid||i<0||i>=suggestions.size()||suggestions[i].trimmed().isEmpty()){dwell.reset();return 0;}
 }
 const bool recovery=key=="Pause"||key=="Recover"||key=="CancelTarget"||key=="Connect"||key=="Tobii"||key=="Settings"||key=="Speak"||key=="StopSpeech"||isSleepKey(key);
 if(!recovery&&(paused||runner.busy())){dwell.inhibit(key,clock.elapsed());return 0;}
 int duration=dwellMs;bool repeatKey=false;QVariantList holdCommands;QString buttonHold;
 if(key.startsWith("key:")){
  bool valid=false;const int i=key.mid(4).toInt(&valid);const auto keys=layout["keys"].toList();
  if(valid&&i>=0&&i<keys.size()){
   const auto entry=keys[i].toMap();const int custom=entry["dwell"].toInt();if(custom>0)duration=custom;
   const auto commands=entry["commands"].toList();
   const auto fixedMode=entry["mode"].toString();
   const auto keyMode=fixedMode.isEmpty()?keyModeValue:fixedMode;
   const auto buttonMode=fixedMode.isEmpty()?buttonModeValue:fixedMode;
   if(!runner.textOnly&&entry["unavailableReason"].toString().isEmpty()&&commands.size()==1){
    const auto resolved=ShortcutMappings::resolve(commands,mappingProfile,bypassPlatformMappings(literal));
    if(resolved.allowed&&resolved.commands.size()==1){
     const auto cmd=resolved.commands.front().toMap();
     const auto button=cmd["tag"]=="Action"?GameController::buttonAction(cmd["text"].toString(),"Press"):QString();
     if(!button.isEmpty()){
      repeatKey=buttonMode=="Repeat";
      if(buttonMode=="Hold")buttonHold=button;
     }
    }
   }
   if(ready()&&entry["unavailableReason"].toString().isEmpty()&&commands.size()==1){
    const bool modeKey=!ActionRunner::dwellKey(commands).first.isEmpty();
    if(modeKey&&keyMode=="Hold"&&!runner.textOnly){
     const auto resolved=ShortcutMappings::resolve(commands,mappingProfile,bypassPlatformMappings(literal));
     if(resolved.allowed&&!ActionRunner::dwellKey(resolved.commands).first.isEmpty())holdCommands=resolved.commands;
    }
    repeatKey=repeatKey||(cycleModifiersOnHover&&!ActionRunner::modifierName(commands).isEmpty())
     ||(modeKey&&keyMode=="Repeat");
   }
  }
 }
 if(dwell.update(key,clock.elapsed(),duration,repeatKey,repeatIntervalMs)){
  if(collectingOverlap){
   bool pending=false;for(const auto &entry:overlapPending)if(entry.target==key)pending=true;
   if(!pending)overlapPending.append({key,holdCommands,buttonHold});
  }else if(!buttonHold.isEmpty()){
   if(gameController.perform(buttonHold)){buttonHoldTarget=key;buttonHoldAction=GameController::buttonAction(buttonHold,"Release");}
   else {actionStatus=gameController.error();emit changed();}
  }else if(!holdCommands.isEmpty()){if(runner.beginDwellHold(holdCommands))heldDwellTarget=key;}else activate(key);
  dwell.consume(key,clock.elapsed());
 }
 return dwell.progress(clock.elapsed(),duration,repeatIntervalMs);
}
void Controller::releaseOverlapTarget(const QString &target){
 runner.endDwellHold(target);
 const auto release=overlapButtons.take(target);
 if(!release.isEmpty()&&!overlapButtons.values().contains(release))gameController.perform(release);
}
void Controller::clearOverlap(){
 overlapPending.clear();
 const auto targets=overlapDwells.keys();overlapDwells.clear();
 for(const auto &target:targets)releaseOverlapTarget(target);
}
QVariantMap Controller::hoverKeys(const QStringList &targets){
 if(layout["layoutMode"]!="canvas"||!layout["activateoverlappingkeys"].toBool()||sizeMoveMode){
  const auto target=targets.value(0);return {{target,hover(target)}};
 }
 const auto keys=layout["keys"].toList();QStringList selected;
 for(const auto &target:targets){
  bool ok=false;const int i=target.mid(4).toInt(&ok);
  if(target.startsWith("key:")&&ok&&i>=0&&i<keys.size()&&keys[i].toMap()["kind"]=="DynamicKey"&&!selected.contains(target))selected<<target;
 }
 // Pause/resume must remain usable during a macro and must never resume input
 // accidentally by activating a second overlapping Sleep key.
 for(const auto &target:selected){
  const auto commands=keys[target.mid(4).toInt()].toMap()["commands"].toList();
  const auto command=commands.size()==1?commands.front().toMap():QVariantMap();
  const auto action=command["text"].toString().trimmed();
  if(isSleepKey(target)||isSizeMoveKey(target)||(command["tag"]=="Action"&&(action=="CycleKeyMode"||action=="CycleButtonMode")))
   return {{target,hover(target)}};
 }
 dwell.reset();
 if(paused||targeting){clearOverlap();return {};}
 endButtonHold();
 if(!heldDwellTarget.isEmpty()){runner.endDwellHold();heldDwellTarget.clear();}
 const auto previous=overlapDwells.keys();
 for(const auto &target:previous)if(!selected.contains(target)){releaseOverlapTarget(target);overlapDwells.remove(target);}
 for(auto it=overlapPending.begin();it!=overlapPending.end();)if(!selected.contains(it->target))it=overlapPending.erase(it);else ++it;
 std::sort(selected.begin(),selected.end(),[&](const QString &a,const QString &b){
  const int ai=a.mid(4).toInt(),bi=b.mid(4).toInt();
  const bool am=!ActionRunner::modifierName(keys[ai].toMap()["commands"].toList()).isEmpty();
  const bool bm=!ActionRunner::modifierName(keys[bi].toMap()["commands"].toList()).isEmpty();
  return am!=bm?am:ai<bi;
 });
 QVariantMap progress;collectingOverlap=true;
 for(const auto &target:selected){
  dwell=overlapDwells.value(target);
  progress[target]=hover(target);
  overlapDwells[target]=dwell;
 }
 collectingOverlap=false;dwell.reset();lastHoldHover=clock.elapsed();
 dispatchOverlap();return progress;
}
void Controller::dispatchOverlap(){
 if(paused||sizeMoveMode||targeting){clearOverlap();return;}
 while(!runner.busy()&&!overlapPending.isEmpty()){
  const auto next=overlapPending.takeFirst();
  if(!overlapDwells.contains(next.target))continue;
  if(!next.buttonHold.isEmpty()){
   const auto release=GameController::buttonAction(next.buttonHold,"Release");
   if(overlapButtons.values().contains(release)||gameController.perform(next.buttonHold))overlapButtons[next.target]=release;
   else {actionStatus=gameController.error();clearOverlap();emit changed();return;}
  }else if(!next.holdCommands.isEmpty()){
   if(!runner.beginDwellHold(next.holdCommands,next.target)){clearOverlap();return;}
  }else{
   dispatchingOverlap=true;activate(next.target);dispatchingOverlap=false;
  }
 }
}
void Controller::clickCommand(QString key){
 activate(key);dwell.consume(key,clock.elapsed());
 if(!sizeMoveMode&&layout["layoutMode"]=="canvas"&&layout["activateoverlappingkeys"].toBool()&&key.startsWith("key:"))
  overlapDwells[key].consume(key,clock.elapsed());
}
void Controller::activate(QString key){
 if(!dispatchingOverlap)clearOverlap();
 if(sizeMoveMode&&(key.startsWith("key:")||key.startsWith("suggest:"))&&!isSizeMoveKey(key)){
  if(paused)return;
  bool ok=false;const int index=key.mid(4).toInt(&ok);
  if(key.startsWith("key:")&&ok&&index>=0&&index<layout["keys"].toList().size()){
   resetCanvasPlacement();sizeMoveSelection=index;emit changed();
  }
  return;
 }
 if(!dispatchingOverlap){endButtonHold();runner.endDwellHold();heldDwellTarget.clear();}
 if(key=="MoveModeToggle"||key=="SizeModeToggle"||key=="CycleKeyMode"||key=="CycleButtonMode"){performAction(key);return;}
 if(key=="Settings"){emit settingsRequested();return;}
 if(key=="Speak"){speak();return;}
 if(key=="StopSpeech"){stopSpeech();return;}
 if(key=="Pause"||isSleepKey(key)){togglePause();return;}
 if(key=="Recover"){recover();return;}
 if(key=="Connect"){connectDesktop();return;}
 if(key=="Tobii"){connectTobii();return;}
 if(key=="CancelTarget"){targeting=false;runner.cancel();endButtonHold();gameController.reset();activeJoystick=-1;portal.releaseAll();pointer.cancel();emit changed();return;}
 if(paused)return;
 if(key.startsWith("suggest:")){
  bool valid=false;int i=key.mid(8).toInt(&valid);if(!valid||i<0||i>=suggestions.size()||suggestions[i].trimmed().isEmpty()||(!runner.textOnly&&!ready()))return;
  if(!runner.heldKeys().isEmpty()){actionStatus="Release held keys before accepting a suggestion";emit changed();return;}
  QString prefix=WordPrediction::prefix(typingContext());auto word=suggestions[i];
  runner.start({command("Text",word.mid(prefix.size())+(completionSpaceEnabled?QString(" "):QString()))});return;
 }
 if(!key.startsWith("key:"))return;
 bool valid=false;int i=key.mid(4).toInt(&valid);auto keys=layout["keys"].toList();if(!valid||i<0||i>=keys.size())return;
 const auto unavailable=keys[i].toMap()["unavailableReason"].toString();
 if(!unavailable.isEmpty()){actionStatus=unavailable;emit changed();return;}
 const auto fixedMode=keys[i].toMap()["mode"].toString();
 const auto keyMode=fixedMode.isEmpty()?keyModeValue:fixedMode;
 const auto buttonMode=fixedMode.isEmpty()?buttonModeValue:fixedMode;
 auto commands=keys[i].toMap()["commands"].toList();bool ok=false;commands=mapped(commands,ok);
 if(!ok){emit changed();return;}
 // Navigation, sleep, and scratchpad clearing work without a portal session.
 bool needsOutput=false;for(auto v:commands){auto c=v.toMap();auto tag=c["tag"].toString(),value=c["text"].toString().trimmed();if(tag!="ChangeKeyboard"&&tag!="Wait"&&!(tag=="Action"&&(value=="MoveModeToggle"||value=="SizeModeToggle"||value=="CycleButtonMode"||value=="CycleKeyMode"||value=="BackFromKeyboard"||value=="MenuKeyboard"||value=="Sleep"||value=="ClearScratchpad"||value=="Speak"||value=="SpeakLabel"||value=="NextSuggestions"||value=="PreviousSuggestions"||GameController::supports(value))))needsOutput=true;}
 if(needsOutput&&!runner.textOnly&&!ready()){actionStatus="Authorize desktop control before sending input";emit changed();return;}
 if(!runner.textOnly&&commands.size()==1){
  auto cmd=commands.front().toMap();
  const auto button=cmd["tag"]=="Action"?GameController::buttonAction(cmd["text"].toString(),buttonMode=="Toggle"?"Toggle":"Tap"):QString();
  if(!button.isEmpty()){cmd["text"]=button;commands={cmd};}
 }
 if(keyMode=="Toggle"&&!runner.textOnly&&!ActionRunner::dwellKey(commands).first.isEmpty())
  runner.toggleDwellKey(commands);
 else runner.start(commands);
 emit changed();
}
void Controller::openLayout(QString name,bool backReturnsHere){
 if(name.endsWith(".xml"))name.chop(4);
 if(!layoutNames.contains(name)){actionStatus="Unknown keyboard: "+name;emit changed();return;}
 const auto sourcePath=QDir(keyboardPackDirectory()).filePath(name+".xml");
 QFile sourceFile(sourcePath);
 if(!sourceFile.open(QIODevice::ReadOnly)){actionStatus=sourceFile.errorString();emit changed();return;}
 const auto sourceBytes=sourceFile.readAll();sourceFile.close();
 auto compiled=KeyboardLayout::fromXml(KeyboardPack::load(sourcePath));
 if(compiled.contains("error")){actionStatus=compiled["error"].toString();emit changed();return;}
 if(nativeMode){
  const auto reason=panelPlacementReason(compiled);
  if(!reason.isEmpty()){actionStatus=reason;emit changed();return;}
  auto keys=compiled["keys"].toList();
  for(auto &value:keys){
   auto key=value.toMap();
   for(const auto &entry:key["commands"].toList()){
    const auto command=entry.toMap();if(command["tag"]!="ChangeKeyboard")continue;
    auto destination=command["text"].toString().trimmed();if(destination.endsWith(".xml"))destination.chop(4);
    if(!layoutNames.contains(destination))continue;
    const auto target=KeyboardLayout::fromXml(KeyboardPack::load(QDir(keyboardPackDirectory()).filePath(destination+".xml")));
    if(target.contains("error"))continue;
    const auto unavailable=panelPlacementReason(target);
    if(!unavailable.isEmpty()){key["unavailableReason"]=unavailable;break;}
   }
   value=key;
  }
  compiled["keys"]=keys;
 }
 const bool textOnly=compiled["simulatekeystrokes"].toString().compare("false",Qt::CaseInsensitive)==0;
 const bool preserve=preserveTogglesBetweenLayouts&&!textOnly&&!runner.textOnly;
 runner.cancel(textOnly==runner.textOnly,preserve);endButtonHold();gameController.reset(preserve);activeJoystick=-1;runner.textOnly=textOnly;runner.capsLock=false;pointer.cancel();dwell.reset();
 if(backReturnsHere&&!layoutName.isEmpty()&&layoutName!=name){backStack<<layoutName;if(backStack.size()>64)backStack.removeFirst();}
 canvasSourcePath=sourcePath;canvasSource=sourceBytes;
 saveCanvasChanges=compiled["persistkeygeometry"].toString().compare("true",Qt::CaseInsensitive)==0;canvasSaveStatus.clear();
 resetCanvasPlacement();canvasEditMode.clear();sizeMoveMode=false;layout=compiled;layoutName=name;updateSuggestions();emit layoutChanged();emit changed();
}
void Controller::recover(){paused=false;backStack.clear();openLayout("main-14x1-bottom",false);}
void Controller::joystick(int index,double x,double y,bool active){
 const auto keys=layout["keys"].toList();
 if(!active||sizeMoveMode||paused||runner.textOnly||index<0||index>=keys.size()){
  if(activeJoystick>=0&&activeJoystick<keys.size())gameController.stick(keys[activeJoystick].toMap()["style"].toMap().value("stick","Left").toString(),0,0);
  activeJoystick=-1;return;
 }
 const auto entry=keys[index].toMap();if(entry["kind"]!="Joystick")return;
 if(activeJoystick>=0&&activeJoystick!=index)joystick(-1,0,0,false);
 activeJoystick=index;
 if(!gameController.stick(entry["style"].toMap().value("stick","Left").toString(),x,y)){
  if(actionStatus!=gameController.error()){actionStatus=gameController.error();emit changed();}
 }
}
bool Controller::performAction(QString action){
 if(action=="MoveModeToggle"||action=="SizeModeToggle"){
  if(layout["layoutMode"]!="canvas"){actionStatus="Size/move mode requires a Canvas layout";emit changed();return true;}
  const QString mode=action=="MoveModeToggle"?"move":"size";
  clearOverlap();
  canvasEditMode=canvasEditMode==mode?QString():mode;sizeMoveMode=!canvasEditMode.isEmpty();resetCanvasPlacement();
  runner.endDwellHold();runner.releaseToggledKeys();heldDwellTarget.clear();endButtonHold();
  gameController.reset();activeJoystick=-1;
  // Keep the activating key consumed until the pointer leaves it.
  actionStatus=sizeMoveMode?"Dwell on a key, then dwell at its new "+QString(canvasEditMode=="move"?"center":"bottom-right corner"):"Canvas editing off";
  emit changed();return true;
 }

 if(action=="CycleButtonMode"){
  clearOverlap();
  endButtonHold();gameController.reset();activeJoystick=-1;
  buttonModeValue=buttonModeValue=="Tap"?"Repeat":buttonModeValue=="Repeat"?"Hold":buttonModeValue=="Hold"?"Toggle":"Tap";
  actionStatus="Button mode: "+buttonModeValue;emit changed();return true;
 }
 if(action=="CycleKeyMode"){
  clearOverlap();
  runner.endDwellHold();heldDwellTarget.clear();
  runner.releaseToggledKeys();
  keyModeValue=keyModeValue=="Tap"?"Repeat":keyModeValue=="Repeat"?"Hold":keyModeValue=="Hold"?"Toggle":"Tap";
  actionStatus="Key mode: "+keyModeValue;emit changed();return true;
 }
 if(GameController::supports(action))return runner.textOnly?true:gameController.perform(action);
 if(action=="MouseMagnifier"){
  setMagnifierEnabled(!magnifierEnabled);actionStatus=magnifierEnabled?"Magnifier enabled: target selection will request desktop capture":"Magnifier disabled";emit changed();return true;
 }
 if(action=="Speak"){speak();return true;}
 if(action=="BackMany"){
  if(!runner.textOnly)return false;
  auto &text=typingContext();
  int end=text.size();while(end>0&&text[end-1].isSpace())--end;
  while(end>0&&!text[end-1].isSpace())--end;
  text.truncate(end);updateSuggestions();return true;
 }
 if(action=="NextSuggestions"||action=="PreviousSuggestions"){
  if(sensitiveInput){suggestions.clear();emit changed();return true;}
  int pageSize=4;for(const auto &v:layout["keys"].toList()){auto k=v.toMap();if(k["kind"]=="SuggestionRow")pageSize=qMax(1,k.value("suggestionCount",k["width"].toInt()/2).toInt());}
  const auto all=predictionSuggestions(1000);
  int next=suggestionOffset+(action=="NextSuggestions"?pageSize:-pageSize);
  if(next>=0&&next<all.size())suggestionOffset=next;
  suggestions=all.mid(suggestionOffset,pageSize);emit changed();return true;
 }
 if(action=="ClearScratchpad"){scratchpad.clear();predictionContext.clear();updateSuggestions();return true;}
 if(action=="Sleep"){togglePause();return true;}
 if(action=="MenuKeyboard"){recover();return true;}
 if(action=="BackFromKeyboard"){openLayout(backStack.isEmpty()?"main-14x1-bottom":backStack.takeLast(),false);return true;}
 if(PointerActions::supports(action)){
  if(PointerActions::needsAbsolute(action)&&!portal.absoluteAvailable()){
   actionStatus="Pointer targeting requires an input region mapped to the primary display";emit changed();return false;
  }
  runner.suspend();
  if(!pointer.start(action,hideKeyboardDuringMouseActions?100:0)){actionStatus="Pointer action unavailable: release held buttons or complete an action before repeating";emit changed();return false;}
  return true;
 }
 actionStatus="Not implemented yet: "+action;emit changed();return false;
}
void Controller::targetAt(double x,double y){
 if(paused||!ready()||!targeting||capturePending||zoomAnimating||!MagnifierGeometry::validPoint({x,y}))return;
 if(magnifierReady){
  const auto point=MagnifierGeometry::map(magnifierCrop,captureSize,{x,y});clearCapture();pointer.select(point);emit changed();return;
 }
 if(magnifierEnabled){
  pointDwell.reset();coarseTarget={x,y};capturePending=true;const auto epoch=++captureEpoch;emit changed();
  QTimer::singleShot(150,this,[this,epoch]{if(epoch==captureEpoch&&capturePending&&!paused&&targeting)screenshot.capture();});return;
 }
 pointer.select(QPointF(x,y));
}
double Controller::dwellAt(double x,double y,bool valid){
 if(!valid||!targetDwellEnabled||!targeting||paused||!ready()||capturePending||zoomAnimating||!MagnifierGeometry::validPoint({x,y})){
  pointDwell.reset();return 0;
 }
 const int duration=magnifierReady?targetConfirmDwellMs:targetStartDwellMs;
 if(pointDwell.update({x*gazeDisplaySize.width(),y*gazeDisplaySize.height()},clock.elapsed(),duration,targetDwellRadius)){
  pointDwell.reset();targetAt(x,y);return 0;
 }
 return pointDwell.progress(clock.elapsed(),duration);
}
void Controller::receiveMagnifierFrame(QImage image){
 if(!targeting||paused||!capturePending)return;
 auto screen=QGuiApplication::primaryScreen();
 // Both capture backends now return only the primary display.
 if(image.isNull()||!screen||qAbs(double(image.width())/image.height()-double(screen->size().width())/screen->size().height())>.01){
  clearCapture();runner.cancel();endButtonHold();gameController.reset();activeJoystick=-1;pointer.cancel();actionStatus="Capture does not match the target display; target selection cancelled";emit changed();return;
 }
 capturedImage=image;captureSize=image.size();capturePending=false;magnifierReady=true;
 activeZoom=qBound(2,magnifierZoom,8);activeZoomDurationMs=qBound(500,zoomDurationMs,5000);
 zoomAnimating=progressiveZoom;zoomClock.start();
 updateMagnifierZoom();
 if(zoomAnimating)zoomTimer.start();
 emit changed();
}
void Controller::updateMagnifierZoom(){
 if(!magnifierReady||capturedImage.isNull())return;
 const double progress=zoomAnimating?qMin(1.,double(zoomClock.elapsed())/activeZoomDurationMs):1.;
 const double zoom=1.+(activeZoom-1)*progress;
 magnifierCrop=MagnifierGeometry::crop(captureSize,coarseTarget,zoom);
 magnifiedImage=capturedImage.copy(magnifierCrop);++magnifierRevision;
 const bool finished=zoomAnimating&&progress>=1.;
 if(progress>=1.){if(zoomAnimating)pointDwell.reset();zoomAnimating=false;zoomTimer.stop();}
 emit magnifierFrameChanged();
 if(finished)emit changed();
}
void Controller::clearCapture(){
 pointDwell.reset();
 zoomTimer.stop();zoomAnimating=false;capturedImage={};
 ++captureEpoch;screenshot.cancel();capturePending=false;magnifierReady=false;magnifiedImage={};magnifierCrop={};captureSize={};
}
void Controller::setMagnifierEnabled(bool enabled){
 if(magnifierEnabled==enabled)return;
 magnifierEnabled=enabled;
 // Save only the toggle; other preferences retain their explicit Save behavior.
 ProfileStore store(configFile("profile.json"));auto saved=store.load().values;
 saved["magnifierEnabled"]=enabled;QString error;
 if(!store.save(saved,error))profileStatus="Could not remember magnifier state. "+error;
 if(!enabled&&(capturePending||magnifierReady)){clearCapture();runner.cancel();endButtonHold();gameController.reset();activeJoystick=-1;pointer.cancel();}
 emit changed();
}
bool Controller::hasScratchpad()const{
 for(const auto &entry:layout.value("keys").toList())
  if(entry.toMap().value("kind").toString()=="Scratchpad")return true;
 return false;
}
QString &Controller::typingContext(){return hasScratchpad()?scratchpad:predictionContext;}
const QString &Controller::typingContext()const{return hasScratchpad()?scratchpad:predictionContext;}
void Controller::updateText(QString text){
 if(sensitiveInput)return;
 auto &context=typingContext();
 for(auto ch:text){if(ch=='\b'){if(!context.isEmpty()){QTextBoundaryFinder boundary(QTextBoundaryFinder::Grapheme,context);boundary.toEnd();context.truncate(qMax(0,boundary.toPreviousBoundary()));}}else{if(ch.isSpace()&&learning){QString word=WordPrediction::completedWord(context);if(!word.isEmpty()&&word.size()<64)frequencies[word]=qMin(1000000,frequencies.value(word)+1);}context+=ch;}}
 if(context.size()>4096)context=context.right(4096);updateSuggestions();
}
QStringList Controller::predictionSuggestions(int limit)const{
 const auto &context=typingContext();
 if(sensitiveInput||predictionModeValue=="off"||(!context.isEmpty()&&!nextWordEnabled&&WordPrediction::prefix(context).isEmpty()))return {};
 return WordPrediction::suggest(context,dictionary,learning?frequencies:QMap<QString,int>{},limit,predictionModeValue=="context",sentenceStartersEnabled);
}
void Controller::setPredictionMode(QString mode){
 if(!QStringList{"context","frequency","off"}.contains(mode)||predictionModeValue==mode)return;
 predictionModeValue=mode;dwell.reset();updateSuggestions();
}
void Controller::setSuggestSentenceStarters(bool enabled){
 if(sentenceStartersEnabled==enabled)return;
 sentenceStartersEnabled=enabled;dwell.reset();updateSuggestions();
}
void Controller::setAddSpaceAfterCompletion(bool enabled){
 if(completionSpaceEnabled==enabled)return;
 completionSpaceEnabled=enabled;dwell.reset();emit changed();
}
void Controller::setPredictNextWord(bool enabled){
 if(nextWordEnabled==enabled)return;
 nextWordEnabled=enabled;dwell.reset();updateSuggestions();
}
void Controller::setLearning(bool enabled){
 if(learning==enabled)return;
 learning=enabled;dwell.reset();updateSuggestions();
}
void Controller::updateSuggestions(){
 suggestionOffset=0;
 suggestions=predictionSuggestions();emit changed();
}
void Controller::resetLearning(){frequencies.clear();QFile::remove(configFile("learned.json"));updateSuggestions();}
void Controller::speak(){if(!sensitiveInput)speech.speak(scratchpad);}
void Controller::stopSpeech(){speech.stop();}

bool Controller::keyToggled(const QVariantList &commands)const{
 const auto resolved=ShortcutMappings::resolve(commands,mappingProfile,bypassPlatformMappings(literal));
 return resolved.allowed&&runner.isToggled(resolved.commands);
}

bool Controller::isSizeMoveKey(const QString &key) const{
 if(!key.startsWith("key:"))return false;
 bool ok=false;const int index=key.mid(4).toInt(&ok);const auto keys=layout["keys"].toList();
 if(!ok||index<0||index>=keys.size())return false;
 const auto commands=keys[index].toMap()["commands"].toList();
 return commands.size()==1&&commands[0].toMap()["tag"]=="Action"&&(commands[0].toMap()["text"].toString().trimmed()=="MoveModeToggle"||commands[0].toMap()["text"].toString().trimmed()=="SizeModeToggle");
}
bool Controller::setCanvasKeyRect(int index,double x,double y,double width,double height){
 auto keys=layout["keys"].toList();
 if(!sizeMoveMode||paused||layout["layoutMode"]!="canvas"||index<0||index>=keys.size()||
    !std::isfinite(x)||!std::isfinite(y)||!std::isfinite(width)||!std::isfinite(height)||
    x<0||y<0||width<=0||height<=0||x+width>layout["cols"].toDouble()||y+height>layout["rows"].toDouble())return false;
 auto entry=keys[index].toMap();entry["col"]=x;entry["row"]=y;entry["width"]=width;entry["height"]=height;
 keys[index]=entry;layout["keys"]=keys;dwell.reset();resetCanvasPlacement();
 canvasSaveStatus="Position and size changes are temporary until saved.";
 if(saveCanvasChanges)saveCanvasGeometry();
 emit layoutChanged();emit changed();return true;
}

void Controller::resetCanvasPlacement(){
 sizeMoveSelection=-1;canvasEditPreview.clear();canvasAnchorValid=false;canvasPlacementArmed=false;canvasPlacementBlocked=false;canvasDwell.reset();
}
double Controller::canvasEditAt(double x,double y,bool valid){
 if(runner.busy()){canvasDwell.reset();return 0;}
 if(!sizeMoveMode||paused||!valid||!std::isfinite(x)||!std::isfinite(y)||x<0||y<0||x>1||y>1){
  canvasDwell.reset();canvasAnchorValid=false;dwell.reset();return 0;
 }
 const QPointF point(x,y);
 const auto distance=[](QPointF a,QPointF b){return std::hypot(a.x()-b.x(),a.y()-b.y());};
 if(canvasPlacementBlocked){
  if(distance(point,canvasAnchor)<.03)return 0;
  canvasPlacementBlocked=false;dwell.reset();
 }
 const auto keys=layout["keys"].toList();const double cols=layout["cols"].toDouble(),rows=layout["rows"].toDouble();
 QString target;
 for(int i=keys.size()-1;i>=0;--i){const auto k=keys[i].toMap();
  if(x*cols>=k["col"].toDouble()&&x*cols<k["col"].toDouble()+k["width"].toDouble()&&
     y*rows>=k["row"].toDouble()&&y*rows<k["row"].toDouble()+k["height"].toDouble()){
   const QString candidate="key:"+QString::number(i);
   if(target.isEmpty()||isSizeMoveKey(candidate))target=candidate;
   if(isSizeMoveKey(candidate))break;
  }
 }
 if(sizeMoveSelection<0||isSizeMoveKey(target)){
  canvasDwell.reset();
  const int before=sizeMoveSelection;const double progress=hover(target);
  if(before<0&&sizeMoveSelection>=0){canvasAnchor=point;canvasAnchorValid=true;}
  return progress;
 }
 dwell.reset();
 if(!canvasAnchorValid){canvasAnchor=point;canvasAnchorValid=true;canvasDwell.reset();return 0;}
 if(!canvasPlacementArmed){
  if(distance(point,canvasAnchor)<.03)return 0;
  canvasPlacementArmed=true;canvasAnchor=point;canvasDwell.reset();
 }
 if(distance(point,canvasAnchor)>.015){canvasAnchor=point;canvasDwell.reset();}
 auto preview=keys[sizeMoveSelection].toMap();
 if(canvasEditMode=="move"){
  preview["col"]=qBound(0.0,x*cols-preview["width"].toDouble()/2,cols-preview["width"].toDouble());
  preview["row"]=qBound(0.0,y*rows-preview["height"].toDouble()/2,rows-preview["height"].toDouble());
 }else{
  preview["width"]=qBound(qMin(cols/100,cols-preview["col"].toDouble()),x*cols-preview["col"].toDouble(),cols-preview["col"].toDouble());
  preview["height"]=qBound(qMin(rows/100,rows-preview["row"].toDouble()),y*rows-preview["row"].toDouble(),rows-preview["row"].toDouble());
 }
 canvasEditPreview=preview;emit changed();
 if(canvasDwell.update("place",clock.elapsed(),dwellMs)){
  setCanvasKeyRect(sizeMoveSelection,preview["col"].toDouble(),preview["row"].toDouble(),preview["width"].toDouble(),preview["height"].toDouble());
  resetCanvasPlacement();canvasPlacementBlocked=true;canvasAnchor=point;emit changed();return 0;
 }
 return canvasDwell.progress(clock.elapsed(),dwellMs);
}

void Controller::updateKeyboardMask(QObject *object,const QVariantList &rectangles,bool enabled){
 auto window=qobject_cast<QWindow*>(object);if(!window)return;
 QRegion region;
 if(enabled){
  for(const auto &value:rectangles){const auto r=value.toRectF();if(!r.isEmpty())region+=r.toAlignedRect();}
  // An empty QWindow mask means the full window; use an off-window region for an empty layout.
  if(region.isEmpty())region=QRegion(-1,-1,1,1);
 }
 if(window->mask()!=region)window->setMask(region);
}

bool Controller::saveCanvasGeometry(){
 if(layout["layoutMode"]!="canvas"){canvasSaveStatus="Only Canvas keyboards support saving key geometry.";emit changed();return false;}
 QString error;
 const bool saved=KeyboardLayout::saveCanvasGeometry(canvasSourcePath,canvasSource,layout["keys"].toList(),error);
 canvasSaveStatus=saved?"Positions and sizes saved to "+canvasSourcePath:"Could not save: "+error+" Changes remain temporary.";
 emit changed();return saved;
}
