// ConsoleClose.h

#ifndef __CONSOLE_CLOSE_H
#define __CONSOLE_CLOSE_H

//namespace NConsoleClose {	// �폜

extern unsigned g_BreakCounter;
namespace NConsoleClose {	// �ǉ�

inline bool TestBreakSignal()
{
  #ifdef UNDER_CE
  return false;
  #else
  return (g_BreakCounter != 0);
  #endif
}

class CCtrlHandlerSetter
{
public:
  CCtrlHandlerSetter();
  virtual ~CCtrlHandlerSetter();
};

class CCtrlBreakException
{};

// void CheckCtrlBreak();

}

#endif
