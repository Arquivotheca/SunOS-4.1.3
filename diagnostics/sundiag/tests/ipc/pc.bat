rem @(#)pc.bat 1.1 7/30/92
rem @(#)pc.bat	188.2 - 89/01/09
:setup
if exist dbg%1.flg erase dbg%1.flg
if exist stop%1.flg erase stop%1.flg
if exist pp%1.flg erase pp%1.flg
if exist fpy%1.flg erase fpy%1.flg
if exist error%1.txt erase error%1.txt
if exist sd-ipc%1 erase sd-ipc%1
echo %2 %3 %4 %5 %6 %7 %8 %9 >sd-ipc%1
if exist sd-ipc%1 parse %1 >error%1.txt
if errorlevel 1 goto eparse
rem if not exist dbg%1.flg echo off
goto cont1

:eparse
echo ERROR: ipc%1 31 parsing arguments. >LPT1:
flush1
echo ERROR: ipc%1 31 parsing arguments.
goto quit

:cont1

if exist dbg%1.flg goto postecho
echo off
:postecho

if exist stop%1.flg goto term
set NO87="Use of coprocessor suppressed"

rem the first test is the memory test

:memory

if not exist dbg%1.flg goto jmp1
echo INFO: ipc%1 beginning memory test. >LPT1:
flush1
:jmp1
echo INFO: ipc%1 beginning memory test. 
tst_mem >>error%1.txt
if errorlevel 1 goto memrr
goto ptst1

:memrr
echo ERROR: ipc%1 35 memory error message. >LPT1:
flush1
echo ERROR: ipc%1 35 memory error message. 
goto quit

:ptst1
echo INFO: ipc%1 completed memory test. 
if not exist dbg%1.flg goto jmp2
echo INFO: ipc%1 completed memory test. >LPT1:
flush1
:jmp2

rem the start of the soft math test.

:math_lib

if not exist dbg%1.flg goto jmp3
echo INFO: ipc%1 beginning soft fp test. >LPT1:
flush1
:jmp3
echo INFO: ipc%1 beginning soft fp test. 
tst_mlib >>error%1.txt
if errorlevel 1 goto mlbrr
goto ptst2

:mlbrr
echo ERROR: ipc%1 38 Soft floating point error message. >LPT1:
flush1
echo ERROR: ipc%1 38 Soft floating point error mesage. 
goto quit

:ptst2
echo INFO: ipc%1 completed soft fp test. 
if not exist dbg%1.flg goto jmp4
echo INFO: ipc%1 completed soft fp test. >LPT1:
flush1
:jmp4

rem the end of the soft math test.

rem 80287 TEST IF ONE IS ON THIS BOARD.
rem THE SOFTWARE WILL USE THE CHIP IF IT IS
rem INSTALLED IN THE BOARD.

:math_287
set NO87=

if not exist dbg%1.flg goto jmp5
echo INFO: ipc%1 beginning 80287 fp test. >LPT1:
flush1
:jmp5
echo INFO: ipc%1 beginning 80287 fp test. 
tst_mlib >>error%1.txt
if errorlevel 1 goto mhbrr
goto ptst3

:mhbrr
echo ERROR: ipc%1 40 80287 error message. >LPT1:
flush1
echo ERROR: ipc%1 40 80287 error message. 
goto quit

:ptst3
echo INFO: ipc%1 completed 80287 floating point test. 
if not exist dbg%1.flg goto jmp6
echo INFO: ipc%1 completed 80287 floating point test. >LPT1:
flush1
:jmp6

rem Check for the printer port test flag,
rem if it is there do the printer port test,
rem if one exists on ipc%1.

:printer
if not exist pp%1.flg goto ptskp

if not exist dbg%1.flg goto jmp7
echo INFO: ipc%1 beginning printer port test. >LPT1:
flush1
:jmp7
echo INFO: ipc%1 beginning printer port test. 
tst_ptr >>error%1.txt
if errorlevel 1 goto ptrrr
goto ptst4

:ptrrr
echo ERROR: ipc%1 42 printer port error message. >LPT1:
flush1
echo ERROR: ipc%1 42 printer port error message. 
goto quit

:ptst4
echo INFO: ipc%1 completed printer port test. 
if not exist dbg%1.flg goto jmp8
echo INFO: ipc%1 completed printer port test. >LPT1:
flush1
:jmp8

:ptskp

rem Check for the floppy test flag,
rem if it is there do the floppy test,
rem if one exists on ipc%1.

:floppy
if not exist fpy%1.flg goto fpskp

if not exist dbg%1.flg goto fjmp7
echo INFO: ipc%1 beginning floppy test. >LPT1:
flush1
:fjmp7
echo INFO: ipc%1 beginning floppy test.
format b:<n.dat
flpy
if errorlevel 4 goto fprr4
if errorlevel 3 goto fprr3
if errorlevel 2 goto fprr2
if errorlevel 1 goto fprr1
goto fpst4

:fprr4
echo ERROR: ipc%1 51 floppy diskette access failed. >LPT1:
flush1
echo ERROR: ipc%1 51 floppy diskette access failed. 
goto quit

:fprr3
echo ERROR: ipc%1 52 floppy format failed. >LPT1:
flush1
echo ERROR: ipc%1 52 floppy format failed.
goto quit

:fprr2
echo ERROR: ipc%1 53 floppy sector write failed. >LPT1:
flush1
echo ERROR: ipc%1 53 floppy sector write failed.
goto quit

:fprr1
echo ERROR: ipc%1 54 floppy sector read failed. >LPT1:
flush1
echo ERROR: ipc%1 54 floppy sector read failed.
goto quit

:fpst4
echo INFO: ipc%1 completed floppy test.
if not exist dbg%1.flg goto fjmp8
echo INFO: ipc%1 completed floppy test. >LPT1:
flush1
:fjmp8

:fpskp

rem draw some pictures on the screen with basic
rem dump them to a file and compare the screen 
rem image with a known good dumped image with scomp

if not exist dbg%1.flg goto jmpa
echo INFO: ipc%1 beginning video test. >LPT1:
flush1
:jmpa
echo INFO: ipc%1 beginning video test.
mode co80
if not exist subdir%1 mkdir subdir%1
cd subdir%1
gwbasic ..\tst_gr
copy ..\pictur1.bas .
scomp >>..\error%1.txt
if errorlevel 1 goto gphrr
goto ptst5

:gphrr
echo ERROR: ipc%1 45 video error message. >LPT1:
flush1
echo ERROR: ipc%1 45 video error message. 
cd ..
mode mono
goto quit 

:ptst5
cd ..
mode mono
echo INFO: ipc%1 completed video test. 
if not exist dbg%1.flg goto jmp9
echo INFO: ipc%1 completed video test. >LPT1:
flush1
:jmp9


:end
echo INFO: ipc%1 Completed testing. 
echo INFO: ipc%1 Completed testing. >LPT1:
flush1
goto quit

:term
echo INFO: ipc%1 Terminated testing. >LPT1:
flush1
echo INFO: ipc%1 Terminated testing. 
:quit
if exist error%1.txt type error%1.txt
if not exist nsd.txt quit

