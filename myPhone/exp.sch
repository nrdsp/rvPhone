EESchema Schematic File Version 2
LIBS:power
LIBS:device
LIBS:transistors
LIBS:conn
LIBS:linear
LIBS:regul
LIBS:74xx
LIBS:cmos4000
LIBS:adc-dac
LIBS:memory
LIBS:xilinx
LIBS:microcontrollers
LIBS:dsp
LIBS:microchip
LIBS:analog_switches
LIBS:motorola
LIBS:texas
LIBS:intel
LIBS:audio
LIBS:interface
LIBS:digital-audio
LIBS:philips
LIBS:display
LIBS:cypress
LIBS:siliconi
LIBS:opto
LIBS:atmel
LIBS:contrib
LIBS:valves
LIBS:switches
LIBS:bav99
LIBS:texas-tps
LIBS:rtl8710af
LIBS:tpa301
LIBS:e310-g000
LIBS:is25lp128
LIBS:sit1533
LIBS:ics-43434
LIBS:max98357
LIBS:cui-sj
LIBS:ft81x
LIBS:fan5333
LIBS:esp32
LIBS:drv2605l
LIBS:myPower
LIBS:sn74
LIBS:SODIMM-204
EELAYER 25 0
EELAYER END
$Descr A3 16535 11693
encoding utf-8
Sheet 3 3
Title ""
Date ""
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L +VSYS #PWR0138
U 1 1 5C8F0FDD
P 3100 700
F 0 "#PWR0138" H 3100 550 50  0001 C CNN
F 1 "+VSYS" H 3100 840 50  0000 C CNN
F 2 "" H 3100 700 50  0001 C CNN
F 3 "" H 3100 700 50  0001 C CNN
	1    3100 700 
	1    0    0    -1  
$EndComp
Wire Wire Line
	6850 900  7150 900 
Wire Wire Line
	3100 700  3100 10900
Wire Wire Line
	7450 900  8100 900 
Connection ~ 3100 1200
Wire Wire Line
	8400 900  8700 900 
Connection ~ 3100 2100
Wire Wire Line
	9000 900  9300 900 
Connection ~ 3100 3000
Wire Wire Line
	9600 900  9900 900 
Connection ~ 3100 3900
Wire Wire Line
	10200 900  10850 900 
Connection ~ 3100 4600
Wire Wire Line
	11150 900  11800 900 
Connection ~ 3100 5500
Wire Wire Line
	12100 900  12750 900 
Connection ~ 3100 6400
Wire Wire Line
	13050 900  13350 900 
Connection ~ 3100 7300
Wire Wire Line
	13650 900  14300 900 
Connection ~ 3100 8200
Wire Wire Line
	14600 900  14900 900 
Connection ~ 3100 9100
Wire Wire Line
	3000 10900 3800 10900
Connection ~ 3100 10000
$Comp
L GND #PWR0139
U 1 1 5C8F1120
P 1400 11000
F 0 "#PWR0139" H 1400 10750 50  0001 C CNN
F 1 "GND" H 1400 10850 50  0000 C CNN
F 2 "" H 1400 11000 50  0001 C CNN
F 3 "" H 1400 11000 50  0001 C CNN
	1    1400 11000
	1    0    0    -1  
$EndComp
Wire Wire Line
	1500 800  1400 800 
Wire Wire Line
	1400 800  1400 11000
Wire Wire Line
	1500 1700 1400 1700
Connection ~ 1400 1700
Wire Wire Line
	1500 2600 1400 2600
Connection ~ 1400 2600
Wire Wire Line
	1500 3500 1400 3500
Connection ~ 1400 3500
Wire Wire Line
	1500 4300 1400 4300
Connection ~ 1400 4300
Wire Wire Line
	1500 5100 1400 5100
Connection ~ 1400 5100
Wire Wire Line
	1500 6000 1400 6000
Connection ~ 1400 6000
Wire Wire Line
	1500 6900 1400 6900
Connection ~ 1400 6900
Wire Wire Line
	1500 7800 1400 7800
Connection ~ 1400 7800
Wire Wire Line
	1500 8700 1400 8700
Connection ~ 1400 8700
Wire Wire Line
	1500 9600 1400 9600
