#include "SignInRegistry.h"
#include <msiquery.h>
namespace {
std::wstring property(MSIHANDLE install,const wchar_t *name){
 DWORD length=0;wchar_t dummy=0;MsiGetPropertyW(install,name,&dummy,&length);
 std::wstring value(length+1,L'\0');++length;
 if(MsiGetPropertyW(install,name,value.data(),&length)!=ERROR_SUCCESS)return {};
 value.resize(length);return value;
}
UINT apply(MSIHANDLE install){
 const auto value=property(install,L"CustomActionData");if(value!=L"0"&&value!=L"1")return ERROR_INSTALL_FAILURE;
 return SignInRegistry::set(HKEY_LOCAL_MACHINE,SignInRegistry::key,value==L"1")==ERROR_SUCCESS?ERROR_SUCCESS:ERROR_INSTALL_FAILURE;
}
}
extern "C" __declspec(dllexport) UINT __stdcall PrepareSignIn(MSIHANDLE install){
 std::wstring previous;if(SignInRegistry::read(HKEY_LOCAL_MACHINE,SignInRegistry::key,previous)!=ERROR_SUCCESS)return ERROR_INSTALL_FAILURE;
 bool remove=property(install,L"REMOVE")==L"ALL";
 bool enable=!remove&&property(install,L"GAZEGRID_TRUST_CERTIFICATE")==L"1"&&property(install,L"GAZEGRID_SIGN_IN")==L"1";
 if(MsiSetPropertyW(install,L"ApplySignIn",enable?L"1":L"0")!=ERROR_SUCCESS)return ERROR_INSTALL_FAILURE;
 return MsiSetPropertyW(install,L"RollbackSignIn",SignInRegistry::contains(previous)?L"1":L"0");
}
extern "C" __declspec(dllexport) UINT __stdcall ApplySignIn(MSIHANDLE install){return apply(install);}
extern "C" __declspec(dllexport) UINT __stdcall RollbackSignIn(MSIHANDLE install){return apply(install);}
