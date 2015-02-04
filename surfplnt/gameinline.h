//===================================================================
//
//	Game-specific inlines for standalone emulator shell
//
//	Copyright (c) 2004, Aaron Giles
//
//===================================================================

#include "mamecompat.h"
#include <stdlib.h>
#pragma intrinsic(_byteswap_ushort)
#pragma intrinsic(_byteswap_ulong)

//--------------------------------------------------
//	68000/68EC020 definitions
//--------------------------------------------------

/*
	AM_RANGE(0x000000, 0x1fffff) AM_ROM
	AM_RANGE(0x400000, 0x40ffff) AM_READWRITE(MRA32_RAM, gaelco3d_paletteram_020_w) AM_BASE(&paletteram32)
	AM_RANGE(0x51000c, 0x51000f) AM_READ(input_port_0_020_r)
	AM_RANGE(0x51001c, 0x51001f) AM_READ(input_port_1_020_r)
	AM_RANGE(0x51002c, 0x51002f) AM_READ(analog_port_020_r)
	AM_RANGE(0x510040, 0x510043) AM_READ(unknown_043_r)
	AM_RANGE(0x510040, 0x510043) AM_WRITE(sound_data_020_w)
	AM_RANGE(0x510100, 0x510103) AM_READ(eeprom_data_020_r)
	AM_RANGE(0x510100, 0x510103) AM_WRITE(irq_ack_020_w)
	AM_RANGE(0x510104, 0x510107) AM_WRITE(unknown_107_w)
	AM_RANGE(0x510110, 0x510113) AM_WRITE(eeprom_data_020_w)
	AM_RANGE(0x510114, 0x510117) AM_WRITE(tms_control3_020_w)
	AM_RANGE(0x510118, 0x51011b) AM_WRITE(eeprom_clock_020_w)
	AM_RANGE(0x510120, 0x510123) AM_WRITE(eeprom_cs_020_w)
	AM_RANGE(0x510124, 0x510127) AM_WRITE(unknown_127_w)
	AM_RANGE(0x510128, 0x51012b) AM_WRITE(tms_reset_020_w)
	AM_RANGE(0x510130, 0x510133) AM_WRITE(tms_irq_020_w)
	AM_RANGE(0x510134, 0x510137) AM_WRITE(unknown_137_w)
	AM_RANGE(0x510138, 0x51013b) AM_WRITE(unknown_13a_w)
	AM_RANGE(0x510144, 0x510147) AM_WRITE(led_0_020_w)
	AM_RANGE(0x510154, 0x510157) AM_WRITE(analog_port_clock_020_w)
	AM_RANGE(0x510164, 0x510167) AM_WRITE(analog_port_latch_020_w)
	AM_RANGE(0x510174, 0x510177) AM_WRITE(led_1_020_w)
	AM_RANGE(0xfe7f80, 0xfe7fff) AM_WRITE(tms_comm_020_w) AM_BASE((UINT32 **)&tms_comm_base)
	AM_RANGE(0xfe0000, 0xfeffff) AM_RAM AM_BASE((data32_t **)&m68k_ram_base)
*/

extern UINT8 g68000MemoryBase[];
void Write68000(UINT32 address, UINT32 data, int size);

static __forceinline UINT8 m68000_prb16b(UINT32 address)
{
	return g68000MemoryBase[address];
}

static __forceinline UINT16 m68000_prw16b(UINT32 address)
{
	return _byteswap_ushort(*(UINT16 *)&g68000MemoryBase[address]);
}

static __forceinline UINT8 m68000_prb32b(UINT32 address)
{
	return g68000MemoryBase[address];
}

static __forceinline UINT16 m68000_prw32b(UINT32 address)
{
	return _byteswap_ushort(*(UINT16 *)&g68000MemoryBase[address]);
}

static __forceinline UINT32 m68000_prd32b(UINT32 address)
{
	return _byteswap_ulong(*(UINT32 *)&g68000MemoryBase[address]);
}

static __forceinline void m68000_pwb16b(UINT32 address, UINT8 data)
{
	if (address >= 0xfe0000)
		g68000MemoryBase[address] = data;
	else
		Write68000(address, data, 1);
}

static __forceinline void m68000_pww16b(UINT32 address, UINT16 data)
{
	if (address >= 0xfe0000)
		*(UINT16 *)&g68000MemoryBase[address] = _byteswap_ushort(data);
	else
		Write68000(address, data, 2);
}

static __forceinline void m68000_pwb32b(UINT32 address, UINT8 data)
{
	if (address >= 0xfe0000)
		g68000MemoryBase[address] = data;
	else
		Write68000(address, data, 1);
}

static __forceinline void m68000_pww32b(UINT32 address, UINT16 data)
{
	if (address >= 0xfe0000)
		*(UINT16 *)&g68000MemoryBase[address] = _byteswap_ushort(data);
	else
		Write68000(address, data, 2);
}

static __forceinline void m68000_pwd32b(UINT32 address, UINT32 data)
{
	if (address >= 0xfe0000)
		*(UINT32 *)&g68000MemoryBase[address] = _byteswap_ulong(data);
	else
		Write68000(address, data, 4);
}


//--------------------------------------------------
//	TMS32031 definitions
//--------------------------------------------------

/*
	AM_RANGE(0x000000, 0x007fff) AM_READWRITE(tms_m68k_ram_r, tms_m68k_ram_w)
	AM_RANGE(0x400000, 0x5fffff) AM_ROM AM_REGION(REGION_USER2, 0)
	AM_RANGE(0xc00000, 0xc00007) AM_WRITE(gaelco3d_render_w)
*/

extern UINT32 g32031MemoryBase[];
void Write32031(UINT32 address, UINT32 data, int size);

#define tms32031_change_pc(x)
#define tms32031_readop32(x)	(g32031MemoryBase[(x)/4])

static __forceinline UINT32 tms32031_prd32l(UINT32 address)
{
	if (address < 0x8000*4)
	{
		register UINT32 val = *(UINT16 *)&g68000MemoryBase[0xfe0000+address/2];
		val = (INT32)_byteswap_ulong(val) >> 16;
		return val;
	}
	else
		return g32031MemoryBase[address/4];
}

static __forceinline void tms32031_pwd32l(UINT32 address, UINT32 data)
{
	if (address < 0x8000*4)
		*(UINT16 *)&g68000MemoryBase[0xfe0000+address/2] = _byteswap_ushort(data);
	else if (address < 0xc00000*4)
		g32031MemoryBase[address/4] = data;
	else
		Write32031(address, data, 4);
}

