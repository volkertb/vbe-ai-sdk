
;   /*\
;---|*|----====< Play MIDI >====----
;---|*|
;---|*| play/record blocks of MIDI
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
#include "midi.h"

#define TRUE    -1
#define FALSE    0
#define ON      TRUE
#define OFF     FALSE


;   /*\
;---|*| External Variables
;   \*/

        extern char *melodicnames[];
        extern char *percussivenames[];


;   /*\
;---|*| Global Variables
;   \*/

        VESAHANDLE hMIDI  = 0;

        GeneralDeviceClass gdc; // receives a copy of the VESA driver info block
        fpMIDServ msv;          // pointer to wave functions


;   /*\
;---|*| prototypes
;   \*/

        long readvalue              ( );
        long GetValue               ( char *,long );
        VESAHANDLE OpenTheDriver    ( int );

        char far *memptr = 0;
        char far *pptr;         // patch pointer

        char notes[16] = {0};
        int  patch     = 0x01;
        int  channel   = 0x01;
        int  UserPref  = 0;
        int  AutoNoteOff = TRUE;

        void far pascal OurMIDIReceiver   ( int, int, char, long );
        void far pascal OurFreeCBCallBack ( int, int, void far *, long );

        int  DelayTimer ( int, int );

#define DN_LOADPATCH    0x01
#define DN_NOTEOFF      0x40
#define DN_NOTEON       0x30
#define DN_PITCHUP      0x04
#define DN_PITCHDN      0x05
#define DN_PITCHCNTR    0x06
#define DN_VELUP        0x07
#define DN_VELDOWN      0x08
#define DN_ERRORCHK     0xEE

        int DoNote(int);


;   /*\
;---|*|------------------==============================-------------------
;---|*|------------------====< Start of execution >====-------------------
;---|*|------------------==============================-------------------
;   \*/