Connection ~ 1400 9600
Wire Wire Line
	1500 9900 1100 9900
Wire Wire Line
	1100 9900 1100 10000
$Comp
L C C301
U 1 1 5C8F1372
P 1100 10150
F 0 "C301" H 1125 10250 50  0000 L CNN
F 1 "4.7uF" H 1125 10050 50  0000 L CNN
F 2 "Capacitors_SMD:C_0603" H 1138 10000 50  0001 C CNN
F 3 "" H 1100 10150 50  0001 C CNN
	1    1100 10150
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR0140
U 1 1 5C8F1395
P 1100 10400
F 0 "#PWR0140" H 1100 10150 50  0001 C CNN
F 1 "GND" H 1100 10250 50  0000 C CNN
F 2 "" H 1100 10400 50  0001 C CNN
F 3 "" H 1100 10400 50  0001 C CNN
	1    1100 10400
	1    0    0    -1  
$EndComp
Wire Wire Line
	1100 10300 1100 10400
$Comp
L C C302
U 1 1 5C8F15E6
P 7150 1100
F 0 "C302" H 7175 1200 50  0000 L CNN
F 1 "0.1uF" H 7175 1000 50  0000 L CNN
F 2 "Capacitors_SMD:C_0603" H 7188 950 50  0001 C CNN
F 3 "" H 7150 1100 50  0001 C CNN
	1    7150 1100
	1    0    0    -1  
$EndComp
$Comp
L C C303
U 1 1 5C8F160D
P 7750 1100
F 0 "C303" H 7775 1200 50  0000 L CNN
F 1 "0.1uF" H 7775 1000 50  0000 L CNN
F 2 "Capacitors_SMD:C_0603" H 7788 950 50  0001 C CNN
F 3 "" H 7750 1100 50  0001 C CNN
	1    7750 1100
	1    0    0    -1  
$EndComp
$Comp
L C C304
U 1 1 5C8F1635
P 8700 1100
F 0 "C304" H 8725 1200 50  0000 L CNN
F 1 "0.1uF" H 8725 1000 50  0000 L CNN
F 2 "Capacitors_SMD:C_0603" H 8738 950 50  0001 C CNN
F 3 "" H 8700 1100 50  0001 C CNN
	1    8700 1100
	1    0    0    -1  
$EndComp
$Comp
L C C305
U 1 1 5C8F168B
P 9300 1100
F 0 "C305" H 9325 1200 50  0000 L CNN
F 1 "0.1uF" H 9325 1000 50  0000 L CNN
F 2 "Capacitors_SMD:C_0603" H 9338 950 50  0001 C CNN
F 3 "" H 9300 1100 50  0001 C CNN
	1    9300 1100
	1    0    0    -1  
$EndComp
$Comp
L C C306
U 1 1 5C8F16BB
P 9900 1100
F 0 "C306" H 9925 1200 50  0000 L CNN
F 1 "0.1uF" H 9925 1000 50  0000 L CNN
F 2 "Capacitors_SMD:C_0603" H 9938 950 50  0001 C CNN
F 3 "" H 9900 1100 50  0001 C CNN
	1    9900 1100
	1    0    0    -1  
$EndComp
$Comp
L C C307
U 1 1 5C8F16F2
P 10500 1100
F 0 "C307" H 10525 1200 50  0000 L CNN
F 1 "0.1uF" H 10525 1000 50  0000 L CNN
F 2 "Capacitors_SMD:C_0603" H 10538 950 50  0001 C CNN
F 3 "" H 10500 1100 50  0001 C CNN
	1    10500 1100
	1    0    0    -1  
$EndComp
$Comp
L C C308
U 1 1 5C8F175A
P 11450 1100
F 0 "C308" H 11475 1200 50  0000 L CNN
F 1 "0.1uF" H 11475 1000 50  0000 L CNN
F 2 "Capacitors_SMD:C_0603" H 11488 950 50  0001 C CNN
F 3 "" H 11450 1100 50  0001 C CNN
	1    11450 1100
	1    0    0    -1  
