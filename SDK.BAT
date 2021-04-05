    @echo off
rem
rem release the VBE/AI Driver Developers code to floppy
rem

    if (%1)==() goto helps
    if (%DEBUG%)==() goto release
    @echo on
:release

    set MISSING=songs\alfre.mid
    if NOT exist %MISSING% goto error
    set MISSING=songs\sakura2a.mid
    if NOT exist %MISSING% goto error

    call flush

    if not exist %1\midi            mkdir %1\midi
    if not exist %1\midi\opl2       mkdir %1\midi\opl2
    if not exist %1\midi\opl2\tools mkdir %1\midi\opl2\tools

rem
rem Documentation & header files
rem
    copy sdk.doc                %1\
    copy rev.doc                %1\
    copy sdk.bat                %1\
    copy is.bat                 %1\
    copy mksdk.bat              %1\

rem
rem driver executables
rem

    copy paswave.com            %1\
    copy sswave.com             %1\
    copy sbwave.com             %1\
    copy opl2.com               %1\
    copy mpu.com                %1\

    copy songs\alfre.mid        %1\
    copy songs\sakura2a.mid     %1\
    copy guppy.wav              %1\

    copy mt32                   %1\

rem
rem Application programs and subroutines
rem

    copy play.*                 %1\
    copy note.*                 %1\
    copy pmidi.*                %1\
    copy pdata.c                %1\
    copy setpref.*              %1\
    copy testw.*                %1\
    copy volume.*               %1\

    copy vesa.c                 %1\

    copy *.h                    %1\
    copy *.inc                  %1\

rem
rem miscellaneous stuff...
rem

    copy midi\opl2\tools\build	      %1\midi\opl2\tools
    copy midi\opl2\tools\credits      %1\midi\opl2\tools
    copy midi\opl2\tools\terms        %1\midi\opl2\tools
    copy midi\opl2\tools\genmid-v.txt %1\midi\opl2\tools
    copy midi\opl2\tools\readme3.txt  %1\midi\opl2\tools
    copy midi\opl2\tools\fatgm        %1\midi\opl2\tools
    copy midi\opl2\tools\b.bat        %1\midi\opl2\tools
    copy midi\opl2\tools\fatv10.bnk   %1\midi\opl2\tools
    copy midi\opl2\tools\build.exe    %1\midi\opl2\tools
    copy midi\opl2\tools\mk.bat       %1\midi\opl2\tools
    copy midi\opl2\tools\bank.h       %1\midi\opl2\tools
    copy midi\opl2\tools\build.c      %1\midi\opl2\tools
    copy midi\opl2\tools\patch.inc    %1\midi\opl2\tools
    copy midi\opl2\patch.asm	      %1\midi\opl2
    copy midi\opl2\opl2.inc	      %1\midi\opl2

rem
rem remove any unwanted material
rem
    if exist %1\midi\opl2\tools\*.zip  del %1\midi\opl2\tools\*.zip

    goto exit

:error
    echo 
    echo "%MISSING%" does not exist! Please try again...

:helps
    echo 
    echo TO USE:  DOS\ SDK [DRIVE:]
    echo 
    echo EXAMPLE: DOS\ SDK B:
    echo 

:exit

