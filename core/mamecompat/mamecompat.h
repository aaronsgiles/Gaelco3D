//===================================================================
//
//	MAME compatibility header file for standalone emulator shell
//
//	Copyright (c) 2004, Aaron Giles
//
//===================================================================

#ifndef _MAMECOMPAT_
#define _MAMECOMPAT_


//--------------------------------------------------
//	Ensure CPU definitions are set
//--------------------------------------------------

#ifndef HAS_M68000
#define HAS_M68000 0
#endif

#ifndef HAS_M68008
#define HAS_M68008 0
#endif

#ifndef HAS_M68010
#define HAS_M68010 0
#endif

#ifndef HAS_M68EC020
#define HAS_M68EC020 0
#endif

#ifndef HAS_M68020
#define HAS_M68020 0
#endif

#ifndef HAS_TMS32031
#define HAS_TMS32031 0
#endif

#ifndef HAS_ADSP2100
#define HAS_ADSP2100 0
#endif

#ifndef HAS_ADSP2101
#define HAS_ADSP2101 0
#endif

#ifndef HAS_ADSP2104
#define HAS_ADSP2104 0
#endif

#ifndef HAS_ADSP2105
#define HAS_ADSP2105 0
#endif

#ifndef HAS_ADSP2115
#define HAS_ADSP2115 0
#endif


//--------------------------------------------------
//	Core macros
//--------------------------------------------------

#define LSB_FIRST
#define INLINE			static __inline


//--------------------------------------------------
//	Core types
//--------------------------------------------------

typedef unsigned __int8 UINT8;
typedef unsigned __int16 UINT16;
typedef unsigned __int32 UINT32;
typedef unsigned __int64 UINT64;
typedef signed __int8 INT8;
typedef signed __int16 INT16;
typedef signed __int32 INT32;
typedef signed __int64 INT64;




#ifndef MEMORY_ACCESSOR
#define MEMORY_ACCESSOR(x) default_##x
#endif

#define MAX_CPU 		8

#define CALL_MAME_DEBUG	do { } while (0)

/* memory stuff */

typedef UINT8			data8_t;
typedef UINT16			data16_t;
typedef UINT32			data32_t;
typedef UINT64			data64_t;
typedef UINT32			offs_t;

#define program_read_byte_8			MEMORY_ACCESSOR(prb8)
#define program_write_byte_8		MEMORY_ACCESSOR(pwb8)
#define program_read_byte_16be		MEMORY_ACCESSOR(prb16b)
#define program_write_byte_16be		MEMORY_ACCESSOR(pwb16b)
#define program_read_word_16be		MEMORY_ACCESSOR(prw16b)
#define program_write_word_16be		MEMORY_ACCESSOR(pww16b)

#define program_read_byte_32be		MEMORY_ACCESSOR(prb32b)
#define program_write_byte_32be		MEMORY_ACCESSOR(pwb32b)
#define program_read_word_32be		MEMORY_ACCESSOR(prw32b)
#define program_write_word_32be		MEMORY_ACCESSOR(pww32b)
#define program_read_dword_32be		MEMORY_ACCESSOR(prd32b)
#define program_write_dword_32be	MEMORY_ACCESSOR(pwd32b)

#define program_read_byte_32le		MEMORY_ACCESSOR(prb32l)
#define program_write_byte_32le		MEMORY_ACCESSOR(pwb32l)
#define program_read_word_32le		MEMORY_ACCESSOR(prw32l)
#define program_write_word_32le		MEMORY_ACCESSOR(pww32l)
#define program_read_dword_32le		MEMORY_ACCESSOR(prd32l)
#define program_write_dword_32le	MEMORY_ACCESSOR(pwd32l)

#define data_read_word_16le			MEMORY_ACCESSOR(drw16l)
#define data_write_word_16le		MEMORY_ACCESSOR(dww16l)

#define io_read_word_16le			MEMORY_ACCESSOR(irw16l)
#define io_write_word_16le			MEMORY_ACCESSOR(iww16l)

