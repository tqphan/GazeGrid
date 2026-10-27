#pragma once
#include <optional>

// Keep waiting and callback delivery on the worker thread. A normal SDK timeout
// only checks for shutdown; an error is returned to the caller's reconnect path.
template<class Wait,class Process,class Interrupted,class Error>
std::optional<Error> tobiiWaitAndProcess(Wait wait,Process process,Interrupted interrupted,Error success,Error timedOut){
 if(interrupted())return std::nullopt;
 const auto error=wait();
 if(interrupted())return std::nullopt;
 if(error==timedOut)return success;
 return error==success?process():error;
}
