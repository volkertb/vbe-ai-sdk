
;   /*\
;---|*|----====< Play >====----
;---|*|
;---|*| play/record blocks of data
;---|*|
;---|*| Copyright (c) 1993,1994  V.E.S.A, Inc. All Rights Reserved.
;---|*|
;---|*| VBE/AI 1.0 Specification
;---|*|    February 2, 1994. 1.00 release
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

    // compile-time record mode selection to generate a different test pgm

#ifndef RMODE
#define RMODE   0
#endif


;   /*\
;---|*| Global Variables
;   \*/

#define DEFBLEN         0x8000
#define PCMMAXSIZE      65536
        unsigned long   blocklen    = DEFBLEN;
        unsigned long   maxblocklen = PCMMAXSIZE;
        unsigned long   blockinc    = 0;
        unsigned long   PassCount   = 0;
        unsigned long   MaxPasses   = 1;

        int  CallBackOccured    = 0;
        int  ErrorBackOccured   = 0;

        int  AutoInit           = FALSE;
        long AutoDivide         = 8;
        long SampleRate         = 22050;
        long RealSampleRate     = 22050;
        int  PCMSize            = 8;
        int  SaveBuffer         = FALSE;
        int  ReloadRate         = FALSE;
        int  StereoState        = 1;
        int  Compression        = 0;
        int  DriverError        = 0;
        int  VerboseMode        = FALSE;
        int  UserPref           = 0;     // highest level to be used

        VESAHANDLE  hWAVE       = 0;
        BLOCKHANDLE hBLOCK      = 0;

        FILE *rfile;
        char far *dmaptr        = 0;
        char far *memptr        = 0;
        char *PCMFileName;
        int  DelayTicks         = 0;

        GeneralDeviceClass gdc; // receives a copy of the VESA driver info block
        fpWAVServ wfn;          // pointer to wave functions

    // callback routine stores data here...

        int  GlobalHandle       = 0;
        void far *GlobalPtr     = 0;
        long Globallen          = 0;
        void far *OldGlobalPtr  = 0;
        long OldGloballen       = 0;


