#include "LookScroll.h"
#include <algorithm>
#include <cmath>
QPointF LookScroll::update(QPointF point,qint64 now,bool overDesktop){
    if(!enabled)return {};
    if(!std::isfinite(point.x())||!std::isfinite(point.y())||now<last||now-last>200){stop();return {};}
    const double seconds=std::min<qint64>(now-last,100)/1000.0;last=now;
    if(!overDesktop)return {};
    auto velocity=[](double distance){
        const double outside=std::abs(distance)-50;
        return outside<0?0.0:std::copysign(.1+outside*.1,distance);
    };
    return QPointF(velocity(point.x()-origin.x()),velocity(point.y()-origin.y()))*(120*seconds);
}
