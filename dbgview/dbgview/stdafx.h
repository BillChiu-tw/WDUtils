
//**************************************************************************************
//	����:	23:2:2004
//	����:	tiamo
//	����:	stdafx
//**************************************************************************************

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif
	#include "ntddk.h"
#ifdef __cplusplus
}
#endif

#if DBG
	#define devDebugPrint DbgPrint
#else
	#define devDebugPrint __noop
#endif
