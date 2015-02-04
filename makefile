#####################################################################
#
#	Master makefile for standalone emulator shell
#
#	Copyright (c) 2004, Aaron Giles
#
#####################################################################

!ifndef GAME
!error Must specify a GAME=gamename
!endif


#--------------------------------------
#	Core game-specific defines
#--------------------------------------

EXENAME = $(GAME).exe
ENABLE_M68EC020 = 1
ENABLE_TMS32031 = 1
#ENABLE_ADSP2115 = 1


#--------------------------------------
#	Program definitions
#--------------------------------------

CC = @cl
RC = @rc
LINK = @link


#--------------------------------------
#	Set up directories and flags for
#	debug/opt versions
#--------------------------------------

!ifdef DEBUG
OUTDIR = $(GAME)\~debug
CFLAGS = /Od /RTC1 /MTd /DDEBUG=1
LINKFLAGS = /subsystem:console
!else
OUTDIR = $(GAME)\~opt
CFLAGS = /O2 /GF /MT
LINKFLAGS = /opt:ref /subsystem:windows
!endif


#--------------------------------------
#	Append common flags
#--------------------------------------

CFLAGS = $(CFLAGS) /nologo /c /wd4142 /Zi /GS /Dvsnprintf=vsnprintf /Fo$(OUTDIR)\ /Fd$(OUTDIR)\ /I$(GAME) /Icore /Icore\zlib /Icore\mamecompat /I$(OUTDIR)
RCFLAGS = $(RCFLAGS) /fo$(OUTDIR)\$(@B).res
LINKFLAGS = $(LINKFLAGS) /nologo /debug /nodefaultlib /libpath:$(OUTDIR)


#--------------------------------------
#	Basic objects list
#--------------------------------------

OBJECTS = \
	$(OUTDIR)\main.obj \
	$(OUTDIR)\game.obj \
	$(OUTDIR)\mamecompat.obj \
	$(OUTDIR)\adler32.obj \
	$(OUTDIR)\compress.obj \
	$(OUTDIR)\crc32.obj \
	$(OUTDIR)\deflate.obj \
	$(OUTDIR)\gzio.obj \
	$(OUTDIR)\inffast.obj \
	$(OUTDIR)\inflate.obj \
	$(OUTDIR)\infback.obj \
	$(OUTDIR)\inftrees.obj \
	$(OUTDIR)\trees.obj \
	$(OUTDIR)\uncompr.obj \
	$(OUTDIR)\zutil.obj \
	$(OUTDIR)\game.res \


#--------------------------------------
#	Basic link libraries
#--------------------------------------

LINKLIBS = \
	kernel32.lib \
	user32.lib \
	gdi32.lib \
	d3d8.lib \
	dsound.lib \
	dxguid.lib \

!ifdef DEBUG
LINKLIBS = $(LINKLIBS) \
	libcmtd.lib 
!else
LINKLIBS = $(LINKLIBS) \
	libcmt.lib 
!endif


#--------------------------------------
#	Default rule
#--------------------------------------

default : all


#--------------------------------------
#	Rules for the M68000-series
#--------------------------------------

!if defined(ENABLE_M68000) || defined(ENABLE_M68010) || defined(ENABLE_M68EC020)

M68000_GENERATED_OBJECTS = \
	$(OUTDIR)\m68kops.obj \
	$(OUTDIR)\m68kopac.obj \
	$(OUTDIR)\m68kopdm.obj \
	$(OUTDIR)\m68kopnz.obj \
	
CFLAGS = $(CFLAGS) /Icore\m68000

!ifdef ENABLE_M68000
CFLAGS = $(CFLAGS) /DHAS_M68000=1
OBJECTS = $(OBJECTS) $(M68000_GENERATED_OBJECTS) \
	$(OUTDIR)\m68kcpu.obj \
	$(OUTDIR)\m68kmame.obj
!endif

!ifdef ENABLE_M68010
CFLAGS = $(CFLAGS) /DHAS_M68010=1
OBJECTS = $(OBJECTS) $(M68000_GENERATED_OBJECTS) \
	$(OUTDIR)\m68kcpu.obj \
	$(OUTDIR)\m68kmame.obj
!endif

!ifdef ENABLE_M68EC020
CFLAGS = $(CFLAGS) /DHAS_M68EC020=1
OBJECTS = $(OBJECTS) $(M68000_GENERATED_OBJECTS) \
	$(OUTDIR)\m68kcpu.obj \
	$(OUTDIR)\m68kmame.obj
