#define GAME_WIDTH				576
#define GAME_HEIGHT				432
#define GAME_FPS				60
#define GAME_SAMPLE_RATE		20833
#define GAME_NAME				"Radikal Bikers"
#define GAME_FILENAME			"radikalb"
#define GAME_SAVED_DATA_VERSION	1

typedef struct
{
	UINT16	eeprom[256];
} GameSavedData;

extern UINT8 *rom68020[4];
extern UINT8 *romADSP2115;
extern UINT8 *romGeometry[2];
extern UINT8 *romTexture[12];

#define GAME_ROM_LIST \
	{ &rom68020[0],    0xccac98c5, 0x080000 },	/* 68020 -- rab.6 */		\
	{ &rom68020[1],    0x26199506, 0x080000 },	/* 68020 -- rab.12 */		\
	{ &rom68020[2],    0x4a0ac8cb, 0x080000 },	/* 68020 -- rab.14 */		\
	{ &rom68020[3],    0x2631bd61, 0x080000 },	/* 68020 -- rab.19 */		\
	{ &romADSP2115,    0xdcf52520, 0x400000 },	/* ADSP2115 -- rab.23 */	\
	{ &romGeometry[0], 0x9c56a06a, 0x400000 },	/* geometry -- rab.48 */	\
	{ &romGeometry[1], 0x7e698584, 0x400000 },	/* geometry -- rab.45 */	\
	{ &romTexture[0],  0x4fbd4737, 0x400000 },	/* texture -- rab.8 */		\
	{ &romTexture[1],  0x870b0ce4, 0x400000 },	/* texture -- rab.10 */		\
	{ &romTexture[2],  0xedb9d409, 0x400000 },	/* texture -- rab.15 */		\
	{ &romTexture[3],  0xe120236b, 0x400000 },	/* texture -- rab.17 */		\
	{ &romTexture[4],  0x9e3e038d, 0x400000 },	/* texture -- rab.9 */		\
	{ &romTexture[5],  0x75672271, 0x400000 },	/* texture -- rab.11 */		\
	{ &romTexture[6],  0x9d595e46, 0x400000 },	/* texture -- rab.16 */		\
	{ &romTexture[7],  0x3084bc49, 0x400000 },	/* texture -- rab.18 */		\
	{ &romTexture[8],  0x2984bc1d, 0x020000 },	/* texture -- rab.24 */		\
	{ &romTexture[9],  0x777758e3, 0x020000 },	/* texture -- rab.25 */		\
	{ &romTexture[10], 0xbd9c1b54, 0x020000 },	/* texture -- rab.26 */		\
	{ &romTexture[11], 0xbbcf6977, 0x020000 },	/* texture -- rab.27 */		\

