@ECHO OFF

ECHO START: %DATE%
ECHO.

SET cnt=1
IF NOT %1.==. SET cnt=%1

SET par=

:loop
SHIFT
IF "%~1"=="" GOTO endloop
IF NOT "%par%"=="" SET par=%par% 
SET par=%par%%1
GOTO loop
:endloop

ECHO Parameters: '%par%'
ECHO.

FOR /L %%n IN (1, 1, %cnt%) DO (
	TIME /t
	ECHO Started measurement %%n of %cnt%...
	START /ABOVENORMAL /WAIT /B vi_metrics_time.exe %par% >vi_metrics_time.%COMPUTERNAME%.%%n.txt
REM	TIMEOUT /T 1
	ECHO done.
	ECHO.
)

ECHO FINISH: %TIME%
