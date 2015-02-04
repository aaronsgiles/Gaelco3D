//===================================================================
//
//	Main code file for standalone emulator shell
//
//	Copyright (c) 2004, Aaron Giles
//
//===================================================================

#include "mamecompat.h"
#include "zlib.h"
#include "resource.h"

#include <stdio.h>
#include <stdarg.h>


//--------------------------------------------------
//	Defines and limits
//--------------------------------------------------

#define WINDOW_STYLE			WS_OVERLAPPEDWINDOW
#define WINDOW_STYLE_EX			0

#define FULLSCREEN_STYLE		WS_POPUP
#define FULLSCREEN_STYLE_EX		WS_EX_TOPMOST

#define MAX_ZIP_HEADER_SIZE		4096
#define MAX_VIDEO_MODES			256

#define MAIN_SAVED_DATA_VERSION	1


//--------------------------------------------------
//	Global variables
//--------------------------------------------------

int gIsPaused = FALSE;
int gFullscreen = FALSE;
int gGameWidth = GAME_WIDTH;
int gGameHeight = GAME_HEIGHT;
D3DFORMAT gGameFormat = D3DFMT_UNKNOWN;
float gGameXScale;
float gGameYScale;

int gCurrentCPU;

UINT32 gFrameIndex;

int gThrottleDisable;
UINT32 gThrottleBaseTicks;
UINT32 gThrottleBaseFrame;

int gFPSDisplay;
UINT32 gFPSBaseFrame;
UINT32 gFPSBaseTicks;

int m68k_ICount;
int gFudgedCycles;
const CPUData *gExecutingCPU;

IDirect3D8 *gD3D;
IDirect3DDevice8 *gD3DDevice;

HWND gD3DWindow;

D3DDISPLAYMODE gVideoMode[MAX_VIDEO_MODES];
UINT32 gVideoModeCount;

LPDIRECTSOUND gDSound;
LPDIRECTSOUNDBUFFER	gDSoundPrimaryBuf;
LPDIRECTSOUNDBUFFER	gDSoundStreamBuf;
LPDIRECTSOUNDNOTIFY gDSoundNotifier;
DSBPOSITIONNOTIFY gDSoundNotification[2];
WAVEFORMATEX gDSoundFormat;
UINT32 gDSoundBufferSize;
UINT8 gCurrentSoundBuffer;

SavedData gSavedData;
 

//--------------------------------------------------
//	Prototypes
//--------------------------------------------------

INT_PTR CALLBACK OptionsDialogProc(HWND dialog, UINT uMsg, WPARAM wParam, LPARAM lParam);
void LoadSavedData();
void HandleMessages(void);
void HandleKey(WPARAM vkCode, int down);
void UpdateFPS(void);
void ThrottleGame(void);
void LoadROMs(void);
void ReadROM(FILE *zipFile, const UINT8 *cdEntry, UINT8 *dest, UINT32 size);
void PreinitDirect3D(void);
void InitDirect3D(void);
void TearDownDirect3D(void);
HWND InitDirect3DWindow(void);
void InitDirectSound(void);
void InitDirectSoundBuffers(void);
LRESULT CALLBACK D3DWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


//--------------------------------------------------
//	Main entry point
//--------------------------------------------------

#ifdef DEBUG
int main(int argc, char *argv[])
#else
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#endif
{
	// load the ROMs
	LoadROMs();
	
	// load any saved data
	LoadSavedData();
	
	// preinitialize D3D
	PreinitDirect3D();

	// present the options dialog
	if (DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOG1), NULL, OptionsDialogProc) == IDCANCEL)
		TerminateProcess(GetCurrentProcess(), 0);
	
	// initialize Direct3D
	InitDirect3D();

	// initialize DirectSound
	InitDirectSound();
	
	// initialize the game
	GameInit(&gSavedData.gamedata);
	
	// main loop
	while (1)
	{
		// process all messages at the beginning of the frame
		HandleMessages();

		// begin the scene
		IDirect3DDevice8_BeginScene(gD3DDevice);
		IDirect3DDevice8_Clear(gD3DDevice, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, RGB(0,0,0), 1.0, 0);
		
		// run the game
		GameExecute();
		
		// end the scene
		IDirect3DDevice8_EndScene(gD3DDevice);
		IDirect3DDevice8_Present(gD3DDevice, NULL, NULL, NULL, NULL);

		// increment the frame counters
		gFrameIndex++;
		
		// spin until done
		ThrottleGame();
		
		// every second, compute the fps
		UpdateFPS();
	}
}


//--------------------------------------------------
//	Handle messages for the dialog
//--------------------------------------------------