main(argc,argv)
    int argc;
    char *argv[];
{
int c;
int loop = -1;
int noteison[10];
int n;

    // process the command line

        CommandLine (argc,argv);

    // disable the ^C so the devices will close properly.

        signal (SIGINT,SIG_IGN);

    // Find one MIDI device

        if (!OpenTheDriver(UserPref)) { // get the driver the user prefer
            printf ("Cannot find any installed VBE/AI devices!\n");
            DoExit(-1);
        }

    // process each note

        patch = -1;
        while (1) {

            if (patch == -1)
                GetChannel();

            if (channel == -1)
                break;

            GetNote();

            if (patch == -1)
                continue;

            printf ("Loading patch %d on channel %d\n",patch,channel);

            if (!DoNote(DN_LOADPATCH))  // load  the patch
                break;

            // process a few commands

                for (n=0;n<10;n++)
                    noteison[n] = FALSE;

                loop = -1;
                while (loop) {

                    printf ("   (P)lay (O)ff :  \b\b");
                    while (!kbhit()) ;

                    if ((c = getch()) == 0)
                        c = getch();

                    printf ("%c\r",c);

                    switch (c) {

                        case '1':
                        case '2':
                        case '3':
                        case '4':
                        case '5':
                        case '6':
                        case '7':
                        case '8':
                        case '9':
                        case '0':
                            if (c == '0')
                                c = 0x3A;

                            n = c - 0x31;
                            if (noteison[n] && AutoNoteOff)
                                DoNote(DN_NOTEOFF+n);

                            DoNote(DN_NOTEON+n);
                            noteison[n] = TRUE;
                            break;

                        case '-':
                            if (noteison[0] && AutoNoteOff)
                                DoNote(DN_NOTEOFF);
                            DoNote(DN_VELDOWN);
                            noteison[0] = TRUE;
                            break;

                        case '+':
                            if (noteison[0] && AutoNoteOff)
                                DoNote(DN_NOTEOFF);
                            DoNote(DN_VELUP);
                            noteison[0] = TRUE;
                            break;

                        case 'p':
                        case 'P':

                            if (noteison[0] && AutoNoteOff)
                                DoNote(DN_NOTEOFF);
                            DoNote(DN_NOTEON);
                            noteison[0] = TRUE;
                            break;

                        case 'u':
                        case 'U':
                            DoNote(DN_PITCHUP);
                            break;

                        case 'd':
                        case 'D':
                            DoNote(DN_PITCHDN);
                            break;

                        case 'c':
                        case 'C':
                            DoNote(DN_PITCHCNTR);
                            break;

                        default:
                        case 'n':
                        case 'N':
                        case 'o':
                        case 'O':

                            for (n=0;n<10;n++) {
                                if (noteison[n] && AutoNoteOff) {
                                    DoNote(DN_NOTEOFF+n);
                                    noteison[n] = FALSE;
                                }
                            }

                            if ((c & 0x5f) != 'O')
                                loop = 0;

                            break;
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
;---|*|----====< buildstr >====----
;---|*|
;---|*| Build the string of instrument names
;---|*|
;   \*/

buildstr(t,n0,n1,n2,n3)
    char *t;
    int n0,n1,n2,n3;
{
char *trg,*src;
int n;

    // blank fill the string

        trg=t;
        for (n=0;n<79;n++)
            *trg++ = ' ';
        *trg = 0;

    // add each string name to the padded string

        src = (channel == 10) ? percussivenames[n0] : melodicnames[n0];
        for(n=0;n<19;n++) {
            if (*src == 0) break;
            t[n] = *src++;
        }

        src = (channel == 10) ? percussivenames[n1] : melodicnames[n1];
        for(n=20;n<39;n++) {
            if (*src ==0) break;
            t[n] = *src++;
        }

        src = (channel == 10) ? percussivenames[n2] : melodicnames[n2];
        for(n=40;n<59;n++) {
            if (*src ==0) break;
            t[n] = *src++;
        }

        src = (channel == 10) ? percussivenames[n3] : melodicnames[n3];
        for(n=60;n<78;n++) {
            if (*src ==0) break;
            t[n] = *src++;
        }

        t[78] = '\n';
}

;
;   /*\
;---|*|----====< ChannelHelps >====----
;---|*|
;---|*| Give the user the commandline option list
;---|*|
;   \*/

ChannelHelps()
{
int n;

    // just print the helps

        printf ("\n");
        printf ("Channels 1 - 9 and 11 - 16 are melodic. Channel 10 is percussive.\n");
        printf ("\n");

}

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
int n,x;
char *s,c;

        GiveHelps();

        n = 1;
        while (n<argc) {

            s = argv[n++];

            if (*s == '-') s++;
            if (*s == '/') s++;

            switch (*s & 0xDF) {

                case 'A':
                    AutoNoteOff = FALSE;
                    printf ("Notes will not automatically turn off.\n");
                    break;

                case 'P':
                    UserPref = (int)GetValue (++s,0);
                    printf ("User Prefers (%d) level drivers\n",UserPref);
                    break;

                default:
                    printf ("Unknown option - %s\n",s);
            }
        }

    // separate the parameter text from the rest of the report

        printf ("\n");
}

;
;   /*\
;---|*|----====< DelayTimer >====----
;---|*|
;---|*| Delay this many clock ticks
;---|*|
;   \*/

DelayTimer(ticks,earlyout)
    int ticks;
    int earlyout;
{
int timer = -1;
int delta = -1;

    // while we have time to waste...

        sysclocktic();

        while (ticks) {

            // watch the keyboard for an exit command

                if (earlyout) {
                    if (kbhit())
                        if (getch() == 0x1b)
                            DoExit(-1);
                }

            // if the clock ticked, count down one...

                if (sysclocktic())
                    ticks--;

        }
}

;
;   /*\
;---|*|----====< DoNote >====----
;---|*|
;---|*| Process a keystroke into a note/pgm change
;---|*|
;   \*/

DoNote (c)
    int c;
{
int len;
VAIDhdr far *ph;
OPL2native far *pt;

static unsigned char dmsg[3] = { 0x90, 60, 0x7F };
static unsigned char pchg[2] = { 0xE0, 0x00 };
static unsigned char bend[3] = { 0xE0, 0x00, 0x40 };

        switch (c) {

            case DN_VELDOWN:

                // adjust the velocity, then send the note ON

                dmsg[2] -= 0x10;
                if (dmsg[2] > 0x7F)
                    dmsg[2] = 0;

                dmsg[1] = (channel == 10) ? patch-1 : 60;
                dmsg[0] = 0x90 | (channel-1);

                (msv->msMIDImsg) ( dmsg, 3 );
                DoNote(DN_ERRORCHK);
                break;

            case DN_VELUP:

                // adjust the velocity, then send the note ON

                dmsg[2] += 0x10;
                if (dmsg[2] > 0x7F)
                    dmsg[2] = 0x7F;

                dmsg[1] = (channel == 10) ? patch-1 : 60;
                dmsg[0] = 0x90 | (channel-1);

                (msv->msMIDImsg) ( dmsg, 3 );
                DoNote(DN_ERRORCHK);
                break;

            case DN_LOADPATCH:

                dmsg[0] =  0x90;
                bend[0] =  0xE0;

                pchg[0] = 0xc0 | (channel-1);
                pchg[1] = patch-1;

                if ((pchg[0] & 0x0F) == 9)
                    printf ("\nPercussive patch: %s\n\n",percussivenames[(patch-1)]);
                else
                    printf ("\nMelodic patch: %s\n\n",melodicnames[(patch-1)]);

                // if there is a patch library, fetch it and load it...

                if (gdc.u.gdmi.milibrary[0])

                    VESAPreLoadPatch ( hMIDI, patch-1, channel-1 );

                // send the program change

                (msv->msMIDImsg)      ( pchg, 2);

                DoNote(DN_ERRORCHK);

                break;

            case DN_NOTEON+0x00:
            case DN_NOTEON+0x01:
            case DN_NOTEON+0x02:
            case DN_NOTEON+0x03:
            case DN_NOTEON+0x04:
            case DN_NOTEON+0x05:
            case DN_NOTEON+0x06:
            case DN_NOTEON+0x07:
            case DN_NOTEON+0x08:
            case DN_NOTEON+0x09:

                // send the note ON

                dmsg[2] = 0x7F;
                dmsg[1] = (channel == 10) ? patch-1 : 60+(c-DN_NOTEON);
                dmsg[0] = 0x90 | (channel-1);

                (msv->msMIDImsg) ( dmsg, 3 );
                DoNote(DN_ERRORCHK);
                break;

            case DN_NOTEOFF+0x00:
            case DN_NOTEOFF+0x01:
            case DN_NOTEOFF+0x02:
            case DN_NOTEOFF+0x03:
            case DN_NOTEOFF+0x04:
            case DN_NOTEOFF+0x05:
            case DN_NOTEOFF+0x06:
            case DN_NOTEOFF+0x07:
            case DN_NOTEOFF+0x08:
            case DN_NOTEOFF+0x09:

                // send the note OFF

                dmsg[1] = (channel == 10) ? patch-1 : 60+(c-DN_NOTEOFF);

                dmsg[0] = 0x80 | (channel-1);
                (msv->msMIDImsg) ( dmsg, 3 );
                DoNote(DN_ERRORCHK);

                DoNote(DN_PITCHCNTR); // bend back to center

                break;

            case DN_PITCHUP:

                // pitch bend up

                bend[0] = 0xE0 | (channel-1);

                bend[2] += 8;
                if (bend[2] > 0x7F) bend[2] = 0x7F;

                (msv->msMIDImsg) ( bend, 3);

                DoNote(DN_ERRORCHK);

                break;

            case DN_PITCHDN:

                // pitch bend down

                bend[0] = 0xE0 | (channel-1);

                bend[2] -= 8;
                if (bend[2] > 0x7F) bend[2] = 0x00;

                (msv->msMIDImsg) ( bend, 3);

                DoNote(DN_ERRORCHK);

                break;

            case DN_PITCHCNTR:

                // pitch bend down

                bend[0] = 0xE0 | (channel-1);
                bend[1] = 0x00;
                bend[2] = 0x40;

                (msv->msMIDImsg) ( bend, 3);

                DoNote(DN_ERRORCHK);

                break;

            case DN_ERRORCHK:

                len = (msv->msGetLastError) ( ); // send off the single msg
                if (len)
                    printf ("The driver reported an error code of %x!\n",len);

                break;

            default:
                break;
        }

    return(c);
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

        if (msv)
            VESACloseDevice(hMIDI);     // close the device

    // flush our callback control

        VESARegisterTimer( 255, 0, VESARateToDivisor(120) );

    // make a distinguishing mark on the screen...

        printf ("\n-------------------------------------------------\n");

    // return to DOS, don't return to caller

        exit(cc);
}

;
;   /*\
;---|*|----====< DumpHex >====----
;---|*|
;---|*| Print out, in hex, the referenced data
;---|*|
;   \*/
DumpHex (pptr,len)
    char far *pptr;
    int len;
{
int n;

        for (n=0;n<len;n++) {

            if (!(n & 0x0F))
                printf ("\n%08lx:",pptr);

            printf ("%02x ",*pptr++ & 0xFF);

        }
}

;
;   /*\
;---|*|----====< GetChannel >====----
;---|*|
;---|*| Directly enter the channel number.
;---|*|
;   \*/

int GetChannel()
{
char str[100];
int chnl;

    // do this till we ge a valid channel & key

        printf ("\n");

        chnl = channel;

        while(1) {

            // get the channel number from the user

            printf ("Enter the channel # (1-16, `?` or -1 to exit) [%d] :",chnl);
            fgets  (str,99,stdin);

            if (str[0] == '?') {
                ChannelHelps();
                continue;
            }
            else {

                chnl = GetValue(str,chnl);

                if (chnl > 16) {
                    chnl = channel;
                    continue;
                }

                if (chnl == -1)
                    break;

                if (chnl > 0)
                    break;

            }
        }

        channel = chnl;
        return(-1);
}

;
;   /*\
;---|*|----====< GetNote >====----
;---|*|
;---|*| Directly enter the channel and note.
;---|*|
;   \*/

int GetNote()
{
char str[100];
int ptch;

    // do this till we ge a valid channel & key

        printf ("\n");

    // get the patch number from the user

        ptch = patch;
        str[0] = '?';

        while (str[0] == '?') {

            printf ("Enter the key # (`?` or -1 to backup) [%d] :",ptch);
            fgets  (str,99,stdin);

            if (str[0] == '?')
                KeyHelps();

            ptch = GetValue(str,ptch);

            if (ptch > 128) {
                ptch = patch;
                str[0] = '?';
            }
        }

        // keep these values

        patch = ptch;

        return(-1);
}

;
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

;
;   /*\
;---|*|----====< GiveHelps >====----
;---|*|
;---|*| Give the user the commandline option list
;---|*|
;   \*/

GiveHelps()
{

        printf ("\n");
        printf ("This program allows the user to enter a channel and note\n");
        printf ("number to be loaded into the target MIDI device. Once the\n");
        printf ("patch has been loaded, a menu of functions will be provided\n");
        printf ("to allow the user to turn the note ON or OFF.\n\n");

        printf ("To Use:  DOS>note [Pxx]\n\n");
        printf ("Where:   [Pxx] is the user preference #.\n\n");

        printf ("NOTE: all values are one based, so the key number ranges\n");
        printf ("      from 1 to 128. Channels range from 1 to 16. Channel\n");
        printf ("      number 10 is the percussive channel.\n");

}

;
;   /*\
;---|*|----====< KeyHelps >====----
;---|*|
;---|*| Print out a list of keys
;---|*|
;   \*/
KeyHelps()
{
int n;
char str[100];

    printf ("\n");

    if (channel != 10) {

        for (n=0;n<32;n++) {
            buildstr(str,n,n+32,n+64,n+96);
            printf ("%s",str);
        }
    }
    else {

        for (n=34;n<34+11;n++) {
            buildstr(str,n,n+11,n+22,n+33);
            printf("%s",str);
        }
    }

    printf ("\n");

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

    // get the device information

        do {

            // Find one MIDI device, else ]ail if non found

                if ((hMIDI = VESAFindADevice(MIDDEVICE)) == 0)
                    return(0);

            // get the device information

                if (VESAQueryDevice(hMIDI, VESAQUERY2 ,&gdc) == 0) {
                    printf ("Cannot query the installed VBE/AI devices!\n");
                    DoExit(-1);
                }
            // make sure ever

                if (gdc.gdclassid != MIDDEVICE) {
                    printf ("The VESA find device query returned a NON MIDI device!\n");
                    DoExit(-1);
                }

            // get the drivers user preference level

                driverpref = gdc.u.gdmi.midevpref;

            // if the caller is not expressing a preference, then use this one

                if (pref == -1)
                    break;

        } while (driverpref != pref);

    // get the memory needed by the device

        if ((memptr = AllocateBuffer(gdc.u.gdmi.mimemreq))== 0) {
            printf ("We don't have memory for the device!\n");
            DoExit(-1);
        }

    // if the MIDI device doesn't open, bomb out...

        if ((msv = (fpMIDServ)VESAOpenADevice(hMIDI,0,memptr)) == 0) {
            printf ("Cannot Open the installed devices!\n");
            DoExit(-1);
        }

    // if there is a patch library, load it now...

        if (gdc.u.gdmi.milibrary[0])
            if (VESALoadPatchBank ( hMIDI, msv, &gdc.u.gdmi.milibrary[0] ) == 0)
                DoExit(-1);

    // callbacks, reset & other things...

        msv->msApplFreeCB = &OurFreeCBCallBack;
        msv->msApplMIDIIn = &OurMIDIReceiver;
        (msv->msGlobalReset)();

        return(hMIDI);
}

;
;   /*\
;---|*|----====< OurFreeCBCallBack >====----
;---|*|
;---|*| The patch block is now free. NOTE: No assumptions can be made about
;---|*| the segment registers! (DS,ES,GS,FS)
;---|*|
;   \*/

void far pascal OurFreeCBCallBack( han, patch, fptr, filler )
    int  han;       // the caller's handle
    int  patch;     // the actual patch #
    void far *fptr; // buffer that just played
    long filler;    // reserved
{
        // do nothing here...
}


;
;   /*\
;---|*|----====< OurMIDIReceiver >====----
;---|*|
;---|*| Print the received data byte.
;---|*|
;   \*/

void far pascal OurMIDIReceiver ( han, delta, mbyte, filler )
    int  han;       // the caller's handle
    int  delta;     // the delta time
    char mbyte;     // the midi byte
    long filler;    // reserved
{
register char far *vidptr = (char far *)0xb80001E0;

    _asm {
        push    ds
        mov     ax,seg UserPref
        mov     ds,ax
    }

    *vidptr += mbyte;
    *vidptr += 0x40;

    if ((int)vidptr > 0x7FF)
        vidptr = (char far *)0xb80001E0;

    _asm {
        pop     ds
    }
}

;
;   /*\
;---|*|----====< readvalue >====----
;---|*|
;---|*| Read a value from the keyboard
;---|*|
;   \*/
long readvalue()
{
char str[80];
int n;

    // get the string

        if (fgets (&str[0],79,stdin) == 0)
            return(0);

        return(GetValue(&str[0],0));
}

;
;   /*\
;---|*|----====< sysclocktic >====----
;---|*|
;---|*| Return the delta of system clock ticks
;---|*|
;   \*/
sysclocktic()
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




