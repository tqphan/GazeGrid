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
inline constexpr wchar_t id[]=L"GazeGrid_GazeGrid_v1";
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
