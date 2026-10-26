#pragma once
#include <QObject>
#include <QMap>
#include <QSet>
#include <functional>
#ifdef _WIN32
#include "WindowsGamepad.h"
#endif

// Shared gamepad actions; Linux uinput or Windows ViGEmBus output.
class GameController : public QObject {
    Q_OBJECT
public:
    explicit GameController(QObject *parent=nullptr):QObject(parent){}
    ~GameController() override;
    static bool supports(const QString &action);
    static QStringList actions();
    static QString buttonAction(const QString &action,const QString &mode);
    bool held(const QString &action) const;
    bool perform(const QString &action);
    bool stick(const QString &name,double x,double y);
    void reset(bool preserveToggles=false);
    QString error() const{return message;}
    // Test/platform boundary: one complete event frame.
    std::function<bool(int,int,int)> output;
signals:
    void changed();
    void failed(QString message);
private:
    bool connectDevice();
    bool send(int type,int code,int value);
    void closeDevice();
    int fd=-1;
#ifdef _WIN32
    WindowsGamepad windowsPad;
#endif
    int sequence=0;
    QString message;
    QMap<int,int> buttons,axes;
    QMap<int,int> generations;
    QSet<int> toggledButtons;
};