;   /*\
;---|*| prototypes
;   \*/

        long GetValue               ( char *, long );
        char huge *MakeZeroOffset   ( char huge *, long );
        void far pascal OurCallback  ( int, void far *, long, long );
        void far pascal ErrorCallback( int, void far *, long, long );
        int  readblock              ( int, char huge *, int );
        int  SaveToDisk             ( long );
        int  writeblock             ( int, char huge *, int );
        VESAHANDLE OpenTheDriver    ( int );

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
int loop = TRUE;
int n,c;
int maxticks,ticks;
int NotHalted;
int exitcode = 0;

    // process the command line

        CommandLine (argc,argv);

    // disable the ^C so the devices will close properly.

        signal (SIGINT,SIG_IGN);

    // do all the work to get a VESA driver

        if (!OpenTheDriver(UserPref)) {    // get the user preference
            printf ("Cannot find any installed VAI devices!\n");
            DoExit(-1);
        }

    // this block should play in this amount of time

        maxticks = CalcMaxTicks();

    // set the Sample rate & stuff, then start the next block

        if (!ReloadRate)
            printf ("wsPCMInfo() returned %ld\n",
                (wfn->wsPCMInfo)(StereoState,SampleRate,Compression,0,PCMSize));

        if (!StartNextBlock(0x00))
            BombOut(-1);

    // wait for the block to complete

        while (loop) {

            // show some of the stats on this block while the block is playing

                GiveSomeStats();

            // wait here for an interrupt, or an exit message

                DeltaTime();
                ticks = maxticks;

                while (!CallBackOccured) {

                    if (kbhit()) {

                        switch (c = getch()) {

                            case ' ':  // pause the operation

                                (wfn->wsPauseIO ) (0);
                                printf ("I/O paused. Type any key to restart...");
                                getch();
                                printf ("\n");
                                (wfn->wsResumeIO) (0);
                                ticks = maxticks;
                                break;

                            case 'r':  // restart the block
                            case 'R':

                                //PassCount--; // dont count the current pass
                                NotHalted = TRUE;
                                ticks = maxticks;
                                StartNextBlock(0x01);
                                ReportCallback();
                                break;

                            case 0x1b: // quit the program
                                loop = 0;
                                exitcode = TRUE;

                            default:
                                break;

                        }

                        if (ErrorBackOccured) {
                            printf ("\aWe received the wrong callback!\n");
                            ErrorBackOccured = 0;
                        }
                    }

                    // report the current position in the buffer

                    ReportCurrentPos();

                    // exit immediately if the user wants out...

                    if (!loop)
                        break;

                    // if the driver posts an error, go report it

                    if ( (DriverError = (wfn->wsGetLastError)()) ) {
                        printf ("Driver is reporting an internal error! (code=%d)\n",DriverError);
                        break;
                    }

                    // if we don't get our interrupt, bomb out...

                    if (DeltaTime()) {
                        if (--ticks == 0) {
                            printf ("No interrupt!\n");
                            BombOut(-1);
                        }
                    }
                }

            // report the callback data

                ReportCallback();

            // if we're to stay, do this...

                if (loop) {

                    // after a record, dump some of the data...

                        if (RMODE) {

                            // if we have to prepare the data after record,
                            //do it now...
#if RMODE==1
                            if (gdc.u.gdwi.wifeatures & WAVEPREPARE)
                                (wfn->wsWavePrepare)(0,PCMSize,StereoState,dmaptr,blocklen);
#endif

                            DumpSomeSamples(16);

                            if (SaveBuffer)
                                SaveToDisk(blocklen/AutoDivide);
                        }

                    // delay between blocks if required

                        if(DelayTicks)
                            DelayTimer(DelayTicks);

                    // adjust the new block length, else give up on errors

                        if (blockinc) {

                            if (hBLOCK) // unregister first to adjust length
                                (wfn->wsWaveRegister)(0,hBLOCK);

                            blocklen += blockinc;

                            if (blocklen > maxblocklen)
                                blocklen = maxblocklen - blocklen;

                            if ((signed long)blocklen < 0)
                                blocklen = maxblocklen;

                            maxticks = CalcMaxTicks();

                            if (hBLOCK) // re-register with adjusted length
                                hBLOCK = (wfn->wsWaveRegister)(dmaptr,blocklen);

                        }
                        else {
                            if (DriverError)
                                BombOut(DriverError);
                        }

                    // okay, if the user has typed an escape, exit now...

                        if (kbhit())
                            if (getch() == 0x1b)
                                break;

                    // if not auto-dma, reload the dma & start the block

                        if (!AutoInit) {

                            // quit if now done

                            if (PassCount+1 >= MaxPasses)
                                break;

                            // restart if more to do

                            NotHalted = TRUE;
                            if (!StartNextBlock(0x01))
                                break;

                        }
                     // else
                      //    CallBackOccured = 0;
                }

            // decrement the pass count

                if (loop)
                    if (++PassCount >= MaxPasses)
                        loop = FALSE;

        }

    // exit now...

        DoExit(exitcode);
}


;   /*\
;---|*|----------------------=======================----------------------
;---|*|----------------------====< Subroutines >====----------------------
;---|*|----------------------=======================----------------------
;   \*/

;   /*\
;---|*|----====< BombOut >====----
;---|*|
;---|*| Give the error #, then bomb out...
;---|*|
;   \*/
BombOut(cc)
    int cc;
{

    // give a message

        printf ("Bombing out! code = %d\n",cc);
        DoExit (cc);
}


;   /*\
;---|*|----====< CalcMaxTicks >====----
;---|*|
;---|*| Calculate the number of seconds to wait
;---|*|
;   \*/
CalcMaxTicks()
{
long retval = 0;

    // calc the sample period, then the max time, the get clock ticks

        retval  = 1000000 / RealSampleRate; // sample period
        retval  = retval * (blocklen+1);    // total time in Microseconds
        retval  = retval / 55000;           // divided by 55mill
        retval += 9;                        // give a half second slop time

        return((int)retval);
}


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
int n;
char *s;
int vh,vl;

        vh = VersionControl >> 8;
        vl = VersionControl &  0xFF;

#if RMODE
        printf ("\nVESA VBE/AI WAVE Input Test Program, %02x.%02x\n",vh,vl);
#else
        printf ("\nVESA VBE/AI WAVE Output Test Program, %02x.%02x\n",vh,vl);
