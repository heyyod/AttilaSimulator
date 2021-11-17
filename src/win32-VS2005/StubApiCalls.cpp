/////////////////////////////////////////////////
// This file has been automatically generated  //
// Do not modify this file                     //
/////////////////////////////////////////////////

#include "StubApiCalls.h"

#ifndef LOAD_JUMPTABLE_STATICALLY
#define CHECK_GL_CALL(call)\
if ( JT.call == NULL ) {\
  ((unsigned long *)(&JT))[APICall_##call] = (unsigned long)JT.wglGetProcAddress(#call);\
  if ( JT.call == NULL )\
  {popup(#call,"Call unsupported. Cannot be found in current gl implementation");}\
}\
if ( JT.call != NULL )
#else
#define CHECK_GL_CALL(call)\
if ( JT.call == NULL )\
{ popup(#call,"Call unsupported. Cannot be found in current gl implementation"); }\
else
#endif

#define CHECK_USER_CALL(call) if ( UCT.call != NULL )

#define READ_PARAM(unionSelector,whichParam)\
    if ( paramFlag & (1 << whichParam) )\
    {\
        Param dummy;\
        TR.read(&dummy.unionSelector);\
        paramFlag &= ~(1 << whichParam);\
    }\
    else\
        TR.read(&param[whichParam].unionSelector);

#define READ_ENUM_PARAM(whichParam)\
    if ( paramFlag & (1 << whichParam) )\
    {\
        Param dummy;\
        TR.readEnum(&dummy.uv32bit);\
        paramFlag &= ~(1 << whichParam);\
    }\
    else\
        TR.readEnum(&param[whichParam].uv32bit);

#define READ_ORING_PARAM(whichParam)\
    if ( paramFlag & (1 << whichParam) )\
    {\
        Param dummy;\
        TR.readOring(&dummy.uv32bit);\
        paramFlag &= ~(1 << whichParam);\
    }\
    else\
        TR.readOring(&param[whichParam].uv32bit);

#define READ_DOUBLE_PARAM(whichParam)\
    if ( paramFlag & (1 << whichParam) )\
    {\
        Param dummy;\
        TR.readDouble(&dummy.fv64bit);\
        paramFlag &= ~(1 << whichParam);\
    }\
    else\
        TR.readDouble(&param[whichParam].fv64bit);

#define READ_BOOLEAN_PARAM(whichParam)\
    if ( paramFlag & (1 << whichParam) )\
    {\
        Param dummy;\
        TR.readBoolean(&dummy.boolean);\
        paramFlag &= ~(1 << whichParam);\
    }\
    else\
        TR.readBoolean(&param[whichParam].boolean);

#define READ_ARRAY_PARAM(whichParam, buf, mode)\
    if ( paramFlag & (1 << whichParam) )\
    {\
        Param dummy;\
        TR.readArray(&dummy.ptr, buf, sizeof(buf), mode);\
        paramFlag &= ~(1 << whichParam);\
    }\
    else\
        TR.readArray(&param[whichParam].ptr, buf, sizeof(buf), mode);

static int paramFlag = 0;
static Param param[32];
static unsigned int maxParam = 0;

Param getLastCallParameter(unsigned int iParam)
{
    if ( iParam >= maxParam )
        panic("StubApiCalls", "getLastCallParameter", "The parameter does not exist");
    return param[iParam];
}

void setLastCallParameter(unsigned int iParam, const Param& p)
{
    // if ( iParam >= maxParam )
    //    panic("StubApiCalls", "setLastCallParameter", "The parameter does not exist");
    param[iParam] = p;
    paramFlag |= (1 << iParam);
}

