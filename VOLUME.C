
;   /*\
;---|*|----====< Vol.exe >====----
;---|*|
;---|*| Test a volume device
;---|*|
;---|*| Copyright (c) 1993,1994  V.E.S.A, Inc. All Rights Reserved.
;---|*|
;---|*| VBE/AI 1.0 Specification
;---|*|    February 2, 1994. 1.00 release
;---|*|
;---|*| Additional Changes:
;---|*|    02/28 - If no parameters are provided, the program prints the
;---|*|            helps, then exits to DOS.
;---|*|
;   \*/

#include <stdio.h>
#include <signal.h>

#include "vbeai.h"
#include "vesa.h"

#define TRUE    -1
#define FALSE    0
#define ON      TRUE
#define OFF     FALSE

        VESAHANDLE hVESA  = 0;

        GeneralDeviceClass gdc; // receives a copy of the VESA driver info block

    // tables for reporting attached volume info and services

        VolumeInfo volinf;
        VolumeService volsrv;


;   /*\
;---|*| prototypes
;   \*/

        long GetValue   ( char *, long );
        int DriverError ( );

    // only allows us to run on this version of the VBE interface. This
    // will be removed after the interface is ratified. Changes may be made
    // that would cause this code to crash if run on other version. Again,
    // this will be removed in the final software.

        int  VersionControl = 0x0100;


;   /*\
;---|*|------------------==============================-------------------
;---|*|------------------====< Start of execution >====-------------------
;---|*|------------------==============================-------------------
;   \*/

main(argc,argv)
    int argc;
    char *argv[];
{
int query;
int pref;

    // process the command line

        CommandLine (argc,argv);

    // disable the ^C so the devices will close properly.

        signal (SIGINT,SIG_IGN);

    // process the specific device handle

        while (TRUE) {

            // make sure there is such a handle

                if (VESAQueryDevice(hVESA, VESAQUERY1 ,0) == 0) {
                    printf ("Cannot query the installed VBE/AI devices!\n");
                    DoExit(-1);
                }

            // make sure it's matches the beta version #

                VESAQueryDevice(hVESA, VESAQUERY2 ,&gdc);

                if (gdc.gdvbever != VersionControl) {
                    printf ("The VESA device version # does not match, cannot continue!\n");
                    break;
                }

            // if we could'nt get the services, just bail now.

                if (!GetAttachedVolumes(hVESA)) {
                    printf ("There are no volume services for this handle!");
                    break;
                }

            // perform the tests...

                PerformTests();
                break;
        }

        DoExit(0);
}


;   /*\
;---|*|----------------------=======================---------------------------
;---|*|----------------------====< Subroutines >====---------------------------
;---|*|----------------------=======================---------------------------
;   \*/

;   /*\
;---|*|----====< CommandLine >====----
;---|*|
;---|*| Process the command line switches
;---|*|
;   \*/
CommandLine(argc,argv)
    int argc;
    char *argv[];
{
int n,vh,vl;
char *s,c;

    // split the version for neatness...

        vh = VersionControl >> 8;
        vl = VersionControl &  0xFF;

    // intro...

        printf ("\nVESA VBE/AI Volume Test Program, %02x.%02x\n",vh,vl);
        printf ("Copyright (c) 1993,1994  VESA, Inc. All Rights Reserved.\n\n");

    // exit if no other parameters

        if (argc < 2) {
            GiveHelps();
            DoExit(0);
        }

    // process all the switches

        n = 1;
        while (n<argc) {

            s = argv[n++];

            switch (*s) {

                case '?':
                case 'H':   // both helps and handle
                case 'h':

                    hVESA  = GetValue (++s,0);  // maybe get the handle

                    if (hVESA) {
                        printf ("Will use handle #%d\n\n",hVESA);
                    }
                    else {

                        GiveHelps();
                        DoExit(0);
                    }
                    break;

                default:
                    printf ("Unknown option - %s\n",s);
            }
        }

}


;   /*\
;---|*|----====< DoExit >====----
;---|*|
;---|*| Shut everything down, then exit to DOS
;---|*|
;   \*/
DoExit(cc)
    int cc;
{

    // return to DOS, don't return to caller

        exit(cc);
}

;
;   /*\
;---|*|----====< GetAttachedVolumes >====----
;---|*|
;---|*| locate any volume info and services structures for the
;---|*| device handle
;---|*|
;   \*/
GetAttachedVolumes(han)
    VESAHANDLE han;
{
fpVolInfo vi;
fpVolServ vs;

    // if there is a valid volume info structure, print it...

        if (VESAQueryDevice ( han, VESAQUERY3, 0 )) {

            if (vi = (fpVolInfo)VESAQueryDevice ( han, VESAQUERY4, &volinf )) {

            // if there is a valid volume info structure, print it...

                if (VESAQueryDevice ( han, VESAQUERY5, 0 )) {

                    if (vs = (fpVolServ)VESAQueryDevice ( han, VESAQUERY6, &volsrv ))

                        return(TRUE);

                }
            }
        }

    // could'nt find both info and services

        return(0);

}

