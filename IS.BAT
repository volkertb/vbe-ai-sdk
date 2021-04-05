    @echo off
rem
rem test the presents of the hardware & software using the VBE/AI driver
rem
    if (%1)==() goto helps

    %1 /V >nul
    if ERRORLEVEL,2 goto hwsw
    if ERRORLEVEL,1 goto hwnosw
    if ERRORLEVEL,0 goto nohw
    goto unkn

:hwsw
    echo there is both hardware and a VBE/AI driver.
    goto done

:hwnosw
    echo there is hardware without a VBE/AI driver.
    goto done

:nohw
    echo there is no hardware.
    goto done

:helps
    echo 
    echo Test to see if hardware and VBE/AI driver are present
    echo 
    echo To Use:  DOS\ is [driver]
    echo 
    echo Where:   [driver] is the name of a VBE/AI executable driver.
    echo 

    goto done
:unkn
    echo the driver's response was invalid!

:done