$EndComp
$Comp
L C C309
U 1 1 5C8F17BA
P 12400 1100
F 0 "C309" H 12425 1200 50  0000 L CNN
F 1 "0.1uF" H 12425 1000 50  0000 L CNN
F 2 "Capacitors_SMD:C_0603" H 12438 950 50  0001 C CNN
F 3 "" H 12400 1100 50  0001 C CNN
	1    12400 1100
	1    0    0    -1  
$EndComp
$Comp
L C C310
U 1 1 5C8F180E
P 13350 1100
F 0 "C310" H 13375 1200 50  0000 L CNN
F 1 "0.1uF" H 13375 1000 50  0000 L CNN
F 2 "Capacitors_SMD:C_0603" H 13388 950 50  0001 C CNN
F 3 "" H 13350 1100 50  0001 C CNN
	1    13350 1100
	1    0    0    -1  
$EndComp
$Comp
L C C311
U 1 1 5C8F1852
P 13950 1100
F 0 "C311" H 13975 1200 50  0000 L CNN
F 1 "0.1uF" H 13975 1000 50  0000 L CNN
F 2 "Capacitors_SMD:C_0603" H 13988 950 50  0001 C CNN
F 3 "" H 13950 1100 50  0001 C CNN
	1    13950 1100
	1    0    0    -1  
$EndComp
$Comp
L C C312
U 1 1 5C8F189B
P 14900 1100
F 0 "C312" H 14925 1200 50  0000 L CNN
F 1 "0.1uF" H 14925 1000 50  0000 L CNN
F 2 "Capacitors_SMD:C_0603" H 14938 950 50  0001 C CNN
F 3 "" H 14900 1100 50  0001 C CNN
	1    14900 1100
	1    0    0    -1  
$EndComp
$Comp
L C C313
U 1 1 5C8F18E5
P 15600 1100
F 0 "C313" H 15625 1200 50  0000 L CNN
F 1 "0.1uF" H 15625 1000 50  0000 L CNN
F 2 "Capacitors_SMD:C_0603" H 15638 950 50  0001 C CNN
F 3 "" H 15600 1100 50  0001 C CNN
	1    15600 1100
	1    0    0    -1  
$EndComp
$Comp
L C C314
U 1 1 5C8F1C4F
P 8100 1100
F 0 "C314" H 8125 1200 50  0000 L CNN
F 1 "22uF" H 8125 1000 50  0000 L CNN
F 2 "Capacitors_SMD:C_0603" H 8138 950 50  0001 C CNN
F 3 "" H 8100 1100 50  0001 C CNN
	1    8100 1100
	1    0    0    -1  
$EndComp
$Comp
L C C315
U 1 1 5C8F1D0B
P 10850 1100
F 0 "C315" H 10875 1200 50  0000 L CNN
F 1 "22uF" H 10875 1000 50  0000 L CNN
F 2 "Capacitors_SMD:C_0603" H 10888 950 50  0001 C CNN
F 3 "" H 10850 1100 50  0001 C CNN
	1    10850 1100
	1    0    0    -1  
$EndComp
$Comp
L C C316
U 1 1 5C8F1D61
P 11800 1100
F 0 "C316" H 11825 1200 50  0000 L CNN
F 1 "22uF" H 11825 1000 50  0000 L CNN
F 2 "Capacitors_SMD:C_0603" H 11838 950 50  0001 C CNN
F 3 "" H 11800 1100 50  0001 C CNN
	1    11800 1100
	1    0    0    -1  
$EndComp
$Comp
L C C317
U 1 1 5C8F1DBA
P 12750 1100
F 0 "C317" H 12775 1200 50  0000 L CNN
F 1 "22uF" H 12775 1000 50  0000 L CNN
F 2 "Capacitors_SMD:C_0603" H 12788 950 50  0001 C CNN
F 3 "" H 12750 1100 50  0001 C CNN
	1    12750 1100
	1    0    0    -1  