#define change_pc					MEMORY_ACCESSOR(change_pc)

#define cpu_readop8					MEMORY_ACCESSOR(readop8)
#define cpu_readop16				MEMORY_ACCESSOR(readop16)
#define cpu_readop32				MEMORY_ACCESSOR(readop32)

#define BYTE_XOR_BE(a)  	((a) ^ 1)				/* read/write a byte to a 16-bit space */
#define BYTE_XOR_LE(a)  	(a)
#define BYTE4_XOR_BE(a) 	((a) ^ 3)				/* read/write a byte to a 32-bit space */
#define BYTE4_XOR_LE(a) 	(a)
#define WORD_XOR_BE(a)  	((a) ^ 2)				/* read/write a word to a 32-bit space */
#define WORD_XOR_LE(a)  	(a)
#define BYTE8_XOR_BE(a) 	((a) ^ 7)				/* read/write a byte to a 64-bit space */
#define BYTE8_XOR_LE(a) 	(a)
#define WORD2_XOR_BE(a)  	((a) ^ 6)				/* read/write a word to a 64-bit space */
#define WORD2_XOR_LE(a)  	(a)
#define DWORD_XOR_BE(a)  	((a) ^ 4)				/* read/write a dword to a 64-bit space */
#define DWORD_XOR_LE(a)  	(a)

#define ADDRESS_SPACES			3						/* maximum number of address spaces */
#define ADDRESS_SPACE_PROGRAM	0						/* program address space */
#define ADDRESS_SPACE_DATA		1						/* data address space */
#define ADDRESS_SPACE_IO		2						/* I/O address space */

typedef struct address_map_t *(*construct_map_t)(struct address_map_t *map);


/* cpuintrf stuff */

enum
{
	/* line states */
	CLEAR_LINE = 0,				/* clear (a fired, held or pulsed) line */
	ASSERT_LINE,				/* assert an interrupt immediately */
	HOLD_LINE,					/* hold interrupt line until acknowledged */
	PULSE_LINE,					/* pulse interrupt line for one instruction */

	/* input lines */
	MAX_INPUT_LINES = 32+3,
	INPUT_LINE_IRQ0 = 0,
	INPUT_LINE_IRQ1 = 1,
	INPUT_LINE_IRQ2 = 2,
	INPUT_LINE_IRQ3 = 3,
	INPUT_LINE_IRQ4 = 4,
	INPUT_LINE_IRQ5 = 5,
	INPUT_LINE_IRQ6 = 6,
	INPUT_LINE_IRQ7 = 7,
	INPUT_LINE_IRQ8 = 8,
	INPUT_LINE_IRQ9 = 9,
	INPUT_LINE_NMI = MAX_INPUT_LINES - 3,
	
	/* special input lines that are implemented in the core */
	INPUT_LINE_RESET = MAX_INPUT_LINES - 2,
	INPUT_LINE_HALT = MAX_INPUT_LINES - 1,
	
	/* output lines */
	MAX_OUTPUT_LINES = 32
};


enum
{
	MAX_REGS = 256				/* maximum number of register of any CPU */
};


enum
{
	/* --- the following bits of info are returned as 64-bit signed integers --- */
	CPUINFO_INT_FIRST = 0x00000,

	CPUINFO_INT_CONTEXT_SIZE = CPUINFO_INT_FIRST,		/* R/O: size of CPU context in bytes */
	CPUINFO_INT_INPUT_LINES,							/* R/O: number of input lines */
	CPUINFO_INT_OUTPUT_LINES,							/* R/O: number of output lines */
	CPUINFO_INT_DEFAULT_IRQ_VECTOR,						/* R/O: default IRQ vector */
	CPUINFO_INT_ENDIANNESS,								/* R/O: either CPU_IS_BE or CPU_IS_LE */
	CPUINFO_INT_CLOCK_DIVIDER,							/* R/O: internal clock divider */
	CPUINFO_INT_MIN_INSTRUCTION_BYTES,					/* R/O: minimum bytes per instruction */
	CPUINFO_INT_MAX_INSTRUCTION_BYTES,					/* R/O: maximum bytes per instruction */
	CPUINFO_INT_MIN_CYCLES,								/* R/O: minimum cycles for a single instruction */
	CPUINFO_INT_MAX_CYCLES,								/* R/O: maximum cycles for a single instruction */

