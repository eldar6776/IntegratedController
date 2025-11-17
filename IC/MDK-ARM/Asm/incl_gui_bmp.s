	AREA    gui_bmp, DATA, READONLY
	EXPORT  thstat

thstat
	INCBIN  ../Src/Display/Termostat.bmp
	ALIGN
thstat_end
thstat_size
	DCD     thstat_end - thstat
	EXPORT  thstat_size
	END  