$EndComp
$Comp
L C C318
U 1 1 5C8F1E4E
P 14300 1100
F 0 "C318" H 14325 1200 50  0000 L CNN
F 1 "22uF" H 14325 1000 50  0000 L CNN
F 2 "Capacitors_SMD:C_0603" H 14338 950 50  0001 C CNN
F 3 "" H 14300 1100 50  0001 C CNN
	1    14300 1100
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR0141
U 1 1 5C8F2209
P 15600 1300
F 0 "#PWR0141" H 15600 1050 50  0001 C CNN
F 1 "GND" H 15600 1150 50  0000 C CNN
F 2 "" H 15600 1300 50  0001 C CNN
F 3 "" H 15600 1300 50  0001 C CNN
	1    15600 1300
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR0142
U 1 1 5C8F2381
P 14900 1300
F 0 "#PWR0142" H 14900 1050 50  0001 C CNN
F 1 "GND" H 14900 1150 50  0000 C CNN
F 2 "" H 14900 1300 50  0001 C CNN
F 3 "" H 14900 1300 50  0001 C CNN
	1    14900 1300
	1    0    0    -1  
$EndComp
Wire Wire Line
	15600 950  15600 900 
Wire Wire Line
	14900 1300 14900 1250
Wire Wire Line
	14900 900  14900 950 
Wire Wire Line
	14300 900  14300 950 
Wire Wire Line
	13950 950  13950 900 
Connection ~ 13950 900 
$Comp
L GND #PWR0143
U 1 1 5C8F26B6
P 13950 1350
F 0 "#PWR0143" H 13950 1100 50  0001 C CNN
F 1 "GND" H 13950 1200 50  0000 C CNN
F 2 "" H 13950 1350 50  0001 C CNN
F 3 "" H 13950 1350 50  0001 C CNN
	1    13950 1350
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR0144
U 1 1 5C8F2788
P 13350 1300
F 0 "#PWR0144" H 13350 1050 50  0001 C CNN
F 1 "GND" H 13350 1150 50  0000 C CNN
F 2 "" H 13350 1300 50  0001 C CNN
F 3 "" H 13350 1300 50  0001 C CNN
	1    13350 1300
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR0145
U 1 1 5C8F27E8
P 12400 1350
F 0 "#PWR0145" H 12400 1100 50  0001 C CNN
F 1 "GND" H 12400 1200 50  0000 C CNN
F 2 "" H 12400 1350 50  0001 C CNN
F 3 "" H 12400 1350 50  0001 C CNN
	1    12400 1350
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR0146
U 1 1 5C8F28E0
P 10500 1350
F 0 "#PWR0146" H 10500 1100 50  0001 C CNN
F 1 "GND" H 10500 1200 50  0000 C CNN
F 2 "" H 10500 1350 50  0001 C CNN
F 3 "" H 10500 1350 50  0001 C CNN
	1    10500 1350
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR0147
U 1 1 5C8F2940
P 9900 1300
F 0 "#PWR0147" H 9900 1050 50  0001 C CNN
F 1 "GND" H 9900 1150 50  0000 C CNN
F 2 "" H 9900 1300 50  0001 C CNN
F 3 "" H 9900 1300 50  0001 C CNN
	1    9900 1300
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR0148
U 1 1 5C8F29FF
P 9300 1300
F 0 "#PWR0148" H 9300 1050 50  0001 C CNN
F 1 "GND" H 9300 1150 50  0000 C CNN
F 2 "" H 9300 1300 50  0001 C CNN
F 3 "" H 9300 1300 50  0001 C CNN
	1    9300 1300
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR0149
U 1 1 5C8F2A5F
P 8700 1300
F 0 "#PWR0149" H 8700 1050 50  0001 C CNN
F 1 "GND" H 8700 1150 50  0000 C CNN
F 2 "" H 8700 1300 50  0001 C CNN
F 3 "" H 8700 1300 50  0001 C CNN
	1    8700 1300
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR0150
U 1 1 5C8F2ABF
P 7750 1350
F 0 "#PWR0150" H 7750 1100 50  0001 C CNN
F 1 "GND" H 7750 1200 50  0000 C CNN
F 2 "" H 7750 1350 50  0001 C CNN
F 3 "" H 7750 1350 50  0001 C CNN
	1    7750 1350
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR0151
U 1 1 5C8F2B1F
P 7150 1300
F 0 "#PWR0151" H 7150 1050 50  0001 C CNN
F 1 "GND" H 7150 1150 50  0000 C CNN
F 2 "" H 7150 1300 50  0001 C CNN
F 3 "" H 7150 1300 50  0001 C CNN
	1    7150 1300
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR0152
U 1 1 5C8F2E11
P 11450 1350
F 0 "#PWR0152" H 11450 1100 50  0001 C CNN
F 1 "GND" H 11450 1200 50  0000 C CNN
F 2 "" H 11450 1350 50  0001 C CNN
F 3 "" H 11450 1350 50  0001 C CNN
	1    11450 1350
	1    0    0    -1  
