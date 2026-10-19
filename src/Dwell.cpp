#include "Dwell.h"
#include <algorithm>
bool Dwell::update(const QString &target, qint64 now, int duration) {
    if (target.isEmpty() || now < last || (last >= 0 && now-last > 200)) reset();
    last=now;
    if (target.isEmpty()) return false;
    if (target != current) { current=target; started=now; fired=false; }
    if (!fired && now-started >= duration) { fired=true; return true; }
    return false;
}
void Dwell::reset() { current.clear(); started=0; last=-1; fired=false; }
double Dwell::progress(qint64 now, int duration) const {
    return current.isEmpty() ? 0 : std::clamp(double(now-started)/std::max(1,duration),0.0,1.0);
}

void Dwell::consume(const QString &target, qint64 now) {current=target;started=now;last=now;fired=true;}
