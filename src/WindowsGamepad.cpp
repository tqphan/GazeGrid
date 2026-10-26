#include "WindowsGamepad.h"
#include "GamepadCodes.h"
#include <utility>
namespace {
const QMap<int,uint16_t> masks{
 {BTN_DPAD_UP,0x0001},{BTN_DPAD_DOWN,0x0002},{BTN_DPAD_LEFT,0x0004},{BTN_DPAD_RIGHT,0x0008},
 {BTN_START,0x0010},{BTN_SELECT,0x0020},{BTN_THUMBL,0x0040},{BTN_THUMBR,0x0080},
 {BTN_TL,0x0100},{BTN_TR,0x0200},{BTN_MODE,0x0400},
 {BTN_SOUTH,0x1000},{BTN_EAST,0x2000},{BTN_WEST,0x4000},{BTN_NORTH,0x8000}
};
bool valid(int type,int code,int value){
 if(type==EV_KEY)return (masks.contains(code)||code==BTN_TL2||code==BTN_TR2)&&(value==0||value==1);
 if(type!=EV_ABS)return false;
 if(code==ABS_HAT0X||code==ABS_HAT0Y)return value>=-1&&value<=1;
 if(code==ABS_Z||code==ABS_RZ)return value>=0&&value<=32767;
 return (code==ABS_X||code==ABS_Y||code==ABS_RX||code==ABS_RY)&&value>=-32767&&value<=32767;
}
}
WindowsGamepad::WindowsGamepad(Backend backend):backend(std::move(backend)){}
WindowsGamepad::~WindowsGamepad(){close();}
#ifndef _WIN32
WindowsGamepad::Backend WindowsGamepad::nativeBackend(){
 return {[](QString &error){error="ViGEmBus requires Windows";return false;},{},[]{}};
}
#endif
WindowsGamepad::Report WindowsGamepad::report()const{
 Report r;
 for(auto it=masks.begin();it!=masks.end();++it)if(buttons.value(it.key()))r.buttons|=it.value();
 if(axes.value(ABS_HAT0X)<0)r.buttons|=0x0004;
 if(axes.value(ABS_HAT0X)>0)r.buttons|=0x0008;
 if(axes.value(ABS_HAT0Y)<0)r.buttons|=0x0001;
 if(axes.value(ABS_HAT0Y)>0)r.buttons|=0x0002;
 r.leftTrigger=buttons.value(BTN_TL2)?255:qRound(axes.value(ABS_Z)*255.0/32767);
 r.rightTrigger=buttons.value(BTN_TR2)?255:qRound(axes.value(ABS_RZ)*255.0/32767);
 r.leftX=axes.value(ABS_X);r.rightX=axes.value(ABS_RX);
 // Shared joystick geometry is positive-down; XInput sticks are positive-up.
 r.leftY=-axes.value(ABS_Y);r.rightY=-axes.value(ABS_RY);
 return r;
}
bool WindowsGamepad::send(int type,int code,int value){
 if(!valid(type,code,value)){message="This button or axis is not supported by the ViGEm Xbox 360 controller";return false;}
 if(!connected){
  if(!backend.open||!backend.submit||!backend.open(message)){
   if(backend.close)backend.close();
   if(message.isEmpty())message="Could not connect to ViGEmBus";
   return false;
  }
  connected=true;
 }
 (type==EV_KEY?buttons:axes)[code]=value;
 if(!backend.submit(report(),message)){
  if(message.isEmpty())message="ViGEmBus report failed; controller disconnected";
  close();return false;
 }
 message.clear();return true;
}
void WindowsGamepad::close(){
 if(connected){
  QString ignored;
  backend.submit(Report{},ignored); // Best effort neutral report before unplugging.
  if(backend.close)backend.close();connected=false;
 }
 buttons.clear();axes.clear();
}