INT_PTR CALLBACK OptionsDialogProc(HWND dialog, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HWND targetWnd;

	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			int modeNum;
			
			// initialize the controller combo box
			targetWnd = GetDlgItem(dialog, IDC_COMBO2);
			if (targetWnd)
			{
				int controllers = 0;
				SendMessage(targetWnd, CB_ADDSTRING, 0, (LPARAM)"Keyboard");	controllers++;
				SendMessage(targetWnd, CB_ADDSTRING, 0, (LPARAM)"Mouse");		controllers++;
				SendMessage(targetWnd, CB_SETCURSEL, (gSavedData.controller < controllers) ? gSavedData.controller : 0, 0);
			}
			SetFocus(targetWnd);
			
			// iterate over modes and add them to the combo box
			targetWnd = GetDlgItem(dialog, IDC_COMBO1);
			if (targetWnd)
			{
				D3DDISPLAYMODE currentMode;
				int preferredSelection = -1;
				int currentSelection = -1;

				// get the current display mode
				IDirect3D8_GetAdapterDisplayMode(gD3D, D3DADAPTER_DEFAULT, &currentMode);
				
				// iterate over modes making strings
				for (modeNum = 0; modeNum < gVideoModeCount; modeNum++)
				{
					D3DDISPLAYMODE *mode = &gVideoMode[modeNum];
					char string[256];
					
					// add the string to the combo box
					sprintf(string, "%d x %d, %d bit (%d Hz)", mode->Width, mode->Height, 
								(mode->Format == D3DFMT_X1R5G5B5 || mode->Format == D3DFMT_R5G6B5) ? 16 : 32, mode->RefreshRate);
					SendMessage(targetWnd, CB_ADDSTRING, 0, (LPARAM)string);
					
					// check for a match against the current mode
					if (gVideoMode[modeNum].Width == currentMode.Width && gVideoMode[modeNum].Height == currentMode.Height && 
						gVideoMode[modeNum].Format == currentMode.Format)
						currentSelection = modeNum;
					
					// check for a match against the previously selected mode
					if (gVideoMode[modeNum].Width == gSavedData.width && gVideoMode[modeNum].Height == gSavedData.height && 
						gVideoMode[modeNum].Format == gSavedData.format)
						preferredSelection = modeNum;
				}
				
				// set the combo to either the preferred, current, or 0th selection
				if (preferredSelection != -1)
					SendMessage(targetWnd, CB_SETCURSEL, preferredSelection, 0);
				else if (currentSelection != -1)
					SendMessage(targetWnd, CB_SETCURSEL, currentSelection, 0);
				else
					SendMessage(targetWnd, CB_SETCURSEL, 0, 0);
				
				// set the default check state
				if (gSavedData.windowed)
				{
					EnableWindow(targetWnd, FALSE);
					targetWnd = GetDlgItem(dialog, IDC_CHECK1);
					if (targetWnd)
						SendMessage(targetWnd, BM_SETCHECK, 1, 0);
				}
			}
			return FALSE;
		}

		case WM_COMMAND:
		{
			int selection;
		
			switch (LOWORD(wParam)) 
			{ 
				case IDOK:
					// get the controller selection
					targetWnd = GetDlgItem(dialog, IDC_COMBO2);
					if (targetWnd)
						gSavedData.controller = SendMessage(targetWnd, CB_GETCURSEL, 0, 0);
				
					// get the windowed state
					targetWnd = GetDlgItem(dialog, IDC_CHECK1);
					if (targetWnd)
						gFullscreen = !(gSavedData.windowed = SendMessage(targetWnd, BM_GETCHECK, 0, 0));

					// get the requested video mode
					if (gFullscreen)
					{
						targetWnd = GetDlgItem(dialog, IDC_COMBO1);
						if (targetWnd)
						{
							selection = SendMessage(targetWnd, CB_GETCURSEL, 0, 0);
							gGameWidth = gSavedData.width = gVideoMode[selection].Width;
							gGameHeight = gSavedData.height = gVideoMode[selection].Height;
							gGameFormat = gSavedData.format = gVideoMode[selection].Format;
						}
					}
					// fall through
					
				case IDCANCEL: 
					EndDialog(dialog, wParam);
					return TRUE;
				
				case IDC_CHECK1:
					// if the checkbox is toggled, enable/disable the combo box
					targetWnd = GetDlgItem(dialog, IDC_CHECK1);
					if (targetWnd)
					{
						int checked = SendMessage(targetWnd, BM_GETCHECK, 0, 0);
						targetWnd = GetDlgItem(dialog, IDC_COMBO1);
						if (targetWnd)
							EnableWindow(targetWnd, checked ? FALSE : TRUE);
					}
					return TRUE;
			}
			break;
		}
		
		case WM_CLOSE:
			EndDialog(dialog, IDCANCEL);
			return TRUE;
	}
	return 0;
}


//--------------------------------------------------
//	Load/save saved data
//--------------------------------------------------

void LoadSavedData(void)
{
	char filename[MAX_PATH], *p;
	FILE *f;

	// get the name of the app
	GetModuleFileName(NULL, filename, sizeof(filename));
	
	// find the last backslash and append our game name
	p = strrchr(filename, '\\');
	if (p == NULL)
		p = filename - 1;
	strcpy(p + 1, GAME_FILENAME ".dat");

	// default to zeros	
	memset(&gSavedData, 0, sizeof(gSavedData));
	f = fopen(filename, "rb");
	if (f)
	{
		int bytes = fread(&gSavedData, 1, sizeof(gSavedData), f);
		
		// if we didn't read enough data, or there is a version mismatch, zap it
		if (bytes != sizeof(gSavedData) || gSavedData.version != ((MAIN_SAVED_DATA_VERSION << 16) | GAME_SAVED_DATA_VERSION))
			memset(&gSavedData, 0, sizeof(gSavedData));
		fclose(f);
	}
}


