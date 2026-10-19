#include "PointerCheck.h"
#include <QCoreApplication>
#include <QGuiApplication>
#include <QScreen>
#include <QJsonDocument>
#include <QTextStream>
#include <QLineF>
PointerCheck::PointerCheck(Controller *controller,bool magnifierTest,QObject *parent):QObject(parent),controller(controller),magnifierTest(magnifierTest){
 if(magnifierTest)actions={"MouseMoveAndLeftClick"};
 timer.setInterval(25);connect(&timer,&QTimer::timeout,this,&PointerCheck::tick);
 connect(&receiver,&QProcess::readyReadStandardOutput,this,&PointerCheck::consume);
 connect(&receiver,&QProcess::errorOccurred,this,[this](QProcess::ProcessError){fail("Could not run pointer receiver");});
 connect(&receiver,&QProcess::finished,this,[this](int,QProcess::ExitStatus){if(!ending)fail("Pointer receiver closed before completion");});
 connect(controller->output(),&PortalInput::status,this,[](QString message){QTextStream(stdout)<<message<<Qt::endl;});
 connect(controller->output(),&PortalInput::readyChanged,this,[this]{
  if(!this->controller->ready()){if(everStarted)fail("Desktop permission or device lost");return;}
  if(receiver.state()==QProcess::NotRunning&&!ending){receiver.start(QCoreApplication::applicationFilePath(),{"--pointer-target"});timer.start();}
 });
}
PointerCheck::~PointerCheck(){
 ending=true;timer.stop();
 if(receiver.state()!=QProcess::NotRunning){receiver.terminate();if(!receiver.waitForFinished(1000)){receiver.kill();receiver.waitForFinished(1000);}}
}
void PointerCheck::start(){
 QTimer::singleShot(0,controller->output(),&PortalInput::connectPortal);
 QTimer::singleShot(magnifierTest?120000:60000,this,[this]{fail("Pointer check timed out; focus the disposable receiver if it appeared");});
}
void PointerCheck::fail(const QString &message){
 if(ending)return;ending=true;timer.stop();
 if(!controller->property("paused").toBool())controller->togglePause();
 QTextStream(stdout)<<"FAIL: "<<message<<Qt::endl;QCoreApplication::exit(8);
}
void PointerCheck::consume(){
 buffer+=receiver.readAllStandardOutput();
 while(buffer.contains('\n')){
  const auto end=buffer.indexOf('\n');const auto object=QJsonDocument::fromJson(buffer.left(end)).object();buffer.remove(0,end+1);
  if(object.contains("active")){
   active=object["active"].toBool();receiverSize={object["width"].toInt(),object["height"].toInt()};
   const bool awaitingCapture=magnifierTest&&(controller->property("capturePending").toBool()||controller->property("magnifierReady").toBool());
   if(!active&&everStarted&&!awaitingCapture){fail("Disposable pointer receiver lost focus");return;}
  }
  if(!started||ending)continue;
  auto event=object["pointer"].toString();const int expected=phase==4?int(Qt::RightButton):int(Qt::LeftButton);
  if(event=="press"&&object["button"].toInt()==expected){
   const auto time=object["time"].toInteger();
   if(!pressTimes.contains(time)){pressTimes.insert(time);++presses;}
  }
  if(event=="release"&&object["button"].toInt()==expected){++releases;releasePoint={object["x"].toDouble(),object["y"].toDouble()};}
  if(event=="move"&&(object["buttons"].toInt()&Qt::LeftButton))++dragMoves;
  if(event=="wheel"&&(object["dx"].toDouble()!=0||object["dy"].toDouble()!=0))++wheels;
 }
}
void PointerCheck::tick(){
 if(ending||!active||!controller->ready())return;
 if(!started){
  const auto screen=QGuiApplication::primaryScreen();
  if(!screen||QGuiApplication::screens().size()!=1||receiverSize!=screen->size()||!controller->output()->absoluteAvailable()){
   fail("Pointer check requires a full-screen receiver and one mapped EIS display");return;
  }
  presses=releases=dragMoves=wheels=targets=0;pressTimes.clear();releasePoint={};settled.invalidate();started=true;everStarted=true;
  if(magnifierTest)controller->setProperty("magnifierEnabled",true);
  if(!controller->executeCommands({QVariantMap{{"tag","Action"},{"text",actions[phase]}}})){fail("Could not start pointer action");return;}
 }
 if(controller->property("targeting").toBool()){
  if(controller->property("capturePending").toBool())return;
  const bool magnified=controller->property("magnifierReady").toBool();
  if(magnified){
   const auto image=controller->magnifierImage();
   if(image.isNull()){fail("Magnifier returned no image");return;}
   int matching=0;
   for(int y=-1;y<=1;++y)for(int x=-1;x<=1;++x){
    const auto colour=image.pixelColor(image.width()/2+x*image.width()/8,image.height()/2+y*image.height()/8);
    if(colour.green()-colour.red()>=15&&colour.blue()>=150)++matching;
   }
   if(matching<5){fail("Magnifier image does not match the disposable receiver target");return;}
   captureSeen=true;
  }
  const QPointF point=magnified?QPointF(.5,.5):phase==2?(targets==0?QPointF(.3,.25):QPointF(.65,.4)):QPointF(.4,.25);
  controller->targetAt(point.x(),point.y());++targets;return;
 }
 if(controller->executingCommands())return;
 if(!settled.isValid()){settled.start();return;}
 if(settled.elapsed()<150)return;
 bool ok=false;
 if(phase==0||phase==4)ok=presses==1&&releases==1&&QLineF(releasePoint,QPointF(.4,.25)).length()<.015;
 if(phase==1)ok=presses==2&&releases==2;
 if(phase==2)ok=presses==1&&releases==1&&dragMoves>=4&&QLineF(releasePoint,QPointF(.65,.4)).length()<.015;
 if(phase==3)ok=wheels>0;
 if(magnifierTest)ok=ok&&captureSeen;
 if(!ok){fail(actions[phase]+QString(" receiver mismatch (press %1, release %2, drag moves %3, wheel %4)").arg(presses).arg(releases).arg(dragMoves).arg(wheels));return;}
 QTextStream(stdout)<<"PASS: "<<(magnifierTest?"portal magnification and precise target click":actions[phase])<<" received by the focused Wayland test window"<<Qt::endl;
 ++phase;started=false;
 if(phase==actions.size()){ending=true;timer.stop();controller->togglePause();QCoreApplication::exit(0);}
}
