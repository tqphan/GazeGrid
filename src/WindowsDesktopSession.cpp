#include "WindowsDesktopSession.h"
#include <QCryptographicHash>
#include <wtsapi32.h>
#include <cwchar>
#include <vector>

namespace {
bool objectName(HANDLE object,std::wstring &name){
 if(!object){SetLastError(ERROR_INVALID_HANDLE);return false;}
 DWORD bytes=0;
 GetUserObjectInformationW(object,UOI_NAME,nullptr,0,&bytes);
 if(bytes<sizeof(wchar_t)||bytes>65536){
  if(!bytes&&GetLastError()!=ERROR_SUCCESS)return false;
  SetLastError(ERROR_INVALID_DATA);return false;
 }
 std::vector<wchar_t> buffer(bytes/sizeof(wchar_t)+1);
 if(!GetUserObjectInformationW(object,UOI_NAME,buffer.data(),bytes,&bytes))return false;
 name=buffer.data();
 if(name.empty()){SetLastError(ERROR_INVALID_DATA);return false;}
 return true;
}
bool sessionConnected(){
 LPWSTR data=nullptr;DWORD bytes=0;
 if(WTSQuerySessionInformationW(WTS_CURRENT_SERVER_HANDLE,WTS_CURRENT_SESSION,WTSConnectState,&data,&bytes)){
  const bool valid=data&&bytes>=sizeof(WTS_CONNECTSTATE_CLASS);
  const auto state=valid?*reinterpret_cast<const WTS_CONNECTSTATE_CLASS*>(data):WTSDown;
  if(data)WTSFreeMemory(data);
  // Connected also covers the sign-in desktop before a user has logged on.
  return valid&&(state==WTSActive||state==WTSConnected);
 }
 // Local desktops still work when Remote Desktop Services is unavailable.
 DWORD session=0;
 return ProcessIdToSessionId(GetCurrentProcessId(),&session)
     &&session==WTSGetActiveConsoleSessionId();
}
}

WindowsDesktop::Instance::Instance(std::wstring prefix){
 std::wstring station,desktop;
 if(!objectName(GetProcessWindowStation(),station)
    ||!objectName(GetThreadDesktop(GetCurrentThreadId()),desktop)){
  lastError=GetLastError();if(lastError==ERROR_SUCCESS)lastError=ERROR_INVALID_DATA;return;
 }
 auto identity=QString::fromStdWString(prefix).toUtf8();identity+='\0';
 identity+=QString::fromStdWString(station).toUtf8();identity+='\0';
 identity+=QString::fromStdWString(desktop).toUtf8();
 // Hash the complete identity to support long or non-ASCII desktop names and
 // prefixes without introducing reserved separators into the object namespace.
 const auto name=(QStringLiteral("Local\\gaze-grid.Keyboard.")
     +QString::fromLatin1(QCryptographicHash::hash(identity,QCryptographicHash::Sha256).toHex())).toStdWString();
 mutex=CreateMutexExW(nullptr,name.c_str(),0,SYNCHRONIZE|MUTEX_MODIFY_STATE);
 if(!mutex){lastError=GetLastError();return;}
 const auto result=WaitForSingleObject(mutex,0);
 if(result==WAIT_OBJECT_0||result==WAIT_ABANDONED){owns=true;return;}
 alreadyRunning=result==WAIT_TIMEOUT;
 lastError=alreadyRunning?ERROR_ALREADY_EXISTS:result==WAIT_FAILED?GetLastError():ERROR_INVALID_DATA;
 CloseHandle(mutex);mutex=nullptr;
}
WindowsDesktop::Instance::~Instance(){
 if(owns)ReleaseMutex(mutex);
 if(mutex)CloseHandle(mutex);
}
bool WindowsDesktop::keyboardInvocation(const QStringList &arguments){
 for(qsizetype i=1;i<arguments.size();++i){
  const auto &argument=arguments[i];
  if(!argument.startsWith("--"))continue;
  if(argument=="--configure-sign-in"||argument=="--editor"||argument=="--diagnose"
     ||argument=="--validate-keyboards"||argument=="--smoke"
     ||argument.endsWith("-check")||argument.endsWith("-target"))return false;
 }
 return true;
}
bool WindowsDesktop::inputDesktopActive(){
 if(!sessionConnected())return false;
 const auto input=OpenInputDesktop(0,FALSE,DESKTOP_READOBJECTS);
 if(!input)return false;
 std::wstring active,current;
 const bool valid=objectName(input,active)
     &&objectName(GetThreadDesktop(GetCurrentThreadId()),current);
 CloseDesktop(input);
 return valid&&_wcsicmp(active.c_str(),current.c_str())==0;
}
WindowsDesktop::Monitor::Monitor(QObject *parent):QObject(parent),timer(this){
 timer.setInterval(100);timer.setTimerType(Qt::PreciseTimer);
 connect(&timer,&QTimer::timeout,this,&Monitor::sample);
}
void WindowsDesktop::Monitor::start(){
 if(timer.isActive())return;
 timer.start();sample();
}
void WindowsDesktop::Monitor::sample(){
 const bool next=inputDesktopActive();
 if(known&&currentActive==next)return;
 known=true;currentActive=next;emit activeChanged(currentActive);
}
