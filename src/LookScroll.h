#pragma once
#include <QPointF>

// Defaults preserve free-axis scrolling, medium speed and a 50-pixel deadzone.
// Time is supplied by the caller so replay is deterministic.
class LookScroll {
public:
    void configure(int speedPercent,int deadzonePixels,int axes);
    void start(QPointF centre, qint64 now){origin=centre;last=now;enabled=true;}
    void stop(){enabled=false;last=-1;}
    void resetTime(qint64 now){last=now;}
    bool active()const{return enabled;}
    QPointF centre()const{return origin;}
    QPointF update(QPointF point,qint64 now,bool overDesktop);
private:
    int speed=100,deadzone=50,axis=0;
    QPointF origin;
    qint64 last=-1;
    bool enabled=false;
};