;
;   /*\
;---|*|----====< PerformTests >====----
;---|*|
;---|*| Perform the hardware any and all tests
;---|*|
;   \*/
PerformTests()
{
int n;
long l;
int left,right;

    // Reset the channel to it's default state

        (*volsrv.vsResetChannel)  ( );
        DriverError();

    // SetVolume tests

        // show & modify the user's settings

        l = (*volsrv.vsSetVolume) ( VOL_USERSETTING, -1, -1 );
        left  = (int) (l      & 0xFFFF);
        right = (int)((l>>16) & 0xFFFF);
        printf ("Current user settings are: left=%d%%, right=%d%%\n",left,right);
        DriverError();

        printf ("Setting the user left/right to 50%\n");
        (*volsrv.vsSetVolume) ( VOL_USERSETTING, 50, 50 );
        DriverError();

        // show & modify the applications setting

        l = (*volsrv.vsSetVolume) ( VOL_APPSETTING, -1, -1 );
        left  = (int) (l      & 0xFFFF);
        right = (int)((l>>16) & 0xFFFF);
        printf ("Current app settings are: left=%d%%, right=%d%%\n",left,right);
        DriverError();

        printf ("Setting the user left/right to 0%\n");
        (*volsrv.vsSetVolume) ( VOL_APPSETTING, volinf.vimin, volinf.vimin );
        DriverError();

    // 3D Set Field Volume tests

        printf ("Setting the 3D audio somewhere...\n");
        (*volsrv.vsSetFieldVol)   ( 250, 900, 450 );
        DriverError();

    // Parametric Equalizer tests

        printf ("Parametric Eqaulizer test\n");
        (*volsrv.vsToneControl)   ( 1, 2500, 128, 5 );
        DriverError();

    // Hi & Low pass filter tests

        printf ("Hi & Low pass filter tests\n");
        (*volsrv.vsFilterControl )( 1,  8000, 128 );
        DriverError();
        (*volsrv.vsFilterControl )( 2, 16000, 128 );
        DriverError();

    // Record/Playback path control

        printf ("Record/Playback path control tests\n");
        (*volsrv.vsOutputPath)    ( TRUE );
        DriverError();
        (*volsrv.vsOutputPath)    ( FALSE );
        DriverError();

    // Reset the channel to it's default state

        (*volsrv.vsResetChannel)  ( );
        DriverError();

}

;   /*\
;---|*|-------------------====< Internal Routines >====----------------------
;   \*/

;   /*\
;---|*|----====< DriverError >====----
;---|*|
;---|*| Query for any errors, report results.
;---|*|
;   \*/
int DriverError()
{
int n;

    // return any error found in the driver.

        if ((n = (*volsrv.vsGetLastError) ( )) != 0) {
            printf ("  VOLUME ERROR, message = %x\n",n);
            return(n);
        }

    // else just return NULL

        return(0);
}

;   /*\
;---|*|----====< GetKey >====----
;---|*|
;---|*| Get a key from the keyboard
;---|*|
;   \*/
int GetKey(flag)
    int flag;
{
int c;

    // flush the keys coming in

        if (flag)
            while (kbhit())
                getch();

    // get the real key

        if ((c = getch()) == 0)
            c = getch();

    // return to the caller

        return(c);
}


;   /*\
;---|*|----====< GetValue >====----
;---|*|
;---|*| Return a value from the string, or the last value
;---|*|
;   \*/
long GetValue (s,orig)
    char * s;
    long orig;
{
long w;
int NegateFlag = FALSE;

    // if the first character is negative, then set out neg flag

        if (*s == '-') {
            s++;
            NegateFlag = TRUE;
        }

    // check the first character, if zero, then it's octal or hex

        if (*s == '0') {

            w = 0;

            if ((*++s & 0x5F) == 'X') {

                if (sscanf (++s,"%lx",&w) != 1)
                    w = orig;

            }
            else {
                if (isdigit(*s)) {

                    if (sscanf (s,"%lo",&w) != 1)
                        w = orig;
                }
                else {
                    w = 0;
                }
            }
        }

    // return a decimal value

        else {

            if (sscanf (s,"%ld",&w) != 1)
                w = orig;

        }

    // we have something...

        if (NegateFlag)
            w = 0 - w;

        return(w);

}


;   /*\
;---|*|----====< GiveHelps >====----
;---|*|
;---|*| Give the user the commandline option list
;---|*|
;   \*/

GiveHelps()
{

        printf ("To Use: DOS>VOL [H|?] [Hxx]\n\n");

        printf ("Where:     [H|?]      gives helps.\n");
        printf ("           [Hxx]      specifies the device handle to test.\n");
        printf ("\n");

}

;   /*\
;---|*| end of VOL.C
;   \*/




