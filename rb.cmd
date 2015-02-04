@if "%1"=="opt" goto :mopt
@if "%1"=="clean" goto :mclean

:mdebug
@nmake /nologo GAME=radikalb DEBUG=1
@goto :eof

:mopt
@nmake /nologo GAME=radikalb
@goto :eof

:mclean
@nmake /nologo clean GAME=radikalb DEBUG=1
@nmake /nologo clean GAME=radikalb 
@goto :eof
