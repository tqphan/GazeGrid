#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <QObject>
#include <QStringList>
#include <QTimer>
#include <string>

namespace WindowsDesktop {
// The handle and ownership survive for the entire keyboard lifetime, including
// the final accessibility notification. Other desktops retain independent locks.
class Instance {
public:
 explicit Instance(std::wstring prefix=L"gaze-grid.Keyboard");
 ~Instance();
 Instance(const Instance &)=delete;
 Instance &operator=(const Instance &)=delete;
 bool owner()const{return owns;}
 bool duplicate()const{return alreadyRunning;}
 DWORD error()const{return lastError;}
private:
 HANDLE mutex=nullptr;
 bool owns=false,alreadyRunning=false;
 DWORD lastError=ERROR_SUCCESS;
};

// The argument list includes argv[0]. Helpers and diagnostics must remain
// runnable while the keyboard holds its desktop's instance lock.
bool keyboardInvocation(const QStringList &arguments);
bool inputDesktopActive();

class Monitor:public QObject {
 Q_OBJECT
public:
 explicit Monitor(QObject *parent=nullptr);
 bool active()const{return currentActive;}
 void start();
signals:
 void activeChanged(bool active);
private:
 void sample();
 QTimer timer;
 bool known=false,currentActive=false;
};
}
