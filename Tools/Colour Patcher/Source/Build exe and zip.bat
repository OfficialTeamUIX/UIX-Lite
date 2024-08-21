@Echo off
title Xboxdash Colourizer to EXE
Mode con:cols=70 lines=11
Color 0B
setlocal enabledelayedexpansion

set "zip=C:\Program Files\7-Zip\7z.exe"
set "archive_name=Xboxdash 5960 Colourizer"

if exist "!archive_name!.zip" del /q /y "!archive_name!.zip"

REM Build EXE (cxfreeze)
start /wait C:\Python312\Scripts\cxfreeze.exe "Xboxdash_5960_Colourizer.py" --target-dir "!archive_name!"

:Start
	ren "!archive_name!\Xboxdash_5960_Colourizer.exe" "!archive_name!.exe"
	call Echo d | XCopy /s /e /i /h /r /y "skins" "!archive_name!\skins"
	md "!archive_name!\xbe file"

	attrib +h +s "!archive_name!\lib"
	attrib +h -s "!archive_name!\frozen_application_license.txt"
	attrib +h -s "!archive_name!\python3.dll"
	attrib +h -s "!archive_name!\python312.dll"

	"!zip!" a "!archive_name!.zip" "!archive_name!" -mx=7 -r -y

	REM Cleanup
	rmdir /s /q "!archive_name!" > NUL