void SaveSavedData(void)
{
	char filename[MAX_PATH], *p;
	FILE *f;
	
	// get the name of the app
	GetModuleFileName(NULL, filename, sizeof(filename));
	
	// find the last backslash and append our game name
	p = strrchr(filename, '\\');
	if (p == NULL)
		p = filename - 1;
	strcpy(p + 1, GAME_FILENAME ".dat");

	// set the version
	gSavedData.version = (MAIN_SAVED_DATA_VERSION << 16) | GAME_SAVED_DATA_VERSION;
	
	// attempt to open the file
	f = fopen(filename, "wb");
	if (f)
	{
		int bytes = fwrite(&gSavedData, 1, sizeof(gSavedData), f);
		fclose(f);
	}
}



//--------------------------------------------------
//	Process messages from the system
//--------------------------------------------------

void HandleMessages(void)
{
	MSG msg;
	
	// loop until we're done
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		// quit message kills us
		if (msg.message == WM_QUIT)
		{
			SaveSavedData();
			TerminateProcess(GetCurrentProcess(), msg.wParam);
		}
		
		// keyup/keydown messages are handled here
		if (msg.message == WM_KEYDOWN || msg.message == WM_KEYUP)
			HandleKey(msg.wParam, msg.message == WM_KEYDOWN);
		
		// same for syskeys
		else if (msg.message == WM_SYSKEYDOWN || msg.message == WM_SYSKEYUP)
			HandleKey(msg.wParam, msg.message == WM_SYSKEYDOWN);
		
		// all other messages are standard
		else
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}


//--------------------------------------------------
//	Handle keyboard presses
//--------------------------------------------------

void HandleKey(WPARAM vkCode, int down)
{
	if (vkCode == VK_F10 && down)
		gThrottleDisable = !gThrottleDisable;
	
	if (vkCode == VK_F11 && down)
		gFPSDisplay = !gFPSDisplay;
	
	if (vkCode == VK_ESCAPE && down)
		PostQuitMessage(0);
	
	if (vkCode == 'P' && down)
	{
		gIsPaused = !gIsPaused;
		while (gIsPaused)
		{
			// feed the sounds system blanks while paused
			UINT32 soundSamples = SoundBufferReady();
			if (soundSamples > 0)
			{
				void *temp = malloc(soundSamples * sizeof(INT16));
				if (temp != NULL)
				{
					memset(temp, 0, soundSamples * sizeof(INT16));
					WriteToSoundBuffer(temp);
					free(temp);
				}
			}
			
			// make sure the cursor is enabled and handle messages
			SetCursor(LoadCursor(NULL, IDC_ARROW));
			ClipCursor(NULL);
			HandleMessages();
			
			// give time back to the system as well
			Sleep(100);
		}
		
		// disable the cursor again
		SetCursor(NULL);
	}
}
		

//--------------------------------------------------
//	Update the FPS counter
//--------------------------------------------------

void UpdateFPS(void)
{
	UINT32 frameCount = gFrameIndex - gFPSBaseFrame;

	// don't bother if we haven't seen enough frames
	if (frameCount < GAME_FPS)
		return;

	// if we have a valid count, update the FPS value
	if (gFPSBaseTicks != 0)
	{
		UINT32 fpsTicks = GetTickCount() - gFPSBaseTicks;
		UINT32 fps = frameCount * 1000 / (fpsTicks ? fpsTicks : 1);
		char buffer[100];

		// set the window caption
		if (gFPSDisplay)
			sprintf(buffer, GAME_NAME " (%d fps)", fps);
		else
			sprintf(buffer, GAME_NAME);
		SetWindowText(gD3DWindow, buffer);
		
		// if we're running too slow, reset the throttling
		if (fps < GAME_FPS - 1)
			gThrottleBaseTicks = 0;
	}

	// reset the counters
	gFPSBaseTicks = GetTickCount();
	gFPSBaseFrame = gFrameIndex;
}


//--------------------------------------------------
//	Throttle the game to run at a fixed FPS
//--------------------------------------------------

void ThrottleGame(void)
{
	UINT32 targetTicks;

	// if we're not throtling, or if we have no basis, reset everything
	if (gThrottleDisable || gThrottleBaseTicks == 0)
	{
		gThrottleBaseTicks = GetTickCount();
		gThrottleBaseFrame = gFrameIndex;
		return;
	}
	
	// compute the target ticks
	targetTicks = gThrottleBaseTicks + (int)((1000.0 / GAME_FPS) * (double)(gFrameIndex - gThrottleBaseFrame));

	// spin until we get there
	while (GetTickCount() < targetTicks) ;
}


//--------------------------------------------------
//	Load all the ROMs from a ZIP
//--------------------------------------------------

