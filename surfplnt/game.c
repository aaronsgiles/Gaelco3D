//===================================================================
//
//	Game code file for standalone emulator shell
//
//	Copyright (c) 2004, Aaron Giles
//
//===================================================================

#include "mamecompat.h"
#include "m68000.h"
#include "tms32031.h"

#include <stdio.h>
#include <setjmp.h>
#include <math.h>

#define HLE_TMS				0

#define MAX_POLYGONS		4000
#define MAX_TEXTURES		100

#define TEXDATA_ROM_SIZE	0x400000
#define TEXDATA_ROM_COUNT	4
#define TEXMASK_ROM_SIZE	0x020000
#define TEXMASK_ROM_COUNT	4

#define TEXTURE_DATA_SIZE	(TEXDATA_ROM_SIZE * TEXDATA_ROM_COUNT)
#define TEXTURE_MASK_SIZE	(TEXMASK_ROM_SIZE * TEXMASK_ROM_COUNT)


//--------------------------------------------------
//	Types
//--------------------------------------------------

typedef struct
{
	void 	(*callback)(int);
	int		value;
} SyncCallbackEntry;


#define VERTEX_FORMAT (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1)
typedef struct
{
	float	x, y, z;
	float	rhw;
	D3DCOLOR color;
	float	u, v;
} Vertex;


typedef struct _Texture
{
	struct _Texture *prev;
	struct _Texture *next;
	UINT32	textureBase;
	float	minu, maxu, minv, maxv;
	float	ooheight, oowidth;
	IDirect3DTexture8 *	texture;
	UINT32	paletteChecksum;
	UINT32	lastUsedFrame;
} Texture;


typedef struct
{
	Texture *texture;
	int		startIndex;
	int		triCount;
	UINT8	noZBuffer;
	UINT8	alphaBlend;
	UINT32	texBase;
} Primitive;


//--------------------------------------------------
//	Global variables
//--------------------------------------------------

UINT8 *rom68000[4];
UINT8 *romADSP2115;
UINT8 *romGeometry[2];
UINT8 *romTexture[8];

__declspec(align(4096)) UINT8 g68000MemoryBase[1 << 24];

__declspec(align(4096)) UINT32 g32031MemoryBase[1 << 24];
__declspec(align(4096)) float g32031FloatMemoryBase[0x810000];

__declspec(align(4096)) UINT32 gADSPProgramMemoryBase[1 << 14];
__declspec(align(4096)) UINT16 gADSPDataMemoryBase[1 << 14];

__declspec(align(4096)) UINT16 gTextureSource[TEXTURE_DATA_SIZE/4096][4096];

UINT8 g32031IsHalted;

CPUData g68000CPU;
CPUData g32031CPU;

SyncCallbackEntry gSyncCallbacks[16];
int gSyncCallbackCount;

IDirect3DVertexBuffer8 *gVertexBuffer;

UINT32 gPolyData[MAX_POLYGONS * 21];
Primitive gPrimitives[MAX_POLYGONS];
int gPolyIndex;

void Ack32031Interrupt(UINT8 val, offs_t addr);
struct tms32031_config g32031Config = { 0x1000, 0, 0, Ack32031Interrupt };

UINT32 gPaletteChecksum[0x80];

Texture *gTextureListHead;
Texture *gTextureListTail;
UINT32 gTextureListCount;

int gAnalogValue = 0x80;
UINT8 gLatchedAnalogValue;

void *gMainFiber;
void *gTMSFiber;
void *gADSPFiber;

volatile UINT8 gTMSInterrupt;
volatile UINT8 gADSPInterrupt;
volatile INT32 gADSPSamplesNeeded;

volatile INT16 gSoundBufferData[16384];
volatile UINT32 gSoundBufferCount;

GameSavedData *gGameSavedData;

UINT8 gEEPROMClockState;
UINT8 gEEPROMCSState;
UINT8 gEEPROMIsReading;
UINT8 gEEPROMDataLatch;
UINT32 gEEPROMWriteBuffer;
UINT32 gEEPROMReadBuffer;


//--------------------------------------------------
//	Default EEPROM data
//--------------------------------------------------

UINT16 gDefaultEEPROMData[256] = 
{
	0x0000
/*
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x37EC,0x0041,0x4243,0x3828,0x0044,0x4546,0x3864,0x0047,0x4849,0x38A0,0x004A,0x4B4C,0x38DC,0x004D,0x4E4F,0x3918,
	0x0050,0x5152,0x3954,0x0053,0x5455,0x3990,0x0056,0x5758,0x39CC,0x0059,0x5A41,0x3A08,0x0042,0x4344,0x3BBF,0x0041,
	0x4243,0x3BFB,0x0044,0x4546,0x3C37,0x0047,0x4849,0x3C73,0x004A,0x4B4C,0x3CAF,0x004D,0x4E4F,0x3CEB,0x0050,0x5152,
	0x3D27,0x0053,0x5455,0x3D63,0x0056,0x5758,0x3D9F,0x0059,0x5A41,0x3DDB,0x0042,0x4344,0x37B4,0x0041,0x4243,0x37F0,
	0x0044,0x4546,0x382C,0x0047,0x4849,0x3868,0x004A,0x4B4C,0x38A4,0x004D,0x4E4F,0x38E0,0x0050,0x5152,0x391C,0x0053,
	0x5455,0x3958,0x0056,0x5758,0x3994,0x0059,0x5A41,0x39D0,0x0042,0x4344,0x005A,0x0041,0x4243,0x0050,0x0044,0x4546,
	0x0046,0x0047,0x4849,0x003C,0x004A,0x4B4C,0x0032,0x004D,0x4E4F,0x0028,0x0050,0x5152,0x001E,0x0053,0x5455,0x0014,
	0x0056,0x5758,0x000A,0x0059,0x5A41,0x0005,0x0042,0x4344,0x0E40,0x0041,0x4243,0x0EB0,0x0044,0x4546,0x0E10,0x0047,
	0x4849,0x0DA0,0x004A,0x4B4C,0x0FB0,0x004D,0x4E4F,0x0EE0,0x0050,0x5152,0x0E53,0x0053,0x5455,0x0F90,0x0056,0x5758,
	0x0F2C,0x0059,0x5A41,0x0E2C,0x0042,0x4344,0x0D80,0x0045,0x4647,0x0D90,0x0048,0x494A,0x3342,0x0000,0x0000,0xFFFF,
	0x0000,0x0006,0xFFF9,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x135D,0x0001,0x0006,0x0001,0xE917,0x007F,0x0100,0x0002,0x0101,0x0101*/
};


//--------------------------------------------------
//	Prototypes
//--------------------------------------------------

void EEPROMReset(void);
void EEPROMWrite(int bit);

float Convert32031ToFloat(UINT32 val);
void UpdateControls(void);
void InitRenderState(void);
void RenderPolys(void);

void InitTMS(void);
void KillTMS(void);
VOID CALLBACK RunTMS(PVOID lpParameter);
void _809CF1(void);
void _809CFD(void);
void _809D18(void);
void _809D4C(void);
void _809D52(int goto_809D43);
void _809DD4(void);
void _809DF6(void);
void _809E8D(void);
void _809E9D(void);
void _809EB9(void);
int _809F1D(void);
void _809F62(void);
void _809F6E(void);
int _809F7F(void);
void _809FE0(void);
void _809FF5(void);

void InitADSP(void);
VOID CALLBACK RunADSP(PVOID lpParameter);


//--------------------------------------------------
//	Game initialization
//--------------------------------------------------

void GameInit(GameSavedData *data)
{
	int i, x, y;
	
	// stash the saved data pointer
	gGameSavedData = data;
	
	// if the data is all zero, overwrite with the defaults
	for (i = 0; i < 256; i++)
		if (data->eeprom[i] != 0)
			break;
	if (i == 256)
		memcpy(data->eeprom, gDefaultEEPROMData, sizeof(data->eeprom));
	
	// make a fiber of us
	gMainFiber = ConvertThreadToFiber(0);
	
	// init the render state
	InitRenderState();

	// expand the texture pixel data
	for (y = 0; y < TEXTURE_DATA_SIZE / 4096; y += 2)
		for (x = 0; x < 4096; x += 2)
		{
			int toffset = (y/2) * 2048 + (x/2);
			int toffshi = toffset / TEXDATA_ROM_SIZE;
			int toffslo = toffset % TEXDATA_ROM_SIZE;
			
			gTextureSource[y + 0][x + 1] = romTexture[0 + 4*toffshi][toffslo];
			gTextureSource[y + 1][x + 1] = romTexture[1 + 4*toffshi][toffslo];
			gTextureSource[y + 0][x + 0] = romTexture[2 + 4*toffshi][toffslo];
			gTextureSource[y + 1][x + 0] = romTexture[3 + 4*toffshi][toffslo];
		}

	// expand the texture mask data
	for (y = 0; y < TEXTURE_MASK_SIZE*8 / 4096; y++)
		for (x = 0; x < 4096; x++)
		{
			int moffset = (x / 1024) * 0x20000 + (y * 1024 + x % 1024) / 8;
			int moffshi = moffset / TEXMASK_ROM_SIZE;
			int moffslo = moffset % TEXMASK_ROM_SIZE;
			if (romTexture[4 + moffshi][moffslo] & (1 << (x % 8)))
				gTextureSource[y][x] |= 0x8000;
		}
	
	// free the texture ROMs
	for (i = 0; i < 8; i++)
		free(romTexture[i]);

	// copy and interleave the 68000 ROMs
	for (i = 0; i < 0x80000; i++)
	{
		g68000MemoryBase[0x000000+i*2+0] = rom68000[0][i];
		g68000MemoryBase[0x000000+i*2+1] = rom68000[1][i];
		g68000MemoryBase[0x100000+i*2+0] = rom68000[2][i];
		g68000MemoryBase[0x100000+i*2+1] = rom68000[3][i];
	}
	
	// copy and interleave the 32031 ROMs
	for (i = 0; i < 0x400000/2; i++)
	{
		g32031MemoryBase[0x400000 + i] = ((UINT16 *)romGeometry[0])[i] + (((UINT16 *)romGeometry[1])[i] << 16);
		if (HLE_TMS)
			g32031FloatMemoryBase[0x400000 + i] = Convert32031ToFloat(g32031MemoryBase[0x400000 + i]);
	}
	
	// initialize the CPUs
	InitCPU(0, m68000_get_info, &g68000CPU);
	InitCPU(1, tms32031_get_info, &g32031CPU);
	InitADSP();
	
	// reset the CPUs
	if (g68000CPU.reset)
		(*g68000CPU.reset)(NULL);
	if (g32031CPU.reset)
		(*g32031CPU.reset)(&g32031Config);
	
	// read the controls
	UpdateControls();
	
	// halt the 32031 for now
	g32031IsHalted = 1;
}


//--------------------------------------------------
//	Game execution
//--------------------------------------------------

void GameExecute(void)
{
	int startCycles68000 = 15000000 / 60;
	int startCycles32031 = 60000000 / 60;
	int startCycles2115 = GAME_SAMPLE_RATE / 60;
	int cycles68000 = startCycles68000;
	int cycles32031 = startCycles32031;
	int cycles2115 = startCycles2115;
	int max68000 = startCycles68000 / 100;
	int max32031 = startCycles32031 / 100;
	int max2115 = startCycles2115 / 100;
	
	// loop until we're out of cycles
	while (cycles68000 > 0 || cycles32031 > 0)
	{
		int cycles, i, samplesNeeded;
		
		// run the 68000
		cycles = (cycles68000 > max68000) ? max68000 : cycles68000;
		cycles = ExecuteCPU(cycles, &g68000CPU);
		cycles68000 -= cycles;
		
		// run the 32031 for the same amount of time
		cycles = (cycles * 4) + 1;
		if (cycles > cycles32031) cycles = cycles32031;
		if (!g32031IsHalted)
		{
			if (!HLE_TMS)
				cycles = ExecuteCPU(cycles, &g32031CPU);
			else
				SwitchToFiber(gTMSFiber);
		}
		cycles32031 -= cycles;
		
		// compute the effective number of samples we expect to produce from the 2115
		cycles = startCycles2115 - (((startCycles68000 - cycles68000) / 1024) * startCycles2115 / (startCycles68000 / 1024));
		if (cycles < cycles2115)
		{
			gADSPSamplesNeeded += cycles2115 - cycles;
			cycles2115 = cycles;
		}

		// compute how many samples we need right now to keep the sound buffer full
		samplesNeeded = SoundBufferReady();
		if (samplesNeeded > 0 && gADSPSamplesNeeded < samplesNeeded)
			gADSPSamplesNeeded = samplesNeeded;

		// if we need samples or if there's an interrupt pending, run the ADSP
		if (gSoundBufferCount < gADSPSamplesNeeded || gADSPInterrupt)
		{
			SwitchToFiber(gADSPFiber);
			if (samplesNeeded != 0 && gSoundBufferCount >= gADSPSamplesNeeded)
			{
				WriteToSoundBuffer((INT16 *)gSoundBufferData);
				if (gSoundBufferCount > gADSPSamplesNeeded)
					memmove((INT16 *)&gSoundBufferData[0], (INT16 *)&gSoundBufferData[gADSPSamplesNeeded], (gSoundBufferCount - gADSPSamplesNeeded) * sizeof(gSoundBufferData[0]));
				gSoundBufferCount -= gADSPSamplesNeeded;
				gADSPSamplesNeeded = 0;
			}
		}
		
		// run any sync callbacks
		for (i = 0; i < gSyncCallbackCount; i++)
			(*gSyncCallbacks[i].callback)(gSyncCallbacks[i].value);
		gSyncCallbackCount = 0;
	}
	
	// signal an IRQ2 interrupt on the main CPU
	SetCPUInt(&g68000CPU, CPUINFO_INT_INPUT_STATE + 2, 1);
	
	// render what we have
	RenderPolys();
	
	// read the controls
	UpdateControls();
}


//--------------------------------------------------
//	Synchronization
//--------------------------------------------------

void AbortAndSetSyncCallback(void (*callback)(int), int value)
{
	AbortExecuteCPU();
	if (gSyncCallbackCount < 16)
	{
		gSyncCallbacks[gSyncCallbackCount].callback = callback;
		gSyncCallbacks[gSyncCallbackCount].value = value;
		gSyncCallbackCount++;
	}
}


//--------------------------------------------------
//	control reading
//--------------------------------------------------

void UpdateControls(void)
{
	// reset everything
	g68000MemoryBase[0x51000d] = 0xff;
	g68000MemoryBase[0x51001d] = 0xff;
	g68000MemoryBase[0x51002c] = 0xef | (g68000MemoryBase[0x51002c] & 0x10);
	
	// check the keys
	
	// port 0
	if (GetAsyncKeyState('1'))
		g68000MemoryBase[0x51000d] ^= 0x80;
	
	// port 2
	if (GetAsyncKeyState('5'))
		g68000MemoryBase[0x51002c] ^= 0x01;
	if (GetAsyncKeyState('9'))
		g68000MemoryBase[0x51002c] ^= 0x04;
	if (GetAsyncKeyState(VK_F2))
		g68000MemoryBase[0x51002c] ^= 0x08;
	
	// analog value
	if (gSavedData.controller == CONTROL_KEYBOARD)
	{
		if (GetAsyncKeyState(VK_LEFT))
			gAnalogValue -= 3;
		else if (GetAsyncKeyState(VK_RIGHT))
			gAnalogValue += 3;
		else if (gAnalogValue > 0x80)
		{
			gAnalogValue -= 3;
			if (gAnalogValue < 0x80)
				gAnalogValue = 0x80;
		}
		else
		{
			gAnalogValue += 3;
			if (gAnalogValue > 0x80)
				gAnalogValue = 0x80;
		}
	}
	else if (gSavedData.controller == CONTROL_MOUSE)
	{
		RECT screenRect;
		POINT pos;
		int width, mid;
		
		GetClientRect(gD3DWindow, &screenRect);
		ClientToScreen(gD3DWindow, &((POINT *)&screenRect)[0]);
		ClientToScreen(gD3DWindow, &((POINT *)&screenRect)[1]);
		
		mid = (screenRect.left + screenRect.right) / 2;
		width = screenRect.right - screenRect.left;

		ClipCursor(&screenRect);
		GetCursorPos(&pos);
		gAnalogValue = 0x80 + ((pos.x - mid) * 0xe0) / width;
	}
	if (gAnalogValue < 0x10)
		gAnalogValue = 0x10;
	if (gAnalogValue > 0xf0)
		gAnalogValue = 0xf0;
}


//--------------------------------------------------
//	Sync callbacks
//--------------------------------------------------

void Ack32031Interrupt(UINT8 val, offs_t addr)
{
//	Information("Setting 32031 interrupt : %d\n", 0);
	SetCPUInt(&g32031CPU, CPUINFO_INT_INPUT_STATE + 0, 0);
}


void IntTo32031(int value)
{
//	Information("Setting 32031 interrupt : %d\n", value);
	if (!HLE_TMS)
		SetCPUInt(&g32031CPU, CPUINFO_INT_INPUT_STATE + 0, value);
	else if (!g32031IsHalted)
		gTMSInterrupt = 1;
}


void Halt32031(int value)
{
//	Information("Halting 32031 : %d\n", value);
	g32031IsHalted = value;
	if (!g32031IsHalted)
	{
		(*g32031CPU.reset)(&g32031Config);
		if (HLE_TMS)
			InitTMS();
	}
	else
	{
		if (HLE_TMS)
			KillTMS();
	}
}


