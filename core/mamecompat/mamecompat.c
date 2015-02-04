//===================================================================
//
//	MAME compatibility file for standalone emulator shell
//
//	Copyright (c) 2004, Aaron Giles
//
//===================================================================

int gCurrentCPU;

char *cpuintrf_temp_str(void)
{
	static char string[256];
	return string;
}


void logerror(const char *fmt, ...)
{
}

