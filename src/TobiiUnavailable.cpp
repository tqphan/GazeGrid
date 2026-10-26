#include "TobiiInput.h"
TobiiInput::~TobiiInput(){requestInterruption();wait();}
QVariantMap TobiiInput::probe(){return {{"compiled",false},{"error","Built without the optional Tobii Stream Engine SDK"}};}
void TobiiInput::run(){emit status("Eye tracking requires a build with the Tobii Stream Engine SDK; mouse dwell remains available");}