void SoundWrite(int value)
{
	gADSPDataMemoryBase[0x2000] = value;
	gADSPInterrupt = 1;
	SwitchToFiber(gADSPFiber);
}


//--------------------------------------------------
//	68000 memory accesses
//--------------------------------------------------

void Write68000(UINT32 address, UINT32 data, int size)
{
	switch (address)
	{
		case 0x510041:	// sound data (1)
			if (size != 1) goto Unknown;
			AbortAndSetSyncCallback(SoundWrite, data);
			break;
			
		case 0x510100:	// interrupt clear
			if (size != 2) goto Unknown;
			SetCPUInt(&g68000CPU, CPUINFO_INT_INPUT_STATE + 2, 0);
			break;

		case 0x510112:	// EEPROM data (2)
			if (size != 2) goto Unknown;
			gEEPROMDataLatch = data & 1;
			break;

		case 0x51011a:	// EEPROM clock (2)
			if (size != 2) goto Unknown;
			if ((data & 1) != gEEPROMClockState)
			{
				gEEPROMClockState = data & 1;
				if (gEEPROMClockState && gEEPROMCSState)
					EEPROMWrite(gEEPROMDataLatch);
			}
			break;

		case 0x510122:	// EEPROM CS (2)
			if (size != 2) goto Unknown;
			if ((data & 1) != gEEPROMCSState)
			{
				gEEPROMCSState = data & 1;
				if (!gEEPROMCSState)
					EEPROMReset();
			}
			break;
		
		case 0x510126:	// unknown - surf planet only
			if (size != 2) goto Unknown;
			break;

		case 0x51012a:	// TMS reset (2)
			if (size != 2) goto Unknown;
			{
				int newHalted = (data != 0xffff);
				if (g32031IsHalted != newHalted)
					AbortAndSetSyncCallback(Halt32031, newHalted);
			}
			break;

		case 0x510132:	// TMS interrupt (2)
			if (size != 2) goto Unknown;
			AbortAndSetSyncCallback(IntTo32031, data & 1);
			break;
		
		case 0x510157:	// analog port clock (1)
			if (size != 1) goto Unknown;
			if (data == 0)
			{
				gLatchedAnalogValue <<= 1;
				g68000MemoryBase[0x51002c] = (g68000MemoryBase[0x51002c] & ~0x10) | (((gLatchedAnalogValue >> 7) & 1) << 4);
			}
			break;
		
		case 0x510167:	// analog port latch (1)
			if (size != 1) goto Unknown;
			if (data == 0)
			{
				gLatchedAnalogValue = gAnalogValue;
				g68000MemoryBase[0x51002c] = (g68000MemoryBase[0x51002c] & ~0x10) | (((gLatchedAnalogValue >> 7) & 1) << 4);
			}
			break;
		
		case 0x510107:	// unknown (1)
		case 0x510127:	// unknown (1)
		case 0x510147:	// led (1)
		case 0x510177:	// led (1)
			if (size != 1) goto Unknown;
			break;

		default:
			if (address >= 0x400000 && address <= 0x40ffff)
			{
				int palette = (address >> 9) & 0x7f;
				data &= 0xffff;
				gPaletteChecksum[palette] += data - *(UINT16 *)&g68000MemoryBase[address];
				*(UINT16 *)&g68000MemoryBase[address] = data;
			}
			else
				goto Unknown;
			break;
	}
	return;

Unknown:
	Information("Write68000: %08X = %08X (%d)\n", address, data, size);
}


//--------------------------------------------------
//	EEPROM accesses
//--------------------------------------------------

void EEPROMReset(void)
{
	gEEPROMIsReading = FALSE;
	gEEPROMWriteBuffer = 0;
	gEEPROMReadBuffer = 0;
	g68000MemoryBase[0x510101] = 0xff ^ 0x04;
}


void EEPROMWrite(int bit)
{
	// if we're currently reading, update the current bit
	if (gEEPROMIsReading)
	{
		gEEPROMReadBuffer = (gEEPROMReadBuffer << 1) | 1;
		g68000MemoryBase[0x510101] = 0xff ^ (((gEEPROMReadBuffer >> 16) & 1) << 2);
		return;
	}
	
	// otherwise accumulate the bit and look for a match
	gEEPROMWriteBuffer = (gEEPROMWriteBuffer << 1) | bit;
	if ((gEEPROMWriteBuffer & 0xffffff00) == 0x600)
	{
		// read
		gEEPROMIsReading = TRUE;
		gEEPROMReadBuffer = gGameSavedData->eeprom[gEEPROMWriteBuffer & 0xff];
		gEEPROMWriteBuffer = 0;
		g68000MemoryBase[0x510101] = 0xff ^ 0x04;
		Information("EEPROM read @ %02X = %04X\n", gEEPROMWriteBuffer & 0xff, gEEPROMReadBuffer);
	}
	else if ((gEEPROMWriteBuffer & 0xff000000) == 0x5000000)
	{
		// write
		gGameSavedData->eeprom[(gEEPROMWriteBuffer >> 16) & 0xff] = gEEPROMWriteBuffer & 0xffff;
		Information("EEPROM write @ %02X = %04X\n", (gEEPROMWriteBuffer >> 16) & 0xff, gEEPROMWriteBuffer & 0xffff);
		gEEPROMIsReading = FALSE;
		gEEPROMReadBuffer = 0;
		gEEPROMWriteBuffer = 0;
		g68000MemoryBase[0x510101] = 0xff ^ 0x04;
	}
	else if ((gEEPROMWriteBuffer & 0xffffff00) == 0x700)
	{
		// erase
		gGameSavedData->eeprom[gEEPROMWriteBuffer & 0xff] = 0;
		Information("EEPROM erase @ %02X\n", gEEPROMWriteBuffer & 0xff);
		gEEPROMIsReading = FALSE;
		gEEPROMReadBuffer = 0;
		gEEPROMWriteBuffer = 0;
		g68000MemoryBase[0x510101] = 0xff ^ 0x04;
	}
}


//--------------------------------------------------
//	TMS32031 memory accesses
//--------------------------------------------------

void Write32031(UINT32 address, UINT32 data, int size)
{
	address /= 4;
	if ((address & 0xfffff8) == 0xc00000)
		gPolyData[gPolyIndex++] = data;
	else
		Information("Write32031: %08X = %08X (%d)\n", address, data, size);
}


//--------------------------------------------------
//	Convert 32031 floats to IEEE floats
//--------------------------------------------------

float Convert32031ToFloat(UINT32 val)
{
	INT32 _mantissa = val << 8;
	INT8 _exponent = (INT32)val >> 24;
	union
	{
		double d;
		float f[2];
		UINT32 i[2];
	} id;

	if (_mantissa == 0 && _exponent == -128)
		return 0;
	else if (_mantissa >= 0)
	{
		int exponent = (_exponent + 127) << 23;
		id.i[0] = exponent + (_mantissa >> 8);
	}
	else
	{
		int exponent = (_exponent + 127) << 23;
		INT32 man = -_mantissa;
		id.i[0] = 0x80000000 + exponent + ((man >> 8) & 0x00ffffff);
	}
	return id.f[0];
}


//--------------------------------------------------
//	Render state initialization
//--------------------------------------------------

