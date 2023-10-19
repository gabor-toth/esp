#ifndef HAJO_N2KRAYMARINE_H
#define HAJO_N2KRAYMARINE_H

/*
1	Manufacturer Code	1851: Raymarine	0 .. 2045
11 bits lookup MANUFACTURER_CODE
2	Reserved			2 bits RESERVED
3	Industry Code	4: Marine Industry
0 .. 6
3 bits lookup INDUSTRY_CODE
4	Proprietary ID	33264: 0x81f0
0 .. 65533
16 bits unsigned NUMBER
5	command	132: 0x84
0 .. 253
8 bits unsigned NUMBER
6	Unknown 1			24 bits BINARY
7	Pilot Mode
0 .. 253
8 bits lookup SEATALK_PILOT_MODE
8	Sub Mode
0 .. 253
8 bits unsigned NUMBER
9	Pilot Mode Data			8 bits BINARY
10	Unknown 2			80 bits BINARY
 */
#endif //HAJO_N2KRAYMARINE_H
