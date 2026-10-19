#pragma once
#include "KeyNames.h"
#include "PointerActions.h"
namespace ActionCatalog {
inline QString implementation(const QString &action){
    if(!KeyNames::functionKey(action).isEmpty())return "physical-key";
    if(PointerActions::supports(action))return "pointer";
    if(action=="MouseMagnifier")return "screenshot-portal";
    if(action=="ClearScratchpad"||action=="Sleep"||action=="MenuKeyboard"||action=="BackFromKeyboard")return "application";
    return "unsupported";
}
inline bool supported(const QString &action){return implementation(action)!="unsupported";}
}