void InitRenderState(void)
{	
	HRESULT result;

	// allocate a vertex buffer to use
	result = IDirect3DDevice8_CreateVertexBuffer(gD3DDevice,
				sizeof(Vertex) * 3 * MAX_POLYGONS, 
				D3DUSAGE_DYNAMIC | D3DUSAGE_SOFTWAREPROCESSING | D3DUSAGE_WRITEONLY,
				VERTEX_FORMAT,
				D3DPOOL_DEFAULT,
				&gVertexBuffer);
	if (result != D3D_OK)
		FatalError("Error creating vertex buffer (%08X)", result);
	
	// set the vertex format
	result = IDirect3DDevice8_SetVertexShader(gD3DDevice, VERTEX_FORMAT);
	if (result != D3D_OK)
		FatalError("Error setting vertex shader (%08X)", result);
	
	// set the fixed render state
	result = IDirect3DDevice8_SetRenderState(gD3DDevice, D3DRS_ZENABLE, D3DZB_TRUE);
	result = IDirect3DDevice8_SetRenderState(gD3DDevice, D3DRS_FILLMODE, D3DFILL_SOLID);
	result = IDirect3DDevice8_SetRenderState(gD3DDevice, D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
	result = IDirect3DDevice8_SetRenderState(gD3DDevice, D3DRS_ZWRITEENABLE, TRUE);
	result = IDirect3DDevice8_SetRenderState(gD3DDevice, D3DRS_ALPHATESTENABLE, TRUE);
	result = IDirect3DDevice8_SetRenderState(gD3DDevice, D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	result = IDirect3DDevice8_SetRenderState(gD3DDevice, D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	result = IDirect3DDevice8_SetRenderState(gD3DDevice, D3DRS_CULLMODE, D3DCULL_NONE);
	result = IDirect3DDevice8_SetRenderState(gD3DDevice, D3DRS_ZFUNC, D3DCMP_LESS);
	result = IDirect3DDevice8_SetRenderState(gD3DDevice, D3DRS_ALPHAREF, 0);
	result = IDirect3DDevice8_SetRenderState(gD3DDevice, D3DRS_ALPHAFUNC, D3DCMP_GREATER);
	result = IDirect3DDevice8_SetRenderState(gD3DDevice, D3DRS_DITHERENABLE, FALSE);
	result = IDirect3DDevice8_SetRenderState(gD3DDevice, D3DRS_ALPHABLENDENABLE, FALSE);
	result = IDirect3DDevice8_SetRenderState(gD3DDevice, D3DRS_FOGENABLE, FALSE);
	result = IDirect3DDevice8_SetRenderState(gD3DDevice, D3DRS_SPECULARENABLE, FALSE);
	result = IDirect3DDevice8_SetRenderState(gD3DDevice, D3DRS_ZBIAS, 0);
	result = IDirect3DDevice8_SetRenderState(gD3DDevice, D3DRS_STENCILENABLE, FALSE);
	result = IDirect3DDevice8_SetRenderState(gD3DDevice, D3DRS_WRAP0, TRUE);
	result = IDirect3DDevice8_SetRenderState(gD3DDevice, D3DRS_CLIPPING, TRUE);
	result = IDirect3DDevice8_SetRenderState(gD3DDevice, D3DRS_LIGHTING, FALSE);
	result = IDirect3DDevice8_SetRenderState(gD3DDevice, D3DRS_COLORVERTEX, TRUE);

	result = IDirect3DDevice8_SetTextureStageState(gD3DDevice, 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
	result = IDirect3DDevice8_SetTextureStageState(gD3DDevice, 0, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
	result = IDirect3DDevice8_SetTextureStageState(gD3DDevice, 0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
}


//--------------------------------------------------
//	Texture list management
//--------------------------------------------------

INLINE AddTextureToList(Texture *tex)
{
	// set up prev/next pointers
	tex->prev = NULL;
	tex->next = gTextureListHead;
	
	// if there is a next, update its prev
	if (tex->next)
		tex->next->prev = tex;
	
	// set the head and potentially the tail
	gTextureListHead = tex;
	if (!gTextureListTail)
		gTextureListTail = tex;
	
	// update the count
	gTextureListCount++;
}


INLINE RemoveTextureFromList(Texture *tex)
{
	// if we have a prev set it, else we're the head
	if (tex->prev)
		tex->prev->next = tex->next;
	else
		gTextureListHead = tex->next;

	// if we have a next set it, else we're the tail	
	if (tex->next)
		tex->next->prev = tex->prev;
	else
		gTextureListTail = tex->prev;
	
	// update the count
	gTextureListCount--;
}


//--------------------------------------------------
//	Texture locator
//--------------------------------------------------

Texture *LocateSuitableTexture(UINT32 texbase, float minu, float maxu, float minv, float maxv, int color)
{
	UINT32 paletteChecksum = gPaletteChecksum[color];
	INT32 startx, starty, stopx, stopy, x, y;
	D3DLOCKED_RECT rect;
	HRESULT result;
	Texture *tex;
	
	// see if we have this one already
	for (tex = gTextureListHead; tex != NULL; tex = tex->next)
		if (tex->paletteChecksum == paletteChecksum && (tex->textureBase == texbase || (tex->minu <= minu && tex->maxu >= maxu && tex->minv <= minv && tex->maxv >= maxv)))
		{
			// we got it; bump to the head of the list
			RemoveTextureFromList(tex);
			tex->lastUsedFrame = gFrameIndex;
			AddTextureToList(tex);

			// return this value
			return tex;
		}

	// compute the bounds of a 512x512 texture centered at the texture base
	minu = (INT32)(texbase & 4095) - 256;
	minv = (INT32)((texbase >> 12) & (TEXTURE_DATA_SIZE/4096 - 1)) - 256;
	maxu = (INT32)(texbase & 4095) + 256;
	maxv = (INT32)((texbase >> 12) & (TEXTURE_DATA_SIZE/4096 - 1)) + 256;
	
	// handle the case where this puts us off the edge
	if (minu < 0)
	{
		maxu += -minu;
		minu = 0;
	}
	if (minv < 0)
	{
		maxv += -minv;
		minv = 0;
	}
	if (maxu > 4096)
	{
		minu -= maxu - 4096;
		maxu = 4096;
	}
	if (maxv > TEXTURE_DATA_SIZE/4096)
	{
		minv -= maxv - TEXTURE_DATA_SIZE/4096;
		maxv = TEXTURE_DATA_SIZE/4096;
	}
	
	// determine the integral bounds of this texture
	startx = minu;
	stopx = maxu;
	starty = minv;
	stopy = maxv;

	// okay, we don't have this texture loaded; make one
	if (gTextureListCount < MAX_TEXTURES || gTextureListTail->lastUsedFrame == gFrameIndex)
	{
		// allocate the texture object
		tex = malloc(sizeof(Texture));
		if (!tex)
			FatalError("Ran out of memory allocating memory for texture!");
		
		// create the texture
		result = IDirect3DDevice8_CreateTexture(gD3DDevice, stopx - startx, stopy - starty, 1, 0, D3DFMT_A1R5G5B5, D3DPOOL_MANAGED, &tex->texture);
		if (result != D3D_OK)
			FatalError("Error allocating %dx%d texture (%08X)", stopx - startx, stopy - starty, result);
	}
	
	// otherwise, we need to reuse the oldest texture
	else
	{
		tex = gTextureListTail;
		RemoveTextureFromList(tex);
	}

	// lock the texture data
	result = IDirect3DTexture8_LockRect(tex->texture, 0, &rect, NULL, D3DLOCK_DISCARD);
	if (result != D3D_OK)
		FatalError("Error locking texture (%08X)", result);
	
	// fill the buffer
	for (y = starty; y < stopy; y++)
	{
		UINT16 *palette = (UINT16 *)&g68000MemoryBase[0x400000 + color * 512];
		UINT16 *dest = (UINT16 *)((UINT8 *)rect.pBits + (y - starty) * rect.Pitch);
		for (x = startx; x < stopx; x++)
		{
			UINT16 source = gTextureSource[y & ((TEXTURE_DATA_SIZE/4096) - 1)][x & 4095];
			*dest++ = palette[source & 0xff] | (~source & 0x8000);
		}
	}
	
	// unlock the texture data
	result = IDirect3DTexture8_UnlockRect(tex->texture, 0);
	if (result != D3D_OK)
		FatalError("Error unlocking texture (%08X)", result);

	// fill in the rest of the tex structure
	tex->textureBase = texbase;
	tex->minu = startx;
	tex->maxu = stopx;
	tex->minv = starty;
	tex->maxv = stopy;
	tex->oowidth = 1.0f / (float)(stopx - startx);
	tex->ooheight = 1.0f / (float)(stopy - starty);
	tex->paletteChecksum = paletteChecksum;
	tex->lastUsedFrame = gFrameIndex;
	
	// add us to the list
	AddTextureToList(tex);
	return tex;
}


//--------------------------------------------------
//	Render the polys
//--------------------------------------------------

#define IS_POLYEND(x)		(((x) ^ ((x) >> 1)) & 0x4000)

void RenderPolys(void)
{
	int vertexCount = 0, primitiveCount = 0;
	Primitive *primitive = gPrimitives;
	IDirect3DTexture8 *lastTexture;
	UINT8 lastAlpha, lastNoZBuf;
	Vertex *vertexBuffer;
	HRESULT result;
	int i, p;
	
	// lock the vertex buffer
	result = IDirect3DVertexBuffer8_Lock(gVertexBuffer, 0, 0, (BYTE **)&vertexBuffer, D3DLOCK_DISCARD);
	if (result != D3D_OK)
		FatalError("Error locking the vertex buffer! (%08X)", result);
	
	// loop over polygons
	for (p = 0; p < gPolyIndex; )
	{
		// these three parameters combine via A * x + B * y + C to produce a 1/z value
		float ooz_dx = HLE_TMS ? *(float *)&gPolyData[p+4] : Convert32031ToFloat(gPolyData[p+4]);
		float ooz_dy = HLE_TMS ? *(float *)&gPolyData[p+3] : Convert32031ToFloat(gPolyData[p+3]);
		float ooz_base = HLE_TMS ? *(float *)&gPolyData[p+8] : Convert32031ToFloat(gPolyData[p+8]);

		// these three parameters combine via A * x + B * y + C to produce a u/z value
		float uoz_dx = HLE_TMS ? *(float *)&gPolyData[p+6] : Convert32031ToFloat(gPolyData[p+6]);
		float uoz_dy = HLE_TMS ? *(float *)&gPolyData[p+5] : Convert32031ToFloat(gPolyData[p+5]);
		float uoz_base = HLE_TMS ? *(float *)&gPolyData[p+9] : Convert32031ToFloat(gPolyData[p+9]);
		
		// these three parameters combine via A * x + B * y + C to produce a v/z value
		float voz_dx = HLE_TMS ? *(float *)&gPolyData[p+2] : Convert32031ToFloat(gPolyData[p+2]);
		float voz_dy = HLE_TMS ? *(float *)&gPolyData[p+1] : Convert32031ToFloat(gPolyData[p+1]);
		float voz_base = HLE_TMS ? *(float *)&gPolyData[p+7] : Convert32031ToFloat(gPolyData[p+7]);

		// this parameter is used to scale 1/z to a formal Z buffer value
		float z0 = (HLE_TMS ? *(float *)&gPolyData[p+0] : Convert32031ToFloat(gPolyData[p+0])) * (1.0f / 65536.0f);

		int color = gPolyData[p+10] & 0x7f;
		int skipit = FALSE;
		
		float x, y, ooz, z, minu, maxu, minv, maxv, clipx, clipy, clipu, clipv;
		Vertex vertexTemp[20];
		Vertex *vert = vertexTemp;
		
		// init the primitive
		primitive->startIndex = vertexCount;
		primitive->triCount = 0;
		primitive->texture = NULL;
		primitive->noZBuffer = (z0 < 0);
		primitive->alphaBlend = (color == 0x7f);
		primitive->texBase = gPolyData[p+11] % TEXTURE_DATA_SIZE;
		
		// extract the first vertex
		p += 13;
		vert->x = (float)((INT32)gPolyData[p] >> 16); 
		vert->y = (float)((INT32)(gPolyData[p] << 18) >> 18);
		ooz = ooz_dy * vert->y + ooz_dx * vert->x + ooz_base;
		z = 1.0f / ooz;
		vert->z = z0 * z;
		vert->rhw = ooz;
		vert->color = D3DCOLOR_ARGB(0x80,0xff,0xff,0xff);
		vert->u = primitive->texBase % 4096 + (uoz_dy * vert->y + uoz_dx * vert->x + uoz_base) * z;
		vert->v = primitive->texBase / 4096 + (voz_dy * vert->y + voz_dx * vert->x + voz_base) * z;
		clipx = (vert->x < -GAME_WIDTH/2) ? -GAME_WIDTH/2 : (vert->x > GAME_WIDTH/2) ? GAME_WIDTH/2 : vert->x;
		clipy = (vert->y < -GAME_HEIGHT/2) ? -GAME_WIDTH/2 : (vert->y > GAME_HEIGHT/2) ? GAME_HEIGHT/2 : vert->y;
		clipu = primitive->texBase % 4096 + (uoz_dy * clipy + uoz_dx * clipx + uoz_base) * z;
		clipv = primitive->texBase / 4096 + (voz_dy * clipy + voz_dx * clipx + voz_base) * z;
		minu = maxu = clipu;
		minv = maxv = clipv;
		if (primitive->noZBuffer)
			vert->z = -vert->z;
		else if (vert->z < 0)
			skipit = TRUE;
		vert++;

		// extract the second vertex
		p += 2;
		vert->x = (float)((INT32)gPolyData[p] >> 16); 
		vert->y = (float)((INT32)(gPolyData[p] << 18) >> 18);
		ooz = ooz_dy * vert->y + ooz_dx * vert->x + ooz_base;
		z = 1.0f / ooz;
		vert->z = z0 * z;
		vert->rhw = ooz;
		vert->color = D3DCOLOR_ARGB(0x80,0xff,0xff,0xff);
		vert->u = primitive->texBase % 4096 + (uoz_dy * vert->y + uoz_dx * vert->x + uoz_base) * z;
		vert->v = primitive->texBase / 4096 + (voz_dy * vert->y + voz_dx * vert->x + voz_base) * z;
		clipx = (vert->x < -GAME_WIDTH/2) ? -GAME_WIDTH/2 : (vert->x > GAME_WIDTH/2) ? GAME_WIDTH/2 : vert->x;
		clipy = (vert->y < -GAME_HEIGHT/2) ? -GAME_WIDTH/2 : (vert->y > GAME_HEIGHT/2) ? GAME_HEIGHT/2 : vert->y;
		clipu = primitive->texBase % 4096 + (uoz_dy * clipy + uoz_dx * clipx + uoz_base) * z;
		clipv = primitive->texBase / 4096 + (voz_dy * clipy + voz_dx * clipx + voz_base) * z;
		if (clipu < minu) minu = clipu;
		if (clipu > maxu) maxu = clipu;
		if (clipv < minv) minv = clipv;
		if (clipv > maxv) maxv = clipv;
		if (primitive->noZBuffer)
			vert->z = -vert->z;
		else if (vert->z < 0)
			skipit = TRUE;
		vert++;
	
		// loop over the remaining verticies
		while (!IS_POLYEND(gPolyData[p]))
		{
			p += 2;
			vert->x = (float)((INT32)gPolyData[p] >> 16); 
			vert->y = (float)((INT32)(gPolyData[p] << 18) >> 18);
			ooz = ooz_dy * vert->y + ooz_dx * vert->x + ooz_base;
			z = 1.0f / ooz;
			vert->z = z0 * z;
			vert->rhw = ooz;
			vert->color = D3DCOLOR_ARGB(0x80,0xff,0xff,0xff);
			vert->u = primitive->texBase % 4096 + (uoz_dy * vert->y + uoz_dx * vert->x + uoz_base) * z;
			vert->v = primitive->texBase / 4096 + (voz_dy * vert->y + voz_dx * vert->x + voz_base) * z;
			clipx = (vert->x < -GAME_WIDTH/2) ? -GAME_WIDTH/2 : (vert->x > GAME_WIDTH/2) ? GAME_WIDTH/2 : vert->x;
			clipy = (vert->y < -GAME_HEIGHT/2) ? -GAME_WIDTH/2 : (vert->y > GAME_HEIGHT/2) ? GAME_HEIGHT/2 : vert->y;
			clipu = primitive->texBase % 4096 + (uoz_dy * clipy + uoz_dx * clipx + uoz_base) * z;
			clipv = primitive->texBase / 4096 + (voz_dy * clipy + voz_dx * clipx + voz_base) * z;
			if (clipu < minu) minu = clipu;
			if (clipu > maxu) maxu = clipu;
			if (clipv < minv) minv = clipv;
			if (clipv > maxv) maxv = clipv;
			if (primitive->noZBuffer)
				vert->z = -vert->z;
			else if (vert->z < 0)
				skipit = TRUE;
			vert++;
			
			// keep track of how many triangles
			primitive->triCount++;
		}
		p += 2;
		
		// add the primitive to the list if we're not supposed to skip it
		if (!skipit)
		{
			// find a texture we can use
			primitive->texture = LocateSuitableTexture(primitive->texBase, minu, maxu, minv, maxv, color);
			
			// now build the final verts
			vert = vertexTemp;
			for (i = 0; i < primitive->triCount + 2; i++, vert++)
			{
				vertexBuffer->x = ((float)(GAME_WIDTH/2) + vert->x) * gGameXScale - 0.5;
				vertexBuffer->y = ((float)(GAME_HEIGHT/2) - vert->y) * gGameYScale - 0.5;
				vertexBuffer->z = vert->z;
				vertexBuffer->rhw = vert->rhw;
				vertexBuffer->color = vert->color;
				vertexBuffer->u = primitive->texture ? ((vert->u - primitive->texture->minu) * primitive->texture->oowidth) : 0;
				vertexBuffer->v = primitive->texture ? ((vert->v - primitive->texture->minv) * primitive->texture->ooheight) : 0;
				vertexBuffer++;
				vertexCount++;
			}
			
			// bump the primitive
			primitive++;
			primitiveCount++;
		}
	}
	
	// unlock the buffer
	result = IDirect3DVertexBuffer8_Unlock(gVertexBuffer);
	if (result != D3D_OK)
		FatalError("Error unlocking vertex buffer! (%08X)", result);

	// set the stream
	result = IDirect3DDevice8_SetStreamSource(gD3DDevice, 0, gVertexBuffer, sizeof(Vertex));
	if (result != D3D_OK)
		FatalError("Error setting the stream source to the vertex buffer! (%08X)", result);
	
	// reset the render state to a known state
	IDirect3DDevice8_SetTexture(gD3DDevice, 0, NULL);
	IDirect3DDevice8_SetRenderState(gD3DDevice, D3DRS_ALPHABLENDENABLE, FALSE);
	IDirect3DDevice8_SetRenderState(gD3DDevice, D3DRS_ZENABLE, D3DZB_TRUE);
	lastTexture = NULL;
	lastAlpha = 0;
	lastNoZBuf = 0;
	
	// render the primitivies
	primitive = gPrimitives;
	for (i = 0; i < primitiveCount; i++, primitive++)
	{
		IDirect3DTexture8 *texture = primitive->texture ? primitive->texture->texture : NULL;
		
		if (primitive->noZBuffer != lastNoZBuf)
		{
			lastNoZBuf = primitive->noZBuffer;
			IDirect3DDevice8_SetRenderState(gD3DDevice, D3DRS_ZENABLE, lastNoZBuf ? D3DZB_FALSE : D3DZB_TRUE);
		}
		if (primitive->alphaBlend != lastAlpha)
		{
			lastAlpha = primitive->alphaBlend;
			IDirect3DDevice8_SetRenderState(gD3DDevice, D3DRS_ALPHABLENDENABLE, lastAlpha ? TRUE : FALSE);
		}
		if (texture != lastTexture)
		{
			lastTexture = texture;
			IDirect3DDevice8_SetTexture(gD3DDevice, 0, (IDirect3DBaseTexture8 *)lastTexture);
		}
		IDirect3DDevice8_DrawPrimitive(gD3DDevice, D3DPT_TRIANGLEFAN, primitive->startIndex, primitive->triCount);
	}
	IDirect3DDevice8_SetTexture(gD3DDevice, 0, NULL);

	// reset the poly count
	gPolyIndex = 0;
}


//--------------------------------------------------
//	HLE TMS startup
//--------------------------------------------------

static float R0F, R1F, R2F, R3F, R4F, R5F, R6F, R7F;
static UINT32 R0, R1, R2, R3, R4, R5, R6, R7;
static UINT32 AR0, AR1, AR2, AR3, AR4, AR5, AR6, AR7;
static UINT32 SP, IR0, IR1, BK, RS, RE, RC;
static UINT8 BRANCH;

static __forceinline UINT32 RD(offs_t address)
{
if (address & 0x7f000000) DebugBreak();
	address &= 0xffffff;
	return tms32031_prd32l(address * 4);
}


static __forceinline void WR(offs_t address, UINT32 data)
{
if (address & 0x7f000000) DebugBreak();
	address &= 0xffffff;
	tms32031_pwd32l(address * 4, data);
}

static __forceinline float RDF(offs_t address)
{
if (address & 0xff000000) DebugBreak();
	address &= 0xffffff;
	if (address < 0x810000)
		return g32031FloatMemoryBase[address];
	else
		return *(float *)&g32031MemoryBase[address];
}

static __forceinline void WRF(offs_t address, float val)
{
if (address & 0xff000000) DebugBreak();
	address &= 0xffffff;
	if ((address & 0xfffff8) == 0xc00000)
		*(float *)&gPolyData[gPolyIndex++] = val;
	else if (address < 0x810000)
		g32031FloatMemoryBase[address] = val;
	else
		*(float *)&g32031MemoryBase[address] = val;
}

#define PUSH(x) WR(SP++, x)
#define POP(x) do { x = RD(--SP); } while (0)


void InitTMS(void)
{
	int addr;

	for (addr = 0x809800; addr < 0x80a000; addr++)
		g32031FloatMemoryBase[addr] = Convert32031ToFloat(g32031MemoryBase[addr]);
	WR(0x3fc0, 0x5555);
	
	// create the fiber to run on
	gTMSFiber = CreateFiber(4096, RunTMS, NULL);
	if (!gTMSFiber)
		FatalError("Can't create fiber for HLE emulation!");
}


void KillTMS(void)
{
	if (gTMSFiber)
		DeleteFiber(gTMSFiber);
	gTMSFiber = NULL;
}


static jmp_buf	gTMSBuffer;


VOID CALLBACK RunTMS(PVOID lpParameter)
{
	while (1)
	{
		int jmpresult;
		
		// allow for an early exit
		jmpresult = setjmp(gTMSBuffer);
		if (jmpresult == 1000)
			goto _809C8A;
		
		// if we don't have an interrupt, we have nothing to do
		while (!gTMSInterrupt)
			SwitchToFiber(gMainFiber);
		
		AR0 = 0x3FC0;
		R0 = RD(AR0 + 0x1);
		R0 &= 0xFFFF;
		R0 >>= 0x0001;
		WR(0x809C63, R0);
		R0 = RD(AR0 + 0x3);
		R1 = RD(AR0 + 0x4);
		R1 &= 0xFFFF;
		R0 <<= 0x0010;
		R0 |= R1;
		WR(0x809C68, R0);
		R0 = RD(AR0 + 0x5);
		R0 |= 0x0001;
		WR(AR0 + 0x5, R0);
		AR0 = RD(0x809C59);
		R0 = 0x0001;
		WR(0x809C6A, R0);
		gTMSInterrupt = 0;		// ack the interrupt
		R2 = RD(0x809FCF);
		WR(0x809C5E, R2);
		SP = RD(0x809C4F);
		R0 = RD(0x809C56);

	_809C8A:
		R0 = 0x0001;
		WR(0x809C65, R0);
		AR1 = RD(0x809C63);
		R0 = 0x0000;
		WR(0x809FFE, R0);
		WR(0x809C69, R0);
		R7 = RD(AR1 + 0xF);
		if (R7 == 0) goto _809CE8;
		AR3 = RD(0x809C53);
		AR1++;
		_809FF5();
		R1 = RD(AR1); AR1 += 0x6;
		R0F = (float) R1;
		WRF(0x809C70, R0F);
		R0F *= RDF(0x809C75);
		WRF(0x809C71, R0F);
		AR5 = RD(0x809C5C);
		AR5 += R1;
		R2F = (float) 0x0120;
		R2F *= RDF(AR5);
		R2F *= RDF(0x809C75);
		WRF(0x809C72, R2F);

	_809CA0:
		PUSH(R7);
		PUSH(AR1);
		R0 = RD(AR1++);
		BRANCH = ((R0 & 0x8000) == 0);
		R1 = R0;
		R0 &= 0x4000;
		WR(0x809C64, R0);
		if (BRANCH) goto _809CAA;
		_809CF1();
		goto _809CB4;

	_809CAA:
		R0 = 0x0028;
		if (R1 & 0x2000) R0 = 0x0005;
		WR(0x809C74, R0);
		R1 &= 0x007F;
		WR(0x809C6B, R1);
		if (_809F7F()) goto _809CB4;
		_809D4C();
		R0 &= 0x020F;
		if (R0 == 0) _809DD4();

	_809CB4:
		R0 = 0x0000;
		WR(0x809C65, R0);
		WR(0x809C69, R0);
		POP(AR1);
		POP(R7);
		AR1 += 0x0010;
		R7 -= 0x0001;
		if (R7 != 0) goto _809CA0;
		_809F62();
		if (R7 == 0) goto _809CE8;
		R0 = 0x0028;
		WR(0x809C74, R0);

	_809CC1:
		PUSH(R7); 
		PUSH(AR1); 
		R0 = RD(AR1++);
		R0 &= 0x4000;
		WR(0x809C64, R0);
		if (_809F7F()) goto _809CB4;
		RC = RD(0x809C61);
		if ((INT32)RC >= 0x00AA) goto _809CE0;
		R5F = (float) (INT32)RD(AR1 + 0x9);
		R6F = (float) (INT32)RD(AR1 + 0xA);
		R7F = (float) (INT32)RD(AR1 + 0xB);
		R0 = RD(AR1 + 0xD);
		R1 = RD(AR1 + 0xE);
		R1 <<= 0x000C;
		R0 |= R1;
		WR(0x809C67, R0);
		AR1 = RD(0x809C51);
		RC -= 0x0001;
		R4 = 0x0100;
		for ( ; (INT32)RC >= 0; RC--)
		{
			R0F = R5F + RDF(AR0++);
			WRF(AR1++, R0F); R1F = R6F + RDF(AR0++);
			WRF(AR1++, R1F); R2F = R7F + RDF(AR0++);
			WRF(AR1++, R2F);
			R0 = (INT32) R0F;
			WR(AR1++, R0);
			R1 = (INT32) R1F;
			WR(AR1++, R1);
			WR(AR1++, R4);
		}
		_809E8D();

	_809CE0:
		POP(AR1);
		POP(R7);
		AR1 += 0x0010;
		R7 -= 0x0001;
		if (R7 != 0) goto _809CC1;
		_809F62();
		if (R7 != 0) goto _809CA0;

	_809CE8:
		AR0 = 0x3FC0;
		R1 = RD(0x809FFF);
		WR(AR0 + 0x6, R1);
		R2 = RD(0x809FFE);
		WR(AR0 + 0x7, R2);
		R0 = RD(AR0 + 0x5);
		R0 &= 0xFFFE;
		WR(AR0 + 0x5, R0);
	}
}


void _809CF1(void)
{
	R5 = RD(AR1 + 0x9);
	R6 = RD(AR1 + 0xA);
	R7 = RD(AR1 + 0xB);
	R5 += RD(AR1);
	R6 += RD(AR1 + 0x1);
	R7 += RD(AR1 + 0x2);
	IR0 = RD(AR1 + 0xC);
	AR1 = RD(0x809C5D);
	WR(0x809BFD, R5);
	WR(0x809BFE, R6);
	WR(0x809BFF, R7);
	AR1 += RD(AR1 + IR0);
	_809CFD();	// fall through
}


void _809CFD(void)
{
	R7 = RD(AR1++);
	WR(0x809C69, R7);

_809CFF:
	PUSH(R7);
	PUSH(AR1); 
	R0 = RD(AR1++);
	if (R0 & RD(0x809C68)) goto _809D12;
	R0 >>= 0x0010;
	R0 |= 0x0001;
	WR(0x809C69, R0);
	if (_809F7F()) FatalError("809F7F jumped to _809CB4 when called from _809CF1");
	R5F = (float) (INT32)RD(0x809BFD);
	R5F += RDF(AR1 += 0x9);
	AR0 = RD(0x809C53);
	R6F = (float) (INT32)RD(0x809BFE);
	R6F = R6F + RDF(AR1 += 0x1);
	R7F = (float) (INT32)RD(0x809BFF);
	R7F = R7F + RDF(AR1 += 0x1);
	_809D52(0);
	R0 &= 0x020F;
	if (R0 == 0) _809DD4();

_809D12:
	POP(AR1);
	POP(R7);
	AR1 += 0x0010;
	R7 -= 0x0001;
	if (R7 != 0) goto _809CFF;
}


void _809D18(void)
{
	R3F = R5F;
	R4F = R6F;
	R0F = RDF(AR0++) * R5F;
	R2F = RDF(AR0++) * R6F;
	R2F = R2F + R0F;
	R0F = RDF(AR0++) * R7F;
	R5F = R2F + R0F;
	R0F = RDF(AR0++) * R3F;
	R2F = RDF(AR0++) * R6F;
	R2F = R2F + R0F;
	R0F = RDF(AR0++) * R7F;
	R6F = R2F + R0F;
	R1F = RDF(AR0++) * R3F;
	R0F = RDF(AR0++) * R4F;
	R2F = R1F + R0F;
	R0F = RDF(AR0++) * R7F;
	R7F = R2F + R0F;
}


void _809D4C(void)
{
	AR0 = RD(0x809C53);
	R0 = RD(0x809C65);
	R5F = (float) (INT32)RD(AR1 += 0x9);
	R6F = (float) (INT32)RD(AR1 += 0x1);
	R7F = (float) (INT32)RD(AR1 += 0x1);
	if (R0 != 0) _809D52(1);
	else _809D52(0);	// fall through
}


void _809D52(int goto_809D43)
{
	if (goto_809D43) goto _809D43;
	
	_809D18();
	R0F = (float) 0x0FA0;
	BRANCH = (R7F < R0F);
	R0F = (float) (INT32)RD(AR1 + 0x3);
	R2F = - R0F;
	R0F += R7F;
	if (BRANCH) goto _809D62;
	R1 = RD(AR1 + 0x2);
	R1 &= 0xFFFF;
	R1F = (float) R1;
	R1F *= R1F;
	R3F = R7F * R7F;
	R4F = R5F * R5F;
	R4F += R3F;
	if (R4F > R1F) goto _809D2A;

_809D62:
	if (R7F < R2F) goto _809D2A;
	if (R5F > R0F) goto _809D2A;
	R0F = - R0F;
	BRANCH = (R5F < R0F);
	AR1 -= 0xB;
	AR0 -= 0x9;
	AR3 = RD(0x809C54);
	if (BRANCH) goto _809D2A;
	R0 = RD(0x809C69);
	if (R0 == 0) goto _809D82;
	if (R0 & 0x8000) goto _809D77;
	if (R0 & 0x4000) goto _809D2C;
	R0F = RDF(AR1++);
	for (RC = 0x0007; (INT32)RC >= 0; RC--)
	{
		WRF(AR3++, R0F); R0F = RDF(AR1++);
	}
	WRF(AR3, R0F); AR3 -= 0x8;
	goto _809D83;

_809D77:
	AR2 = RD(0x809C50);
	R0F = RDF(AR0++);
	for (RC = 0x007; (INT32)RC >= 0; RC--)
	{
		WRF(AR2++, R0F); R0F = RDF(AR0++);
	}
	WRF(AR2, R0F);
	R5F = (float) (INT32)RD(0x809BFD);
	R6F = (float) (INT32)RD(0x809BFE);
	R7F = (float) (INT32)RD(0x809BFF);
	AR0 = RD(0x809C53);
	_809D18();
	goto _809D85;

_809D82:
	_809FF5();

_809D83:
	AR2 = RD(0x809C50);
	_809F6E();

_809D85:
	AR0 = RD(0x809C5F);
	AR1 = RD(0x809C51);
	AR3 = RD(0x809C50);
	AR5 = RD(0x809C5C);
	AR2 = RD(0x809C54);
	R0 = 0x0FFF;
	RC = RD(0x809C61);
	if ((INT32)RC > 0x00AA) return;
	RC -= 0x0001;
	BK = 0x0009;
	R0 = 0x030F;
	WR(AR2++, R0);
	for ( ; (INT32)RC >= 0; RC--)
	{
		R4F = RDF(AR0++);
		R0F = RDF(AR3++) * R4F;
		R3F = RDF(AR0++);
		R2F = RDF(AR3++) * R3F;
		WRF(AR2, R3F); R1F = RDF(AR0++);
		R2F = R2F + R0F;
		WRF(AR2 + 0x1, R1F); R0F = R1F * RDF(AR3++);
		R1F = R2F + R0F;
		R0F = RDF(AR3++) * R4F;
		R1F += R5F;
		WRF(AR1++, R1F); R2F = R3F * RDF(AR3++);
		R2F = R2F + R0F; R0F = RDF(AR3++) * RDF(AR2 + 0x1);
		R2F += R0F;
		R3F = R2F + R6F;
		WRF(AR1++, R3F); R0F = R4F * RDF(AR3++);
		R2F = R7F + R0F; R0F = RDF(AR3++) * RDF(AR2);
		R2F = R2F + R0F; R0F = RDF(AR3++) * RDF(AR2 + 0x1);
		AR3 -= 9;
		R2F += R0F;
		WRF(AR1++, R2F);
		IR0 = (INT32) R2F;
		R2F *= RDF(0x809C72);
		R4 = 0x0000;
		if (R1F >= R2F) R4 = 0x0001;
		R2F = - R2F;
		R0 = 0x0000;
		if (R1F <= R2F) R0 = 0x0002;
		R4 |= R0;
		R2F *= 0.750000;
		R0 = 0x0000;
		if (R3F <= R2F) R0 = 0x0008;
		R4 |= R0;
		R0 = 0x0000;
		R2F = - R2F;
		BRANCH = ((INT32)IR0 >= (INT32)RD(0x809C74));
		if (R3F >= R2F) R0 = 0x0004;
		R4 |= R0;
		if (BRANCH) goto _809DBE;
		R4 |= 0x0200;
		goto _809DC3;

	_809DBE:
		R4 |= 0x0100;
		R1F *= RDF(AR5 + IR0);
		R3F *= RDF(AR5 + IR0);
		R1F *= RDF(0x809C71);
		R3F *= RDF(0x809C71);

	_809DC3:
		R1 = (INT32) R1F;
		R3 = (INT32) R3F;
		if ((INT32)R1 > 0x1FFE) R1 = 0x1FFE;
		if ((INT32)R1 < -0x1FFE) R1 = -0x1FFE;
		if ((INT32)R3 > 0x1FFE) R3 = 0x1FFE;
		if ((INT32)R3 < -0x1FFE) R3 = -0x1FFE;
		WR(AR1++, R1); R0 = R4 & RD(AR2 - 0x1);
		WR(AR1++, R3); WR(AR2 - 0x1, R0);
		WR(AR1++, R4);
	}
	R0 = RD(AR2 - 0x1);
	return;

_809D2A:
	R0 = 0x000F;
	return;

_809D2C:
	R3F = (float) (INT32)RD(0x809BFD);
	R3F += RDF(AR1 + 0x9);
	R4F = (float) (INT32)RD(0x809BFF);
	R4F += RDF(AR1 + 0xB);
	R0F = R3F * R3F;
	R1F = R4F * R4F;
	R0F += R1F;
	_809FE0();
	R3F *= R0F;
	R4F *= R0F;
	R0F = 0.000000;
	R1F = 1.000000;
	WRF(AR3 + 0x4, R1F);
	WRF(AR3 + 0x1, R0F);
	WRF(AR3 + 0x3, R0F);
	WRF(AR3 + 0x5, R0F);
	WRF(AR3 + 0x7, R0F);
	R1F = - R3F;
	WRF(AR3, R4F);
	WRF(AR3 + 0x8, R4F);
	WRF(AR3 + 0x6, R3F);
	WRF(AR3 + 0x2, R1F);
	goto  _809D83;

_809D43:
	AR2 = RD(0x809C50);
	R0F = 0.000000;
	for (RC = 0x0006; (INT32)RC >= 0; RC--)
	{
		WRF(AR2 += 0x1, R0F);
	}
	R0F = 1.000000;
	WRF(AR2 + 0x1, R0F);
	WRF(AR2 - 0x3, R0F);
	WRF(AR2 - 0x7, R0F);
	goto  _809D85;
}


void _809DD4(void)
{
	AR0 = RD(0x809C60);
	R6 = RD(0x809C62);
	BK = 0x0009;
	R0 = 0x0000;
	WR(0x809C67, R0);

_809DD9:
	PUSH(R6);
	RC = RD(AR0++);
	AR1 = RD(AR0);
	WR(0x809BFC, RC);
	RC &= 0x0007;
	R1 = AR0 + RC;
	R1 += 0x0008;
	PUSH(R1);
	AR6 = RD(AR0 + 0x1);
	R1 = RD(0x809FFE);
	R1 += 0x0001;
	R0 = 0x030F;
	R0 &= RD(AR1 + 0x2);
	R0 &= RD(AR6 + 0x2);
	AR6 = RD(AR0 + 0x2);
	BRANCH = (RC == 0x0003);
	R0 &= RD(AR6 + 0x2);
	AR6 = RD(AR0 + 0x3);
	WR(0x809FFE, R1);
	if (BRANCH) goto _809DEE;
	R0 &= RD(AR6 + 0x2);

_809DEE:
	R1 = R0;
	R0 &= 0x020F;
	if (R0 == 0) _809DF6();
	POP(AR0);
	POP(R6);
	R6 -= 0x0001;
	if (R6 != 0) goto _809DD9;
}


void _809DF6(void)
{
	AR3 = RD(0x809C50);
	AR2 = RD(0x809C52);
	AR1 -= 0x0003;
	AR7 = AR0 + RC;
	AR5 = AR2;
	R5 = R1;
	R4F = RDF(AR7 += 0x1);
	R7F = RDF(AR7 += 0x1);
	R0F = RDF(AR3++) * R4F;
	R2F = RDF(AR3++) * R7F;
	R2F = R2F + R0F; R0F = RDF(AR3++) * RDF(AR7 + 0x1);
	R2F = R2F + R0F;
	WRF(AR2++, R2F); R0F = R4F * RDF(AR3++);
	R2F = RDF(AR3++) * R7F;
	R2F = R2F + R0F; R0F = RDF(AR3++) * RDF(AR7 + 0x1);
	R3F = R2F + R0F; R0F = RDF(AR3++) * RDF(AR7 - 0x1);
	R2F = RDF(AR3++) * RDF(AR7++);
	R2F = R2F + R0F; R0F = RDF(AR3++) * RDF(AR7++);
	AR3 -= 9;
	R2F = R2F + R0F; R0F = RDF(AR1++) * RDF(AR2 - 0x1);
	WRF(AR2 + 0x1, R2F); R1F = R3F * RDF(AR1);
	R2F = R0F + R1F; R1F = RDF(AR1 + 0x1) * RDF(AR2 + 0x1);
	R0F = R2F + R1F;
	BRANCH = (R0F > 0);
	WRF(0x809C66, R0F);
	WRF(AR2, R3F); AR2 += 0x2;
	AR4 = AR2;
	if (BRANCH) goto _809E11;
	return;

_809E11:
	AR4 += 0x0001;
	if (R5 & 0x0100) goto _809E1D;
	PUSH(AR4);
	PUSH(AR5);
	_809EB9();
	BRANCH = (R0 != 0x0000);
	POP(AR5);
	POP(AR4);
	if (BRANCH) goto _809E1D;
	return;

_809E1D:
	R0F = RDF(AR3++) * RDF(AR7++);
	R5F = RDF(AR7);
	R2F = RDF(AR3++) * R5F;
	R2F = R2F + R0F; R0F = RDF(AR3++) * RDF(AR7 + 0x1);
	R2F = R2F + R0F; R0F = RDF(AR3++) * RDF(AR7 - 0x1);
	WRF(AR2++, R2F); R2F = R5F * RDF(AR3++);
	R2F = R2F + R0F; R0F = RDF(AR3++) * RDF(AR7 + 0x1);
	R3F = R2F + R0F; R0F = RDF(AR3++) * RDF(AR7 - 0x1);
	WRF(AR2, R3F); R2F = R5F * RDF(AR3++);
	R2F = R2F + R0F; R0F = RDF(AR3++) * RDF(AR7 += 0x1);
	AR3 -= 9;
	R2F = R2F + R0F;
	WRF(AR2 += 0x1, R2F);
	R5F = RDF(AR7 + 0x1);
	R1F = RDF(AR5 += 0x1) * RDF(AR4 + 0x1);
	R0F = RDF(AR4) * RDF(AR5 + 0x1);
	R2F = R0F - R1F; R0F = RDF(AR4 + 0x1) * RDF(AR5 - 0x1);
	R3F = R2F * R5F;
	WRF(AR2 += 0x1, R3F); R4F = RDF(AR4);
	R1F = RDF(AR5 + 0x1) * RDF(AR4 - 0x1);
	R2F = R0F - R1F; R0F = RDF(AR4 - 0x1) * RDF(AR5);
	R3F = R2F * R5F;
	WRF(AR2 += 0x1, R3F); R1F = R4F * RDF(AR5 - 0x1);
	R2F = R0F - R1F;
	R3F = R2F * R5F;
	WRF(AR2 + 0x1, R3F);
	AR5 = AR2;
	AR3 = RD(0x809C5A);			// pointer to rasterizer
	R2F = RDF(0x809C66);
	R5F = RDF(0x809C70);
	R2F *= R5F;
	R0 = RD(0x809C65);
	if (R0 != 0) R2F = RDF(0x809C78);
	R1F = R2F;
	R1F = -R1F;	//	R1 = ~R1;
	R0 = RD(0x809C64);
	if (R0 != 0) R2F = R1F;
	R0F = RDF(AR4 + 0x1) * RDF(AR1 - 0x1);
	WRF(AR3, R2F); R3F = RDF(AR4 + 0x1);
	R1F = RDF(AR4 - 0x1) * RDF(AR1 + 0x1);
	R2F = R0F - R1F; R0F = RDF(AR4) * RDF(AR1 + 0x1);
	WRF(AR3, R2F); R1F = R3F * RDF(AR1);
	R2F = R0F - R1F;
	WRF(AR3, R2F); R3F = RDF(AR1 + 0x1);
	R2F = RDF(AR2 -= 0x6);
	WRF(AR3, R2F); R2F = RDF(AR2 - 0x1);
	WRF(AR3, R2F); R0F = R3F * RDF(AR5 - 0x1);
	R1F = RDF(AR5 + 0x1) * RDF(AR1 - 0x1);
	R2F = R0F - R1F; R0F = RDF(AR5 + 0x1) * RDF(AR1);
	R1F = RDF(AR5) * RDF(AR1 + 0x1);
	WRF(AR3, R2F); R3F = RDF(AR4);
	R2F = R0F - R1F; R0F = RDF(AR4 - 0x1) * RDF(AR1);
	WRF(AR3, R2F); R1F = R3F * RDF(AR1 - 0x1);
	R3F = R5F * RDF(AR2 + 0x1);
	R2F = R0F - R1F; R0F = RDF(AR5) * RDF(AR1 - 0x1);
	R2F *= R5F;
	WRF(AR3, R2F); R4F = RDF(AR5 - 0x1);
	WRF(AR3, R3F); R1F = R4F * RDF(AR1);
	R2F = R0F - R1F;
	R2F *= R5F;
	WRF(AR3, R2F);
	R0 = RD(0x809BFC);
	R0 >>= 0x0010;
	R1 = RD(0x809C6B);
	if (R1 != 0) R0 = R1;
	WR(AR3, R0);
	R3 = RD(AR7 -= 0x6);
	WR(AR3, R3);

_809E60:
	AR5 = RD(AR0++);
	R3 = 0x3FFF;
	AR2 = RD(0x809C5B);
	PUSH(AR5);
	R4 = RD(0x809C77);
	RC -= 0x0003;
	R5 = RD(AR5);
	R0 = RD(AR5 + 0x1) & R3;
	R1 = (((INT32)RD(AR5) << 8) >> 8) * (((INT32)R4 << 8) >> 8);
	R7 = RD(AR5 + 0x1);
	AR5 = RD(AR0++);
	R2 = R0 | R1;
	PUSH(R2);
	for ( ; (INT32)RC >= 0; RC--)
	{
		WR(AR3, R2); R0 = R3 & RD(AR5 + 0x1);
		R2 = RD(AR5) - R5; R1 = (((INT32)RD(AR5) << 8) >> 8) * (((INT32)R4 << 8) >> 8);
		IR0 = R7 - RD(AR5 + 0x1);
		R7 -= IR0;
		R5 = R2 + R5;
		AR5 = RD(AR0++);
		R1 = R0 | R1;
		R2F = (float) R2;
		R2F = R2F * RDF(AR2 + IR0);
		WR(AR3, R1);
		R2 = (INT32) R2F;
	}
	WR(AR3, R2); R0 = R3 & RD(AR5 + 0x1);
	R2 = RD(AR5) - R5; R1 = (((INT32)RD(AR5) << 8) >> 8) * (((INT32)R4 << 8) >> 8);
	IR0 = R7 - RD(AR5 + 0x1);
	POP(R7);
	POP(AR4);
	R5 = R2 + R5;
	R1 = R0 | R1;
	R2F = (float) R2;
	R2F = R2F * RDF(AR2 + IR0);
	WR(AR3, R1);
	R2 = (INT32) R2F;
	WR(AR3, R2); R2 = R5 - RD(AR4);
	IR0 = RD(AR5 + 0x1) - RD(AR4 + 0x1);
	R7 |= 0x4000;
	R2F = (float) R2;
	WR(AR3, R7);
	R2F = R2F * RDF(AR2 + IR0);
	R2 = (INT32) R2F;
	WR(AR3, R2);
}


void _809E8D(void)
{
	AR0 = RD(0x809C60);
	R6 = RD(0x809C62);

_809E8F:
	PUSH(R6);
	RC = RD(AR0++);
	WR(0x809BFC, RC);
	RC &= 0x00FF;
	AR1 = AR0 + RC;
	AR1 += 0x0008;
	PUSH(AR1);
	AR1 = RD(AR0);
	_809E9D();
	POP(AR0);
	POP(R6);
	R6 -= 0x0001;
	if (R6 != 0) goto _809E8F;
}


void _809E9D(void)
{
	AR1 -= 0x0002;
	R0 = RC;
	AR6 = AR0 + R0;
	AR3 = RD(0x809C5A);			// pointer to rasterizer
	R5F = RDF(0x809C70);
	R2F = R5F * RDF(AR1 + 0x1);
	R2F = -R2F;	// originally R2 = ~R2
	WRF(AR3, R2F); R2F = RDF(AR1 + 0x1);
	R2F = -R2F;
	WRF(AR3, R2F);
	R3F = 0.000000;
	WRF(AR3, R3F);
	WRF(AR3, R3F);
	WRF(AR3, R3F);
	WRF(AR3, R3F);
	R2F = -R2F;
	WRF(AR3, R2F); R2F = R5F * RDF(AR1);
	WRF(AR3, R2F);
	WRF(AR3, R5F); R0F = R5F * RDF(AR1 - 0x1);
	R0F = -R0F;
	WRF(AR3, R0F);
	R0 = RD(0x809BFC);
	R0 >>= 0x0010;
	WR(AR3, R0);
	R3 = RD(AR6);
	R3 += RD(0x809C67);
	WR(AR3, R3);
//	goto _809E60;

_809E60:
	AR5 = RD(AR0++);
	R3 = 0x3FFF;
	AR2 = RD(0x809C5B);
	PUSH(AR5);
	R4 = RD(0x809C77);
	RC -= 0x0003;
	R5 = RD(AR5);
	R0 = RD(AR5 + 0x1) & R3;
	R1 = (((INT32)RD(AR5) << 8) >> 8) * (((INT32)R4 << 8) >> 8);
	R7 = RD(AR5 + 0x1);
	AR5 = RD(AR0++);
	R2 = R0 | R1;
	PUSH(R2);
	for ( ; (INT32)RC >= 0; RC--)
	{
		WR(AR3, R2); R0 = R3 & RD(AR5 + 0x1);
		R2 = RD(AR5) - R5; R1 = (((INT32)RD(AR5) << 8) >> 8) * (((INT32)R4 << 8) >> 8);
		IR0 = R7 - RD(AR5 + 0x1);
		R7 -= IR0;
		R5 = R2 + R5;
		AR5 = RD(AR0++);
		R1 = R0 | R1;
		R2F = (float) R2;
		R2F = R2F * RDF(AR2 + IR0);
		WR(AR3, R1);
		R2 = (INT32) R2F;
	}
	WR(AR3, R2); R0 = R3 & RD(AR5 + 0x1);
	R2 = RD(AR5) - R5; R1 = (((INT32)RD(AR5) << 8) >> 8) * (((INT32)R4 << 8) >> 8);
	IR0 = R7 - RD(AR5 + 0x1);
	POP(R7);
	POP(AR4);
	R5 = R2 + R5;
	R1 = R0 | R1;
	R2F = (float) R2;
	R2F = R2F * RDF(AR2 + IR0);
	WR(AR3, R1);
	R2 = (INT32) R2F;
	WR(AR3, R2); R2 = R5 - RD(AR4);
	IR0 = RD(AR5 + 0x1) - RD(AR4 + 0x1);
	R7 |= 0x4000;
	R2F = (float) R2;
	WR(AR3, R7);
	R2F = R2F * RDF(AR2 + IR0);
	R2 = (INT32) R2F;
	WR(AR3, R2);
}


void _809EB9(void)
{
	int bkmask;
	UINT32 temp;

	PUSH(AR3);
	PUSH(AR1);
	WR(0x809C6C, AR0);
	R0 = RD(0x809C74);
	R0 += 0x0004;
	WR(0x809C6D, R0);

_809EBF:
	IR0 = RD(0x809C6D);
	AR3 = RD(0x809C5C);
	AR1 = RD(0x809C54);
	AR4 = RD(0x809C55);
	R5F = (float) IR0;
	R0 = 0x0000;
	R7 = 0x0001;
	R6F = RDF(AR3 + IR0);
	R6F *= RDF(0x809C75);
	R0 = RD(AR0++);
	WR(AR4++, R0); R0 = RD(AR0++);
	WR(AR4++, R0); R0 = RD(AR0++);
	WR(AR4++, R0); R0 = RD(AR0++);
	WR(AR4++, R0);
	AR0 = RD(0x809C55);
	BK = RC;
	{
		UINT32 temp = BK;
		bkmask = temp;
		while (temp >>= 1)
			bkmask |= temp;
	}
	R0 = RC;

_809ED0:
	AR5 = RD(AR0); temp = (AR0 & bkmask) + 1; if (temp >= BK) temp -= BK; AR0 = (AR0 & ~bkmask) | (temp & bkmask);
	if (R5F < RDF(AR5 - 0x1)) goto _809ED9;
	R0 -= 0x0001;
	if ((INT32)R0 > 0) goto _809ED0;
	POP(AR1);
	POP(AR3);
	BK = 0x0009;
	return;
	
_809ED9:
	AR4 = RD(0x809C52);
	AR4 += 0x0003;
	WR(AR4++, AR5);
	AR6 = RD(AR0); temp = (AR0 & bkmask) + 1; if (temp >= BK) temp -= BK; AR0 = (AR0 & ~bkmask) | (temp & bkmask);
	if (R5F > RDF(AR6 - 0x1)) goto _809EFA;
	WR(AR4++, AR6);
	R7 += 0x0001;
	AR5 = AR6;
	AR6 = RD(AR0); temp = (AR0 & bkmask) + 1; if (temp >= BK) temp -= BK; AR0 = (AR0 & ~bkmask) | (temp & bkmask);
	if (RC == 0x0003) goto _809EEB;
	if (R5F > RDF(AR6 - 0x1)) goto _809EF1;
	WR(AR4++, AR6);
	R7 += 0x0001;
	AR5 = AR6;
	AR6 = RD(AR0); temp = (AR0 & bkmask) + 1; if (temp >= BK) temp -= BK; AR0 = (AR0 & ~bkmask) | (temp & bkmask);

_809EEB:
	if (_809F1D()) goto _809EBF;

_809EEC:
	AR5 = AR6;
	AR6 = RD(0x809C52);
	AR6 = RD(AR6 + 0x3);
	if (_809F1D()) goto _809EBF;
	goto _809F0F;

_809EF1:
	if (_809F1D()) goto _809EBF;
	AR5 = AR6;
	AR6 = RD(AR0); temp = (AR0 & bkmask) + 1; if (temp >= BK) temp -= BK; AR0 = (AR0 & ~bkmask) | (temp & bkmask);
	if (R5F > RDF(AR6 - 0x1)) goto _809EEC;
	if (_809F1D()) goto _809EBF;
	WR(AR4++, AR6);
	R7 += 0x0001;
	goto _809F0F;

_809EFA:
	if (_809F1D()) goto _809EBF;
	AR5 = AR6;
	AR6 = RD(AR0); temp = (AR0 & bkmask) + 1; if (temp >= BK) temp -= BK; AR0 = (AR0 & ~bkmask) | (temp & bkmask);
	if (RC == 0x0003) goto _809F0A;
	if (R5F > RDF(AR6 - 0x1)) goto _809F08;
	if (_809F1D()) goto _809EBF;
	WR(AR4++, AR6);
	R7 += 0x0001;
	AR6 = RD(AR0); temp = (AR0 & bkmask) + 1; if (temp >= BK) temp -= BK; AR0 = (AR0 & ~bkmask) | (temp & bkmask);
	WR(AR4++, AR6);
	R7 += 0x0001;
	goto _809F0F;

_809F08:
	AR5 = AR6;
	AR6 = RD(AR0); temp = (AR0 & bkmask) + 1; if (temp >= BK) temp -= BK; AR0 = (AR0 & ~bkmask) | (temp & bkmask);

_809F0A:
	if (R5F > RDF(AR6 - 0x1)) goto _809EEC;
	if (_809F1D()) goto _809EBF;
	WR(AR4++, AR6);
	R7 += 0x0001;

_809F0F:
	AR4 = RD(0x809C55);
	AR0 = RD(0x809C52);
	AR0 += 0x3;
	R0 = RD(AR0++);
	for (RC = 0x0003; (INT32)RC >= 0; RC--)
	{
		WR(AR4++, R0); R0 = RD(AR0++);
	}
	WR(AR4++, R0);
	AR0 = RD(0x809C55);
	POP(AR1);
	POP(AR3);
	RC = R7;
	BK = 0x0009;
	R0 = 0x0001;
}


int _809F1D(void)
{
	R0F = RDF(AR6 - 0x1) - RDF(AR5 - 0x1);
	WR(AR4++, AR1);
	R0F = fabs(R0F);
	R0F *= 16.000000;
	R0F += 0.500000;
	R1F = R5F - RDF(AR5 - 0x1);
	IR1 = (INT32) R0F;
	R1F = fabs(R1F);
	R2F = R5F - RDF(AR6 - 0x1);
	R2F = fabs(R2F);
	R3F = R2F;
	R0F = R6F * RDF(AR3 + IR1);
	R0F *= RDF(0x809C71);
	R0F *= 16.000000;
	R1F *= RDF(AR6 - 0x3);
	R2F *= RDF(AR5 - 0x3);
	R1F += R2F;
	R1F *= R0F;
	R1 = (INT32) R1F;
	R2 = labs(R1);
	BRANCH = ((INT32)R2 > 0x1FFE);
	WR(AR1++, R1);
	R1F = R5F - RDF(AR5 - 0x1);
	R1F = fabs(R1F);
	if (BRANCH) goto _809F41;
	R1F *= RDF(AR6 - 0x2);
	R3F *= RDF(AR5 - 0x2);
	R1F += R3F;
	R1F *= R0F;
	R1 = (INT32) R1F;
	R2 = labs(R1);
	if ((INT32)R2 > 0x1FFE) goto _809F41;
	WR(AR1++, R1);
	R7 += 0x0001;
	return 0;

_809F41:
	R0 = RD(0x809C6D);
	R0 += 0x0008;
	WR(0x809C6D, R0);
	AR0 = RD(0x809C6C);
//	POP(R0);		// popping off the return
	R0 = RD(0x809C6E);	// = 809EBF
//	PUSH(R0);		// pushing the new value on
	return 1;		// return 1 to indicate we want to jump to 809EBF
}


void _809F62(void)
{
	AR0 = 0x3FC0;

_809F63:
	R0 = RD(AR0 + 0x2);
	R0 &= 0x0001;
	if (R0 != 0) { SwitchToFiber(gMainFiber); if (gTMSInterrupt) longjmp(gTMSBuffer, 1); goto _809F63; }
	R7 = RD(AR1++);
	R7 &= 0xFFFF;
	if (R7 == 0) return;
	R0 = RD(AR1 + 0x1);
	R0 &= 0xFFFF;
	R0 >>= 0x0001;
	AR1 = R0;
}


void _809F6E(void)
{
	RC = 0x0002;
	for ( ; (INT32)RC >= 0; RC--)
	{
		R0F = RDF(AR0++) * RDF(AR3++);
		R1F = RDF(AR0++) * RDF(AR3);
		R2F = R1F + R0F; R0F = RDF(AR0++) * RDF(AR3 + 0x1);
		R2F = R2F + R0F; R0F = RDF(AR0++) * RDF(AR3 - 0x1);
		WRF(AR2++, R2F);
		R1F = RDF(AR0++) * RDF(AR3);
		R2F = R1F + R0F; R0F = RDF(AR0++) * RDF(AR3 + 0x1);
		R2F = R2F + R0F; R0F = RDF(AR0++) * RDF(AR3 - 0x1);
		WRF(AR2 + 0x2, R2F);
		R1F = RDF(AR0++) * RDF(AR3++);
		R2F = R1F + R0F; R0F = RDF(AR0++) * RDF(AR3++);
		R2F = R2F + R0F;
		WRF(AR2 + 0x5, R2F);
		AR0 -= 0x9;
	}
}


int _809F7F(void)
{
	R0 = RD(AR1 + 0xC);
	R1 = RD(0x809C5E);
	AR3 = R1 + R0;
	AR0 = R1 + RD(AR3);
	R0 = RD(AR0++);
	BRANCH = ((INT32)R0 < 0);
	R0 &= 0x7fffffff;
	WR(0x809C61, R0);
	R2 = RD(AR0++);
	if (BRANCH) goto _809F8F;
	WR(0x809C62, R2);
	WR(0x809C5F, AR0);
	R0 = (((INT32)R0 << 8) >> 8) * 0x0003;
	R0 += AR0;
	WR(0x809C60, R0);
	return 0;

_809F8F:
//	POP(R2);			// popping off the return 
	R0 = RD(AR1 + 0xE);
	AR3 = R1 + R0;
	AR3 = R1 + RD(AR3);
	AR3 += 0x0002;
	R2 = AR3 + 1; R2 += RD(AR3++);
	R2 -= 0x0001;
	WR(0x809C5E, R2);
	AR0 += 0x1;
	AR3 = RD(0x809C54);
	_809FF5();
	R5F = (float) (INT32)RD(AR1++);
	R6F = (float) (INT32)RD(AR1++);
	R7F = (float) (INT32)RD(AR1++);
	IR0 = RD(AR1 + 0x1);
	AR1 = AR0;
	AR1 += RD(AR1 + IR0);
	AR0 = AR3;
	_809D18();
	R5 = (INT32) R5F;
	WR(0x809BFD, R5);
	R6 = (INT32) R6F;
	WR(0x809BFE, R6);
	R7 = (INT32) R7F;
	WR(0x809BFF, R7);
	AR2 = RD(0x809C57);
	AR0 = RD(0x809C53);
	R0F = RDF(AR0++);
	for (RC = 0x0008; (INT32)RC >= 0; RC--)
	{
		WRF(AR2++, R0F); R0F = RDF(AR0++);
	}
	AR0 = RD(0x809C57);
	AR3 = RD(0x809C54);
	AR2 = RD(0x809C53);
	_809F6E();
	_809CFD();
	R2 = RD(0x809FCF);
	WR(0x809C5E, R2);
	AR0 = RD(0x809C57);
	AR2 = RD(0x809C53);
	R0F = RDF(AR0++);
	for (RC = 0x0008; (INT32)RC >= 0; RC--)
	{
		WRF(AR2++, R0F); R0F = RDF(AR0++);
	}
	return 1;		// return 1 to indicate that we need to jump to _809CB4....
}


void _809FE0(void)
{
	if (R0F <= 0)
		return;
	R0F = 1.0F / sqrt(R0F);
}


void _809FF5(void)
{
	R1F = RDF(0x809C76);
	R0F = (float) (INT32)RD(AR1++);
	R0F *= R1F;
	RC = 0x0007;
	for ( ; (INT32)RC >= 0; RC--)
	{
		WRF(AR3++, R0F); R0F = (float) (INT32)RD(AR1++);
		R0F *= R1F;
	}
	WRF(AR3, R0F); AR3 -= 0x8;
}




static int gMemoryBank;

INLINE UINT32 PMR(offs_t addr)
{
	return gADSPProgramMemoryBase[addr];
}

INLINE void PMW(offs_t addr, data32_t data)
{
	gADSPProgramMemoryBase[addr] = data & 0xffffff;
}

INLINE UINT16 DMR(offs_t addr)
{
	UINT16 temp;
	if (addr < 0x2000)
	{
//		Information("Read from %04X\n", addr);
		return ((UINT16 *)romADSP2115)[gMemoryBank * 0x2000 + addr];
	}
	
	return gADSPDataMemoryBase[addr];
}

INLINE void DMW(offs_t addr, data16_t data)
{
	if (addr < 2)
		gMemoryBank = (addr * 0x80) + (data & 0x7f);
	else
		if (addr < 0x2000) Information("Write to %04X = %04X\n", addr, data);
	gADSPDataMemoryBase[addr] = data;
}

#define CALL(x,ret) { PCSTACK[PCSP++] = ret; PC = x; break; }
#define RTS()		{ PC = PCSTACK[--PCSP]; break; }
#define JUMP(x)		{ PC = x; break; }
#define SETCNTR(x)	{ CNTRSTACK[CNTRSP++] = CNTR; CNTR = x; }
#define ENDLOOP(x)	{ if (--CNTR != 0) { PC = x; break; } else { CNTR = CNTRSTACK[--CNTRSP]; } }

void InitADSP(void)
{
	/* see how many words we need to copy */
	UINT16 *srcdata = (UINT16 *)romADSP2115;
	int pagelen = (srcdata[3] + 1) * 8;
	int i;
	for (i = 0; i < pagelen; i++)
	{
		UINT32 opcode = ((srcdata[i*4+0] & 0xff) << 16) | ((srcdata[i*4+1] & 0xff) << 8) | (srcdata[i*4+2] & 0xff);
		PMW(i, opcode);
	}

	// create the fiber to run on
	gADSPFiber = CreateFiber(4096, RunADSP, NULL);
	if (!gADSPFiber)
		FatalError("Can't create fiber for HLE emulation!");
	
	// start running
	SwitchToFiber(gADSPFiber);
}


VOID CALLBACK RunADSP(PVOID lpParameter)
{
	union
	{
		UINT32 srx;
		struct { UINT16 sr0, sr1; };
	} SR, SRa;
	union
	{
		UINT64 mrx;
		struct { UINT16 mr0, mr1, mr2, mrzero; };
	} MR, MRa;
	UINT16 I0 = 0, I1 = 0, I2 = 0, I3 = 0, I4 = 0, I5 = 0, I6 = 0, I7 = 0;
	UINT16 M0 = 0, M1 = 0, M2 = 0, M3 = 0, M4 = 0, M5 = 0, M6 = 0, M7 = 0;
	UINT16 L0 = 0, L1 = 0, L2 = 0, L3 = 0, L4 = 0, L5 = 0, L6 = 0, L7 = 0;
	UINT16 AX0 = 0, AX1 = 0, AY0 = 0, AY1 = 0, AR = 0, AF = 0, CNTR = 0;
	UINT16 MX0 = 0, MX1 = 0, MY0 = 0, MY1 = 0, MF = 0, SI;
	UINT16 CNTRSTACK[4], CNTRSP = 0;
	UINT16 PCSTACK[4], PCSP = 0;
	UINT16 PC = 0;
	UINT8 C;
	INT8 SE;
	INT32 temp;

	while (1)
	{
		switch (PC)
		{
			case 0x000: JUMP(0x001C);
			case 0x001: 
			case 0x002: 
			case 0x003: 
			case 0x004: JUMP(0x012A);				// IRQ2
			case 0x005: 
			case 0x006: 
			case 0x007: 
			case 0x008: //RTI;
			case 0x009: 
			case 0x00A: 
			case 0x00B: 
			case 0x00C: //RTI;
			case 0x00D: 
			case 0x00E: 
			case 0x00F: 
			case 0x010: JUMP(0x011C);				// IRQ1/SPORT1_TX
			case 0x011: 
			case 0x012: 
			case 0x013: 
			case 0x014: //RTI;
			case 0x015: 
			case 0x016: 
			case 0x017: 
			case 0x018: //RTI;
			case 0x019: 
			case 0x01A: 
			case 0x01B: 

			case 0x01C: //IMASK = 0x0000;
			case 0x01D: //ICNTL = 0x0004;
			case 0x01E: //MSTAT = 0x0050;
			case 0x01F: AX0 = 0x0008;
			case 0x020: DMW(0x3FFF, AX0);
			case 0x021: AX0 = 0x1249;
			case 0x022: DMW(0x3FFE, AX0);
			case 0x023: L0 = 0x0000;
			case 0x024: L1 = 0x0000;
			case 0x025: L2 = 0x0000;
			case 0x026: L3 = 0x0000;
			case 0x027: L4 = 0x0000;
			case 0x028: L5 = 0x0000;
			case 0x029: L6 = 0x0000;
			case 0x02A: L7 = 0x0000;
			case 0x02B: M0 = 0x0000;
			case 0x02C: M1 = 0x0001;
			case 0x02D: M3 = 0xFFFF;
			case 0x02E: AX0 = 0x0000;
			case 0x02F: I0 = 0x381A;				// 0x381A, length 0x180
			case 0x030: SETCNTR(0x0180);
			case 0x031: //DO 0x0032 UNTIL CE;
			case 0x032: DMW(I0++, AX0); ENDLOOP(0x031);
			case 0x033: I0 = 0x380D;				// 0x380D, length 0x008;
			case 0x034: SETCNTR(0x0008);
			case 0x035: //DO 0x0036 UNTIL CE;
			case 0x036: DMW(I0++, AX0); ENDLOOP(0x035);
			case 0x037: AX0 = 0x0000;
			case 0x038: DMW(0x380C, AX0);			// 0x380C
			case 0x039: AX0 = 0x3F00;
			case 0x03A: DMW(0x3815, AX0);			// 0x3815
			case 0x03B: I0 = 0x3816;
			case 0x03C: DMW(I0++, 0x1FDF);			// 0x3816, length 0x004
			case 0x03D: DMW(I0++, 0x1FDF);
			case 0x03E: DMW(I0++, 0x1FDF);
			case 0x03F: DMW(I0++, 0x1FDF);
			case 0x040: AX0 = 0x0020;
			case 0x041: DMW(0x399A, AX0);
			case 0x042: CALL(0x100,0x043);

			// main loop
			case 0x043: AX0 = DMR(0x3804);
			case 0x044: AY0 = DMR(0x3805);
			case 0x045: AR = AX0 - AY0;
			case 0x046: if ((INT16)AR >= 0) CALL(0x0143,0x047);		// look for data coming in
			case 0x047: AX0 = DMR(0x380C);
			case 0x048: AR = AX0;
			case 0x049: if (AR != 0) CALL(0x004B,0x04A);			// look for a signal from the TX interrupt
			case 0x04A: {
							static INT16 lastDiff = -1000;
							INT16 newDiff = DMR(0x3804) - DMR(0x3805);
							if (newDiff == lastDiff && DMR(0x380C) == 0)
							{
								if (gADSPInterrupt)
								{
									gADSPInterrupt = 0;
									CALL(0x004,0x043);
								}
								if (gSoundBufferCount < gADSPSamplesNeeded)
									CALL(0x010,0x043);
								SwitchToFiber(gMainFiber);
								lastDiff = -1000;
							}
							lastDiff = newDiff;
						}
						JUMP(0x0043);

			// process a TX interrupt;
			case 0x04B: AX0 = 0x0000;
			case 0x04C: DMW(0x380C, AX0);
			case 0x04D: I1 = 0x380D;
			case 0x04E: SETCNTR(0x0004);
			case 0x04F: //DO 0x0050 UNTIL CE;
			case 0x050: 	DMW(I1++, AX0); ENDLOOP(0x04F);
			case 0x051: I0 = 0x381A;
			case 0x052: SETCNTR(0x0010);
			case 0x053: //DO 0x0095 UNTIL CE;
			case 0x054: 	M2 = 0x0016;
			case 0x055: 	AR = 1; AY0 = DMR(I0++);									// read voice+0 (enable)
			case 0x056: 	AR = AR & AY0; AX1 = DMR(I0++);								// read voice+1 (step)
			case 0x057: 	if (AR == 0) JUMP(0x0095);
			case 0x058: 	AF = AY0; AY0 = DMR(I0++);									// read voice+2 (position lo)
			case 0x059: 	AR = AX1 + AY0; C = (AR < AX1); AX0 = DMR(I0--);			// read voice+3 (position hi)
			case 0x05A: 	SR.sr0 = AR;												// SR0 = low position
			case 0x05B: 	DMW(I0++, AR); AR = AX0 + 0 + C;							// write voice+2 (position lo)
			case 0x05C: 	DMW(I0++, AR);												// write voice+3 (position hi)
			case 0x05D: 	MR.mr1 = AR;												// MR1 = high position
			case 0x05E: 	AY1 = DMR(I0++);											// read voice+4 (end lo)
			case 0x05F: 	C = (AR >= AY1); AR = AR - AY1; AY1 = DMR(I0++);			// read voice+5 (end hi)
			case 0x060: 	if (AR == 0) { C = (SR.sr0 >= AY1); AR = SR.sr0 - AY1; }
			case 0x061: 	if (C) JUMP(0x00DD);
			case 0x062: 	if (AR == 0) JUMP(0x00DD);
			case 0x063: 	AR = 0x0004;
			case 0x064: 	AR = AR & AF; AX0 = DMR(I0++);								// read voice+6 (prev sample data)
			case 0x065: 	if (AR != 0) JUMP(0x00A1);
			case 0x066: 	AF = SR.sr0 ^ AY0; AY0 = DMR(I0++);							// read voice+7 (next sample data)
			case 0x067: 	AX1 = 0xF000;
			case 0x068: 	AR = AX1 & AF; AY1 = SR.sr0;
			case 0x069: 	if (AR == 0) JUMP(0x007A);									// if (position lo ^ new position lo) & 0xF000
				case 0x06A: 	AX0 = AY0;
				case 0x06B: 	AY0 = 0x1000;
				case 0x06C: 	AR = SR.sr0 + AY0; C = (AR < SR.sr0);
				case 0x06D: 	SI = AR; AR = MR.mr1 + 0 + C;
				case 0x06E: 	SE = 0xFFF7;
				case 0x06F: 	M2 = 0x000B;
				case 0x070: 	SR.srx = (AR << 16) >> -SE; AR = DMR(I0); I0 += M2;
				case 0x071: 	M2 = 0xFFF3;
				case 0x072: 	SR.srx = SR.srx | (SI >> -SE); AR = DMR(I0); I0 += M2;	// read voice+19 (bank register)
				case 0x073: 	I1 = AR;
				case 0x074: 	DMW(I1, SR.sr1);
				case 0x075: 	SR.srx = SR.sr0 >> 3;
				case 0x076: 	I1 = SR.sr0;
				case 0x077: 	DMW(I0++, AX0);											// write voice+6
				case 0x078: 	AY0 = DMR(I1);
				case 0x079: 	DMW(I0++, AY0);											// write voice+7
			case 0x07A: 	AX1 = 0x0FFF;
			case 0x07B: 	AR = AX1 & AY1; MY0 = AY0;
			case 0x07C: 	MR.mrx = (INT64)(UINT16)AR * (INT64)(INT16)MY0; AY1 = AX1;
			case 0x07D: 	AR = AY1 - AR; MY0 = AX0;
			case 0x07E: 	MR.mrx = MR.mrx + (INT64)(UINT16)AR * (INT64)(INT16)MY0;
			case 0x07F: 	SR.srx = (INT32)(MR.mr1 << 16) >> 12;
			case 0x080: 	SR.srx = SR.srx | (MR.mr0 >> 12);
			case 0x081: 	AR = SR.sr0; MY0 = DMR(I0++);								// read voice+8 (overall volume)
			case 0x082: 	MR.mrx = (INT64)(INT16)AR * (INT64)(UINT16)MY0; MY0 = DMR(I0++);			// read voice+9 (channel volume)
			case 0x083: 	SR.srx = (INT32)((MR.mr1 << 16) >> 14);
			case 0x084: 	SR.srx = SR.srx | (MR.mr0 >> 14);
			case 0x085: 	I1 = 0x380D;
			case 0x086: 	//ENA AR_SAT ;
			case 0x087: 	MR.mrx = (INT64)(INT16)SR.sr0 * (INT64)(UINT16)MY0; AY0 = DMR(I1);
			case 0x088: 	temp = (INT16)MR.mr1 + (INT16)AY0; AR = (temp < -32768) ? -32768 : (temp > 32767) ? 32767 : temp; MY0 = DMR(I0++); // read voice+10
			case 0x089: 	DMW(I1++, AR);
			case 0x08A: 	MR.mrx = (INT64)(INT16)SR.sr0 * (INT64)(UINT16)MY0; AY0 = DMR(I1);
			case 0x08B: 	temp = (INT16)MR.mr1 + (INT16)AY0; AR = (temp < -32768) ? -32768 : (temp > 32767) ? 32767 : temp; MY0 = DMR(I0++); // read voice+11
			case 0x08C: 	DMW(I1++, AR);
			case 0x08D: 	MR.mrx = (INT64)(INT16)SR.sr0 * (INT64)(UINT16)MY0; AY0 = DMR(I1);
			case 0x08E: 	temp = (INT16)MR.mr1 + (INT16)AY0; AR = (temp < -32768) ? -32768 : (temp > 32767) ? 32767 : temp; MY0 = DMR(I0++); // read voice+12
			case 0x08F: 	DMW(I1++, AR);
			case 0x090: 	MR.mrx = (INT64)(INT16)SR.sr0 * (INT64)(UINT16)MY0; AY0 = DMR(I1);
			case 0x091: 	temp = (INT16)MR.mr1 + (INT16)AY0; AR = (temp < -32768) ? -32768 : (temp > 32767) ? 32767 : temp; 
			case 0x092: 	DMW(I1++, AR);
			case 0x093: 	//DIS AR_SAT ;
			case 0x094: 	M2 = 0x000B;
			case 0x095: 	I0 += M2; ENDLOOP(0x053);		// point to next voice
			case 0x096: I1 = 0x380D;
			case 0x097: I0 = 0x3811;
			case 0x098: SETCNTR(0x0004);
			case 0x099: //DO 0x009B UNTIL CE;
			case 0x09A: 	SR.sr0 = DMR(I1++);
			case 0x09B: 	DMW(I0++, SR.sr0); ENDLOOP(0x099);
			case 0x09C: AY0 = DMR(0x399A);
			case 0x09D: AR = AY0 - 1;
			case 0x09E: if ((INT16)AR <= 0) CALL(0x02A8,0x09F);
			case 0x09F: DMW(0x399A, AR);
			case 0x0A0: RTS();

			case 0x0A1: AR = SR.sr0 ^ AY0; AY0 = DMR(I0++);								// read voice+7
			case 0x0A2: AY1 = 0xFFFE;
			case 0x0A3: AR = AR & AY1; SI = AY0;
			case 0x0A4: if (AR != 0) JUMP(0x00AA);
			case 0x0A5: SE = 0xFFFF;
			case 0x0A6: SR.srx = (INT16)SI >> -SE; SI = AX0;
			case 0x0A7: AY0 = SR.sr0; SR.srx = (INT16)SI >> -SE;
			case 0x0A8: AR = SR.sr0 + AY0; MY0 = DMR(I0++);								// read voice+8
			case 0x0A9: JUMP(0x0082);

			case 0x0AA: M2 = 0x000B;
			case 0x0AB: MY0 = DMR(I0); I0 += M2;										// read voice+8
			case 0x0AC: M2 = 0xFFF3;
			case 0x0AD: AR = DMR(I0); I0 += M2;											// read voice+19
			case 0x0AE: I1 = AR;
			case 0x0AF: DMW(I1, MR.mr1);			// set bank
			case 0x0B0: SI = SR.sr0;
			case 0x0B1: SR.srx = SR.sr0 >> 3;
			case 0x0B2: I1 = SR.sr0;
			case 0x0B3: M2 = 0x0007;
			case 0x0B4: DMW(I0, AY0); I0 += M2;											// write voice+6
			case 0x0B5: SR.srx = SI << 1;
			case 0x0B6: AY1 = 0x000C;
			case 0x0B7: AR = SR.sr0 & AY1; AX0 = AY0;
			case 0x0B8: AY0 = DMR(I1);
			case 0x0B9: AR = AR - AY1; SI = AY0;
			case 0x0BA: SE = AR;
			case 0x0BB: SR.srx = (SE < 0) ? (SI >> -SE) : (SI << SE); AY1 = DMR(I0);	// read voice+13
			case 0x0BC: I5 = 0x0380;
			case 0x0BD: M4 = AY1;
			case 0x0BE: I5 += M4;
			case 0x0BF: AY0 = 0x0007;
			case 0x0C0: AR = SR.sr0 & AY0; AY0 = SR.sr0;
			case 0x0C1: SI = AR;
			case 0x0C2: I4 = 0x02DB;
			case 0x0C3: M4 = SI;
			case 0x0C4: I4 += M4;
			case 0x0C5: AX1 = PMR(I4) >> 8; I4 += M4;
			case 0x0C6: AR = AX1 + AY1; MY0 = PMR(I5) >> 8; I5 += M4;
			case 0x0C7: if ((INT16)AR < 0) AR = 0;
			case 0x0C8: AY1 = 0x0058;
			case 0x0C9: AF = AR - AY1;
			case 0x0CA: if ((INT16)AF > 0) AR = AY1;
			case 0x0CB: M2 = 0xFFFA;
			case 0x0CC: DMW(I0, AR); I0 += M2; AF = 1;									// write voice+13
			case 0x0CD: SR.srx = SI << 1;
			case 0x0CE: AR = SR.sr0 + AF;
			case 0x0CF: MR.mrx = (INT64)(UINT16)AR * (INT64)(UINT16)MY0;
			case 0x0D0: SR.srx = (MR.mr1 << 16) >> 4;
			case 0x0D1: SR.srx = SR.srx | MR.mr0 >> 4;
			case 0x0D2: AX1 = 0x0008;
			case 0x0D3: AF = SR.sr0;
			case 0x0D4: AR = AX1 & AY0;
			case 0x0D5: if (AR != 0) AF = -SR.sr0;
			case 0x0D6: //ENA AR_SAT ;
			case 0x0D7: temp = (INT16)AX0 + (INT16)AF; AR = (temp < -32768) ? -32768 : (temp > 32767) ? 32767 : temp;
			case 0x0D8: temp = (INT16)AR + (INT16)AF; AR = (temp < -32768) ? -32768 : (temp > 32767) ? 32767 : temp;
			case 0x0D9: //DIS AR_SAT ;
			case 0x0DA: DMW(I0++, AR); AR = AX0; 										// write voice+7
			case 0x0DB: MY0 = DMR(I0++);
			case 0x0DC: JUMP(0x0082);
			
			case 0x0DD: AR = 0x0008;
			case 0x0DE: AR = AR & AF;
			case 0x0DF: if (AR != 0) JUMP(0x00E7);
			case 0x0E0: AR = 0x0000;
			case 0x0E1: M2 = 0xFFFA;
			case 0x0E2: I0 += M2;
			case 0x0E3: M2 = 0x0009;
			case 0x0E4: DMW(I0, AR); I0 += M2;
			case 0x0E5: MY0 = 0x0000;
			case 0x0E6: JUMP(0x0082);
			
			case 0x0E7: M2 = 0x000A;
			case 0x0E8: I0 += M2;
			case 0x0E9: MR.mr1 = DMR(I0++);
			case 0x0EA: M2 = 0xFFF1;
			case 0x0EB: SR.sr0 = DMR(I0); I0 += M2;
			case 0x0EC: DMW(I0++, SR.sr0);
			case 0x0ED: M2 = 0x0005;
			case 0x0EE: DMW(I0, MR.mr1); I0 += M2; AR = 0;
			case 0x0EF: AX0 = 0x0004;
			case 0x0F0: AY0 = 0x0000;
			case 0x0F1: AF = AX0 & AF;
			case 0x0F2: if (AF == 0) JUMP(0x006A);
			case 0x0F3: M2 = 0xFFFA;
			case 0x0F4: I0 += M2;
			case 0x0F5: AY1 = 0x0002;
			case 0x0F6: C = (SR.sr0 >= AY1); AR = SR.sr0 - AY1;
			case 0x0F7: DMW(I0++, AR); AR = MR.mr1 + C - 1;
			case 0x0F8: M2 = 0x0004;
			case 0x0F9: DMW(I0, AR); I0 += M2; AR = 0;
			case 0x0FA: M2 = 0x0006;
			case 0x0FB: DMW(I0, AR); I0 += M2;
			case 0x0FC: M2 = 0xFFFC;
			case 0x0FD: DMW(I0, AR); I0 += M2;
			case 0x0FE: MY0 = 0x0000;
			case 0x0FF: JUMP(0x0082);

			case 0x100: AX0 = 0x0000;
			case 0x101: DMW(0x3804, AX0);		// reset sound in count to 0
			case 0x102: AX0 = 0x0001;
			case 0x103: DMW(0x3805, AX0);
			case 0x104: I7 = 0x0300;
			case 0x105: M7 = 0x0001;
			case 0x106: L7 = 0x0080;
			case 0x107: DMW(0x3807, I7);		// reset sound in/out pointers to 0x0300
			case 0x108: DMW(0x3808, I7);
			case 0x109: I6 = 0x3800;
			case 0x10A: L6 = 0x0004;
			case 0x10B: SETCNTR(0x0004);
			case 0x10C: AX0 = 0x0000;
			case 0x10D: //DO 0x010E UNTIL CE;
			case 0x10E: 	DMW(I6++, AX0); if ((I6 & 3) == 0) I6 -= 4; ENDLOOP(0x10D);
			case 0x10F: AX0 = 0x0D82;
			case 0x110: DMW(0x3FEF, AX0);		// SPORT1 autobuffer = 0x0D82 (transmit autobuffering; regs: I6,M7)
			case 0x111: AX0 = 0x0005;
			case 0x112: DMW(0x3FF1, AX0);		// SPORT1 serial clock = 0x0005 --> (16000000 / 12)
			case 0x113: AX0 = 0x4A0F;
			case 0x114: DMW(0x3FF2, AX0);		// SPORT1 control reg = 0x4A0F (internal clock; word length = 16 bits
			case 0x115: //IFC = 0x0007;
			case 0x116: //IMASK = 0x0024;
			case 0x117: AX0 = 0x0C08;
			case 0x118: DMW(0x3FFF, AX0);		// SPORT1 enable
			case 0x119: AX0 = DMR(0x3800);
			case 0x11A: //TX1 = AX0;
			case 0x11B: RTS();

			// IRQ1/SPORT_TX;
			case 0x11C: //ENA SEC_REG 			// swap registers
			case 0x11D: MRa.mr1 = PMR(I6++) >> 8;// MR.mr1 = next value
			case 0x11E: I6 = 0x3800;
			case 0x11F: SRa.sr0 = DMR(0x3814);
			case 0x120: DMW(0x3803, SRa.sr0);	// (0x3803) = (0x3814)
			case 0x121: SRa.sr0 = DMR(0x3813);
			case 0x122: DMW(0x3800, SRa.sr0);	// (0x3800) = (0x3813)
			case 0x123: SRa.sr0 = DMR(0x3812);
			case 0x124: DMW(0x3801, SRa.sr0);	// (0x3801) = (0x3812)
			case 0x125: SRa.sr0 = DMR(0x3811);
			case 0x126: DMW(0x3802, SRa.sr0);	// (0x3802) = (0x3811)
			case 0x127: DMW(0x380C, M7);		// (0x380C) = 1
			case 0x128: //DIS SEC_REG ;
						gSoundBufferData[gSoundBufferCount++] = DMR(0x3800);
//						gSoundBufferData[gSoundBufferCount++] = DMR(0x3801);
//						gSoundBufferData[gSoundBufferCount++] = DMR(0x3802);
						gSoundBufferData[gSoundBufferCount++] = DMR(0x3803);
			case 0x129: RTS();	// RTI

			// IRQ2;
			case 0x12A: DMW(0x3809, AY0);		// save AY0
			case 0x12B: DMW(0x380A, AR);		// save AR
			case 0x12C: DMW(0x380B, I7);		// save I7
			case 0x12D: I7 = DMR(0x3807);		// I7 = dest addr
			case 0x12E: AR = DMR(0x2000);		// AR = value from main board
			case 0x12F: AY0 = 0x00FF;
			case 0x130: AR = AR & AY0;			// AR &= 0xff
			case 0x131: PMW(I7++, AR << 8); if ((I7 & 0x7f) == 0) I7 -= 0x80;	// store in program memory
			case 0x132: DMW(0x3807, I7);		// save the updated I7
			case 0x133: AY0 = DMR(0x3804);
			case 0x134: AR = AY0 + 1;
			case 0x135: DMW(0x3804, AR);		// update count
			case 0x136: I7 = DMR(0x380B);		// restore values
			case 0x137: AR = DMR(0x380A);
			case 0x138: AY0 = DMR(0x3809);
			case 0x139: RTS();	// RTI

			case 0x13A: DMW(0x3808, I7);
			case 0x13B: //IMASK = 0x0004;
			case 0x13C: AX0 = DMR(0x3804);
			case 0x13D: AR = AX0 - AY0;
			case 0x13E: DMW(0x3804, AR);
			case 0x13F: //IMASK = 0x0024;
			case 0x140: AX0 = 0x0001;
			case 0x141: DMW(0x3805, AX0);
			case 0x142: RTS();

			// process input data
			case 0x143: I7 = DMR(0x3808);		// I7 = next input
			case 0x144: AX1 = PMR(I7++) >> 8; if ((I7 & 0x7f) == 0) I7 -= 0x80;	// AX1 = data
			case 0x145: AY0 = 0x00FF;
			case 0x146: AR = AX1 - AY0;
			case 0x147: if (AR == 0) JUMP(0x0171);	// if (data == 0xff) goto 171
			case 0x148: AY0 = 0x00E3;
			case 0x149: AR = AX1 - AY0;
			case 0x14A: if (AR == 0) JUMP(0x0294);	// if (data == 0xe3) goto 294
			case 0x14B: AY0 = 0x00E0;
			case 0x14C: AR = AX1 & AY0;
			case 0x14D: SR.sr0 = AR;
			case 0x14E: AY0 = 0x00C0;
			case 0x14F: AR = SR.sr0 - AY0;			// if ((data & 0xe0) == 0xc0) goto 17F
			case 0x150: if (AR == 0) JUMP(0x017F);
			case 0x151: AY0 = 0x0003;
			case 0x152: DMW(0x3805, AY0);
			case 0x153: AR = AX0 - AY0;
			case 0x154: if ((INT16)AR < 0) RTS();	// if not enough parameters, return for now
			case 0x155: AY0 = 0x00A0;
			case 0x156: AR = SR.sr0 - AY0;
			case 0x157: if (AR == 0) JUMP(0x01E4);	// if ((data & 0xe0) == 0xa0) goto 1E4
			case 0x158: AY0 = 0x00E2;
			case 0x159: AR = AX1 - AY0;
			case 0x15A: if (AR == 0) JUMP(0x029C);	// if (data == 0xe2) goto 29C
			case 0x15B: MR.mrx = 0;
			case 0x15C: MR.mr0 = 0x381A;
			case 0x15D: AY0 = 0x000F;
			case 0x15E: AR = AX1 & AY0;				// AR = data & 0xf
			case 0x15F: MY0 = 0x0018;
			case 0x160: MR.mrx = MR.mrx + (INT64)(UINT16)AR * (INT64)(UINT16)MY0;
			case 0x161: I0 = MR.mr0;				// I0 = 0x381A + (data & 0xf) * 0x18
			case 0x162: AR = SR.sr0;
			case 0x163: if (AR == 0) JUMP(0x021F);	// if ((data & 0xe0) == 0x00) goto 21F
			case 0x164: AY0 = 0x0020;
			case 0x165: AR = SR.sr0 - AY0;
			case 0x166: if (AR == 0) JUMP(0x021F);	// if ((data & 0xe0) == 0x20) goto 21F
			case 0x167: AY0 = 0x0040;
			case 0x168: AR = SR.sr0 - AY0;
			case 0x169: if (AR == 0) JUMP(0x0239);	// if ((data & 0xe0) == 0x40) goto 239
			case 0x16A: AY0 = 0x0060;
			case 0x16B: AR = SR.sr0 - AY0;
			case 0x16C: if (AR == 0) JUMP(0x027B);	// if ((data & 0xe0) == 0x60) goto 27B
			case 0x16D: AY0 = 0x0080;
			case 0x16E: AR = SR.sr0 - AY0;
			case 0x16F: if (AR == 0) JUMP(0x0282);	// if ((data & 0xe0) == 0x80) goto 282
			case 0x170: RTS();

			// sound command = 0xff;
			case 0x171: AY0 = 0x0001;
			case 0x172: CALL(0x013A,0x173);
			case 0x173: AR = 0;
			case 0x174: DMW(0x0000, AR);
			case 0x175: I0 = 0x381A;
			case 0x176: SETCNTR(0x0010);
			case 0x177: AY1 = 0x0001;
			case 0x178: //DO 0x017D UNTIL CE;
			case 0x179: 	AR = DMR(I0);
			case 0x17A: 	AR = AR & AY1;
			case 0x17B:		if (AR != 0) CALL(0x018A,0x17C);
			case 0x17C: 	M2 = 0x0018;
			case 0x17D: 	I0 += M2; ENDLOOP(0x178);
			case 0x17E: RTS();

			// sound command & 0xe0 = 0xc0;
			case 0x17F: MR.mrx = 0;
			case 0x180: MR.mr0 = 0x381A;
			case 0x181: AY0 = 0x000F;
			case 0x182: AR = AX1 & AY0;
			case 0x183: MY0 = 0x0018;
			case 0x184: MR.mrx = MR.mrx + (INT64)(UINT16)AR * (INT64)(UINT16)MY0;
			case 0x185: I0 = MR.mr0;
			case 0x186: AY0 = 0x0001;
			case 0x187: CALL(0x013A,0x188);
			case 0x188: AR = 0;
			case 0x189: DMW(0x0000, AR);		// set bank?
			case 0x18A: M2 = 0x0008;
			case 0x18B: AX1 = DMR(I0); I0 += M2;
			case 0x18C: M2 = 0x000A;
			case 0x18D: SR.sr0 = DMR(I0); I0 += M2;
			case 0x18E: M2 = 0xFFEE;
			case 0x18F: AR = SR.sr0; AY0 = DMR(I0); I0 += M2;
			case 0x190: if (AR != 0) JUMP(0x0194);
			case 0x191: AR = 0;
			case 0x192: DMW(I0, AR);
			case 0x193: RTS();

			case 0x194: I1 = AY0;
			case 0x195: M2 = 0x000B;
			case 0x196: I1 += M2;
			case 0x197: SR.sr0 = DMR(I1);
			case 0x198: AY0 = 0x0010;
			case 0x199: AR = AX1 | AY0;
			case 0x19A: M2 = 0x000F;
			case 0x19B: DMW(I0, AR); I0 += M2; AR = 0;
			case 0x19C: M2 = 0xFFFF;
			case 0x19D: DMW(I0, AR); I0 += M2;
			case 0x19E: M2 = 0xFFF2;
			case 0x19F: DMW(I0, SR.sr0); I0 += M2;
			case 0x1A0: RTS();

			case 0x1A1: I1 = 0x1000;
			case 0x1A2: SR.srx = SI << 4;
			case 0x1A3: M2 = SR.sr0;
			case 0x1A4: I1 += M2;
			case 0x1A5: M2 = 0x000F;
			case 0x1A6: I0 += M2;
			case 0x1A7: AX0 = 0x4000;
			case 0x1A8: M2 = 0xFFF9;
			case 0x1A9: DMW(I0, AX0); I0 += M2; AR = 0;							// write to voice+15
			case 0x1AA: DMW(0x0000, AR);
			case 0x1AB: M2 = 0x000A;
			case 0x1AC: DMW(I0, AR); I0 += M2;									// write to voice+8
			case 0x1AD: AX1 = I1;
			case 0x1AE: M2 = 0xFFFE;
			case 0x1AF: DMW(I0, AX1); I0 += M2;									// write to voice+18
			case 0x1B0: AY1 = DMR(I1++);
			case 0x1B1: DMW(I0++, AY1);											// write to voice+16
			case 0x1B2: M2 = 0x000B;
			case 0x1B3: AY0 = DMR(I1); I1 += M2;
			case 0x1B4: M2 = 0xFFF1;
			case 0x1B5: DMW(I0, AY0); I0 += M2;									// write to voice+17
			case 0x1B6: M2 = 0xFFF6;
			case 0x1B7: AR = 0x0004;
			case 0x1B8: AF = AR; MR.mr0 = DMR(I1); I1 += M2;
			case 0x1B9: AR = 0x0002;
			case 0x1BA: AX0 = 0x1000;
			case 0x1BB: AF = MR.mr0 & AF;
			case 0x1BC: if (AF == 0) AR = AX0;
			case 0x1BD: C = (AY0 >= AR); AR = AY0 - AR; AX0 = AY1;
			case 0x1BE: DMW(I0++, AR); AR = AX0 + C - 1;						// write to voice+2
			case 0x1BF: DMW(I0++, AR);											// write to voice+3
			case 0x1C0: AR = 0x0008;
			case 0x1C1: AF = AR; SR.sr1 = DMR(I1++);
			case 0x1C2: M2 = 0x0005;
			case 0x1C3: AR = MR.mr0 & AF; SR.sr0 = DMR(I1); I1 += M2;
			case 0x1C4: if (AR == 0) JUMP(0x01C9);
			case 0x1C5: M2 = 0xFFFE;
			case 0x1C6: I1 += M2;
			case 0x1C7: SR.sr1 = DMR(I1++);
			case 0x1C8: SR.sr0 = DMR(I1++);
			case 0x1C9: AR = SR.sr0 + AY0; C = (AR < SR.sr0);
			case 0x1CA: AX0 = AR; AR = SR.sr1 + AY1 + C;
			case 0x1CB: DMW(I0++, AR);											// write to voice+4
			case 0x1CC: M2 = 0xFFFC;
			case 0x1CD: DMW(I0, AX0); I0 += M2;									// write to voice+5
			case 0x1CE: AX0 = DMR(I1++);
			case 0x1CF: M2 = 0x0015;
			case 0x1D0: DMW(I0, AX0); I0 += M2;									// write to voice+1
			case 0x1D1: AX0 = DMR(I1++);
			case 0x1D2: M2 = 0xFFF8;
			case 0x1D3: DMW(I0, AX0); I0 += M2;									// write to voice+22
			case 0x1D4: M2 = 0x0003;
			case 0x1D5: AX0 = DMR(I1); I1 += M2;
			case 0x1D6: M2 = 0xFFF2;
			case 0x1D7: DMW(I0, AX0); I0 += M2;									// write to voice+14
			case 0x1D8: AY0 = 0x00EE;
			case 0x1D9: AR = MR.mr0 & AY0; AX0 = DMR(I1++);
			case 0x1DA: M2 = 0x0013;
			case 0x1DB: DMW(I0, AR); AR = 0; I0 += M2;							// write to voice+0
			case 0x1DC: M2 = 0xFFFA;
					Information("Bank = %d.%02X\n", AX0, DMR(I0 - 19 + 3));
			case 0x1DD: DMW(I0, AX0); I0 += M2;									// write to voice+19
			case 0x1DE: M2 = 0xFFF9;
			case 0x1DF: DMW(I0, AR); I0 += M2;									// write to voice+13
			case 0x1E0: DMW(I0++, AR);											// write to voice+6
			case 0x1E1: M2 = 0xFFF9;
			case 0x1E2: DMW(I0, AR); I0 += M2;									// write to voice+7
			case 0x1E3: RTS();

			// sound command & 0xe0 = 0xa0;
			case 0x1E4: AY0 = 0x0004;
			case 0x1E5: DMW(0x3805, AY0);
			case 0x1E6: AR = AX0 - AY0;
			case 0x1E7: if ((INT16)AR < 0) RTS();
			case 0x1E8: AY0 = 0x001F;
			case 0x1E9: AR = AX1 & AY0;
			case 0x1EA: SR.srx = AR << 11;
			case 0x1EB: I0 = 0x38DA;
			case 0x1EC: AR = 0x0002;
			case 0x1ED: AF = 1;
			case 0x1EE: M2 = 0x0018;
			case 0x1EF: AX0 = DMR(I0); I0 += M2;
			case 0x1F0: AY0 = AR; AR = AX0 & AF;
			case 0x1F1: if (AR == 0) JUMP(0x020B);
			case 0x1F2: AR = AY0 - 1;
			case 0x1F3: if ((INT16)AR > 0) JUMP(0x01EF);
			case 0x1F4: I0 = 0x38DA;
			case 0x1F5: AR = 0xF800;
			case 0x1F6: AF = AR; AX0 = DMR(I0); I0 += M2;
			case 0x1F7: I1 = I0;
			case 0x1F8: AR = AX0 & AF;
			case 0x1F9: AY0 = AR;
			case 0x1FA: AR = 0x0001;
			case 0x1FB: AX0 = DMR(I0); I0 += M2;
			case 0x1FC: AY1 = AR; AR = AX0 & AF;
			case 0x1FD: AX1 = AR; C = (AR >= AY0); AR = AR - AY0;
			case 0x1FE: if (C) JUMP(0x0201);
			case 0x1FF: AY0 = AX1;
			case 0x200: I1 = I0;
			case 0x201: AR = AY1 - 1;
			case 0x202: if ((INT16)AR > 0) JUMP(0x01FB);
			case 0x203: I0 = I1;
			case 0x204: C = (SR.sr0 >= AY0); AR = SR.sr0 - AY0;
			case 0x205: if (C) JUMP(0x020B);
			case 0x206: I7++; if ((I7 & 0x7f) == 0) I7 -= 0x80;
			case 0x207: I7++; if ((I7 & 0x7f) == 0) I7 -= 0x80;
			case 0x208: I7++; if ((I7 & 0x7f) == 0) I7 -= 0x80;
			case 0x209: AY0 = 0x0004;
			case 0x20A: JUMP(0x013A);
			
			case 0x20B: DMW(0x3806, SR.sr0);
			case 0x20C: M2 = 0xFFE8;
			case 0x20D: I0 += M2;
			case 0x20E: SI = PMR(I7++) >> 8; if ((I7 & 0x7f) == 0) I7 -= 0x80;
			case 0x20F: CALL(0x01A1,0x210);
			case 0x210: M2 = 0x0009;
			case 0x211: I0 += M2;
			case 0x212: AX0 = PMR(I7++) >> 8; if ((I7 & 0x7f) == 0) I7 -= 0x80;
			case 0x213: AX1 = PMR(I7++) >> 8; if ((I7 & 0x7f) == 0) I7 -= 0x80;
			case 0x214: CALL(0x026E,0x215);
			case 0x215: M2 = 0xFFF3;
			case 0x216: I0 += M2;
			case 0x217: AY0 = 0x0001;
			case 0x218: AR = DMR(0x3806);
			case 0x219: AR = AR | AY0;
			case 0x21A: AY0 = DMR(I0);
			case 0x21B: AR = AR | AY0;
			case 0x21C: DMW(I0, AR);
			case 0x21D: AY0 = 0x0004;
			case 0x21E: JUMP(0x013A);

			// sound command & 0xe0 = 0x00 or 0x20;
			case 0x21F: AY0 = 0x0004;
			case 0x220: DMW(0x3805, AY0);
			case 0x221: AR = AX0 - AY0;
			case 0x222: if ((INT16)AR < 0) RTS();
			case 0x223: DMW(0x3806, AX1);
			case 0x224: SI = PMR(I7++) >> 8; if ((I7 & 0x7f) == 0) I7 -= 0x80;
			case 0x225: CALL(0x01A1,0x226);
			case 0x226: M2 = 0x0009;
			case 0x227: I0 += M2;
			case 0x228: AX0 = PMR(I7++) >> 8; if ((I7 & 0x7f) == 0) I7 -= 0x80;
			case 0x229: AX1 = PMR(I7++) >> 8; if ((I7 & 0x7f) == 0) I7 -= 0x80;
			case 0x22A: CALL(0x026E,0x22B);
			case 0x22B: AX0 = DMR(0x3806);
			case 0x22C: AY0 = 0x0020;
			case 0x22D: AR = AX0 & AY0;
			case 0x22E: if (AR != 0) JUMP(0x0237);
			case 0x22F: M2 = 0xFFF3;
			case 0x230: I0 += M2;
			case 0x231: AX0 = DMR(I0);
			case 0x232: AY0 = 0x07FF;
			case 0x233: AR = AX0 & AY0;
			case 0x234: AY0 = 0x7801;
			case 0x235: AR = AR | AY0;
			case 0x236: DMW(I0, AR);
			case 0x237: AY0 = 0x0004;
			case 0x238: JUMP(0x013A);

			// sound command & 0xe0 = 0x40;
			case 0x239: AR = PMR(I7++) >> 8; if ((I7 & 0x7f) == 0) I7 -= 0x80;
			case 0x23A: SR.srx = AR << 8;
			case 0x23B: AX1 = PMR(I7++) >> 8; if ((I7 & 0x7f) == 0) I7 -= 0x80;
			case 0x23C: AY0 = 0x00FF;
			case 0x23D: AF = AX1 & AY0;
			case 0x23E: AR = SR.sr0 | AF;
			case 0x23F: MY0 = AR;
			case 0x240: M2 = 0x0012;
			case 0x241: I0 += M2;
			case 0x242: M2 = 0xFFEF;
			case 0x243: AR = 0; AX0 = DMR(I0); I0 += M2;
			case 0x244: DMW(0x0000, AR);
			case 0x245: I1 = AX0;
			case 0x246: M2 = 0x0008;
			case 0x247: I1 += M2;
			case 0x248: MX0 = DMR(I1);
			case 0x249: MR.mrx = (INT64)(UINT16)MX0 * (INT64)(UINT16)MY0;
			case 0x24A: SR.srx = (MR.mr1 << 16) >> 12;
			case 0x24B: SR.srx = SR.srx | (MR.mr0 >> 12);
			case 0x24C: DMW(I0, SR.sr0);
			case 0x24D: AY0 = 0x0003;
			case 0x24E: JUMP(0x013A);

			case 0x24F: AR = SR.sr0;
			case 0x250: if (AR == 0) RTS();
			case 0x251: AY0 = 0x000F;
			case 0x252: AR = SR.sr0 & AY0; SI = SR.sr0;
			case 0x253: SR.srx = AR << 1;
			case 0x254: AY0 = 0x0021;
			case 0x255: AR = SR.sr0 + AY0;
			case 0x256: SR.srx = SI >> 4;
			case 0x257: AY1 = 0x0007;
			case 0x258: SI = AR; AR = SR.sr0 & AY1;
			case 0x259: SE = AR;
			case 0x25A: SR.srx = (SE < 0) ? (SI >> -SE) : (SI << SE);
			case 0x25B: AR = SR.sr0 - AY0;
			case 0x25C: SR.srx = AR << 1;
			case 0x25D: RTS();

			case 0x25E: AY0 = 0x00F0;
			case 0x25F: AR = AX0 & AY0;
			case 0x260: AY0 = 0x000F;
			case 0x261: MR.mr1 = AR; AR = AX0 & AY0;
			case 0x262: MF = MR.mr1;
			case 0x263: MR.mrx = (INT64)(UINT16)AR * (INT64)(UINT16)MF;
			case 0x264: AY0 = 0x0010;
			case 0x265: AR = AY0 - AR; MX0 = MR.mr0;
			case 0x266: MR.mrx = (INT64)(UINT16)AR * (INT64)(UINT16)MF;
			case 0x267: MR.mrx = (INT64)(UINT16)MR.mr0 * (INT64)(UINT16)MY0;
			case 0x268: SR.srx = (MR.mr1 << 16) >> 10;
			case 0x269: SR.srx = SR.srx | (MR.mr0 >> 10);
			case 0x26A: MR.mrx = (INT64)(UINT16)MX0 * (INT64)(UINT16)MY1; AR = SR.sr0;
			case 0x26B: SR.srx = (MR.mr1 << 16) >> 10;
			case 0x26C: SR.srx = SR.srx | (MR.mr0 >> 10);
			case 0x26D: RTS();

			case 0x26E: SR.sr0 = AX0;
			case 0x26F: MY0 = DMR(0x3816);
			case 0x270: MY1 = DMR(0x3817);
			case 0x271: CALL(0x025E,0x272);
			case 0x272: DMW(I0++, AR);
			case 0x273: DMW(I0++, SR.sr0);
			case 0x274: AX0 = AX1;
			case 0x275: MY0 = DMR(0x3818);
			case 0x276: MY1 = DMR(0x3819);
			case 0x277: CALL(0x025E,0x278);
			case 0x278: DMW(I0++, AR);
			case 0x279: DMW(I0++, SR.sr0);
			case 0x27A: RTS();

			// sound command & 0xe0 = 0x60;
			case 0x27B: AX0 = PMR(I7++) >> 8; if ((I7 & 0x7f) == 0) I7 -= 0x80;
			case 0x27C: AX1 = PMR(I7++) >> 8; if ((I7 & 0x7f) == 0) I7 -= 0x80;
			case 0x27D: M2 = 0x0009;
			case 0x27E: I0 += M2;
			case 0x27F: CALL(0x026E,0x280);
			case 0x280: AY0 = 0x0003;
			case 0x281: JUMP(0x013A);

			// sound command & 0xe0 = 0x60;
			case 0x282: SR.sr0 = PMR(I7++) >> 8; if ((I7 & 0x7f) == 0) I7 -= 0x80;
			case 0x283: AX1 = PMR(I7++) >> 8; if ((I7 & 0x7f) == 0) I7 -= 0x80;
			case 0x284: CALL(0x024F,0x285);
			case 0x285: M2 = 0x000F;
			case 0x286: I0 += M2;
			case 0x287: DMW(I0, SR.sr0);
			case 0x288: SR.sr0 = AX1;
			case 0x289: CALL(0x024F,0x28A);
			case 0x28A: M2 = 0xFFFF;
			case 0x28B: I0 += M2;
			case 0x28C: M2 = 0xFFF2;
			case 0x28D: DMW(I0, SR.sr0); I0 += M2;
			case 0x28E: AX0 = DMR(I0);
			case 0x28F: AY0 = 0x0001;
			case 0x290: AR = AX0 | AY0;
			case 0x291: DMW(I0, AR);
			case 0x292: AY0 = 0x0003;
			case 0x293: JUMP(0x013A);

			// sound command = 0xe3;
			case 0x294: AY0 = 0x0002;
			case 0x295: AR = AX0 - AY0;
			case 0x296: if ((INT16)AR < 0) RTS();
			case 0x297: SR.sr0 = PMR(I7++) >> 8; if ((I7 & 0x7f) == 0) I7 -= 0x80;
			case 0x298: CALL(0x024F,0x299);
			case 0x299: DMW(0x3815, SR.sr0);
			case 0x29A: AY0 = 0x0002;
			case 0x29B: JUMP(0x013A);

			// sound command = 0xe2;
			case 0x29C: AY0 = 0x0005;
			case 0x29D: DMW(0x3805, AY0);
			case 0x29E: AR = AX0 - AY0;
			case 0x29F: if ((INT16)AR < 0) RTS();
			case 0x2A0: SETCNTR(0x0004);
			case 0x2A1: I0 = 0x3816;
			case 0x2A2: //DO 0x02A5 UNTIL CE;
			case 0x2A3: 	SR.sr0 = PMR(I7++) >> 8; if ((I7 & 0x7f) == 0) I7 -= 0x80;
			case 0x2A4: 	CALL(0x024F,0x2A5);
			case 0x2A5: 	DMW(I0++, SR.sr0); ENDLOOP(0x2A2);
			case 0x2A6: AY0 = 0x0005;
			case 0x2A7: JUMP(0x013A);

			case 0x2A8: I0 = 0x381A;
			case 0x2A9: SI = 0x0000;
			case 0x2AA: SETCNTR(0x0010);
			case 0x2AB: //DO 0x02D6 UNTIL CE;
			case 0x2AC: 	SR.srx = SI << 1;
			case 0x2AD: 	M2 = 0x0017;
			case 0x2AE: 	AR = 1; AY0 = DMR(I0++);
			case 0x2AF: 	AF = AR & AY0; SI = SR.sr0;
			case 0x2B0: 	if (AF == 0) JUMP(0x02D6);
			case 0x2B1: 	AR = SR.sr0 | AF;
			case 0x2B2: 	SI = AR;
			case 0x2B3: 	M2 = 0x000E;
			case 0x2B4: 	I0 += M2;
			case 0x2B5: 	M2 = 0x0007;
			case 0x2B6: 	MX0 = DMR(I0); I0 += M2;
			case 0x2B7: 	M2 = 0xFFF2;
			case 0x2B8: 	MY0 = DMR(0x3815);
			case 0x2B9: 	MR.mrx = (INT64)(UINT16)MX0 * (INT64)(UINT16)MY0;
			case 0x2BA: 	SE = 0xFFF2;
			case 0x2BB: 	SR.srx = (MR.mr1 << 16) >> -SE;
			case 0x2BC: 	SR.srx = SR.srx | (MR.mr0 >> -SE); MY0 = DMR(I0); I0 += M2;
			case 0x2BD: 	MR.mrx = (INT64)(UINT16)SR.sr0 * (INT64)(UINT16)MY0;
			case 0x2BE: 	M2 = 0x0006;
			case 0x2BF: 	SR.srx = (MR.mr1 << 16) >> -SE; AY0 = DMR(I0); I0 += M2;
			case 0x2C0: 	M2 = 0xFFFA;
			case 0x2C1: 	SR.srx = SR.srx | (MR.mr0 >> -SE); AX0 = DMR(I0); I0 += M2;
			case 0x2C2: 	AR = SR.sr0 - AY0;
			case 0x2C3: 	if (AR == 0) JUMP(0x02D5);
			case 0x2C4: 	if ((INT16)AR < 0) JUMP(0x02C9);
			case 0x2C5: 	AR = AX0 + AY0; AY0 = SR.sr0;
			case 0x2C6: 	AF = AR - AY0;
			case 0x2C7: 	if ((INT16)AF > 0) AR = AY0;
			case 0x2C8: 	JUMP(0x02D4);
			case 0x2C9: 	AR = AY0 - AX0; AY0 = SR.sr0;
			case 0x2CA: 	AF = AR - AY0;
			case 0x2CB: 	if ((INT16)AF < 0) { AR = AY0; if ((INT16)AR > 0) JUMP(0x02D4); } else if ((INT16)AF > 0) JUMP(0x02D4);
			case 0x2CC: 	//if ((INT16)AF > 0) JUMP(0x02D4);
			case 0x2CD: 	M2 = 0xFFF8;
			case 0x2CE: 	I0 += M2;
			case 0x2CF: 	M2 = 0x0008;
			case 0x2D0: 	AX0 = DMR(I0);
			case 0x2D1: 	AY0 = 0xFFFE;
			case 0x2D2: 	AR = AX0 & AY0;
			case 0x2D3: 	DMW(I0, AR); AR = 0; I0 += M2;
			case 0x2D4: 	DMW(I0, AR);
			case 0x2D5: 	M2 = 0x0010;
			case 0x2D6: 	I0 += M2; ENDLOOP(0x2AB);
			case 0x2D7: SR.srx = SI >> 8;
			case 0x2D8: if (SR.sr0 != g68000MemoryBase[0x510043]) Information("Status = %02X\n", SR.sr0); g68000MemoryBase[0x510043] = SR.sr0; //DMW(0x2000, SR.sr0);
			case 0x2D9: AR = 0x0020;
			case 0x2DA: RTS()
		}
	}
}