void LoadROMs(void)
{
	static ROMListEntry romList[] =
	{
		GAME_ROM_LIST
		{ 0 }
	};
	UINT8 headerBuffer[MAX_ZIP_HEADER_SIZE];
	UINT32 cdSize, cdOffset, cdEntries;
	UINT32 bytes, bytesRead, length;
	UINT8 *ecdHeader, *centralDir;
	ROMListEntry *entry;
	FILE *zipFile;
	
	// attempt to open the ZIP file
	zipFile = fopen(GAME_FILENAME ".zip", "rb");
	if (zipFile == NULL)
		FatalError("Unable to locate " GAME_FILENAME ".zip");
	
	// determine the file size
	fseek(zipFile, 0, SEEK_END);
	length = ftell(zipFile);
	
	// load the last 16k of the file
	memset(headerBuffer, 0, sizeof(headerBuffer));
	bytes = (length > sizeof(headerBuffer)) ? sizeof(headerBuffer) : length;
	fseek(zipFile, length - bytes, SEEK_SET);
	bytesRead = fread(headerBuffer, 1, bytes, zipFile);
	if (bytesRead != bytes)
		FatalError("Error while reading header from " GAME_FILENAME ".zip");
	
	// attempt to find the ECD structure in the last 16k of the file
	for (ecdHeader = &headerBuffer[sizeof(headerBuffer) - 22]; ecdHeader >= headerBuffer; ecdHeader--)
		if (ecdHeader[0] == 'P' && ecdHeader[1] == 'K' && ecdHeader[2] == 5 && ecdHeader[3] == 6)
			break;
	if (ecdHeader < headerBuffer)
		FatalError("Unable to located ECD structure in " GAME_FILENAME ".zip");
	
	// extract data from the header
	cdEntries = *(UINT16 *)&ecdHeader[0x0a];
	cdSize = *(UINT32 *)&ecdHeader[0x0c];
	cdOffset = *(UINT32 *)&ecdHeader[0x10];
	if (cdOffset + cdSize > length)
		FatalError("Invalid central directory information in " GAME_FILENAME ".zip");
	
	// allocate memory for the central directory
	centralDir = malloc(cdSize + 1);
	if (!centralDir)
		FatalError("Can't allocate %d bytes for central directory", cdSize + 1);
	
	// read in the central directory
	fseek(zipFile, cdOffset, SEEK_SET);
	bytesRead = fread(centralDir, 1, cdSize, zipFile);
	if (bytesRead != cdSize)
		FatalError("Error while reading central directory from " GAME_FILENAME ".zip");
	
	// loop over files and load them
	for (entry = romList; entry->pointer != NULL; entry++)
	{
		// find the entry
		for (cdOffset = 0; cdOffset < cdSize; cdOffset += 0x2e + *(UINT16 *)&centralDir[cdOffset + 0x1c] + *(UINT16 *)&centralDir[cdOffset + 0x1e])
			if (entry->crc == *(UINT32 *)&centralDir[cdOffset + 0x10])
				break;
		
		// error if we didn't find it or if we can't handle it
		if (cdOffset >= cdSize)
			FatalError("Could not find ROM in " GAME_FILENAME ".zip with CRC %08X", entry->crc);
		
		// allocate memory for the entry
		*entry->pointer = malloc(entry->length);
		if (!*entry->pointer)
			FatalError("Can't allocate %d bytes for ROM with CRC %08X", entry->length, entry->crc);
		
		// read the data
		ReadROM(zipFile, &centralDir[cdOffset], *entry->pointer, entry->length);
	}
	
	// close the file and free the memory
	fclose(zipFile);
	free(centralDir);
}


//--------------------------------------------------
//	Read a single ROM file from a ZIP
//--------------------------------------------------

