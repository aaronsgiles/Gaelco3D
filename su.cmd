@if "%1"=="opt" goto :mopt
@if "%1"=="clean" goto :mclean

:mdebug
@nmake /nologo GAME=speedup DEBUG=1
@goto :eof

:mopt
@nmake /nologo GAME=speedup
@goto :eof

:mclean
@nmake /nologo clean GAME=speedup DEBUG=1
@nmake /nologo clean GAME=speedup
@goto :eof
