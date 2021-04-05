
;   /*\
;---|*|----====< Play >====----
;---|*|
;---|*| play small a list of wave files
;---|*|
;---|*| Copyright (c) 1993,1994  V.E.S.A, Inc. All Rights Reserved.
;---|*|
;---|*| VBE/AI 1.0 Specification
;---|*|    April 6, 1994. 1.00 release
;---|*|
;   \*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <io.h>

#include "vbeai.h"
#include "vesa.h"


;   /*\
;---|*| Global Variables
;   \*/

        int  CallBackOccured  = 0;
        int  ErrorBackOccured = 0;

        VESAHANDLE  hWAVE   = 0;
        FILE *rfile;
        char far *memptr    = 0;

        GeneralDeviceClass gdc; // receives a copy of the VESA driver info block
        fpWAVServ wfn;          // pointer to wave functions


;   /*\
;---|*| Local Variables
;   \*/

        typedef struct {
            char *fname;        // file name
            char huge *ptr;     // pointer to the buffer
            long len;           // file length
            int  han;           // registered handle
            long sr;            // sample rate
            int  ch;            // channels
            int  sz;            // data size (8 or 16)
            int  cp;            // compression
        } wave;

        int MaxWaves   =  0;    // maximum number of registered waves
        wave Waves[10] = {0};   // up to 10 registered waves

    // 4. Wave format control block

        typedef struct {
            int  formatTag;     // format category
            int  nChannels;     // stereo/mono
            long nSamplesPerSec;    // sample rate
            long nAvgBytesPerSec;   // stereo * sample rate
            int  nBlockAlign;       // block alignment (1=byte)
            int  nBitsPerSample;    // # byte bits per sample
        } WaveInfo;

    // 3. Wave detailed information Block

        typedef struct {
            char name[4];   // "fmt "
            long length;
            WaveInfo info;
        } WaveFormat;

    // 3. Data header which follows a WaveFormat Block

        typedef struct {
            char name[4];   // "data"
            unsigned long length;
        } DataHeader;

    // 2. Total Wave Header data in a wave file

        typedef struct {
            char name[4];   // "WAVE"
            WaveFormat fmt;
            DataHeader data;
        } WaveHeader;

    // 2. Riff wrapper around the WaveFormat Block (optional)

        typedef struct {
            char name[4];   // "RIFF"
            long length;
        } RiffHeader;

    // 1. Riff wrapped WaveFormat Block

        typedef struct {
            RiffHeader riff;
            WaveHeader wave;
        } RiffWave;

#define VALIDWAVE           0
#define UNKNOWNFILETYPE     1

;   /*\
;---|*| prototypes
;   \*/

        void far pascal OurCallback  ( int, void far *, long, long );
        void far pascal ErrorCallback( int, void far *, long, long );
        int  readblock               ( int, char huge *, int );
        VESAHANDLE OpenTheDriver     ( int );
        int ProcessWAVHeader         ( FILE *, int );

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
int idx;
int n;

    // process the command line

        CommandLine (argc,argv);

    // disable the ^C so the devices will close properly.

        signal (SIGINT,SIG_IGN);

    // do all the work to get a VESA driver

        if (!OpenTheDriver(0)) {    // open the highest driver
            printf ("Cannot find any installed VAI devices!\n");
            DoExit(-1);
        }

    // Register all the blocks

        for (n=0;n<MaxWaves;n++)
            Waves[n].han =
                (wfn->wsWaveRegister) (Waves[n].ptr,Waves[n].len);

    // present a list and play each sound as required

        while (1) {

            if ((idx = GetOption()) == -1)
                break;

            CallBackOccured = 0;    // no callbacks yet

            (wfn->wsPCMInfo)        // set the sample rate, etc.
              (
                Waves[idx].ch,
                Waves[idx].sr,
                Waves[idx].cp,
                0,
                Waves[idx].sz
              );

            (wfn->wsPlayBlock)(Waves[idx].han,0); // start output

            if (!DriverError()) {

                printf ("Type ESC to stop, SPACE to pause ");

                while (!CallBackOccured) {

                    if (kbhit()) {

                        switch (getch()) {

                            case 0x1b:  // escape
                                CallBackOccured++;
                                (wfn->wsStopIO)(Waves[idx].han);
                                break;

                            case 0x20:  // space
                                (wfn->wsPauseIO)(Waves[idx].han);
                                printf ("\nType SPACE to continue ");
                                while (getch() != 0x20) ;
                                (wfn->wsResumeIO)(Waves[idx].han);
                                break;

                            default:
                                break;
                        }
                    }
                }
            }
        }


    // exit now...

        DoExit(0);
}


;   /*\
;---|*|----------------------=======================----------------------
;---|*|----------------------====< Subroutines >====----------------------
;---|*|----------------------=======================----------------------
;   \*/