#endif
        printf ("Copyright (c) 1993,1994  VESA, Inc. All Rights Reserved.\n\n");

    // select the source/target PCM file name

        if (RMODE)
            PCMFileName = "output.dmp";
        else
            PCMFileName = "guppy.wav";

    // exit in "GiveHelps" if no other parameters

        if (argc == 1)
            GiveHelps();

    // process all the switches...

        n = 1;
        while (n<argc) {

            s = argv[n++];

            if (*s == '-') s++;
            if (*s == '/') s++;

            switch (*s & 0xDF) {

                case '1' & 0xDF:
                    if (*++s == '6') {
                        PCMSize = 16;
                        printf ("16 bit PCM\n");
                    }
                    break;

                case 'A':
                    AutoInit = TRUE;
                    AutoDivide = (int) GetValue (++s,AutoDivide);
                    printf ("Continuous driver I/O modes will be used (size divider=%ld)\n",AutoDivide);
                    break;

                case 'F':
                    PCMFileName = ++s;
                    printf ("Using \"%s\" file for play/record\n",PCMFileName);
                    break;

                case 'D':
                    DelayTicks = (int) GetValue (++s,DelayTicks);
                    printf ("Delay Timer Ticks = %d\n",DelayTicks);
                    break;

                case 'I':
                    blockinc = GetValue (++s,blockinc);
                    printf ("New block inc.   = %ld\n",blockinc);
                    break;

                case 'L':
                    blocklen = GetValue (++s,blocklen) & 0x000FFFFF;
                    printf ("New block length = %ld\n",blocklen);
                    if (maxblocklen < blocklen) {
                        maxblocklen = blocklen;
                        printf ("New max block len= %ld\n",maxblocklen);
                    }
                    break;

#if RMODE
                case 'O':
                    printf ("Saving each buffer to disk!\n");
                    SaveBuffer = 1;
                    if (*++s == '+')
                        SaveBuffer++;
                    break;
#endif

                case 'P':
                    UserPref     = (int)GetValue (++s,0);
                    printf ("Will use user preference level %d\n",UserPref);
                    break;

                case 'R':
                    if (*s == '+') {
                        printf ("Rate is reloaded after each block");
                        ReloadRate = TRUE;
                        s++;
                    }
                    SampleRate = GetValue (++s,(long)SampleRate);
                    printf ("New Sample Rate  = %ld\n",SampleRate);
                    break;

                case 'S':
                    StereoState = 2; // # of channels
                    printf ("Running in stereo mode\n");
                    break;

                case 'T':
                    MaxPasses  = GetValue (++s,MaxPasses );
                    printf ("Number of Passes  = %ld\n",MaxPasses );
                    break;

                case 'V':
                    VerboseMode = TRUE;
                    break;

                default:
                    printf ("Unknown option - %s\n",s);
                    BombOut(-1);
            }
        }

    // if doing auto-init buffer, then make the passes count complete buffers

        if (AutoInit)
            MaxPasses *= AutoDivide;

    // separate the parameter text from the rest of the report

        printf ("\n");
}


;   /*\
;---|*|----====< DelayTimer >====----
;---|*|
;---|*| Delay this many clock ticks
;---|*|
;   \*/
DelayTimer(ticks)
    int ticks;
{
int timer = -1;
int delta = -1;

    // while we have time to waste...

        DeltaTime();

        while (ticks) {

            // watch the keyboard for an exit command

                if (kbhit())
                    if (getch() == 0x1b)
                        DoExit(-1);

            // if the clock ticked, count down one...

                if (DeltaTime())
                    ticks--;

        }

}

;
;   /*\
;---|*|----====< DelayUs >====----
;---|*|
;---|*| Delay this many microseconds
;---|*|
;   \*/
DelayUs(ticks)
    int ticks;
{

    // while we have time to waste...

        _asm {
            mov     cx,ticks
            mov     dx,0x388
        ;
        dlyus05:

            in      al,dx
            loop    dlyus05

        }
}

