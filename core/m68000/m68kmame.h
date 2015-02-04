#ifndef M68KMAME__HEADER
#define M68KMAME__HEADER

#include "mamecompat.h"
#include "m68000.h"

/* Configuration switches (see m68kconf.h for explanation) */
#if HAS_M68008
#define M68K_EMULATE_008            OPT_ON
#else
#define M68K_EMULATE_008            OPT_OFF
#endif

#if HAS_M68010
#define M68K_EMULATE_010            OPT_ON
#else
#define M68K_EMULATE_010            OPT_OFF
#endif

#if HAS_M68EC020
#define M68K_EMULATE_EC020          OPT_ON
#else
#define M68K_EMULATE_EC020          OPT_OFF
#endif

#if HAS_M68020
#define M68K_EMULATE_020            OPT_ON
#else
#define M68K_EMULATE_020            OPT_OFF
#endif

#define M68K_SEPARATE_READS         OPT_OFF

#define M68K_SIMULATE_PD_WRITES     OPT_OFF

#define M68K_EMULATE_INT_ACK        OPT_OFF
#define M68K_INT_ACK_CALLBACK(A)

#define M68K_EMULATE_BKPT_ACK       OPT_OFF
#define M68K_BKPT_ACK_CALLBACK()

#define M68K_EMULATE_TRACE          OPT_OFF

#define M68K_EMULATE_RESET          OPT_OFF
#define M68K_RESET_CALLBACK()

#define M68K_EMULATE_FC             OPT_OFF
#define M68K_SET_FC_CALLBACK(A)

#define M68K_MONITOR_PC             OPT_OFF
#define M68K_SET_PC_CALLBACK(A)

#define M68K_INSTRUCTION_HOOK       OPT_OFF
#define M68K_INSTRUCTION_CALLBACK()

#define M68K_EMULATE_PREFETCH       OPT_OFF

#define M68K_EMULATE_ADDRESS_ERROR  OPT_OFF

#define M68K_LOG_ENABLE             OPT_OFF
#define M68K_LOG_1010_1111          OPT_OFF
#define M68K_LOG_FILEHANDLE

#define M68K_USE_64_BIT             OPT_OFF

#if (HAS_M68020 || HAS_M68EC020)
#define m68k_read_memory_8			program_read_byte_32be
#define m68k_read_memory_16			program_read_word_32be
#define m68k_read_memory_32			program_read_dword_32be
#define m68k_write_memory_8			program_write_byte_32be
#define m68k_write_memory_16		program_write_word_32be
#define m68k_write_memory_32		program_write_dword_32be
#else
#define m68k_read_memory_8			program_read_byte_16be
#define m68k_read_memory_16			program_read_word_16be
#define m68k_read_memory_32			program_read_dword_16be
#define m68k_write_memory_8			program_write_byte_16be
#define m68k_write_memory_16		program_write_word_16be
#define m68k_write_memory_32		program_write_dword_16be
#endif

/* ======================================================================== */
/* ============================== END OF FILE ============================= */
/* ======================================================================== */

#endif /* M68KMAME__HEADER */
