#include "Dwell.h"
#include <algorithm>
bool Dwell::update(const QString &target, qint64 now, int duration, bool repeat, int repeatDuration) {
    if (target.isEmpty() || now < last || (!fired && last >= 0 && now-last > 200)) reset();
    last=now;
    if (target.isEmpty()) return false;
    if (target != current) { current=target; started=now; fired=false; repeating=false; }
    if (repeating && !repeat) { fired=true; repeating=false; }
    if (fired && repeat) { fired=false; repeating=true; started=now; }
    const int interval=repeating&&repeatDuration>0?repeatDuration:duration;
    if (!fired && now-started >= interval) { fired=true; return true; }
    return false;
}
void Dwell::inhibit(const QString &target, qint64 now) {
    // Busy actions must not rearm the key that started them. A different
    // target starts from zero once input becomes available again.
    if (target.isEmpty() || target != current || !fired || now < last) reset();
    else last=now;
}
void Dwell::reset() { current.clear(); started=0; last=-1; fired=false; repeating=false; }
void Dwell::pause(qint64 now) { if(last>=0&&now>=last){started+=now-last;last=now;} }
double Dwell::progress(qint64 now, int duration, int repeatDuration) const {
    const int interval=repeating&&repeatDuration>0?repeatDuration:duration;
    return current.isEmpty() || fired ? 0 : std::clamp(double(now-started)/std::max(1,interval),0.0,1.0);
}

void Dwell::consume(const QString &target, qint64 now) {current=target;started=now;last=now;fired=true;repeating=false;}
