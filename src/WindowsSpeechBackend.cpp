#include "SpeechOutput.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <sapi.h>
#include <memory>
SpeechOutput::Backend SpeechOutput::nativeBackend(){
 struct State {ISpVoice *voice=nullptr;bool initialized=false;};
 auto state=std::make_shared<State>();
 return {
  [state]{
   const auto hr=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
   if(FAILED(hr))return false;
   state->initialized=true;
   if(SUCCEEDED(CoCreateInstance(CLSID_SpVoice,nullptr,CLSCTX_INPROC_SERVER,IID_ISpVoice,reinterpret_cast<void **>(&state->voice))))return true;
   CoUninitialize();state->initialized=false;return false;
  },
  [state](const QString &text){return SUCCEEDED(state->voice->Speak(reinterpret_cast<const wchar_t *>(text.utf16()),SPF_ASYNC|SPF_IS_NOT_XML,nullptr));},
  [state]{return SUCCEEDED(state->voice->Speak(nullptr,SPF_ASYNC|SPF_PURGEBEFORESPEAK,nullptr));},
  [state]{if(state->voice){state->voice->Release();state->voice=nullptr;}if(state->initialized){CoUninitialize();state->initialized=false;}}
 };
}
