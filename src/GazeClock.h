#pragma once
#include <QtGlobal>
#include <chrono>
namespace GazeClock {
inline qint64 now(){return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();}
}
