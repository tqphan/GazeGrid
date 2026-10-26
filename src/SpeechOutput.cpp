#include "SpeechOutput.h"
SpeechOutput::SpeechOutput(QObject *parent):SpeechOutput(nativeBackend(),parent){}
SpeechOutput::SpeechOutput(Backend backend,QObject *parent):QObject(parent),backend(std::move(backend)),worker(new QObject){
 worker->moveToThread(&thread);connect(&thread,&QThread::finished,worker,&QObject::deleteLater);thread.start();
}
SpeechOutput::~SpeechOutput(){
 ++generation;
 QMetaObject::invokeMethod(worker,[this]{if(connected){backend.cancel();backend.close();connected=false;}},Qt::BlockingQueuedConnection);
 thread.quit();thread.wait();
}
void SpeechOutput::speak(QString text){
 if(text.isEmpty()){emit status("Scratchpad is empty");return;}
 const auto epoch=++generation;
 QMetaObject::invokeMethod(worker,[this,epoch,text=std::move(text)]{
  if(epoch!=generation)return;
  if(!connected)connected=backend.open();
  if(epoch!=generation)return;
  if(!connected){emit status("Speech service is unavailable; typing remains available");return;}
  if(!backend.say(text)){backend.close();connected=false;emit status("Speech request failed; retry after checking Speech service");return;}
  emit status("Speech queued");
 },Qt::QueuedConnection);
}
void SpeechOutput::stop(){
 ++generation;
 QMetaObject::invokeMethod(worker,[this]{
  const bool ok=!connected||backend.cancel();
  if(!ok){backend.close();connected=false;}
  emit status(ok?"Speech stopped":"Could not cancel speech; connection closed");
 },Qt::QueuedConnection);
}
