#pragma once
#include <QPointF>

// Legacy default: free-axis scrolling, medium speed, 50-pixel deadzone.
// Time is supplied by the caller so replay is deterministic.
class LookScroll {
public:
    void start(QPointF centre, qint64 now){origin=centre;last=now;enabled=true;}
    void stop(){enabled=false;last=-1;}
    bool active()const{return enabled;}
    QPointF update(QPointF point,qint64 now,bool overDesktop);
private:
    QPointF origin;
    qint64 last=-1;
    bool enabled=false;
};
