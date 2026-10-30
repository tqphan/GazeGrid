#include "SpeechOutput.h"
#include <speech-dispatcher/libspeechd.h>
#include <memory>

SpeechOutput::Backend SpeechOutput::nativeBackend(){
 struct State{SPDConnection *connection=nullptr;};auto state=std::make_shared<State>();
 return {
  [state]{state->connection=spd_open("gaze-grid","scratchpad",nullptr,SPD_MODE_SINGLE);return state->connection!=nullptr;},
  [state](const QString &text){const auto utf8=text.toUtf8();return spd_say(state->connection,SPD_TEXT,utf8.constData())>=0;},
  [state]{return spd_cancel(state->connection)==0;},
  [state]{if(state->connection){spd_close(state->connection);state->connection=nullptr;}}
 };
}
