	AREA    logo_img, DATA, READONLY
	EXPORT  logo
logo
	INCBIN ../Src/Display/logo.png
	ALIGN
logo_end
logo_size
	DCD logo_end - logo
	EXPORT logo_size
	END