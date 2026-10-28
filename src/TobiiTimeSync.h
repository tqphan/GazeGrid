#pragma once
#include <QtGlobal>

// Maintain device clock synchronization independently of gaze validity.
// Steady-clock microseconds keep this testable.
class TobiiTimeSync {
public:
 template<class Synchronize> void update(qint64 now,Synchronize synchronize){
  if(lastAttempt>=0&&now>=lastAttempt&&now-lastAttempt<interval)return;
  lastAttempt=now;
  // Packet loss can make synchronization fail. Retry without flooding the SDK
  // or stopping otherwise usable gaze input.
  interval=synchronize()?30000000:1000000;
 }
 void reset(){lastAttempt=-1;interval=0;}
private:
 qint64 lastAttempt=-1,interval=0;
};
