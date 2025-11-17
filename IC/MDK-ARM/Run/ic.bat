@ECHO OFF
C:\Keil\ARM\Utilities\srecord\srec_cat Exe\ic.hex -intel -offset -0x08010000 -crop 0 0xEFFF0 -Exclusive_Length_Little_Endian 0xEFFF0 -fill 0xFF 0 0xEFFF0 -o Exe\tmp1.hex -intel
C:\Keil\ARM\Utilities\srecord\srec_cat Exe\tmp1.hex -intel -crop 0 0x2000 0x2008 0xEFFF0 Exe\tmp1.hex -intel -crop 0xEFFF0 0xEFFF8 -offset -0xEDFF0 -o Exe\tmp2.hex -intel 
C:\Keil\ARM\Utilities\srecord\srec_cat Exe\tmp2.hex -intel -unfill 0xFF 4 -o Exe\tmp3.hex -intel
C:\Keil\ARM\Utilities\srecord\srec_cat Exe\tmp3.hex -intel -fill 0xFF -over Exe\tmp3.hex -intel -o Exe\tmp4.hex -intel
C:\Keil\ARM\Utilities\srecord\srec_cat Exe\tmp4.hex -intel -STM32 0xEFFF4 -crop 0xEFFF4 0xEFFF8 -offset -0xEDFF0 -o Exe\tmp5.hex -intel 
C:\Keil\ARM\Utilities\srecord\srec_cat Exe\tmp4.hex -intel -exclude 0x2004 0x2008 Exe\tmp5.hex -intel -o Exe\tmp6.hex -intel
C:\Keil\ARM\Utilities\srecord\srec_cat Exe\tmp6.hex -intel -o Out\IMG20.RAW -binary
C:\Keil\ARM\Utilities\srecord\srec_cat Exe\tmp6.hex -intel  -offset 0x08010000 -o Out\ic.hex -intel
C:\Keil\ARM\Utilities\srecord\srec_cat Out\ER_QSPI1 -binary -offset 0x90000000 -o Out\ext.hex -intel
C:\Keil\ARM\Utilities\srecord\srec_cat ..\..\ICBL\MDK-ARM\Out\icbl.hex -intel Out\ic.hex -intel Out\ext.hex -intel -o Out\ic_all_in_one.hex -intel
exit