;
;   /*\
;---|*|----====< DeltaTime >====----
;---|*|
;---|*| Return the delta of clock ticks
;---|*|
;   \*/
DeltaTime()
{
int retval;
static int timer = -1;

    // get the clock tick, save, sub from last tick & return the value

        _asm {
            mov     ah,0
            int     1ah
            xchg    dx,[timer]
            sub     dx,[timer]
            mov     [retval],dx
        }

    // return the delta

        return(retval);
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

    // close the device if already opened

        if (wfn) {

            (wfn->wsStopIO)(0);         // kill any running I/O

            if (hBLOCK)
                (wfn->wsWaveRegister)(0,hBLOCK);

            VESACloseDevice(hWAVE);     // close the device
        }

    // save the output file data to disk

        if (SaveBuffer)
            fclose (rfile);

    // make a distinguishing mark on the screen...

        printf ("\n-------------------------------------------------\n");

    // return to DOS, don't return to caller

        exit(cc);
}


;   /*\
;---|*|----====< DumpSomeSamples >====----
;---|*|
;---|*| Dump some of the DMA'ed samples
;---|*|
;   \*/
DumpSomeSamples(len)
    int len;
{
char far *s;
int n;

    // if we have the pointer, do it...

        if ((s = dmaptr) != 0) {

            for (n=0;n<len;n++) {

                if ((n & 0x0F) == 0)
                    printf ("\n%04x ",n);

                printf ("%02x ",*s++ & 0xFF);

            }
            printf ("\n");
        }
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

#if RMODE
        printf ("\nTo Use:  DOS>rec  [16] [Axxx] [Fxxx] [Dxxx] [Ixxx] [Lxxx] [O{+}] [R] [S] [Txxx]\n\n");
#else
        printf ("\nTo Use:  DOS>play [16] [Axxx] [Fxxx] [Dxxx] [Ixxx] [Lxxx] [R] [S] [Txxx]\n\n");
#endif
        printf ("Where: [16]   enables 16 bit audio (default=8 bit).\n");
        printf ("       [Axxx] uses PlayCont/RecordCont. xxx is block size divisor (default=8).\n");
        printf ("       [Fxxx] user selects a new File by the name of xxx.\n");
        printf ("       [Dxxx] allows for delays between blocks of xxx clock ticks (18.2tps).\n");
        printf ("       [Ixxx] after each block, Increment the next length by xxx.\n");
        printf ("       [Lxxx] block Length of first pcm block.\n");

#if RMODE
        printf ("       [O{+}] Save one recorded block. '+' saves all blocks.\n");
#endif
        printf ("       [R]    Reload the sample rate after each block finishes.\n");
        printf ("       [S]    selects Stereo mode operation.\n");
        printf ("       [Txxx] number of Times to play the block.\n");

        exit(0);
}


;   /*\
;---|*|----====< GiveSomeStats >====----
;---|*|
;---|*| Print some useful information to the user
;---|*|
;   \*/
GiveSomeStats()
{
register char far *vidptr = (char far *)0xb8000000;
char sstr[80],*s;

    // create the string

        sprintf (sstr,"pass=%5lu len=%lx ",PassCount,blocklen);

    // blast it out...

        s = sstr;
        while (*s) {
            *vidptr++ = *s++;
            *vidptr++ = 0x3F;
        }
}


;   /*\
;---|*|----====< LoadPCMFile >====----
;---|*|
;---|*| Load the pcm file into the DMA buffer
;---|*|
;   \*/
LoadPCMFile()
{
char huge *t;
int han,n;
long l;

    // get some memory to hold the audio data

        l = maxblocklen;

        if (AutoInit)
            l <<= 1;

        if ((dmaptr = t = AllocateBuffer( l+16 )) == 0) {
            printf ("Unable to allocate DMA buffer memory!\n");
            return (FALSE);
        }

    // if in autoinit mode, we have to guarrantee no 64k crossings

        if (AutoInit)
            dmaptr = t = MakeZeroOffset(t,maxblocklen);

    // load a triangle wave into the buffer

        for (l=maxblocklen>>1;l;l--)
            *t++ = l & 0xff;

        for (l=maxblocklen>>1;l;l--)
            *t++ = (255 - (l & 0xff));

#if RMODE==0

    // open the file. If we can't read it, then forget it...

        if ((rfile = fopen (PCMFileName,"rb")) == 0) {
            printf ("Unable to load the PCM file named \"%s\"\n",PCMFileName);
            return (FALSE);
        }

    // read the entire file into the DMA buffer

        han = fileno(rfile);

    // flush the header from the file

        readblock (han,(t=dmaptr),0x20);// read past the .VOC header

        if ((*(int far *)t) == 0x4952)
            readblock (han,t,0x0C);     // read more for a .WAV header

    // read the entire file into a far pointer

        for (l=maxblocklen;l>0;l-=0x1000) {

            if (readblock (han,t,0x1000) != 0x1000)
                break;

            t+= 0x1000;
        }

        if (l < 0)
            readblock (han,t,(int)l + 0x1000);

        fclose (rfile);

#else

    // open the file for output

        if ((rfile = fopen (PCMFileName,"wb")) == 0) {
            printf ("Unable to load the PCM file named \"%s\"\n",PCMFileName);
            return (FALSE);
        }

#endif

    // register the block with the driver.

        if (!AutoInit) {
            if ((hBLOCK = (wfn->wsWaveRegister)(dmaptr,blocklen)) == 0) {
                printf ("Block registration failed! error=%02x\n",(wfn->wsGetLastError)());
                return (FALSE);
            }
        }

    // all is okay...

        return(TRUE);
}