;
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
int n,fhan;
int vh,vl;
char str[100];

        vh = VersionControl >> 8;
        vl = VersionControl &  0xFF;

        printf ("\nVESA VBE/AI WAVE Output Program, %02x.%02x\n",vh,vl);
        printf ("Copyright (c) 1993,1994  VESA, Inc. All Rights Reserved.\n\n");

    // exit in "GiveHelps" if no other parameters

        if (argc == 1) {
            printf ("\nTo Use:  DOS>play [file.wav] [file.wav] [file.wav]...\n\n");
            printf ("Where: [file.wav] is a list of up to 10 files to play.\n");
            DoExit(0);
        }

    // process all the switches...

        MaxWaves = 0;
        while (MaxWaves+1<argc) {

            Waves[MaxWaves].fname = argv[MaxWaves+1];

            if ((rfile = fopen (Waves[MaxWaves].fname,"rb")) == 0) {

                strcpy (str,Waves[MaxWaves].fname);
                strcat (str,".WAV");

                if ((rfile = fopen (str,"rb")) == 0) {
                    printf ("cannot open the WAVE file named \"%s\"\n",Waves[MaxWaves].fname);
                    DoExit(1);
                }
            }
            fseek (rfile,0,SEEK_SET);   // point to the start

            if (ProcessWAVHeader(rfile,MaxWaves) != VALIDWAVE) {
                printf ("Invalid .WAV file: \"%s\"\n",&str[0]);
                DoExit(1);
            }

            Waves[MaxWaves].ptr   = AllocateBuffer(Waves[MaxWaves].len);

            if (Waves[MaxWaves].ptr == 0) {
                printf ("Ran out of memory loading the files!\n");
                DoExit(1);
            }

            readblock
              (
                fileno(rfile),
                Waves[MaxWaves].ptr,
                (int)Waves[MaxWaves].len
              );

            fclose (rfile);
            MaxWaves++;
        }
}

;
;   /*\
;---|*|----====< DoExit >====----
;---|*|
;---|*| Shut everything down, then exit to DOS
;---|*|
;   \*/

DoExit(cc)
    int cc;
{

    // close the device if already opened

        if (wfn)
            VESACloseDevice(hWAVE);     // close the device

    // return to DOS, don't return to caller

        exit(cc);
}


;
;   /*\
;---|*|----====< DriverError >====----
;---|*|
;---|*| Report any errors by the driver
;---|*|
;   \*/

int DriverError()
{
int err;

    if ( (err = (wfn->wsGetLastError)()) ) {
        printf ("Driver reported an error! (code=%02X)\n",err);
        return (err);
    }
    return(0);
}

;
;   /*\
;---|*|----====< ErrorCallback >====----
;---|*|
;---|*| If we get this, we received the wrong callback!
;---|*|
;   \*/

void far pascal ErrorCallback( han, fptr, len, filler )
    int han;        // device handle
    void far *fptr; // buffer that just played
    long len;       // length of completed transfer
    long filler;    // reserved...
{

    // setup our data segment

        _asm {

            push    ds
            mov     ax,seg ErrorBackOccured
            mov     ds,ax
            inc     [ErrorBackOccured]
            pop     ds
        }
}

;
;   /*\
;---|*|----====< GetOption >====----
;---|*|
;---|*| Ask the user for a file to be played.
;---|*|
;   \*/
GetOption()
{
char str[100];
int n;

    // print all the files

        printf ("\n  0. Exit\n");
        for (n=0;n<MaxWaves;n++)
            printf ("  %d. %s\n",n+1,Waves[n].fname);

    // ask the user for a file name

        while (1) {

            printf ("\nEnter an option # ");
            fgets  (str,99,stdin);

            if (sscanf(str,"%d",&n) == 1)
                if (n<=MaxWaves)
                    return(n-1);

            // if an out of range #, we'll keep trying

        }
}

;
;   /*\
;---|*|----====< OpenTheDriver >====----
;---|*|
;---|*| Find the driver with the highest user preference, and return it to
;---|*| the caller
;---|*|
;   \*/
int OpenTheDriver(pref)
    int pref;
{
int driverpref = 256;   // real low preference
long l;

    // find a matching driver

        do {

            // Find one DAC device, else bail if non found

                if ((hWAVE = VESAFindADevice(WAVDEVICE)) == 0)
                    return(0);

            // get the device information

                if (VESAQueryDevice(hWAVE, VESAQUERY2 ,&gdc) == 0) {
                    printf ("Cannot query the installed VAI devices!\n");
                    DoExit(-1);
                }

            // make sure its a wave device

                if (gdc.gdclassid != WAVDEVICE) {
                    printf ("The VESA find device query returned a NON DAC device!\n");
                    DoExit(-1);
                }

            // make sure it's matches the beta version #

                if (gdc.gdvbever != VersionControl) {
                    printf ("The VESA device version # does not match, cannot continue!\n");
                    DoExit(-1);
                }

            // get the drivers user preference level

                driverpref = gdc.u.gdwi.widevpref;

            // if the caller is not expressing a preference, then use this one

                if (pref == -1)
                    break;

        } while (driverpref != pref);

    // get the memory needed by the device

        if (!(memptr = AllocateBuffer(gdc.u.gdwi.wimemreq))) {
            printf ("We don't have memory for the device!\n");
            DoExit(-1);
        }

    // if the DAC device doesn't open, bomb out...

        if ((wfn = (fpWAVServ)VESAOpenADevice(hWAVE,0,memptr)) == 0) {
            printf ("Cannot Open the installed devices!\n");
            DoExit(-1);
        }

    // setup the record and playback callbacks

        wfn->wsApplPSyncCB = &OurCallback;
        wfn->wsApplRSyncCB = &ErrorCallback;

    // return the handle

        return(hWAVE);
}