$EndComp
Wire Wire Line
	11450 1250 11450 1350
Wire Wire Line
	12400 1250 12400 1350
Wire Wire Line
	13350 1250 13350 1300
Wire Wire Line
	13950 1250 13950 1350
Wire Wire Line
	14300 1250 14300 1300
Wire Wire Line
	14300 1300 13950 1300
Connection ~ 13950 1300
Wire Wire Line
	15600 1250 15600 1300
Wire Wire Line
	13350 900  13350 950 
Wire Wire Line
	12400 900  12400 950 
Wire Wire Line
	12750 900  12750 950 
Connection ~ 12400 900 
Wire Wire Line
	12750 1250 12750 1300
Wire Wire Line
	12750 1300 12400 1300
Connection ~ 12400 1300
Wire Wire Line
	11450 900  11450 950 
Wire Wire Line
	11800 900  11800 950 
Connection ~ 11450 900 
Wire Wire Line
	11800 1250 11800 1300
Wire Wire Line
	11800 1300 11450 1300
Connection ~ 11450 1300
Wire Wire Line
	10500 1250 10500 1350
Wire Wire Line
	10850 1250 10850 1300
Wire Wire Line
	10850 1300 10500 1300
Connection ~ 10500 1300
Wire Wire Line
	10500 900  10500 950 
Wire Wire Line
	10850 900  10850 950 
Connection ~ 10500 900 
Wire Wire Line
	9900 900  9900 950 
Wire Wire Line
	9900 1250 9900 1300
Wire Wire Line
	9300 1250 9300 1300
Wire Wire Line
	9300 900  9300 950 
Wire Wire Line
	8700 900  8700 950 
Wire Wire Line
	8700 1300 8700 1250
Wire Wire Line
	7750 1350 7750 1250
Wire Wire Line
	8100 1250 8100 1300
Wire Wire Line
	8100 1300 7750 1300
Connection ~ 7750 1300
Wire Wire Line
	7750 900  7750 950 
Wire Wire Line
	8100 900  8100 950 
Connection ~ 7750 900 
Wire Wire Line
	7150 900  7150 950 
Wire Wire Line
	7150 1250 7150 1300