	CPUINFO_INT_DATABUS_WIDTH,							/* R/O: data bus size for each address space (8,16,32,64) */
	CPUINFO_INT_DATABUS_WIDTH_LAST = CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACES - 1,
	CPUINFO_INT_ADDRBUS_WIDTH,							/* R/O: address bus size for each address space (12-32) */
	CPUINFO_INT_ADDRBUS_WIDTH_LAST = CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACES - 1,
	CPUINFO_INT_ADDRBUS_SHIFT,							/* R/O: shift applied to addresses each address space (+3 means >>3, -1 means <<1) */
	CPUINFO_INT_ADDRBUS_SHIFT_LAST = CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACES - 1,

	CPUINFO_INT_SP,										/* R/W: the current stack pointer value */
	CPUINFO_INT_PC,										/* R/W: the current PC value */
	CPUINFO_INT_PREVIOUSPC,								/* R/W: the previous PC value */
	CPUINFO_INT_INPUT_STATE,							/* R/W: states for each input line */
	CPUINFO_INT_INPUT_STATE_LAST = CPUINFO_INT_INPUT_STATE + MAX_INPUT_LINES - 1,
	CPUINFO_INT_OUTPUT_STATE,							/* R/W: states for each output line */
	CPUINFO_INT_OUTPUT_STATE_LAST = CPUINFO_INT_OUTPUT_STATE + MAX_OUTPUT_LINES - 1,
	CPUINFO_INT_REGISTER,								/* R/W: values of up to MAX_REGs registers */
	CPUINFO_INT_REGISTER_LAST = CPUINFO_INT_REGISTER + MAX_REGS - 1,

	CPUINFO_INT_CPU_SPECIFIC = 0x08000,					/* R/W: CPU-specific values start here */

	/* --- the following bits of info are returned as pointers to data or functions --- */
	CPUINFO_PTR_FIRST = 0x10000,

	CPUINFO_PTR_SET_INFO = CPUINFO_PTR_FIRST,			/* R/O: void (*set_info)(UINT32 state, INT64 data, void *ptr) */
	CPUINFO_PTR_GET_CONTEXT,							/* R/O: void (*get_context)(void *buffer) */
	CPUINFO_PTR_SET_CONTEXT,							/* R/O: void (*set_context)(void *buffer) */
	CPUINFO_PTR_INIT,									/* R/O: void (*init)(void) */
	CPUINFO_PTR_RESET,									/* R/O: void (*reset)(void *param) */
	CPUINFO_PTR_EXIT,									/* R/O: void (*exit)(void) */
	CPUINFO_PTR_EXECUTE,								/* R/O: int (*execute)(int cycles) */
	CPUINFO_PTR_BURN,									/* R/O: void (*burn)(int cycles) */
	CPUINFO_PTR_DISASSEMBLE,							/* R/O: void (*disassemble)(char *buffer, offs_t pc) */
	CPUINFO_PTR_IRQ_CALLBACK,							/* R/W: int (*irqcallback)(int state) */
	CPUINFO_PTR_INSTRUCTION_COUNTER,					/* R/O: int *icount */
	CPUINFO_PTR_REGISTER_LAYOUT,						/* R/O: struct debug_register_layout *layout */
	CPUINFO_PTR_WINDOW_LAYOUT,							/* R/O: struct debug_window_layout *layout */
	CPUINFO_PTR_INTERNAL_MEMORY_MAP,					/* R/O: construct_map_t map */
	CPUINFO_PTR_INTERNAL_MEMORY_MAP_LAST = CPUINFO_PTR_INTERNAL_MEMORY_MAP + ADDRESS_SPACES - 1,

