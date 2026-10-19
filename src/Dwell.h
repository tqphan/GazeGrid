#pragma once
#include <QString>
// Monotonic caller-supplied time keeps dwell behavior deterministic in tests.
class Dwell {
public:
    bool update(const QString &target, qint64 now, int duration);
    void reset();
    void consume(const QString &target, qint64 now);
    double progress(qint64 now, int duration) const;
private:
    QString current;
    qint64 started = 0, last = -1;
    bool fired = false;
};
