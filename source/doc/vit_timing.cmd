REM @echo *** %DATE% %TIME% *** START
REM start /D .  /ABOVENORMAL /WAIT /B vit_timing_x64.exe 10>vit_timing_%COMPUTERNAME%_x64.txt
REM @echo *** %DATE% %TIME% ***
REM start /D .  /ABOVENORMAL /WAIT /B vit_timing_x86.exe 10>vit_timing_%COMPUTERNAME%_x86.txt
REM @echo *** %DATE% %TIME% *** FINISH


start /D . /ABOVENORMAL /WAIT /B vit_timing_x64.exe >vit_timing_%COMPUTERNAME%_x64.txt