;
;   /*\
;---|*|----====< MakeZeroOffset >====----
;---|*|
;---|*| Make the buffer address linear, with a zero offset,
;---|*| not crossing a 64k boundary! (whew!). The true address pointer must
;---|*| be pointing to a buffer of the length of len*2+16
;---|*|
;   \*/
char huge *MakeZeroOffset (addr,len)
    char huge *addr;
    long len;
{
long l;

        _asm {
            mov     ax,word ptr [addr+0]
            mov     dx,word ptr [addr+2]

            mov     bx,0xFFF0               // get the linear address
            rol     dx,4
            and     bx,dx
            xor     dx,bx
            add     ax,bx
            adc     dx,0

            and     ax,0xFFF0               // go to the next 16 byte boundary
            add     ax,0x0010

            mov     bx,dx                   // check to see if we wrap a
            mov     cx,ax                   // 64k boundary
            add     cx,word ptr [len+0]
            adc     bx,word ptr [len+2]     // bx = dx, or bx = dx + 1

            sub     bx,dx                   // bx = 0x0000 if same, else 0x0001
            neg     bx                      // bx = 0x0000 if same, else 0xFFFF
            sub     dx,bx                   // add 1 if it wrapped
            not     bx                      // bx = 0xFFFF if same, else 0x0000
            and     ax,bx                   // flush if wrapped, else save

            ror     dx,4                    // make a seg:off out of the linear
            add     dx,ax                   // address
            sub     ax,ax

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

    // just for fun, report the driver state

        printf ("The driver state = %x\n",(wfn->wsDeviceCheck) (WAVEDRIVERSTATE,0) );

    // tell the user how close the driver comes to the requested sample rate

        if ( !(l = (wfn->wsDeviceCheck)(WAVESAMPLERATE,SampleRate)) )
            printf ("The driver does not support a %ldhz sample rate!\n",RealSampleRate=SampleRate);
        else
            printf ("The drivers closest playback sample rate is %ldhz!\n",RealSampleRate=l);

    // setup the record and playback callbacks

        if (RMODE) {
            wfn->wsApplRSyncCB = &OurCallback;
            wfn->wsApplPSyncCB = &ErrorCallback;
        }
        else {
            wfn->wsApplPSyncCB = &OurCallback;
            wfn->wsApplRSyncCB = &ErrorCallback;
        }

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

        }

    // save for later reporting...

        OldGlobalPtr = GlobalPtr;
        OldGloballen = Globallen;

        GlobalHandle = han;
        GlobalPtr    = fptr;
        Globallen    = len;
        CallBackOccured++;

    // we're done here...

        _asm {
            pop     ds
        }
}


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
;---|*|----====< ReportCallback >====----
;---|*|
;---|*| Print some useful information to the user
;---|*|
;   \*/
ReportCallBack()
{
register char far *vidptr = (char far *)0xb80000A0;
char sstr[80],*s;

static int cbcnt = 0;

    // flush the callback indicator

        CallBackOccured = 0;

    // create the string

        sprintf (sstr,"%5d han=%5d ptr=%8lx len=%8ld ",cbcnt++,GlobalHandle,GlobalPtr,Globallen);

    // blast it out...

        s = sstr;
        while (*s) {
            *vidptr++ = *s++;
            *vidptr++ = 0x3F;
        }
}


