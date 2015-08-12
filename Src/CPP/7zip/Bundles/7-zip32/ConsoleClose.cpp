// ConsoleClose.cpp

#include "StdAfx.h"

#include "ConsoleClose.h"

#if !defined(UNDER_CE) && defined(_WIN32)
#include "../../../Common/MyWindows.h"
#endif

//namespace NConsoleClose {							// �폜

unsigned g_BreakCounter = 0;
static const unsigned kBreakAbortThreshold = 2;

#if !defined(UNDER_CE) && defined(_WIN32)
//static BOOL WINAPI HandlerRoutine(DWORD ctrlType)		// �폜
BOOL HandlerRoutine()									// �ǉ�
{
  /* �폜��������
  if (ctrlType == CTRL_LOGOFF_EVENT)
  {
    // printf("\nCTRL_LOGOFF_EVENT\n");
    return TRUE;
  }
     �폜�����܂� */

  g_BreakCounter++;
  if (g_BreakCounter < kBreakAbortThreshold)
    return TRUE;
  return FALSE;
  /*
  switch(ctrlType)
  {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
      if (g_BreakCounter < kBreakAbortThreshold)
      return TRUE;
  }
  return FALSE;
  */
}
#endif

namespace NConsoleClose {			// �ǉ�
/*
void CheckCtrlBreak()
{
  if (TestBreakSignal())
    throw CCtrlBreakException();
}
*/

CCtrlHandlerSetter::CCtrlHandlerSetter()
{
  #if !defined(UNDER_CE) && defined(_WIN32)
//  if(!SetConsoleCtrlHandler(HandlerRoutine, TRUE))	// �폜
//    throw "SetConsoleCtrlHandler fails";				// �폜
  #endif
}

CCtrlHandlerSetter::~CCtrlHandlerSetter()
{
  #if !defined(UNDER_CE) && defined(_WIN32)
//  if(!SetConsoleCtrlHandler(HandlerRoutine, FALSE))	// �폜
//    throw "SetConsoleCtrlHandler fails";				// �폜
	g_BreakCounter = 0;									// �ǉ�
  #endif
}

}