!endif

{core\m68000}.c{$(OUTDIR)}.obj :
	$(CC) $(CFLAGS) /FImamem68000.h $<

$(M68000_GENERATED_OBJECTS) :
	$(CC) $(CFLAGS) /FImamem68000.h $**

$(OUTDIR)\m68kopac.c $(OUTDIR)\m68kopdm.c $(OUTDIR)\m68kopnz.c : $(OUTDIR)\m68kops.c

$(OUTDIR)\m68kops.c : $(OUTDIR)\m68kmake.exe
	@$(OUTDIR)\m68kmake $(OUTDIR) core\m68000\m68k_in.c

$(OUTDIR)\m68kmake.exe : $(OUTDIR) $(OUTDIR)\m68kmake.obj
	$(LINK) /nologo /subsystem:console $(OUTDIR)\m68kmake.obj /out:$@

!endif


#--------------------------------------
#	Rules for the TMS32031-series
#--------------------------------------

!if defined(ENABLE_TMS32031)

CFLAGS = $(CFLAGS) /Icore\tms32031

!ifdef ENABLE_TMS32031
CFLAGS = $(CFLAGS) /DHAS_TMS32031=1
OBJECTS = $(OBJECTS) \
	$(OUTDIR)\tms32031.obj
!endif

{core\tms32031}.c{$(OUTDIR)}.obj :
	$(CC) $(CFLAGS) /FImametms32031.h $<

!endif


#--------------------------------------
#	Rules for the ADSP210x-series
#--------------------------------------

!if defined(ENABLE_ADSP2100) || defined(ENABLE_ADSP2101) || defined(ENABLE_ADSP2104) || defined(ENABLE_ADSP2105) || defined(ENABLE_ADSP2115)

CFLAGS = $(CFLAGS) /Icore\adsp2100

!ifdef ENABLE_ADSP2100
CFLAGS = $(CFLAGS) /DHAS_ADSP2100=1
OBJECTS = $(OBJECTS) \
	$(OUTDIR)\adsp2100.obj
!endif

!ifdef ENABLE_ADSP2101
CFLAGS = $(CFLAGS) /DHAS_ADSP2100=1
OBJECTS = $(OBJECTS) \
	$(OUTDIR)\adsp2100.obj
!endif

!ifdef ENABLE_ADSP2104
CFLAGS = $(CFLAGS) /DHAS_ADSP2100=1
OBJECTS = $(OBJECTS) \
	$(OUTDIR)\adsp2100.obj
!endif

!ifdef ENABLE_ADSP2105
CFLAGS = $(CFLAGS) /DHAS_ADSP2100=1
OBJECTS = $(OBJECTS) \
	$(OUTDIR)\adsp2100.obj
!endif

!ifdef ENABLE_ADSP2115
CFLAGS = $(CFLAGS) /DHAS_ADSP2100=1
OBJECTS = $(OBJECTS) \
	$(OUTDIR)\adsp2100.obj
!endif

{core\adsp2100}.c{$(OUTDIR)}.obj :
	$(CC) $(CFLAGS) /FImameadsp2100.h $<

!endif


#--------------------------------------
#	Core rules
#--------------------------------------

all : $(OUTDIR)\$(EXENAME)

clean :
	@-rd /s/q $(OUTDIR) 2>nul

$(OUTDIR) :
	@-mkdir $@

$(OUTDIR)\$(EXENAME) : $(OUTDIR) $(OBJECTS) $(ZLIB)
	$(LINK) $(LINKFLAGS) $(OBJECTS) $(LINKLIBS) /pdb:$(OUTDIR)\$(@B).pdb /out:$@

{core}.c{$(OUTDIR)}.obj :
	$(CC) $(CFLAGS) /FIcorecommon.h $<

{$(GAME)}.c{$(OUTDIR)}.obj :
	$(CC) $(CFLAGS) /FIcorecommon.h $<

{core\mamecompat}.c{$(OUTDIR)}.obj :
	$(CC) $(CFLAGS) $<

{core\zlib}.c{$(OUTDIR)}.obj :
	$(CC) $(CFLAGS) $<

{$(GAME)}.rc{$(OUTDIR)}.res :
	$(RC) $(RCFLAGS) $<
