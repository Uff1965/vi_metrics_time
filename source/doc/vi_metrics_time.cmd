@echo START: %DATE% %TIME%
START /D . /ABOVENORMAL /WAIT /B vi_metrics_time.exe -s name -r 20 >vi_metrics_time.%COMPUTERNAME%.txt
@echo FINISH: %TIME%
