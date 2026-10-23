#pragma once
#include <QObject>
#include <QMap>
#include <functional>

// Standard Linux gamepad plus extended joystick axes/buttons.
class GameController : public QObject {
    Q_OBJECT
public:
    explicit GameController(QObject *parent=nullptr):QObject(parent){}
    ~GameController() override;
    static bool supports(const QString &action);
    static QStringList actions();
    bool held(const QString &action) const;
    bool perform(const QString &action);
    bool stick(const QString &name,double x,double y);
    void reset();
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
    int sequence=0;
    QString message;
    QMap<int,int> buttons,axes;
    QMap<int,int> generations;
};