NoConn ~ 1500 900 
NoConn ~ 1500 1000
NoConn ~ 1500 1800
NoConn ~ 1500 1900
NoConn ~ 1500 2000
NoConn ~ 1500 2100
NoConn ~ 1500 2200
NoConn ~ 1500 2300
NoConn ~ 1500 2400
NoConn ~ 1500 2500
NoConn ~ 1500 2700
NoConn ~ 1500 2800
NoConn ~ 1500 2900
NoConn ~ 1500 3000
NoConn ~ 1500 3100
NoConn ~ 1500 3200
NoConn ~ 1500 3300
NoConn ~ 1500 3400
NoConn ~ 3000 800 
NoConn ~ 3000 900 
NoConn ~ 3000 1000
NoConn ~ 3000 1100
NoConn ~ 3000 1300
NoConn ~ 3000 1400
NoConn ~ 3000 1500
NoConn ~ 3000 1600
NoConn ~ 3000 1700
NoConn ~ 3000 1800
NoConn ~ 3000 1900
NoConn ~ 3000 2000
NoConn ~ 3000 2200
NoConn ~ 3000 2300
NoConn ~ 3000 2400
NoConn ~ 3000 2500
NoConn ~ 3000 2600
NoConn ~ 3000 2700
NoConn ~ 3000 2800
NoConn ~ 3000 2900
NoConn ~ 3000 3100
NoConn ~ 3000 3200
NoConn ~ 3000 3300
NoConn ~ 3000 3400
NoConn ~ 3000 3500
NoConn ~ 1500 3600
NoConn ~ 1500 3700
NoConn ~ 1500 3800
NoConn ~ 1500 3900
NoConn ~ 1500 4000
NoConn ~ 1500 4100
NoConn ~ 1500 4200
NoConn ~ 1500 4400
NoConn ~ 1500 4500
NoConn ~ 3000 4500
NoConn ~ 3000 4400
NoConn ~ 3000 4300
NoConn ~ 3000 4200
NoConn ~ 3000 4100
NoConn ~ 3000 4000
NoConn ~ 3000 3800
NoConn ~ 3000 3700
NoConn ~ 3000 3600
NoConn ~ 3000 4700
NoConn ~ 3000 4800
NoConn ~ 3000 4900
NoConn ~ 3000 5000
NoConn ~ 3000 5100
NoConn ~ 3000 5200
NoConn ~ 3000 5300
NoConn ~ 3000 5400
NoConn ~ 1500 4600
NoConn ~ 1500 4700
NoConn ~ 1500 4800
NoConn ~ 1500 4900
NoConn ~ 1500 5000
NoConn ~ 1500 5200
NoConn ~ 1500 5300
NoConn ~ 1500 5400
NoConn ~ 1500 5500
NoConn ~ 1500 5600
NoConn ~ 1500 5700
NoConn ~ 1500 5800
NoConn ~ 1500 5900
NoConn ~ 1500 6100
NoConn ~ 1500 6200
NoConn ~ 1500 6300
NoConn ~ 1500 6400
NoConn ~ 3000 5600
NoConn ~ 3000 5700
NoConn ~ 3000 5800
NoConn ~ 3000 5900
NoConn ~ 3000 6000
NoConn ~ 3000 6100
NoConn ~ 3000 6200
NoConn ~ 3000 6300
NoConn ~ 3000 6500
NoConn ~ 3000 6600
NoConn ~ 3000 6700
NoConn ~ 3000 6800
NoConn ~ 3000 6900
NoConn ~ 3000 7000
NoConn ~ 3000 7100
NoConn ~ 3000 7200
NoConn ~ 3000 7400
NoConn ~ 3000 7500
NoConn ~ 3000 7600
NoConn ~ 3000 7700
NoConn ~ 3000 7800
NoConn ~ 3000 7900
NoConn ~ 3000 8000
NoConn ~ 3000 8100
NoConn ~ 3000 8300
NoConn ~ 3000 8400
NoConn ~ 3000 8500
NoConn ~ 3000 8600
NoConn ~ 3000 8700
NoConn ~ 3000 8800
NoConn ~ 3000 8900
NoConn ~ 3000 9000
NoConn ~ 3000 9200
NoConn ~ 3000 9300
NoConn ~ 3000 9400
NoConn ~ 3000 9500
NoConn ~ 3000 9600
NoConn ~ 3000 9700
NoConn ~ 3000 9800
NoConn ~ 3000 9900
NoConn ~ 3000 10100
NoConn ~ 3000 10200
NoConn ~ 3000 10300
NoConn ~ 3000 10400
NoConn ~ 3000 10500
NoConn ~ 3000 10600
NoConn ~ 3000 10700
NoConn ~ 3000 10800
NoConn ~ 1500 10900
NoConn ~ 1500 10800
NoConn ~ 1500 10700
NoConn ~ 1500 10600
NoConn ~ 1500 10500
NoConn ~ 1500 10400
NoConn ~ 1500 10300
NoConn ~ 1500 10200
NoConn ~ 1500 10100
NoConn ~ 1500 10000
NoConn ~ 1500 9800
NoConn ~ 1500 9700
NoConn ~ 1500 9500
NoConn ~ 1500 9400
NoConn ~ 1500 9300
NoConn ~ 1500 9200
NoConn ~ 1500 9100
NoConn ~ 1500 9000
NoConn ~ 1500 8900
NoConn ~ 1500 8800
NoConn ~ 1500 8600
NoConn ~ 1500 8500
NoConn ~ 1500 8400
NoConn ~ 1500 8300
NoConn ~ 1500 8200
NoConn ~ 1500 8100
NoConn ~ 1500 8000
NoConn ~ 1500 7900
NoConn ~ 1500 7700
NoConn ~ 1500 7600
NoConn ~ 1500 7500
NoConn ~ 1500 7400
NoConn ~ 1500 7300
NoConn ~ 1500 7200
NoConn ~ 1500 7100
NoConn ~ 1500 7000
NoConn ~ 1500 6800
NoConn ~ 1500 6700
NoConn ~ 1500 6600
NoConn ~ 1500 6500
Text GLabel 1300 1300 0    55   BiDi ~ 0
ESP32.HSPI.MOSI
Wire Wire Line
	1300 1300 1500 1300
