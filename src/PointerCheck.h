#pragma once
#include <QObject>
#include <QProcess>
#include <QTimer>
#include <QElapsedTimer>
#include <QJsonObject>
#include "Controller.h"

// Explicit integration harness: output is restricted to a focused full-screen receiver.
class PointerCheck:public QObject {
 Q_OBJECT
public:
 explicit PointerCheck(Controller *controller,bool magnifierTest=false,QObject *parent=nullptr);
 ~PointerCheck()override;
 void start();
private:
 void consume();
 void tick();
 void fail(const QString &message);
 Controller *controller;
 QProcess receiver;
 QTimer timer;
 QElapsedTimer settled;
 QByteArray buffer;
 bool active=false,started=false,everStarted=false,ending=false;
 bool magnifierTest=false,captureSeen=false;
 int phase=0,targets=0,presses=0,releases=0,dragMoves=0,wheels=0;
 QPointF releasePoint;
 QSize receiverSize;
 QSet<qint64> pressTimes;
 QStringList actions{"MouseMoveAndLeftClick","MouseMoveAndLeftDoubleClick","MouseDrag","MouseScrollToBottom","MouseMoveAndRightClick"};
};
