#pragma once
#include <QObject>
#include <QThread>
#include <atomic>
#include <functional>

class SpeechOutput:public QObject {
 Q_OBJECT
public:
 struct Backend {
  std::function<bool()> open;
  std::function<bool(const QString &)> say;
  std::function<bool()> cancel;
  std::function<void()> close;
 };
 static Backend nativeBackend();
 explicit SpeechOutput(QObject *parent=nullptr);
 explicit SpeechOutput(Backend backend,QObject *parent=nullptr);
 ~SpeechOutput()override;
 void speak(QString text);
 void stop();
signals:
 void status(QString message);
private:
 Backend backend;
 QThread thread;
 QObject *worker;
 std::atomic<quint64> generation{0};
 bool connected=false; // Worker thread only.
};
