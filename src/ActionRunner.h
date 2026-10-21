#pragma once
#include <QObject>
#include <QTimer>
#include <QSet>
#include <QVariantList>
#include <functional>
// Ordered, nonblocking execution. Callbacks form the platform boundary.
class ActionRunner:public QObject {
    Q_OBJECT
public:
    explicit ActionRunner(QObject *parent=nullptr);
    std::function<bool(QString,bool)> sendKey;
    std::function<bool(QString)> sendText;
    std::function<bool()> canCommitText;
    std::function<bool(QString)> supportsPhysicalKey;
    std::function<bool(QString)> performAction;
    std::function<bool(QString)> supportsAction;
    std::function<void(QString,bool)> navigate;
    bool start(const QVariantList &commands);
    void cancel(bool preserveModifiers=false);
    void suspend();
    void resume(bool success);
    bool busy()const{return running;}
    bool isHeld(const QString &key)const;
    QStringList heldKeys()const{return held.values();}
    QVariantMap modifierStates()const;
    static QString modifierName(const QVariantList &commands);
    static QString validate(const QVariantList &commands,bool unicodeText=false);
    static QString signature(const QVariantList &commands);
    bool shifted()const;
signals:
    void stateChanged();
    void failed(QString message);
    void textCommitted(QString text);
    void finished();
private:
    void step();
    bool key(QString name,bool down,bool persistent=false);
    bool tap(QString name);
    bool shortcutHeld()const;
    bool cycleModifier(const QString &name);
    void fail(QString message);
    QVariantList queue;
    int index=0;
    bool running=false,waiting=false;
    QTimer timer;
    QSet<QString> held,transient,latched;
};
