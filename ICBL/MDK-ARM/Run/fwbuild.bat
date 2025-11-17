for /f "tokens=1,2,3,4 delims=/ " %%a in ("%date%") do set wday=%%a&set day=%%b&set month=%%c&set year=%%d
for /f "tokens=1,2,3 delims=:" %%a in ("%time: =0%") do set hour=%%a&set minute=%%b&set seconds=%%c
(
echo #define FW_DATE                                	0x00%year:~2,2%%month:~0,2%%day:~0,2%
echo #define FW_TIME                        			0x00%hour:~0,2%%minute:~0,2%%seconds:~0,2% 
)>"..\..\Common\fwbuild.h"

cscript ..\..\Common\fwbuild.vbs
..\..\Common\incbild.exe "..\..\Common\common.h"