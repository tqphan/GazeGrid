#pragma once
#include <QThread>
#include <QVariantMap>
class TobiiInput:public QThread {
    Q_OBJECT
public:
    explicit TobiiInput(QObject *parent=nullptr):QThread(parent){}
    ~TobiiInput() override;
    static QVariantMap probe();
signals:
    void status(QString message);
    // Timestamp is sample time mapped into the host steady clock, in microseconds.
    void gaze(double x,double y,bool valid,qint64 timestamp);
protected:
    void run() override;
};
