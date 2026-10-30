#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <string>
#include <vector>
#include <cwchar>
namespace SignInRegistry {
inline constexpr wchar_t key[]=L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Accessibility";
inline constexpr wchar_t id[]=L"gaze-grid_gaze-grid_v1";
inline constexpr wchar_t temporaryKey[]=L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AccessibilityTemp";
// Windows consumes the signal when it receives Win+U. Keeping the sender
// injectable lets tests exercise registry ordering without sending real keys.
template<class Sender>
inline bool notify(HKEY root,const wchar_t *path,bool running,Sender send){
 HKEY handle=nullptr;
 if(RegCreateKeyExW(root,path,0,nullptr,0,KEY_SET_VALUE|KEY_WOW64_64KEY,nullptr,&handle,nullptr)!=ERROR_SUCCESS)return false;
 const DWORD signal=running?0x0003:0x0002;
 auto error=RegSetValueExW(handle,id,0,REG_DWORD,reinterpret_cast<const BYTE*>(&signal),sizeof(signal));
 bool sent=false;
 if(error==ERROR_SUCCESS){
  INPUT inputs[4]{};
  for(auto &input:inputs)input.type=INPUT_KEYBOARD;
  inputs[0].ki.wVk=VK_LWIN;inputs[1].ki.wVk='U';
  inputs[2].ki.wVk='U';inputs[2].ki.dwFlags=KEYEVENTF_KEYUP;
  inputs[3].ki.wVk=VK_LWIN;inputs[3].ki.dwFlags=KEYEVENTF_KEYUP;
  const auto inserted=send(inputs,UINT(4));sent=inserted==4;
  if(!sent){
   RegDeleteValueW(handle,id);
   // A partial insertion must not leave Windows or U held down.
   if(inserted>0)send(inputs+2,UINT(2));
  }
 }
 RegCloseKey(handle);return sent;
}
inline std::wstring updated(const std::wstring &list,bool enabled){
 std::wstring result;bool found=false;
 for(size_t start=0;start<list.size();){
  auto end=list.find(L',',start);if(end==std::wstring::npos)end=list.size();
  auto item=list.substr(start,end-start);auto first=item.find_first_not_of(L" \t"),last=item.find_last_not_of(L" \t");
  auto trimmed=first==std::wstring::npos?L"":item.substr(first,last-first+1);
  bool ours=_wcsicmp(trimmed.c_str(),id)==0;
  if(!item.empty()&&(!ours||(enabled&&!found))){if(!result.empty())result+=L',';result+=item;}
  found|=ours;start=end+1;
 }
 if(!enabled&&!found)return list;
 if(enabled&&!found){if(!result.empty())result+=L',';result+=id;}
 return result;
}
inline LONG read(HKEY root,const wchar_t *path,std::wstring &value){
 DWORD type=0,bytes=0;auto error=RegGetValueW(root,path,L"Configuration",RRF_RT_REG_SZ|RRF_SUBKEY_WOW6464KEY,&type,nullptr,&bytes);
 if(error==ERROR_FILE_NOT_FOUND){value.clear();return ERROR_SUCCESS;}
 if(error!=ERROR_SUCCESS)return error;
 if(bytes>65536)return ERROR_INVALID_DATA;
 std::vector<wchar_t> data(bytes/sizeof(wchar_t)+1);
 error=RegGetValueW(root,path,L"Configuration",RRF_RT_REG_SZ|RRF_SUBKEY_WOW6464KEY,&type,data.data(),&bytes);
 if(error==ERROR_SUCCESS)value=data.data();return error;
}
inline LONG set(HKEY root,const wchar_t *path,bool enabled){
 std::wstring before;auto error=read(root,path,before);if(error!=ERROR_SUCCESS)return error;
 auto after=updated(before,enabled);if(before==after)return ERROR_SUCCESS;
 HKEY handle=nullptr;error=RegCreateKeyExW(root,path,0,nullptr,0,KEY_SET_VALUE|KEY_WOW64_64KEY,nullptr,&handle,nullptr);
 if(error!=ERROR_SUCCESS)return error;
 error=RegSetValueExW(handle,L"Configuration",0,REG_SZ,reinterpret_cast<const BYTE*>(after.c_str()),DWORD((after.size()+1)*sizeof(wchar_t)));
 RegCloseKey(handle);return error;
}
inline bool contains(const std::wstring &list){
 for(size_t start=0;start<list.size();){
  auto end=list.find(L',',start);if(end==std::wstring::npos)end=list.size();
  auto item=list.substr(start,end-start);auto first=item.find_first_not_of(L" \t"),last=item.find_last_not_of(L" \t");
  if(first!=std::wstring::npos&&_wcsicmp(item.substr(first,last-first+1).c_str(),id)==0)return true;
  start=end+1;
 }
 return false;
}
}
