#define GAME_WIDTH				576
#define GAME_HEIGHT				432
#define GAME_FPS				60
#define GAME_SAMPLE_RATE		20833
#define GAME_NAME				"Surf Planet"
#define GAME_FILENAME			"surfplnt"
#define GAME_SAVED_DATA_VERSION	1

typedef struct
{
	UINT16	eeprom[256];
} GameSavedData;

extern UINT8 *rom68000[4];
extern UINT8 *romADSP2115;
extern UINT8 *romGeometry[2];
extern UINT8 *romTexture[8];

#define GAME_ROM_LIST \
	{ &rom68000[0],    0xc96e0a18, 0x080000 },	/* 68000 -- surfplnt.u5 */		\
	{ &rom68000[1],    0x99211d2d, 0x080000 },	/* 68000 -- surfplnt.u11 */		\
	{ &rom68000[2],    0xaef9e1d0, 0x080000 },	/* 68000 -- surfplnt.u8 */		\
	{ &rom68000[3],    0xd9754369, 0x080000 },	/* 68000 -- surfplnt.u13 */		\
	{ &romADSP2115,    0xa1b64695, 0x400000 },	/* ADSP2115 -- pls.18 */		\
	{ &romGeometry[0], 0x26877ad3, 0x400000 },	/* geometry -- pls.40 */		\
	{ &romGeometry[1], 0x75893062, 0x400000 },	/* geometry -- pls.37 */		\
	{ &romTexture[0],  0x04bd1605, 0x400000 },	/* texture -- pls.7 */			\
	{ &romTexture[1],  0xf4400160, 0x400000 },	/* texture -- pls.9 */			\
	{ &romTexture[2],  0xedc2e826, 0x400000 },	/* texture -- pls.12 */			\
	{ &romTexture[3],  0xb0f6b8da, 0x400000 },	/* texture -- pls.15 */			\
	{ &romTexture[4],  0x691bd7a7, 0x020000 },	/* texture -- surfplnt.u19 */	\
	{ &romTexture[5],  0xfb293318, 0x020000 },	/* texture -- surfplnt.u20 */	\
	{ &romTexture[6],  0xb80611fb, 0x020000 },	/* texture -- surfplnt.u21 */	\
	{ &romTexture[7],  0xccf88f7e, 0x020000 },	/* texture -- surfplnt.u22 */	\

