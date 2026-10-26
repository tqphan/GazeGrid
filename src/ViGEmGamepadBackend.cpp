#include "WindowsGamepad.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <ViGEm/Client.h>
#include <memory>
WindowsGamepad::Backend WindowsGamepad::nativeBackend(){
 struct State {
  PVIGEM_CLIENT client=nullptr;
  PVIGEM_TARGET target=nullptr;
  bool attached=false;
  void close(){
   if(target){if(attached)vigem_target_remove(client,target);vigem_target_free(target);target=nullptr;}
   attached=false;
   if(client){vigem_disconnect(client);vigem_free(client);client=nullptr;}
  }
  ~State(){close();}
 };
 auto state=std::make_shared<State>();
 auto failure=[](QString operation,VIGEM_ERROR error){return operation+QString(" (ViGEm error 0x%1)").arg(quint32(error),8,16,QChar('0'));};
 return {
  [state,failure](QString &error){
   state->close();state->client=vigem_alloc();
   if(!state->client){error="Could not allocate ViGEm client";return false;}
   auto result=vigem_connect(state->client);
   if(!VIGEM_SUCCESS(result)){error=failure("Cannot connect to ViGEmBus. Install the ViGEmBus driver and retry",result);state->close();return false;}
   state->target=vigem_target_x360_alloc();
   if(!state->target){error="Could not allocate the virtual Xbox 360 controller";state->close();return false;}
   result=vigem_target_add(state->client,state->target);
   if(!VIGEM_SUCCESS(result)){error=failure("Could not attach the virtual Xbox 360 controller",result);state->close();return false;}
   state->attached=true;return true;
  },
  [state,failure](const Report &r,QString &error){
   XUSB_REPORT report{};
   report.wButtons=r.buttons;report.bLeftTrigger=r.leftTrigger;report.bRightTrigger=r.rightTrigger;
   report.sThumbLX=r.leftX;report.sThumbLY=r.leftY;report.sThumbRX=r.rightX;report.sThumbRY=r.rightY;
   auto result=vigem_target_x360_update(state->client,state->target,report);
   if(VIGEM_SUCCESS(result))return true;
   error=failure("ViGEmBus controller update failed",result);return false;
  },
  [state]{state->close();}
 };
}