void ReadROM(FILE *zipFile, const UINT8 *cdEntry, UINT8 *dest, UINT32 size)
{
	UINT32 dataOffset, compressedLength, dataLength, dataCRC, actualCRC, bytesRead;
	UINT8 dataBuffer[MAX_ZIP_HEADER_SIZE];
	UINT16 compression;

	// entry data from our entry
	dataCRC = *(UINT32 *)&cdEntry[0x10];
	dataOffset = *(UINT32 *)&cdEntry[0x2a];
	compressedLength = *(UINT32 *)&cdEntry[0x14];
	dataLength = *(UINT32 *)&cdEntry[0x18];
	compression = *(UINT16 *)&cdEntry[0x0a];

	Information("Loading ROM %08X from %08X compressed=%d data=%d...\n", dataCRC, dataOffset, compressedLength, dataLength);
		
	// check for valid compression
	if (compression != 0 && compression != 8)
		FatalError("Invalid compression type for ROM %08X (%d)", dataCRC, compression);

	// read the local header
	fseek(zipFile, dataOffset, SEEK_SET);
	bytesRead = fread(dataBuffer, 1, 0x1e, zipFile);
	if (bytesRead != 0x1e)
		FatalError("Error reading local header for ROM %08X", dataCRC);
	
	// skip past any filename data that may be present
	fseek(zipFile, *(UINT16 *)&dataBuffer[0x1a] + *(UINT16 *)&dataBuffer[0x1c], SEEK_CUR);
	
	// uncompressed is easy
	if (compression == 0)
	{
		bytesRead = fread(dest, 1, dataLength, zipFile);
		if (bytesRead != dataLength)
			FatalError("Error reading %d uncompressed bytes for ROM %08X", dataLength, dataCRC);
	}
	
	// compressed is trickier
	else
	{
		z_stream stream;
		int error;

		// initialize zlib
		memset(&stream, 0, sizeof(stream));
		error = inflateInit2(&stream, -MAX_WBITS);
		if (error != Z_OK)
			FatalError("Error %d while initializing the compression library", error);
	
		// set the output pointer and count
		stream.avail_out = dataLength;
		stream.next_out = dest;

		// loop until done
		while (stream.avail_out != 0)
		{
			// if we're out of input, read some more
			if (stream.avail_in == 0)
			{
				UINT32 bytesToRead = (compressedLength > sizeof(dataBuffer)) ? sizeof(dataBuffer) : compressedLength;
				bytesRead = fread(dataBuffer, 1, bytesToRead, zipFile);
				if (bytesRead != bytesToRead)
					FatalError("Error reading %d compressed bytes for ROM %08X", bytesToRead, dataCRC);
					
				// update the stream and track how much data remains
				stream.avail_in = bytesRead;
				stream.next_in = dataBuffer;
				compressedLength -= bytesRead;
			}
			
			// inflate the next chunk
			error = inflate(&stream, Z_NO_FLUSH);
			if (error != Z_OK && error != Z_STREAM_END)
				FatalError("Error %d while decompressing data for ROM %08X", error, dataCRC);
		}

		// finish inflating	
		inflateEnd(&stream);
	}
	
	// CRC check the final result
	actualCRC = crc32(0, NULL, 0);
	actualCRC = crc32(actualCRC, dest, size);
	if (actualCRC != dataCRC)
		FatalError("CRC of uncompressed data (%08X) does not match header (%08X) -- ZIP file is likely corrupt!", actualCRC, dataCRC);
}


//--------------------------------------------------
//	Create the D3D object and enumerate valid modes
//--------------------------------------------------

void PreinitDirect3D(void)
{
	HRESULT result = D3D_OK;
	int modeNum;

	// create the D3D object
	gD3D = Direct3DCreate8(D3D_SDK_VERSION);
	if (gD3D == NULL)
		FatalError("Error creating Direct3D (%08X)", GetLastError());

	// enumerate the video modes
	for (modeNum = 0; result == D3D_OK && gVideoModeCount < MAX_VIDEO_MODES; modeNum++)
	{
		result = IDirect3D8_EnumAdapterModes(gD3D, D3DADAPTER_DEFAULT, modeNum, &gVideoMode[gVideoModeCount]);
		if (result == D3D_OK && gVideoMode[gVideoModeCount].RefreshRate == 60)
			switch (gVideoMode[gVideoModeCount].Format)
			{
				case D3DFMT_X1R5G5B5:
				case D3DFMT_R5G6B5:
				case D3DFMT_X8R8G8B8:
				case D3DFMT_A8R8G8B8:
					gVideoModeCount++;
					break;
			}
	}
}
	

//--------------------------------------------------
//	Initialize D3D
//--------------------------------------------------

void InitDirect3D(void)
{
	D3DPRESENT_PARAMETERS d3dPresentation;
	D3DDISPLAYMODE currentMode;
	HRESULT result;
	
	// compute the scale factors
	gGameXScale = (float)gGameWidth / (float)GAME_WIDTH;
	gGameYScale = (float)gGameHeight / (float)GAME_HEIGHT;
	
	// create the window
	gD3DWindow = InitDirect3DWindow();

	// get the current display mode
	result = IDirect3D8_GetAdapterDisplayMode(gD3D, D3DADAPTER_DEFAULT, &currentMode);
	if (result != D3D_OK)
		FatalError("Can't get current display mode (%08X)", result);
	printf("Current display mode: %dx%d @ %dHz (mode=%d)\n", currentMode.Width, currentMode.Height, currentMode.RefreshRate, currentMode.Format);

	// initialize the D3D presentation parameters
	d3dPresentation.BackBufferWidth					= gGameWidth;
	d3dPresentation.BackBufferHeight				= gGameHeight;
	d3dPresentation.BackBufferFormat				= gFullscreen ? gGameFormat : currentMode.Format;
	d3dPresentation.BackBufferCount					= 1;
	d3dPresentation.MultiSampleType					= D3DMULTISAMPLE_NONE;
	d3dPresentation.SwapEffect						= D3DSWAPEFFECT_DISCARD;
	d3dPresentation.hDeviceWindow					= gD3DWindow;
	d3dPresentation.Windowed						= !gFullscreen;
	d3dPresentation.EnableAutoDepthStencil			= TRUE;
	d3dPresentation.AutoDepthStencilFormat			= D3DFMT_D16;
	d3dPresentation.Flags							= 0;
	d3dPresentation.FullScreen_RefreshRateInHz		= gFullscreen ? D3DPRESENT_RATE_DEFAULT : 0;
	d3dPresentation.FullScreen_PresentationInterval = gFullscreen ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_DEFAULT;

	// create the D3D device
	result = IDirect3D8_CreateDevice(gD3D, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, gD3DWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dPresentation, &gD3DDevice);
	if (result != D3D_OK)
		FatalError("Error creating Direct3D device (%08X)", result);
}