Text GLabel 1300 1400 0    55   BiDi ~ 0
ESP32.HSPI.SCK
Text GLabel 1300 1500 0    55   BiDi ~ 0
ESP32.HSPI.MISO
Text GLabel 1300 1600 0    55   BiDi ~ 0
ESP32.HSPI.SS0
Wire Wire Line
	1300 1600 1500 1600
Wire Wire Line
	1300 1500 1500 1500
Wire Wire Line
	1300 1400 1500 1400
Wire Wire Line
	3000 10000 3800 10000
Wire Wire Line
	3000 9100 3800 9100
Wire Wire Line
	3000 8200 3800 8200
Wire Wire Line
	3000 7300 3800 7300
Wire Wire Line
	3000 6400 3800 6400
Wire Wire Line
	3000 4600 3800 4600
Wire Wire Line
	3000 3900 3800 3900
Wire Wire Line
	3000 3000 3800 3000
Wire Wire Line
	3000 2100 3800 2100
Wire Wire Line
	3000 1200 3800 1200
Text Label 3800 1200 2    55   ~ 0
VSYS1
Text Label 3800 2100 2    55   ~ 0
VSYS2
Text Label 3800 3000 2    55   ~ 0
VSYS3
Text Label 3800 3900 2    55   ~ 0
VSYS4
Text Label 3800 4600 2    55   ~ 0
VSYS5
Text Label 3800 5500 2    55   ~ 0
VSYS6
Text Label 3800 6400 2    55   ~ 0
VSYS7
Text Label 3800 7300 2    55   ~ 0
VSYS8
Text Label 3800 8200 2    55   ~ 0
VSYS9
Text Label 3800 9100 2    55   ~ 0
VSYS10
Text Label 3800 10000 2    55   ~ 0
VSYS11
Text Label 3800 10900 2    55   ~ 0
VSYS12
Connection ~ 3100 10900
Wire Wire Line
	3000 5500 3800 5500
Text Label 6850 900  0    55   ~ 0
VSYS1
Text Label 7450 900  0    55   ~ 0
VSYS2
Text Label 8400 900  0    55   ~ 0
VSYS3
Text Label 9000 900  0    55   ~ 0
VSYS4
Text Label 9600 900  0    55   ~ 0
VSYS5
Text Label 10200 900  0    55   ~ 0
VSYS6
Text Label 11150 900  0    55   ~ 0
VSYS7
Text Label 12100 900  0    55   ~ 0
VSYS8
Text Label 13050 900  0    55   ~ 0
VSYS9
Text Label 13650 900  0    55   ~ 0
VSYS10
Text Label 14600 900  0    55   ~ 0
VSYS11
Text Label 15300 900  0    55   ~ 0
VSYS12
Wire Wire Line
	15600 900  15300 900 
$Comp
L iMX8 J301
U 1 1 5C8FA2B8
P 2250 5850
F 0 "J301" H 2250 11050 50  0000 C CNN
F 1 "iMX8" H 2250 650 50  0000 C CNN
F 2 "footprints:Socket_SODIMM_DDR3_TE_2013289" H 2150 10950 50  0001 C CNN
F 3 "" H 2250 5850 50  0001 C CNN
	1    2250 5850
	1    0    0    -1  
$EndComp
Text GLabel 1300 1100 0    55   BiDi ~ 0
ESP32.IO36
Text GLabel 1300 1200 0    55   BiDi ~ 0
ESP32.IO27
Wire Wire Line
	1300 1200 1500 1200
Wire Wire Line
	1300 1100 1500 1100
$EndSCHEMATC
