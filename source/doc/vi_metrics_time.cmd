@ECHO OFF

ECHO START: %DATE%

SET cnt=1
IF NOT %1.==. SET cnt=%1

SET rep=20
IF NOT %2.==. SET rep=%2

IF NOT %3.==. SET filter=-i %3

FOR /l %%n IN (1, 1, %cnt%) DO (
	TIME /t
	ECHO Started measurement %%n of %cnt%...
	START /D . /ABOVENORMAL /WAIT /B vi_metrics_time.exe %filter% -r %rep% >vi_metrics_time.%COMPUTERNAME%.%%n.txt
	ECHO done.
)

ECHO FINISH: %TIME%