//--------------------------------------------------
//	Tear down D3D
//--------------------------------------------------

void TearDownDirect3D(void)
{
	// kill the device
	if (gD3DDevice != NULL)
		IDirect3DDevice8_Release(gD3DDevice);
	gD3DDevice = NULL;
	
	// kill the window
	if (gD3DWindow != NULL)
		DestroyWindow(gD3DWindow);
	gD3DWindow = NULL;

	// kill the D3D object
	if (gD3D != NULL)
		IDirect3D8_Release(gD3D);
	gD3D = NULL;
}


//--------------------------------------------------
//	Create the Direct3D window
//--------------------------------------------------

HWND InitDirect3DWindow(void)
{
	static ATOM wndAtom = 0;
	DWORD style, styleEx;
	int xoffs, yoffs;
	HWND window;
	RECT bounds;
	
	// create the window class if we need to
	if (wndAtom == 0)
	{
		WNDCLASS wndclass;

		// create the window class
		memset(&wndclass, 0, sizeof(wndclass));
		wndclass.style = WS_OVERLAPPED;
		wndclass.lpfnWndProc = D3DWndProc;
		wndclass.lpszClassName = GAME_NAME;
		wndAtom = RegisterClass(&wndclass);
		if (wndAtom == 0)
			FatalError("Error registering window class (%08X)", GetLastError());
	}
	
	// pick our styles
	style = gFullscreen ? FULLSCREEN_STYLE : WINDOW_STYLE;
	styleEx = gFullscreen ? FULLSCREEN_STYLE_EX : WINDOW_STYLE_EX;

	// determine the window's bounds
	memset(&bounds, 0, sizeof(bounds));
	if (gFullscreen)
	{
		bounds.right = GetSystemMetrics(SM_CXSCREEN);
		bounds.bottom = GetSystemMetrics(SM_CYSCREEN);
	}
	else
	{
		bounds.right = gGameWidth;
		bounds.bottom = gGameHeight;
		AdjustWindowRectEx(&bounds, style, FALSE, styleEx);
	}
	
	// center the window
	xoffs = (GetSystemMetrics(SM_CXSCREEN) - (bounds.right - bounds.left)) / 2;
	yoffs = (GetSystemMetrics(SM_CYSCREEN) - (bounds.bottom - bounds.top)) / 2;
	OffsetRect(&bounds, xoffs - bounds.left, yoffs - bounds.top);

	// create the window
	window = CreateWindowEx(styleEx, (LPCSTR)wndAtom, GAME_NAME, style, bounds.left, bounds.top, 
		bounds.right - bounds.left, bounds.bottom - bounds.top, NULL, NULL, NULL, NULL);
	if (window == 0)
		FatalError("Error creating window (%08X)", GetLastError());

	// show the window
	ShowWindow(window, SW_SHOW);
	SetFocus(window);
	return window;
}


//--------------------------------------------------
//	Window proc for the D3D window
//--------------------------------------------------

LRESULT CALLBACK D3DWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_PAINT:
			{
				PAINTSTRUCT paint;
				HDC dc = BeginPaint(hwnd, &paint);
				RECT client;
				GetClientRect(hwnd, &client);
				FillRect(dc, &client, (HBRUSH)GetStockObject(BLACK_BRUSH));
			}
			break;
		
		case WM_DESTROY:
			PostQuitMessage(0);
			break;

		case WM_SETCURSOR:
			SetCursor(gIsPaused ? LoadCursor(NULL, IDC_ARROW) : NULL);
			return TRUE;

		default:
			return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
	return 0;
}


//--------------------------------------------------
//	DirectSound initialization
//--------------------------------------------------

