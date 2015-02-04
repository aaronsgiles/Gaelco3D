//===================================================================
//
//	Core common header file for standalone emulator shell
//
//	Copyright (c) 2004, Aaron Giles
//
//===================================================================

#ifndef _CORECOMMON_
#define _CORECOMMON_

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0400 
#include <windows.h>
#include <mmsystem.h>
#include <d3d8.h>
#include <dsound.h>
#include "gameconfig.h"
#include "mamecompat.h"

#undef EXCEPTION_ILLEGAL_INSTRUCTION
 

//--------------------------------------------------
//	Core constants
//--------------------------------------------------

#define CONTROL_KEYBOARD		0
#define CONTROL_MOUSE			1
#define CONTROL_JOYSTICKn		2


//--------------------------------------------------
//	Core types
//--------------------------------------------------

typedef struct
{
	UINT8 **	pointer;
	UINT32		crc;
	UINT32		length;
} ROMListEntry;

typedef struct
{
	void	(*reset)(void *param);
	int		(*execute)(int cycles);
	void	(*getinfo)(UINT32 state, union cpuinfo *info);
	void	(*setinfo)(UINT32 state, union cpuinfo *info);
	void	(*getcontext)(void *context);
	void	(*setcontext)(void *context);
	int *	icount;
} CPUData;

typedef struct
{
	UINT32	version;
	UINT32	controller;
	UINT32	width;
	UINT32	height;
	UINT32	format;
	UINT32	windowed;
	GameSavedData gamedata;
} SavedData;


//--------------------------------------------------
//	Core globals
//--------------------------------------------------

extern int gGameWidth, gGameHeight;
extern float gGameXScale, gGameYScale;

extern int gCurrentCPU;
extern int gCurrentCPUCycles;

extern UINT32 gFrameIndex;

extern IDirect3D8 *gD3D;
extern IDirect3DDevice8 *gD3DDevice;

extern SavedData gSavedData;

extern HWND gD3DWindow;


//--------------------------------------------------
//	Useful functions
//--------------------------------------------------

UINT32 SoundBufferReady(void);
void WriteToSoundBuffer(INT16 *data);

void InitCPU(int cpunum, void (*getinfo)(UINT32, union cpuinfo *), CPUData *data);
void SetCPUInt(const CPUData *data, int selector, int value);
int ExecuteCPU(int cycles, const CPUData *data);
void AbortExecuteCPU(void);

void FatalError(const char *string, ...);
void WarningMessage(const char *string, ...);
void Information(const char *string, ...);


//--------------------------------------------------
//	Functions to be implemented by the game-specific
//	code
//--------------------------------------------------

void GameInit(GameSavedData *data);
void GameExecute(void);


//--------------------------------------------------
//	Game-specific inlines
//--------------------------------------------------

#include "gameinline.h"

#endif
