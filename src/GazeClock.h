#pragma once
#include <QtGlobal>
#include <chrono>
namespace GazeClock {
inline qint64 now(){return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();}
inline bool fresh(qint64 sample,qint64 current,qint64 previous){return sample>previous&&current>=sample&&current-sample<=200000;}
}