void InitDirectSound(void)
{
	HRESULT result;

	// now attempt to create it
	result = DirectSoundCreate(NULL, &gDSound, NULL);
	if (result != DS_OK)
	{
		WarningMessage("Error creating DirectSound (%08X)\n", result);
		return;
	}

	// set the cooperative level
	result = IDirectSound_SetCooperativeLevel(gDSound, gD3DWindow, DSSCL_PRIORITY);
	if (result != DS_OK)
	{
		WarningMessage("Error setting DirectSound cooperative level (%08X)\n", result);
		IDirectSound_Release(gDSound);
		gDSound = NULL;
		return;
	}

	// make a format description for what we want
	gDSoundFormat.wBitsPerSample	= 16;
	gDSoundFormat.wFormatTag		= WAVE_FORMAT_PCM;
	gDSoundFormat.nChannels			= 2;
	gDSoundFormat.nSamplesPerSec	= GAME_SAMPLE_RATE;
	gDSoundFormat.nBlockAlign		= gDSoundFormat.wBitsPerSample * gDSoundFormat.nChannels / 8;
	gDSoundFormat.nAvgBytesPerSec	= gDSoundFormat.nSamplesPerSec * gDSoundFormat.nBlockAlign;

	// compute the buffer sizes
	gDSoundBufferSize = (GAME_SAMPLE_RATE / 5) * gDSoundFormat.nBlockAlign;
	gDSoundBufferSize = (gDSoundBufferSize / 1024) * 1024;

	// create the buffers
	InitDirectSoundBuffers();

	// create the notifier object
	if (gDSound && gDSoundStreamBuf)
	{
		result = IDirectSoundBuffer_QueryInterface(gDSoundStreamBuf, &IID_IDirectSoundNotify, &gDSoundNotifier);
		if (result != DS_OK)
		{
			WarningMessage("Error getting DirectSound notifier interface (%08X)\n", result);
			IDirectSound_Release(gDSound);
			gDSound = NULL;
			return;
		}

		// create the events
		gDSoundNotification[0].hEventNotify = CreateEvent(NULL, FALSE, FALSE, NULL);
		gDSoundNotification[1].hEventNotify = CreateEvent(NULL, FALSE, FALSE, NULL);
		if (!gDSoundNotification[0].hEventNotify || !gDSoundNotification[1].hEventNotify)
			FatalError("Can't create events for DirectSound notification!");
		
		// set the offsets
		gDSoundNotification[0].dwOffset = 0;
		gDSoundNotification[1].dwOffset = gDSoundBufferSize / 2;
 		result = IDirectSoundNotify_SetNotificationPositions(gDSoundNotifier, 2, gDSoundNotification);
		if (result != DS_OK)
			FatalError("Can't set the notifications for DirectSound! (%08X)", result);
	}

	// start playing
	if (gDSound && gDSoundStreamBuf)
	{
		result = IDirectSoundBuffer_Play(gDSoundStreamBuf, 0, 0, DSBPLAY_LOOPING);
		if (result != DS_OK)
		{
			WarningMessage("Error starting DirectSound playback (%08X)\n", result);
			IDirectSound_Release(gDSound);
			gDSound = NULL;
			return;
		}
	}
}


//--------------------------------------------------
//	DirectSound buffer creation
//--------------------------------------------------

void InitDirectSoundBuffers(void)
{
	DSBUFFERDESC primaryBufDesc;
	WAVEFORMATEX primaryBufFormat;
	DSBUFFERDESC streamBufDesc;
	HRESULT result;
	void *buffer;
	DWORD locked;

	// create a buffer desc for the primary buffer
	memset(&primaryBufDesc, 0, sizeof(primaryBufDesc));
	primaryBufDesc.dwSize = sizeof(primaryBufDesc);
	primaryBufDesc.dwFlags = DSBCAPS_PRIMARYBUFFER | DSBCAPS_GETCURRENTPOSITION2;
	primaryBufDesc.lpwfxFormat = NULL;

	// create the primary buffer
	result = IDirectSound_CreateSoundBuffer(gDSound, &primaryBufDesc, &gDSoundPrimaryBuf, NULL);
	if (result != DS_OK)
	{
		WarningMessage("Error creating primary buffer (%08X)\n", result);
		goto error;
	}

	// attempt to set the primary format
	result = IDirectSoundBuffer_SetFormat(gDSoundPrimaryBuf, &gDSoundFormat);
	if (result != DS_OK)
	{
		WarningMessage("Error setting primary format (%08X)\n", result);
		goto error;
	}

	// get the primary format
	result = IDirectSoundBuffer_GetFormat(gDSoundPrimaryBuf, &primaryBufFormat, sizeof(primaryBufFormat), NULL);
	if (result != DS_OK)
	{
		WarningMessage("Error getting primary format (%08X)\n", result);
		goto error;
	}
	Information("Primary buffer: %d Hz, %d bits, %d channels\n",
			(int)primaryBufFormat.nSamplesPerSec, (int)primaryBufFormat.wBitsPerSample, (int)primaryBufFormat.nChannels);

	// create a buffer desc for the stream buffer
	memset(&streamBufDesc, 0, sizeof(streamBufDesc));
	streamBufDesc.dwSize = sizeof(streamBufDesc);
	streamBufDesc.dwFlags = DSBCAPS_CTRLVOLUME | DSBCAPS_GLOBALFOCUS | DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_CTRLPOSITIONNOTIFY;
	streamBufDesc.dwBufferBytes = gDSoundBufferSize;
	streamBufDesc.lpwfxFormat = &gDSoundFormat;

	// create the stream buffer
	result = IDirectSound_CreateSoundBuffer(gDSound, &streamBufDesc, &gDSoundStreamBuf, NULL);
	if (result != DS_OK)
	{
		WarningMessage("Error creating DirectSound buffer (%08X)\n", result);
		goto error;
	}

	// lock the buffer
	result = IDirectSoundBuffer_Lock(gDSoundStreamBuf, 0, gDSoundBufferSize, &buffer, &locked, NULL, NULL, 0);
	if (result != DS_OK)
	{
		WarningMessage("Error locking stream buffer (%08X)\n", result);
		goto error;
	}

	// clear the buffer and unlock it
	memset(buffer, 0, locked);
	IDirectSoundBuffer_Unlock(gDSoundStreamBuf, buffer, locked, NULL, 0);
	return;

	// error handling
error:
	IDirectSound_Release(gDSound);
	gDSound = NULL;
}


