#define GAME_WIDTH				576
#define GAME_HEIGHT				432
#define GAME_FPS				60
#define GAME_SAMPLE_RATE		20833
#define GAME_NAME				"Speed Up"
#define GAME_FILENAME			"speedup"
#define GAME_SAVED_DATA_VERSION	1

typedef struct
{
	UINT16	eeprom[256];
} GameSavedData;

extern UINT8 *rom68000[2];
extern UINT8 *romADSP2115;
extern UINT8 *romGeometry[2];
extern UINT8 *romTexture[8];

#define GAME_ROM_LIST \
	{ &rom68000[0],    0x07e70bae, 0x080000 },	/* 68000 -- sup10.bin */		\
	{ &rom68000[1],    0x7947c28d, 0x080000 },	/* 68000 -- sup15.bin */		\
	{ &romADSP2115,    0x284c7cd1, 0x400000 },	/* ADSP2115 -- sup25.bin */		\
	{ &romGeometry[0], 0xaed151de, 0x200000 },	/* geometry -- sup32.bin */		\
	{ &romGeometry[1], 0x9be6ab7d, 0x200000 },	/* geometry -- sup33.bin */		\
	{ &romTexture[0],  0x311f3247, 0x400000 },	/* texture -- sup12.bin */		\
	{ &romTexture[1],  0x3ad3c089, 0x400000 },	/* texture -- sup14.bin */		\
	{ &romTexture[2],  0xb993e65a, 0x400000 },	/* texture -- sup11.bin */		\
	{ &romTexture[3],  0xad00023c, 0x400000 },	/* texture -- sup13.bin */		\
	{ &romTexture[4],  0x34737d1d, 0x020000 },	/* texture -- ic35.bin */		\
	{ &romTexture[5],  0xe89e829b, 0x020000 },	/* texture -- ic34.bin */		\