	CPUINFO_PTR_CPU_SPECIFIC = 0x18000,					/* R/W: CPU-specific values start here */

	/* --- the following bits of info are returned as NULL-terminated strings --- */
	CPUINFO_STR_FIRST = 0x20000,

	CPUINFO_STR_NAME = CPUINFO_STR_FIRST,				/* R/O: name of the CPU */
	CPUINFO_STR_CORE_FAMILY,							/* R/O: family of the CPU */
	CPUINFO_STR_CORE_VERSION,							/* R/O: version of the CPU core */
	CPUINFO_STR_CORE_FILE,								/* R/O: file containing the CPU core */
	CPUINFO_STR_CORE_CREDITS,							/* R/O: credits for the CPU core */
	CPUINFO_STR_FLAGS,									/* R/O: string representation of the main flags value */
	CPUINFO_STR_REGISTER,								/* R/O: string representation of up to MAX_REGs registers */
	CPUINFO_STR_REGISTER_LAST = CPUINFO_STR_REGISTER + MAX_REGS - 1,

	CPUINFO_STR_CPU_SPECIFIC = 0x28000					/* R/W: CPU-specific values start here */
};


union cpuinfo
{
	INT64	i;											/* generic integers */
	void *	p;											/* generic pointers */
	char *	s;											/* generic strings */

	void	(*setinfo)(UINT32 state, union cpuinfo *info);/* CPUINFO_PTR_SET_INFO */
	void	(*getcontext)(void *context);				/* CPUINFO_PTR_GET_CONTEXT */
	void	(*setcontext)(void *context);				/* CPUINFO_PTR_SET_CONTEXT */
	void	(*init)(void);								/* CPUINFO_PTR_INIT */
	void	(*reset)(void *param);						/* CPUINFO_PTR_RESET */
	void	(*exit)(void);								/* CPUINFO_PTR_EXIT */
	int		(*execute)(int cycles);						/* CPUINFO_PTR_EXECUTE */
	void	(*burn)(int cycles);						/* CPUINFO_PTR_BURN */
	offs_t	(*disassemble)(char *buffer, offs_t pc);	/* CPUINFO_PTR_DISASSEMBLE */
	int		(*irqcallback)(int state);					/* CPUINFO_PTR_IRQCALLBACK */
	int *	icount;										/* CPUINFO_PTR_INSTRUCTION_COUNTER */
	construct_map_t internal_map;						/* CPUINFO_PTR_INTERNAL_MEMORY_MAP */
};


/* get_reg/set_reg constants */
enum
{
	/* This value is passed to activecpu_get_reg to retrieve the previous
	 * program counter value, ie. before a CPU emulation started
	 * to fetch opcodes and arguments for the current instrution. */
	REG_PREVIOUSPC = CPUINFO_INT_PREVIOUSPC - CPUINFO_INT_REGISTER,

	/* This value is passed to activecpu_get_reg to retrieve the current
	 * program counter value. */
	REG_PC = CPUINFO_INT_PC - CPUINFO_INT_REGISTER,

	/* This value is passed to activecpu_get_reg to retrieve the current
	 * stack pointer value. */
	REG_SP = CPUINFO_INT_SP - CPUINFO_INT_REGISTER
};


/* endianness constants */
enum
{
	CPU_IS_LE = 0,				/* emulated CPU is little endian */
	CPU_IS_BE					/* emulated CPU is big endian */
};

char *cpuintrf_temp_str(void);

INLINE int cpu_getactivecpu(void)
{
	extern int gCurrentCPU;
	return gCurrentCPU;
}


/* state stuff */

#define state_save_register_func_postload (void)
#define state_save_register_func_presave (void)
#define state_save_register_int (void)
#define state_save_register_UINT16 (void)
#define state_save_register_UINT32 (void)



void logerror(const char *fmt, ...);

#endif