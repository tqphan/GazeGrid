#pragma once
#include <QPointF>
#include <QtGlobal>
#include <algorithm>
#include <cmath>

// Dwell around a fixed anchor, not a screen grid boundary.
class PointDwell {
public:
 bool update(QPointF point,qint64 now,int duration,double radius){
  if(!std::isfinite(point.x())||!std::isfinite(point.y())){reset();return false;}
  if(last<0||now<last||now-last>200||std::hypot(point.x()-anchor.x(),point.y()-anchor.y())>radius){
   anchor=point;started=now;fired=false;
  }
  last=now;
  if(!fired&&now-started>=duration){fired=true;return true;}
  return false;
 }
 double progress(qint64 now,int duration)const{return last<0?0:std::clamp(double(now-started)/std::max(1,duration),0.,1.);}
 void reset(){last=-1;started=0;fired=false;}
private:
 QPointF anchor;
 qint64 last=-1,started=0;
 bool fired=false;
};
