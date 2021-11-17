/////////////////////////////////////////////////
// This file has been automatically generated  //
// Do not modify this file                     //
/////////////////////////////////////////////////

#include "glWrapper.h"
#include "Specific.h"
#include "SpecificStats.h"
#include <cstdlib> // for support exit() call

#define _W GLInterceptor::tw
#define _JT GLInterceptor::jt
#define _COUNT(call) GLInterceptor::statManager.incCall(call)
#define _ISHACK GLInterceptor::isHackMode()
#define RESOLVE_CALL(call) ((unsigned long *)(&_JT))[APICall_##call] = (unsigned long)_JT.wglGetProcAddress(#call)
#define CHECKCALL(call) if (_JT.##call == NULL) { RESOLVE_CALL(call); if (_JT.##call == NULL) { panic("Wrapper.cpp", "Call cannot be resolved", #call); }}

