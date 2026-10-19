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
#include "ProfileStore.h"
#include <QTimer>
#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QStandardPaths>
#include <QTextBoundaryFinder>
namespace {
QVariantMap command(QString tag,QString value){return {{"tag",tag},{"text",value}};}
QString configFile(const QString &name){auto dir=QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);QDir().mkpath(dir);return QDir(dir).filePath(name);}
QString panelPlacementReason(const QVariantMap &layout){
 if(layout["position"].toString()!="Bottom")return "Moving the keyboard to the top is unavailable in Plasma keyboard mode. Use standalone mode for top placement.";
 if(layout.value("verticaloffset","0").toString().remove('%').toDouble()!=0||layout.value("horizontaloffset","0").toString().remove('%').toDouble()!=0)
  return "Moving the keyboard off screen is unavailable in Plasma keyboard mode. Use standalone mode for this layout.";
 return {};
}
}
Controller::Controller(){
 connect(&speech,&SpeechOutput::status,this,[this](QString message){actionStatus=message;emit changed();});
 clock.start();auto profile=ProfileStore(configFile("profile.json")).load();
 dwellMs=profile.values["dwellMs"].toInt();literal=profile.values["literal"].toBool();learning=profile.values["learning"].toBool();profileStatus=profile.message;
 scalePercent=profile.values["keyboardScale"].toInt(100);highContrast=profile.values["highContrast"].toBool();
 rememberDesktop=profile.values["rememberDesktop"].toBool();
 layoutNames=QDir(QStringLiteral(DEFAULT_PACK_PATH)).entryList({"*.xml"},QDir::Files,QDir::Name);for(auto &name:layoutNames)name.chop(4);
 for(const auto &name:layoutNames){
  auto item=KeyboardLayout::fromXml(KeyboardPack::load(QDir(QStringLiteral(DEFAULT_PACK_PATH)).filePath(name+".xml")));
  if(!item.contains("error")&&item["hidefromkeyboardmenu"].toString().compare("true",Qt::CaseInsensitive)!=0)menuLayoutNames<<name;
 }
 QFile words(":/resources/EnglishUS.dic");if(words.open(QIODevice::ReadOnly)){dictionary=QString::fromUtf8(words.readAll()).remove(QChar(0xfeff)).split('\n',Qt::SkipEmptyParts);for(auto &word:dictionary)word=word.trimmed();dictionary.sort(Qt::CaseInsensitive);}
 QFile learned(configFile("learned.json"));if(learned.open(QIODevice::ReadOnly)){auto obj=QJsonDocument::fromJson(learned.readAll()).object();for(auto it=obj.begin();it!=obj.end();++it)frequencies[it.key()]=qBound(0,it.value().toInt(),1000000);}
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
 pointer.move=[this](double x,double y){return portal.moveAbsolute(x,y);};
 pointer.moveRelative=[this](double x,double y){return portal.move(x,y);};
 pointer.button=[this](int code,bool down){return portal.button(code,down);};
 pointer.scroll=[this](double amount){return portal.scroll(amount);};
 pointer.scrollAxes=[this](double x,double y){return portal.scrollAxes(x,y);};
 pointer.releaseAll=[this]{portal.releaseAll();};
 connect(&pointer,&PointerActions::changed,this,[this]{targeting=pointer.targeting();if(!targeting)clearCapture();emit changed();});
 connect(&screenshot,&ScreenshotPortal::failed,this,[this](QString error){
  clearCapture();runner.cancel();pointer.cancel();actionStatus=error;emit changed();
 });
 connect(&screenshot,&ScreenshotPortal::captured,this,[this](QImage image){
  if(!capturePending||!targeting||paused)return;
  auto screen=QGuiApplication::primaryScreen();
  if(!screen||QGuiApplication::screens().size()!=1||qAbs(double(image.width())/image.height()-double(screen->size().width())/screen->size().height())>.01){
   clearCapture();runner.cancel();pointer.cancel();actionStatus="Capture does not match the single display; target selection cancelled";emit changed();return;
  }
  captureSize=image.size();magnifierCrop=MagnifierGeometry::crop(captureSize,coarseTarget);
  magnifiedImage=image.copy(magnifierCrop);capturePending=false;magnifierReady=true;++magnifierRevision;emit changed();
 });
 connect(&pointer,&PointerActions::completed,this,[this](bool ok){actionStatus=pointer.prompt();runner.resume(ok);emit changed();});
 runner.sendKey=[this](QString name,bool down){return portal.key(KeyNames::linuxCode(name),down);};
 runner.performAction=[this](QString action){return performAction(action);};
 runner.supportsAction=ActionCatalog::supported;
 runner.navigate=[this](QString name,bool back){openLayout(name,back);};
 connect(&runner,&ActionRunner::failed,this,[this](QString m){actionStatus=m;emit changed();});
 connect(&runner,&ActionRunner::stateChanged,this,&Controller::changed);
 connect(&runner,&ActionRunner::textCommitted,this,&Controller::updateText);
 connect(&portal,&PortalInput::status,this,[this](QString m){portalStatus=m;emit changed();});
 connect(&portal,&PortalInput::readyChanged,this,[this]{dwell.reset();if(!ready()){runner.cancel();pointer.cancel();}emit changed();});
 connect(&tobii,&TobiiInput::status,this,[this](QString m){tobiiStatus=m;emit changed();});
 connect(&tobii,&TobiiInput::gaze,this,[this](double x,double y,bool valid,qint64 sampledAt){
  valid=valid&&GazeClock::fresh(sampledAt,GazeClock::now(),lastGazeSample)&&x>=0&&x<=1&&y>=0&&y<=1;
  if(valid){lastGazeSample=sampledAt;gazeAge.restart();}else gazeAge.invalidate();
  if(!valid){dwell.reset();runner.cancel();portal.releaseAll();pointer.cancel();}emit gazePoint(x,y,valid);
 });
 auto timer=new QTimer(this);timer->setInterval(25);connect(timer,&QTimer::timeout,this,[this]{
  if(gazeAge.isValid()&&GazeClock::now()-lastGazeSample>200000){gazeAge.invalidate();dwell.reset();runner.cancel();portal.releaseAll();pointer.cancel();emit gazePoint(0,0,false);}
 });timer->start();
 auto start=profile.values["startLayout"].toString();
 if(!layoutNames.contains(start)){start=ProfileStore::defaults()["startLayout"].toString();profileStatus="Saved keyboard is unavailable. Opened the default keyboard.";}
 openLayout(start,false);
}
Controller::~Controller(){runner.cancel();pointer.cancel();portal.releaseAll();portal.close();
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
 runner.cancel();pointer.cancel();portal.releaseAll();dwell.reset();speech.stop();scratchpad.clear();suggestions.clear();emit changed();
}
void Controller::inputContextChanged(bool active,bool sensitive){
 nativeActive=active;sensitiveInput=sensitive;
 if(!active||sensitive)invalidateInputContext();
 actionStatus=active?(sensitive?"Native input ready — private field":"Native text input ready"):"Select a text field to use native input";emit changed();
}
bool Controller::savePreferences(){
 QJsonObject values{{"version",1},{"dwellMs",dwellMs},{"literal",literal},{"learning",learning},{"startLayout",layoutName},{"keyboardScale",scalePercent},{"highContrast",highContrast},{"rememberDesktop",rememberDesktop}};
 QString error;bool ok=ProfileStore(configFile("profile.json")).save(values,error);
 profileStatus=ok?"Preferences saved. This keyboard will open at startup.":error;emit changed();return ok;
}
void Controller::resetPreferences(){
 QString error;auto values=ProfileStore::defaults();
 if(!ProfileStore(configFile("profile.json")).save(values,error,true)){profileStatus=error;emit changed();return;}
 if(!paused)togglePause();
 dwellMs=values["dwellMs"].toInt();literal=false;learning=false;backStack.clear();
 setKeyboardScale(100);highContrast=false;rememberDesktop=false;
 openLayout(values["startLayout"].toString(),false);
 profileStatus="Preferences reset. Input remains paused.";emit changed();
}
void Controller::setKeyboardScale(int scale){
 scale=qBound(50,scale,150);if(scale==scalePercent)return;
 runner.cancel();pointer.cancel();dwell.reset();scalePercent=scale;emit geometryPreferencesChanged();emit changed();
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
 auto result=ShortcutMappings::resolve(commands,mappingProfile,literal);ok=result.allowed;actionStatus=result.message;return result.commands;
}
void Controller::togglePause(){paused=!paused;dwell.reset();runner.cancel();portal.releaseAll();pointer.cancel();emit changed();}
void Controller::setGazeEnabled(bool enabled){
 gazeEnabled=enabled;dwell.reset();
 if(!enabled){runner.cancel();pointer.cancel();}
 emit changed();
}
void Controller::trackGaze(double x,double y,bool overDesktop){
 if(gazeEnabled&&!paused&&ready()&&gazeAge.isValid()&&GazeClock::now()-lastGazeSample<=200000)pointer.follow({x,y},overDesktop);
}
void Controller::displayChanged(){
 if(!paused)togglePause();else{runner.cancel();pointer.cancel();dwell.reset();}
 gazeAge.invalidate();emit gazePoint(0,0,false);
 actionStatus="Display configuration changed. Check tracker calibration and resume when ready.";emit changed();
}
double Controller::hover(QString key){
 const bool recovery=key=="Pause"||key=="Recover"||key=="CancelTarget"||key=="Connect"||key=="Tobii"||key=="Settings"||key=="Speak"||key=="StopSpeech";
 if(!recovery&&(paused||runner.busy())){dwell.reset();return 0;}
 int duration=dwellMs;
 if(key.startsWith("key:")){int i=key.mid(4).toInt();auto keys=layout["keys"].toList();if(i>=0&&i<keys.size()){int custom=keys[i].toMap()["dwell"].toInt();if(custom>0)duration=custom;}}
 if(dwell.update(key,clock.elapsed(),duration)){activate(key);dwell.consume(key,clock.elapsed());}
 return dwell.progress(clock.elapsed(),duration);
}
void Controller::clickCommand(QString key){activate(key);dwell.consume(key,clock.elapsed());}
void Controller::activate(QString key){
 if(key=="Settings"){if(!paused)togglePause();emit settingsRequested();return;}
 if(key=="Speak"){speak();return;}
 if(key=="StopSpeech"){stopSpeech();return;}
 if(key=="Pause"){togglePause();return;}
 if(key=="Recover"){recover();return;}
 if(key=="Connect"){connectDesktop();return;}
 if(key=="Tobii"){connectTobii();return;}
 if(key=="CancelTarget"){targeting=false;runner.cancel();portal.releaseAll();pointer.cancel();emit changed();return;}
 if(paused)return;
 if(key.startsWith("suggest:")){
  bool valid=false;int i=key.mid(8).toInt(&valid);if(!valid||i<0||i>=suggestions.size()||!ready())return;
  if(!runner.heldKeys().isEmpty()){actionStatus="Release held keys before accepting a suggestion";emit changed();return;}
  QString prefix=WordPrediction::prefix(scratchpad);auto word=suggestions[i];
  runner.start({command("Text",word.mid(prefix.size())+" ")});return;
 }
 if(!key.startsWith("key:"))return;
 bool valid=false;int i=key.mid(4).toInt(&valid);auto keys=layout["keys"].toList();if(!valid||i<0||i>=keys.size())return;
 const auto unavailable=keys[i].toMap()["unavailableReason"].toString();
 if(!unavailable.isEmpty()){actionStatus=unavailable;emit changed();return;}
 auto commands=keys[i].toMap()["commands"].toList();bool ok=false;commands=mapped(commands,ok);
 if(!ok){emit changed();return;}
 // Navigation, sleep, and scratchpad clearing work without a portal session.
 bool needsOutput=false;for(auto v:commands){auto c=v.toMap();auto tag=c["tag"].toString(),value=c["text"].toString().trimmed();if(tag!="ChangeKeyboard"&&tag!="Wait"&&!(tag=="Action"&&(value=="BackFromKeyboard"||value=="MenuKeyboard"||value=="Sleep"||value=="ClearScratchpad")))needsOutput=true;}
 if(needsOutput&&!ready()){actionStatus="Authorize desktop control before sending input";emit changed();return;}
 runner.start(commands);emit changed();
}
void Controller::openLayout(QString name,bool backReturnsHere){
 if(name.endsWith(".xml"))name.chop(4);
 if(!layoutNames.contains(name)){actionStatus="Unknown keyboard: "+name;emit changed();return;}
 auto compiled=KeyboardLayout::fromXml(KeyboardPack::load(QDir(QStringLiteral(DEFAULT_PACK_PATH)).filePath(name+".xml")));
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
    const auto target=KeyboardLayout::fromXml(KeyboardPack::load(QDir(QStringLiteral(DEFAULT_PACK_PATH)).filePath(destination+".xml")));
    if(target.contains("error"))continue;
    const auto unavailable=panelPlacementReason(target);
    if(!unavailable.isEmpty()){key["unavailableReason"]=unavailable;break;}
   }
   value=key;
  }
  compiled["keys"]=keys;
 }
 runner.cancel();portal.releaseAll();pointer.cancel();dwell.reset();
 if(backReturnsHere&&!layoutName.isEmpty()&&layoutName!=name){backStack<<layoutName;if(backStack.size()>64)backStack.removeFirst();}
 layout=compiled;layoutName=name;emit layoutChanged();emit changed();
}
void Controller::recover(){paused=false;backStack.clear();openLayout("main-14x1-bottom",false);}
bool Controller::performAction(QString action){
 if(action=="MouseMagnifier"){
  magnifierEnabled=!magnifierEnabled;actionStatus=magnifierEnabled?"Magnifier enabled: target selection will request desktop capture":"Magnifier disabled";emit changed();return true;
 }
 if(action=="ClearScratchpad"){scratchpad.clear();updateSuggestions();return true;}
 if(action=="Sleep"){togglePause();return true;}
 if(action=="MenuKeyboard"){recover();return true;}
 if(action=="BackFromKeyboard"){openLayout(backStack.isEmpty()?"main-14x1-bottom":backStack.takeLast(),false);return true;}
 if(PointerActions::supports(action)){
  if(PointerActions::needsAbsolute(action)&&!portal.absoluteAvailable()){
   actionStatus="Pointer targeting requires one mapped EIS display region";emit changed();return false;
  }
  runner.suspend();
  if(!pointer.start(action)){actionStatus="Pointer action unavailable: release held buttons or complete an action before repeating";emit changed();return false;}
  return true;
 }
 actionStatus="Not implemented yet: "+action;emit changed();return false;
}
void Controller::targetAt(double x,double y){
 if(paused||!ready()||!targeting||capturePending||!MagnifierGeometry::validPoint({x,y}))return;
 if(magnifierReady){
  const auto point=MagnifierGeometry::map(magnifierCrop,captureSize,{x,y});clearCapture();pointer.select(point);emit changed();return;
 }
 if(magnifierEnabled){
  if(QGuiApplication::screens().size()!=1){actionStatus="Magnification requires one display";emit changed();return;}
  coarseTarget={x,y};capturePending=true;const auto epoch=++captureEpoch;emit changed();
  QTimer::singleShot(150,this,[this,epoch]{if(epoch==captureEpoch&&capturePending&&!paused&&targeting)screenshot.capture();});return;
 }
 pointer.select(QPointF(x,y));
}
void Controller::clearCapture(){
 ++captureEpoch;screenshot.cancel();capturePending=false;magnifierReady=false;magnifiedImage={};magnifierCrop={};captureSize={};
}
void Controller::updateText(QString text){
 if(sensitiveInput)return;
 for(auto ch:text){if(ch=='\b'){if(!scratchpad.isEmpty()){QTextBoundaryFinder boundary(QTextBoundaryFinder::Grapheme,scratchpad);boundary.toEnd();scratchpad.truncate(qMax(0,boundary.toPreviousBoundary()));}}else{if(ch.isSpace()&&learning){QString word=WordPrediction::completedWord(scratchpad);if(!word.isEmpty()&&word.size()<64)frequencies[word]=qMin(1000000,frequencies.value(word)+1);}scratchpad+=ch;}}
 if(scratchpad.size()>4096)scratchpad=scratchpad.right(4096);updateSuggestions();
}
void Controller::updateSuggestions(){
 suggestions=WordPrediction::suggest(scratchpad,dictionary,frequencies);emit changed();
}
void Controller::resetLearning(){frequencies.clear();QFile::remove(configFile("learned.json"));updateSuggestions();}
void Controller::speak(){if(!sensitiveInput)speech.speak(scratchpad);}
void Controller::stopSpeech(){speech.stop();}
