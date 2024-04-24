@ECHO OFF

ECHO START: %DATE% %TIME%

SET cnt=1
IF NOT %1.==. SET cnt=%1

SET rep=20
IF NOT %2.==. SET rep=%2

FOR /l %%n IN (1, 1, %cnt%) DO (
	START /D . /ABOVENORMAL /WAIT /B vi_metrics_time.exe -s name -r %rep% >vi_metrics_time.%COMPUTERNAME%.%%n.txt
	ECHO %%n from %cnt% done.
	TIME /T
)

ECHO FINISH: %TIME%
