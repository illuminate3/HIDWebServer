#ifndef __COMMANDS_H__
#define __COMMANDS_H__

// Decoration to let C++ code be used from within plain C modules
#ifdef __cplusplus
extern "C" {
#endif

void CommandInit(void);
void CommandQuit(void);
void CmdRecognize(void);
void CommandDispatcher(char XMLSnapShot[], const char Cmd[]);
	
// Decoration to let C++ mocde be used from within plain C modules
#ifdef __cplusplus
}
#endif


#endif