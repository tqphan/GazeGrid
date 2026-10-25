#pragma once
#include "KeyNames.h"
#include "GameController.h"
#include "PointerActions.h"
namespace ActionCatalog {
inline QString implementation(const QString &action){
    if(!KeyNames::functionKey(action).isEmpty())return "physical-key";
    if(GameController::supports(action))return "game-controller";
    if(PointerActions::supports(action))return "pointer";
    if(action=="MouseMagnifier")return "screenshot-portal";
    if(action=="MoveModeToggle"||action=="SizeModeToggle"||action=="CycleButtonMode"||action=="CycleKeyMode"||action=="Speak"||action=="SpeakLabel"||action=="BackMany"||action=="NextSuggestions"||action=="PreviousSuggestions"||action=="ClearScratchpad"||action=="Sleep"||action=="MenuKeyboard"||action=="BackFromKeyboard")return "application";
    return "unsupported";
}
inline bool supported(const QString &action){return implementation(action)!="unsupported";}
}