;   /*\
;---|*|----====< OurCallback >====----
;---|*|
;---|*| Block End callback. NOTE: No assumptions can be made about
;---|*| the segment registers! (DS,ES,GS,FS)
;---|*|
;   \*/

void far pascal OurCallback( han, fptr, len, filler )
    VESAHANDLE han; // device handle
    void far *fptr; // buffer that just played
    long len;       // length of completed transfer
    long filler;    // reserved...
{

    // setup our data segment

        _asm {

            push    ds
            mov     ax,seg CallBackOccured
            mov     ds,ax
            inc     [CallBackOccured]
            pop     ds
        }
}

;
;   /*\
;---|*|----====< ProcessWAVHeader >====----
;---|*|
;---|*| load the header from our WAV file format
;---|*|
;   \*/

int ProcessWAVHeader(f,idx)
    FILE *f;
    int idx;
{
int n;
RiffWave rw;

    // eat the RIFF portion of the header

        readblock ( fileno(f), (void far *)&rw, sizeof (RiffWave) );

    // make sure its says RIFF

        n  = rw.riff.name[0] - 'R';
        n += rw.riff.name[1] - 'I';
        n += rw.riff.name[2] - 'F';
        n += rw.riff.name[3] - 'F';

        if (n)
            return(UNKNOWNFILETYPE);

    // make sure it says WAVE

        n  = rw.wave.name[0] - 'W';
        n += rw.wave.name[1] - 'A';
        n += rw.wave.name[2] - 'V';
        n += rw.wave.name[3] - 'E';

        if (n)
            return(UNKNOWNFILETYPE);

    // make sure it says 'fmt '

        n  = rw.wave.fmt.name[0] - 'f';
        n  = rw.wave.fmt.name[1] - 'm';
        n  = rw.wave.fmt.name[2] - 't';
        n  = rw.wave.fmt.name[3] - ' ';

        if (n)
            return(UNKNOWNFILETYPE);

        Waves[idx].ch    = rw.wave.fmt.info.nChannels;
        Waves[idx].sr    = rw.wave.fmt.info.nSamplesPerSec;
        Waves[idx].sz    = rw.wave.fmt.info.nBitsPerSample;
        Waves[idx].cp    = 0;

    // make sure it says 'data'

        n  = rw.wave.data.name[0] - 'd';
        n  = rw.wave.data.name[1] - 'a';
        n  = rw.wave.data.name[2] - 't';
        n  = rw.wave.data.name[3] - 'a';

        if (n)
            return(UNKNOWNFILETYPE);

        Waves[idx].len   = rw.wave.data.length;

    // return OKAY

        return(VALIDWAVE);

}

;
;   /*\
;---|*|----====< readblock >====----
;---|*|
;---|*| read a chunk of the PCM file into the huge buffer
;---|*|
;   \*/
int readblock (han,tptr,len)
    int han;
    char huge *tptr;
    int len;
{
int siz = 0;

    // go get it...

        _asm {
            push    ds

            mov     cx,[len]
            mov     ax,cx
            add     cx,word ptr [tptr]  // wrap?
            jnc     rdbl05              // no, go get the size

            sub     ax,cx               // ax holds the # of bytes to read
            mov     cx,ax
            sub     [len],ax

            mov     ah,03fh             // cx holds the length
            mov     bx,[han]
            lds     dx,[tptr]
            int     21h

            mov     [siz],ax            // we moved this much
            add     word ptr [tptr+2],0x1000

            cmp     ax,cx               // same size?
            jnz     rdbldone            // no, exit...
        ;
        rdbl05:

            mov     ah,03fh
            mov     bx,[han]
            mov     cx,[len]

            jcxz    rdbldone

            lds     dx,[tptr]
            int     21h

            add     [siz],ax            // we moved this much
        ;
        rdbldone:
            pop     ds

        }

    // return the amount read

        return(siz);
}

;   /*\
;---|*| end of PLAY.C
;   \*/