//--------------------------------------------------
//	Do we have a sound buffer ready?
//--------------------------------------------------

UINT32 SoundBufferReady(void)
{
	if (gDSound && gDSoundStreamBuf)
	{
		HANDLE handles[2];
		DWORD result;

		// see if either handle is ready
		handles[0] = gDSoundNotification[0].hEventNotify;
		handles[1] = gDSoundNotification[1].hEventNotify;
		result = WaitForMultipleObjects(2, handles, FALSE, 0);
		if (result == WAIT_TIMEOUT)
			return 0;
		
		// we got one; see which one
		gCurrentSoundBuffer = (result == WAIT_OBJECT_0) ? 0 : 1;
		return gDSoundBufferSize / 2 / sizeof(INT16);
	}
	return 0;
}


//--------------------------------------------------
//	Write data to the given sound buffer
//--------------------------------------------------

void WriteToSoundBuffer(INT16 *data)
{
	if (gDSound && gDSoundStreamBuf)
	{
		HRESULT result;
		void *buffer;
		DWORD locked;

		// lock the buffer
		result = IDirectSoundBuffer_Lock(gDSoundStreamBuf, gCurrentSoundBuffer ? 0 : gDSoundBufferSize / 2, gDSoundBufferSize / 2, &buffer, &locked, NULL, NULL, 0);
		if (result == DS_OK && locked == gDSoundBufferSize / 2)
		{
			memcpy(buffer, data, gDSoundBufferSize / 2);
			IDirectSoundBuffer_Unlock(gDSoundStreamBuf, buffer, locked, NULL, 0);
		}
	}
}


//--------------------------------------------------
//	CPU helper
//--------------------------------------------------

void InitCPU(int cpunum, void (*getinfo)(UINT32, union cpuinfo *), CPUData *data)
{
	union cpuinfo info;
	
	// first fill in the data pointers
	(*getinfo)(CPUINFO_PTR_RESET, &info);
	data->reset = info.reset;

	(*getinfo)(CPUINFO_PTR_EXECUTE, &info);
	data->execute = info.execute;

	data->getinfo = getinfo;

	(*getinfo)(CPUINFO_PTR_SET_INFO, &info);
	data->setinfo = info.setinfo;

	(*getinfo)(CPUINFO_PTR_GET_CONTEXT, &info);
	data->getcontext = info.getcontext;

	(*getinfo)(CPUINFO_PTR_SET_CONTEXT, &info);
	data->setcontext = info.setcontext;

	(*getinfo)(CPUINFO_PTR_INSTRUCTION_COUNTER, &info);
	data->icount = info.icount;
	
	// now initialize the CPU
	(*getinfo)(CPUINFO_PTR_INIT, &info);
	if (info.init)
		(*info.init)();
}


//--------------------------------------------------
//	Set info wrapper
//--------------------------------------------------

void SetCPUInt(const CPUData *data, int selector, int value)
{
	union cpuinfo info;
	info.i = value;
	(*data->setinfo)(selector, &info);
}


//--------------------------------------------------
//	CPU executer
//--------------------------------------------------

int ExecuteCPU(int cycles, const CPUData *data)
{
	int result;
	
	gFudgedCycles = 0;
	gExecutingCPU = data;
	result = (*data->execute)(cycles);
	return result - gFudgedCycles;
}


//--------------------------------------------------
//	CPU execution abort
//--------------------------------------------------

void AbortExecuteCPU(void)
{
	if (gExecutingCPU != NULL)
	{
		gFudgedCycles += *gExecutingCPU->icount + 1;
		*gExecutingCPU->icount = -1;
	}
}


//--------------------------------------------------
//	Error reporting
//--------------------------------------------------

void Information(const char *string, ...)
{
#ifdef DEBUG
	va_list arg;

	// expand the message string
	va_start(arg, string);
	vprintf(string, arg);
	va_end(arg);
#endif
}


void WarningMessage(const char *string, ...)
{
	char textBuffer[1024];
	va_list arg;

	// expand the message string
	va_start(arg, string);
	vsprintf(textBuffer, string, arg);
	va_end(arg);
	
	// message box
	MessageBox(NULL, textBuffer, "Warning", MB_OK | MB_ICONWARNING | MB_SETFOREGROUND | MB_TOPMOST);
}


void FatalError(const char *string, ...)
{
	char textBuffer[1024];
	va_list arg;
	
	// tear down D3D first
//	TearDownDirect3D();

	// expand the message string
	va_start(arg, string);
	vsprintf(textBuffer, string, arg);
	va_end(arg);
	
	// message box and exit
	MessageBox(NULL, textBuffer, "Fatal Error", MB_OK | MB_ICONERROR | MB_SETFOREGROUND | MB_TOPMOST);
	TerminateProcess(GetCurrentProcess(), -1);
}
