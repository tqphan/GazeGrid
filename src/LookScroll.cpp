#include "LookScroll.h"
#include <algorithm>
#include <cmath>
void LookScroll::configure(int speedPercent,int deadzonePixels,int axes){
    speed=std::clamp(speedPercent,10,300);deadzone=std::clamp(deadzonePixels,10,200);axis=std::clamp(axes,0,2);
}
QPointF LookScroll::update(QPointF point,qint64 now,bool overDesktop){
    if(!enabled)return {};
    if(!std::isfinite(point.x())||!std::isfinite(point.y())||now<last||now-last>200){stop();return {};}
    const double seconds=std::min<qint64>(now-last,100)/1000.0;last=now;
    if(!overDesktop)return {};
    auto velocity=[this](double distance){
        const double outside=std::abs(distance)-deadzone;
        return outside<0?0.0:std::copysign(.1+outside*.1,distance);
    };
    return QPointF(axis==1?0:velocity(point.x()-origin.x()),axis==2?0:velocity(point.y()-origin.y()))*(120*seconds*speed/100.0);
}
