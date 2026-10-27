#include "WindowsSignIn.h"
#include "SignInRegistry.h"
#include "ProfileStore.h"
#include "AppPaths.h"
#include <QCoreApplication>
#include <QJsonDocument>
#include <QSettings>
#include <QFileInfo>
#include <shellapi.h>
namespace {
const QString settingsPath="HKEY_LOCAL_MACHINE\\SOFTWARE\\GazeGrid\\SignIn";
}
QJsonObject WindowsSignIn::validatedProfile(const QJsonObject &source){
 auto result=ProfileStore::defaults();
 // Only keyboard presentation and gaze preferences cross the desktop boundary.
 for(const auto &name:{"startLayout","keyboardScale","keyboardTheme","dwellMs","dwellIndicatorStyle","gazeEnabled","clampGazeForKeyboard","clampGazeForPointSelection","invalidGazeToleranceMs","gazeFilterType","gazeFilterGain","gazeSaccadeDistance","gazeSmoothingLevel","smoothWhenChangingGazeTarget","shortLabelScale","longLabelScale","scratchpadTextScale"})
  if(source.contains(name))result[name]=source[name];
 result["predictionMode"]="off";result["learning"]=false;result["magnifierEnabled"]=false;
 if(!ProfileStore::valid(result))return {};
 const auto layout=result["startLayout"].toString();
 if(layout.isEmpty()||layout.contains('/')||layout.contains('\\')||layout.contains("..")||layout.startsWith("minimized-off-screen-")
    ||!QFileInfo::exists(keyboardPackDirectory()+"/"+layout+".xml"))return {};
 return result;
}
bool WindowsSignIn::secureDesktop(){
 wchar_t name[256]{};DWORD length=0;
 return GetUserObjectInformationW(GetThreadDesktop(GetCurrentThreadId()),UOI_NAME,name,sizeof(name),&length)
     &&_wcsicmp(name,L"Winlogon")==0;
}
bool WindowsSignIn::registered(){
 QSettings registration("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Accessibility\\ATs\\GazeGrid_GazeGrid_v1",QSettings::Registry64Format);
 const auto installed=QFileInfo(registration.value("StartExe").toString()).canonicalFilePath();
 return !installed.isEmpty()&&installed.compare(QFileInfo(QCoreApplication::applicationFilePath()).canonicalFilePath(),Qt::CaseInsensitive)==0;
}
bool WindowsSignIn::notify(bool running){
 // Portable/development copies must not announce the installed application's ID.
 if(!registered())return false;
 return SignInRegistry::notify(HKEY_CURRENT_USER,SignInRegistry::temporaryKey,running,
     [](INPUT *inputs,UINT count){return SendInput(count,inputs,sizeof(INPUT));});
}
bool WindowsSignIn::enabled(){
 std::wstring configuration;
 return SignInRegistry::read(HKEY_LOCAL_MACHINE,SignInRegistry::key,configuration)==ERROR_SUCCESS
     &&SignInRegistry::contains(configuration);
}
QJsonObject WindowsSignIn::profile(){
 QSettings settings(settingsPath,QSettings::NativeFormat);
 auto bytes=settings.value("Profile").toByteArray();
 auto result=bytes.size()<=16384?validatedProfile(QJsonDocument::fromJson(bytes).object()):QJsonObject{};
 if(result.isEmpty())result=validatedProfile(ProfileStore::defaults());
 return result;
}
int WindowsSignIn::setup(const QStringList &arguments){
 if(!arguments.contains("--configure-sign-in"))return -1;
 if(secureDesktop())return 2;
 auto index=arguments.indexOf("--configure-sign-in");
 if(arguments.size()!=index+3||index!=1)return 2;
 auto mode=arguments[index+1];auto data=arguments[index+2].toLatin1();
 if((mode!="enable"&&mode!="disable")||data.size()>24000)return 2;
 if(!registered())return 2;
 auto values=validatedProfile(QJsonDocument::fromJson(QByteArray::fromBase64(data)).object());
 if(values.isEmpty())return 2;
 QSettings settings(settingsPath,QSettings::NativeFormat);
 if(mode=="enable")settings.setValue("Profile",QString::fromUtf8(QJsonDocument(values).toJson(QJsonDocument::Compact)));
 settings.sync();if(settings.status()!=QSettings::NoError)return 2;
 if(SignInRegistry::set(HKEY_LOCAL_MACHINE,SignInRegistry::key,mode=="enable")!=ERROR_SUCCESS)return 2;
 QSettings installer("HKEY_LOCAL_MACHINE\\SOFTWARE\\GazeGrid\\Installer",QSettings::NativeFormat);
 installer.setValue("SignIn",mode=="enable"?"1":"0");installer.sync();
 return installer.status()==QSettings::NoError?0:2;
}
bool WindowsSignIn::configure(bool enabled,const QJsonObject &values){
 const auto safe=validatedProfile(enabled?values:ProfileStore::defaults());if(safe.isEmpty())return false;
 auto parameters=QString("--configure-sign-in %1 %2").arg(enabled?"enable":"disable",QString::fromLatin1(QJsonDocument(safe).toJson(QJsonDocument::Compact).toBase64()));
 const auto executable=QCoreApplication::applicationFilePath().toStdWString();const auto args=parameters.toStdWString();
 SHELLEXECUTEINFOW info{};info.cbSize=sizeof(info);info.fMask=SEE_MASK_NOCLOSEPROCESS;info.lpVerb=L"runas";info.lpFile=executable.c_str();info.lpParameters=args.c_str();info.nShow=SW_HIDE;
 if(!ShellExecuteExW(&info))return false;
 DWORD exitCode=2;
 if(WaitForSingleObject(info.hProcess,120000)==WAIT_OBJECT_0)GetExitCodeProcess(info.hProcess,&exitCode);
 CloseHandle(info.hProcess);return exitCode==0;
}
