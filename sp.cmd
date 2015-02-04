@if "%1"=="opt" goto :mopt
@if "%1"=="clean" goto :mclean

:mdebug
@nmake /nologo GAME=surfplnt DEBUG=1
@goto :eof

:mopt
@nmake /nologo GAME=surfplnt
@goto :eof

:mclean
@nmake /nologo clean GAME=surfplnt DEBUG=1
@nmake /nologo clean GAME=surfplnt
@goto :eof