;   /*\
;---|*|----====< ReportCurrentPos >====----
;---|*|
;---|*| Report the current block position reported via a DeviceCheck call
;---|*|
;   \*/
ReportCurrentPos()
{
register char far *vidptr = (char far *)0xb8000140;
char sstr[80],*s;
long length;

static int cbcnt = 0;

    // create the string

   #if 0
        _asm {
            mov     dx,0xb8a
            mov     cx,1024
        waithere:
            in      al,dx
            loop    waithere

            push    ax

            and     al,0x3F
            out     dx,al

        }
   #endif

        length = (wfn->wsDeviceCheck) (WAVEGETCURRENTPOS,0);
        sprintf (sstr,"%5d han=%5d offset=%8lx",cbcnt++,GlobalHandle,length);

    // blast it out...

        s = sstr;
        while (*s) {
            *vidptr++ = *s++;
            *vidptr++ = 0x3F;
        }

   #if 0
        _asm {
            mov     dx,0xb8a
            pop     ax
            out     dx,al
        }
   #endif

}


;   /*\
;---|*|----====< SaveToDisk >====----
;---|*|
;---|*| Save the buffer to disk
;---|*|
;   \*/

SaveToDisk(len)
    long len;
{
char huge *s;
int han,n;
long l;

    // if we have the pointer, do it...

        n = fileno(rfile);

    // if there is a buffer, write the data

        if ((s=dmaptr) != 0) {

            // point to the start of the file unless the user wants everything

            if (SaveBuffer != 2) {

                _asm {

                    mov     ax,4200h    ; point to the start of the file
                    mov     bx,[n]      ; get the handle
                    sub     cx,cx
                    sub     dx,dx
                    int     21h         ; file points to zero
                }
            }

            // write the data to disk

            han = fileno(rfile);

            for (l=len;l>0;l-=0x1000) {

                if (writeblock (han,s,0x1000) != 0x1000)
                    break;

                s+= 0x1000;
            }

            if (l < 0)
                writeblock (han,s,(int)l + 0x1000);
        }
}


;   /*\
;---|*|----====< StartNextBlock >====----
;---|*|
;---|*| Just read the buffer during the record process
;---|*|
;   \*/

int StartNextBlock(flag)
    int flag;
{
int retval;

static int pass = 0;

    // on the 1st pass, load the playback data

        if (pass == 0) {

            // load the file into the DMA buffer, get the delay ticks

            if (!LoadPCMFile()) {
                return(FALSE);
                pass++;
            }


        }
        else {

    // else, every other pass, possibly setup the transfer type and rate

            if (ReloadRate)
                printf ("wsPCMInfo() returned %ld\n",
                    (wfn->wsPCMInfo)(StereoState,SampleRate,Compression,0,PCMSize));

        }

    // if we have to prepare the data for playback, do it now...

#if RMODE==0
        if (pass == 0) {
            if (gdc.u.gdwi.wifeatures & WAVEPREPARE)
            (wfn->wsWavePrepare)(0,PCMSize,StereoState,dmaptr,blocklen);
        }
#endif


    // flush the old IRQ occured flag

     // CallBackOccured = 0;

#if RMODE==0

    // let it rip

        if (AutoInit)
            retval = (wfn->wsPlayCont)    (dmaptr,blocklen,blocklen/AutoDivide);
        else
            retval = (wfn->wsPlayBlock)   (hBLOCK,0);

#else

    // let it rip

        if (AutoInit)
            retval = (wfn->wsRecordCont)  (dmaptr,blocklen,blocklen/AutoDivide);
        else
            retval = (wfn->wsRecordBlock) (hBLOCK,0);

#endif

        if ( (DriverError = (wfn->wsGetLastError)()) )
            printf ("Driver is reporting an internal error! (code=%d)\n",DriverError);

        pass++;
        return(retval);

}


;   /*\
;---|*|----====< writeblock >====----
;---|*|
;---|*| write a chunk of the PCM buffer to the disk file.
;---|*|
;   \*/
int writeblock (han,tptr,len)
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

            mov     ah,040h             // cx holds the length
            mov     bx,[han]
            lds     dx,[tptr]
            int     21h

            mov     [siz],ax            // we moved this much
            add     word ptr [tptr+2],0x1000

            cmp     ax,cx               // same size?
            jnz     rdbldone            // no, exit...
        ;
        rdbl05:

            mov     ah,040h
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





