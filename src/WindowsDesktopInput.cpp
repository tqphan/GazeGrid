#include "WindowsDesktopInput.h"
#include "KeyNames.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cmath>
QVariantMap PortalInput::probe(){
 HANDLE token=nullptr;DWORD uiAccess=0,size=0;
 bool queried=false;
 if(OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&token)){
  queried=GetTokenInformation(token,TokenUIAccess,&uiAccess,sizeof(uiAccess),&size);
  CloseHandle(token);
 }
 return {{"backend","Win32 SendInput"},{"uiAccess",queried&&uiAccess!=0},{"tokenQuerySucceeded",queried}};
}
bool PortalInput::key(int code,bool down){return available&&input.key(KeyNames::fromLinuxCode(code),down);}
bool PortalInput::moveAbsolute(double x,double y){
 if(!available||!std::isfinite(x)||!std::isfinite(y))return false;
 // Gaze/capture use the primary display. SendInput uses the virtual desktop.
 const double px=qBound(0.0,x,1.0)*(GetSystemMetrics(SM_CXSCREEN)-1);
 const double py=qBound(0.0,y,1.0)*(GetSystemMetrics(SM_CYSCREEN)-1);
 const int width=GetSystemMetrics(SM_CXVIRTUALSCREEN),height=GetSystemMetrics(SM_CYVIRTUALSCREEN);
 if(width<2||height<2)return false;
 return input.moveAbsolute((px-GetSystemMetrics(SM_XVIRTUALSCREEN))/(width-1),
                           (py-GetSystemMetrics(SM_YVIRTUALSCREEN))/(height-1));
}

bool PortalInput::moveRelativeTo(double x,double y){
 if(!available||!std::isfinite(x)||!std::isfinite(y)||x<0||x>1||y<0||y>1)return false;
 POINT cursor;
 if(!GetCursorPos(&cursor))return false;
 // Use the same primary-display physical coordinates as moveAbsolute, even
 // when the cursor starts on another monitor or Qt uses a scaled geometry.
 const int width=GetSystemMetrics(SM_CXSCREEN),height=GetSystemMetrics(SM_CYSCREEN);
 if(width<2||height<2)return false;
 const auto delta=relativePointer.step({double(cursor.x),double(cursor.y)},{x*(width-1),y*(height-1)});
 if(delta.isNull())return true;
 if(input.move(delta.x(),delta.y()))return true;
 relativePointer.reset();return false;
}
