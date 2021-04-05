
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
;---|*| Additional Changes:
;---|*|   04/05 - Changed the handling of the delta and track processing
;---|*|           because the cheap quantization threw things out of order.
;---|*|   04/05 - Program didn't exit since the break was nested 2 deep.
;---|*|   03/10 - Added cheap quantization of the tempo to reduce the
;---|*|           interrupt rate to drive the tempo.
;---|*|   02/27 - Removed a bunch of unused code to clean up the pgm
;---|*|   02/16 - Added a switch to allow sending Active Sensing as an option.
;---|*|   02/16 - Added a routine to send All Notes Off before exiting.
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

#if DEBUG
#define BREAKPOINT _asm{int 3};
#else
#define BREAKPOINT _asm{nop};
#endif


;   /*\
;---|*| External Variables
;   \*/

        extern char *melodicnames[];
        extern char *percussivenames[];


;   /*\
;---|*| Global Variables
;   \*/

        typedef struct {
            char fileid[4];     // file ID - "MThd"
            long flen;          // header length
            int  ftype;         // midi file type
            int  ftracks;       // number of tracks
            int  fdiv;          // time division
        } MIDIheader;

        typedef struct {
            char fileid[4];     // file ID - "MTrk"
            long flen;          // midi track len
        } MDtrack;

        typedef struct {
            MDtrack mt;         // Track Header
            int  tnum;          // track number
            long delta;         // delta time counting
            char far *p;        // pointer to the data
        } MIDItrack;

        int  deltastep = 1;     // incrementing step count
        long deltacount= 0;     // full delta count

#define MAXTRACKS   32
        MIDIheader mhdr     = { 0 };    // midi file header
        MIDItrack  mtrk[32] = { 0 };    // track structures

        int  CallBackOccured;
        int  clocktick = 0;
        int  ScatteredMessages = TRUE;

        VESAHANDLE hMIDI  = 0;

        FILE *rfile;
        char far *memptr  = 0;
        char far *midptr  = 0;
        char *MIDFileName;

        GeneralDeviceClass gdc; // receives a copy of the VESA driver info block
        fpMIDServ msv;          // pointer to MIDI functions

#define FILE_UNK    0
#define FILE_MIDI   1
        int  filetype = FILE_UNK;

        char DoTracks[16] = {
            TRUE, TRUE, TRUE, TRUE,
            TRUE, TRUE, TRUE, TRUE,
            TRUE, TRUE, TRUE, TRUE,
            TRUE, TRUE, TRUE, TRUE
        };

        unsigned char PatchXlate[256];  // patch translation table
        unsigned char ChannelXlate[16]; // channel translation table

        int  keyoffset      = 0;        // key (note) offset by one octave
        int  patchoffset    = 0;        // no patch offset
        int  tempooffset    = 0;        // no tempo offset
        int  MicrosoftGMfmt = FALSE;    // microsoft GM file format
        long micros         = 500000;   // 500ms per quarter note
        int  sigdenom       = 2;        // qnote gets the beat
        int  SystemMsgTick  = 0;        // system msg (0xFE) needed
        int  msgsent        = FALSE;    // any msg has been sent
        int  ActiveSensing  = FALSE;    // defaults to OFF


;   /*\
;---|*| prototypes
;   \*/

        int  LoadMIDITrack          ( MIDItrack * );
        int  LoadMIDIFile           ( );
        int  ProcessMidiMsg         ( MIDItrack * );
        int  SystemMessage          ( unsigned int, MIDItrack * );

        long GetValue               ( char *, long );
        int  readblock              ( int, char huge *, int );

        int  swapdw                 ( int );
        long swapdd                 ( long );
        long ReadVariable           ( MIDItrack * );

        void far pascal OurSystemMSGCallBack  ( );
        void far pascal OurTimerCallBack  ( );
        void far pascal OurMIDIReceiver   ( int, int, char, long );

        void SetSongTempo           ( long, int );
        VESAHANDLE OpenTheDriver    ( int );
        void OutputMidiMsg          ( char far *, int );
             PrintMsg               ( int, char far *, int );

        int  DriverError;
        int  VerboseMode  = FALSE;
        int  StatusMode   = FALSE;
        int  UserPref     = 0;      // highest level to be used
        int  maxtones;              // maximum # of tones supported by the h/w
        int  InputOnly    = FALSE;  // MIDI receiver tests
		int  Breakneckspeed = FALSE;// helps me debug, not useful otherwise...

        char *msgnames[8] = {
            "Note Off", // 0x80
            "Note On ", // 0x90
            "Poly Key", // 0xA0
            "Ctlr Chg", // 0xB0
            "Pgm  Chg", // 0xC0
            "Chan Pre", // 0xD0
            "Pitch wh", // 0xE0
            "System  "  // 0xF0
        };

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
long l;
int trk,deadcnt;
int laps = 10;
int all,nxt;

    // process the command line

        CommandLine (argc,argv);

    // disable the ^C so the devices will close properly.

        signal (SIGINT,SIG_IGN);

    // Find one MIDI device

        if (!OpenTheDriver(UserPref)) { // get the driver the user prefer
            printf ("Cannot find any installed VBE/AI devices!\n");
            DoExit(-1);
        }

    // This is playback, so load the MIDI file header, then each track

        if (!LoadMIDIFile())
            DoExit(-1);

        for (n=0;n<mhdr.ftracks;n++) {
            mtrk[n].p=midptr;
            LoadMIDITrack(&mtrk[n]);
            midptr += mtrk[n].mt.flen;              // get the next ptr
            mtrk[n].delta = ReadVariable(&mtrk[n]); // read the 1st delta from the file
        }
        deltacount = 0;

    // setup a callback for active system messages

        VESARegisterTimer( 254, &OurSystemMSGCallBack, VESARateToDivisor(5)   );
        VESARegisterTimer( 255, &OurTimerCallBack,     VESARateToDivisor(120) );

    // process the tracks

        while (loop) {

        // see if we need an active sensing message to keep the device active

            KeepActive();

        // Callbacks occur to give us a tempo for MIDI timing

            if (CallBackOccured) {

                ReportCallback();
                CallBackOccured--;

                // if we have run out of data, exit the program...

                deltacount += deltastep;

                while(1) {

                    // find the track with the lowest delta time

                    deadcnt = 0;
                    for (nxt=n=0;n<mhdr.ftracks;n++) {

                        if (mtrk[n].mt.flen > 0) {
                            if (mtrk[n].delta < mtrk[nxt].delta)
                                nxt = n;
                        }
                        else {
                            deadcnt++;
                            if (nxt == n) nxt++;
                        }
                    }

                    // if all tracks are dead, exit

                    if (deadcnt >= mhdr.ftracks) {
                        loop=FALSE;
                        break;
                    }

                    // if higher that the current delta count, exit

                    if (mtrk[nxt].delta > deltacount)
                        break;

                    ProcessMidiMsg(&mtrk[nxt]);

                }
            }

		// blast through the file for debugging purposes. Not for normal use

			if (Breakneckspeed)
				CallBackOccured++;

        // see if the user wishes to exit

            if (kbhit()) {

                switch (c = getch()) {

                    // quit the program

                    case 's':   // do a fast forward in the song
                    case 'S':
                        for (n=0;n<mhdr.ftracks;n++)
                            mtrk[n].delta = deltacount;
                        break;

                    case 0x20:

                        // flush our callback control

                        VESARegisterTimer( 255, 0, 0 );

                        // wait on the user...

                        printf ("Song Now Paused!\n");
                        while (!kbhit())
                            KeepActive();
                        getch();

                        // set the tempo & let'er rip!

                        SetSongTempo ( micros, sigdenom );
                        break;

                    case 0x1b:
                        AllNotesOff();
                        loop = FALSE;
                        exitcode = TRUE;

                    default:
                        break;

                }
            }

        // if the driver posts an error, go report it

            if ( (DriverError = (msv->msGetLastError)()) )
                printf ("Driver is reporting an internal error! (code=%d)\n",DriverError);

        // if in status mode, dump status every x times through the loop

            if (StatusMode) {
                if (!--laps) {
                    laps = 10;
                    DumpSomeInfo();
                }
            }
        }

    // exit now...

        DoExit(exitcode);
}

;   /*\
;---|*|----------------------=======================---------------------------
;---|*|----------------------====< Subroutines >====---------------------------
;---|*|----------------------=======================---------------------------
;   \*/

;   /*\
;---|*|----====< AllNotesOff >====----
;---|*|
;---|*| Turn each track off
;---|*|
;   \*/
AllNotesOff()
{
int n;
char msg[3] = { 0,0x7b,0x00 };

    for(n=0;n<16;n++) {
        msg[0] = 0xb0 + n;
        OutputMidiMsg (msg,3);
    }
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
int n,x;
char *s,c;
FILE *mapnam;
int vh,vl;

    // print the hello...

        vh = VersionControl >> 8;
        vl = VersionControl &  0xFF;

        printf ("\nVESA VBE/AI MIDI Output Test Program, %02x.%02x\n",vh,vl);
        printf ("Copyright (c) 1993,1994  VESA, Inc. All Rights Reserved.\n\n");

    // setup the patch translation table defaults

        for (n=0;n<256;n++)
            PatchXlate[n] = n & 0x7F;   // only 7 bits worth

    // exit if no other parameters

        if (argc < 2) {
            GiveHelps();
            DoExit(0);
        }

        MIDFileName = argv[1];
        printf ("Attempting to use \"%s\" input file",MIDFileName);
        filetype = FILE_MIDI;
        printf (" as a MIDI file\n");

        n = 2;
        while (n<argc) {

            s = argv[n++];

            if (*s == '-') s++;
            if (*s == '/') s++;

            switch (*s & 0xDF) {

                case 'A':
                    if (*++s == '-')
                        ActiveSensing = FALSE;
                    else
                        ActiveSensing = TRUE;

                    printf ("Active sensing is turned ");
                    (ActiveSensing) ? printf ("ON\n") : printf ("OFF\n");
                    break;

				case 'B':
					Breakneckspeed = TRUE;
					printf ("Will move through file with deltatime=0\n");
					break;

                case 'C':

                    printf ("Using Channel map file \"%s\"\n",++s);

                    if (!(mapnam = fopen (s,"rb")) ) {
                        printf ("Unable to open the channel map file!\n");
                        DoExit(-1);
                    }
                    if (fread (&ChannelXlate,sizeof(char),16,mapnam) != 16) {
                        printf ("Unable to load the whole channel map file!\n");
                        DoExit(-1);
                    }

                    // if the channel is 0xFF, kill it...

                    BREAKPOINT

                    for (n=0;n<16;n++)
                        if (ChannelXlate[n] == 0xFF)
                            DoTracks[n] = FALSE;

                    fclose (mapnam);

                    break;

                case 'F':
                    ScatteredMessages = FALSE;
                    printf ("Will process messages in full length\n");
                    break;

                case 'H':
                    for (x=0;x<16;x++) {
                        if (x < 12)
                            DoTracks[x] = TRUE;
                        else
                            DoTracks[x] = FALSE;
                    }
                    printf ("High end synth for tracks 1 - 12\n");
                    break;

                case 'I':
                    InputOnly = TRUE;
                    printf ("In INPUT mode only - no output performed\n");
                    break;

                case 'K':
                    s++;

                    keyoffset = 1;      // there will be an offset

                    if (*s == '-')      // get the PLUS or MINUS direction
                        c = *s++;

                    if (*s == '+')
                        c = *s++;

                    keyoffset = (int)GetValue(s,(unsigned long)keyoffset);

                    if (c == '-')
                        keyoffset = -keyoffset;

                    printf ("Key offset by %d keys\n",keyoffset);
                    break;

                case 'L':
                    MicrosoftGMfmt = TRUE;
                    for (x=0;x<16;x++) {
                        if (x < 12)
                            DoTracks[x] = FALSE;
                        else
                            DoTracks[x] = TRUE;
                    }

                    printf ("Low end synth for tracks 13 - 16\n");
                    break;

                case 'M':

                    printf ("Using patch Map file \"%s\"\n",++s);

                    if (!(mapnam = fopen (s,"rb")) ) {
                        printf ("Unable to open the patch map file!\n");
                        DoExit(-1);
                    }

                    if (fread (&PatchXlate,sizeof(char),256,mapnam) != 256) {
                        printf ("Unable to load the whole patch map file!\n");
                        DoExit(-1);
                    }

                    fclose (mapnam);

                    break;

                case 'P':
                    UserPref     = (int)GetValue (++s,0);
                    printf ("User Prefers (%d) level drivers\n",UserPref);
                    break;

                case 'R':               // program change offset
                    s++;

                    patchoffset = 1;    // there will be an offset

                    c = 0;
                    if (*s == '-')      // get the PLUS or MINUS direction
                        c = *s++;

                    if (*s == '+')
                        c = *s++;

                    patchoffset = (int)GetValue(s,(unsigned long)patchoffset);

                    if (c == '-')
                        patchoffset = -patchoffset;

                    printf ("Patch offset by %d program steps\n",patchoffset);
                    break;

                case 'S':               // status dumps
                    StatusMode = GetValue(++s,(StatusMode = 10));
                    printf ("Status mode = %d\n",StatusMode);
                    break;

                case 'T':               // tempo offset percentage
                    s++;

                    c = 0;
                    if (*s == '-')      // get the PLUS or MINUS direction
                        c = *s++;
                    if (*s == '+')
                        c = *s++;

                    tempooffset = (int)GetValue(s,10L);

                    if (c == '-')
                        tempooffset = -tempooffset;

                    printf ("Tempo offset by %d%%\n",tempooffset);
                    break;

                case 'V':   // print everything at a given level
                    VerboseMode = GetValue(++s,(VerboseMode = 10));
                    printf ("Verbose level = %d\n",VerboseMode);
                    break;

                default:
                    printf ("Unknown option - %s\n",s);
                    DoExit(-1);
            }
        }

    // separate the parameter text from the rest of the report

        printf ("\n");
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

        if (msv)
            VESACloseDevice(hMIDI);     // close the device

    // flush our callback control

        VESARegisterTimer( 255, 0, 0 );
        VESARegisterTimer( 254, 0, 0 );

    // return to DOS, don't return to caller

        exit(cc);
}


;   /*\
;---|*|----====< ProcessMidiMsg >====----
;---|*|
;---|*| Process the MIDI notes
;---|*|
;   \*/

int ProcessMidiMsg(m)
    MIDItrack *m;
{
unsigned int status;
char far *msg;
char far *patch;
int  psize,ch,doit,idx;
unsigned int ptch;

static  unsigned int running = 0;

    // if no more data, bail...

        if (!m->mt.flen)
            return(0);

    // get the status, make it running

        status = running;

    // maintain running status

        if (*m->p & 0x80) {
            if (((status = *m->p++ & 0xFF) & 0xF0) != 0xF0)
                running = status;
            m->mt.flen--;
        }

    // always send the message. Running status will be done later.

        *(msg=m->p-1) = status;       // load the message into memory

    // make sound if this is a track we want to hear

        if (DoTracks[ch=status & 0x0F]) {

            switch (status & 0xF0) {

                case 0x80:  // Note Off

                    if(((ch==9)&&DoTracks[9]) || ((MicrosoftGMfmt)&&(ch==15))) {
                        msg[0] = 0x89;          // make it channel #9
						msg[1] = PatchXlate[(msg[1]|0x80) & 0xFF] & 0x7F;
                    }
                    else {
                        msg[1] += keyoffset;    // offset the note
                        msg[1] &= 0x7F;         // keep it in 7 bits
                    }

                    PrintMsg (9,msg,3);         // display the msg
                    OutputMidiMsg (msg,3);      // send it to the h/w
                    m->mt.flen -= 2;            // adjust the length
                    m->p += 2;
                    break;

                case 0x90:  // Note On

                    // make sure all GM MIDI percussive channel patches
                    // are loaded before starting

                    if  ( ((ch==9)&&DoTracks[9]) || ((MicrosoftGMfmt)&&(ch == 15)) ) {

                        // for Microsofts MIDI channel 15 to channel 9

                        if ((ch == 15) && MicrosoftGMfmt)
                            msg[0] = 0x99;  // make it channel #9

                        // translate the patch from the 2nd half of the tbl

						ptch = PatchXlate[(msg[1]|0x80) & 0xFF];

                        // if there are external patches, load them now

                        if (gdc.u.gdmi.milibrary[0]) {

                            // skip if preloaded

							if (!PatchIsLoaded(ptch|0x80)) {

                                // load the percussive patch now

                                VESAPreLoadPatch(hMIDI, ptch, (msg[0]&0x0F));

                                if (VerboseMode >= 7)
                                    printf ("%s  ",percussivenames[ptch]);
                                PrintMsg (7,msg,3);

                            }
                        }

                        // save the translated patch #

                        msg[1] = ptch;

                    }
                    else {

                        msg[1] += keyoffset;    // offset the note
                        msg[1] &= 0x7F;         // keep it in 7 bits

                    }

                    PrintMsg (9,msg,3);         // display the msg
                    OutputMidiMsg (msg,3);      // send it to the h/w
                    m->mt.flen -= 2;            // adjust the length
                    m->p += 2;
                    break;

                case 0xA0:  // Poly Key Pressure

                    if(((ch==9)&&DoTracks[9]) || ((MicrosoftGMfmt)&&(ch == 15)))
						msg[1] = PatchXlate[(msg[1]|0x80) & 0xFF] & 0x7F;
                    else
                        msg[1] += keyoffset;    // offset the note

                    msg[1] &= 0x7F;             // keep it in 7 bits

                    // falls through...

                case 0xB0:  // Control Change
                case 0xE0:  // Pitch Wheel change

                    PrintMsg (6,msg,3);         // display the msg
                    OutputMidiMsg (msg,3);      // send it to the h/w
                    m->mt.flen -= 2;            // adjust the length
                    m->p += 2;
                    break;

                case 0xC0:  // Program Change

                    // handle the percussive channel separate from melodics

                    if (((ch==9)&&DoTracks[9]) || ((MicrosoftGMfmt)&&(ch==15))) {

                        // patch xlate on percussive patches

						ptch = (msg[1] = PatchXlate[(msg[1]|0x80) & 0xFF]);

                        // if MSGMFMT & channel 15, force to channel #9

                        if  ((MicrosoftGMfmt)&&(ch == 15)) {
                            msg[0] = 0xC9;  // make it channel #9
                        }

                        if (VerboseMode >= 7)
                            printf ("%s  ",percussivenames[ptch]);
                        PrintMsg (7,msg,2);
                    }
                    else {

                        // patch xlate on melodic patches

                        ptch =  PatchXlate[msg[1] & 0x7F];  // translate to a new patch
                        ptch =  (ptch + patchoffset) & 0x7F;// offset the patch
                        msg[1] = ptch;                      // save the patch

                        if (VerboseMode >= 7)
                            printf ("%s  ",melodicnames[ptch]);
                        PrintMsg (7,msg,2);
                    }

                    // if there is an external library, load a patch now

                    if (gdc.u.gdmi.milibrary[0]) {

                        // if preloaded, skip the process

                        doit=FALSE;
                        if (((ch==9)&&DoTracks[9]) || ((MicrosoftGMfmt)&&(ch==15))) {
							if (!PatchIsLoaded(ptch|0x80))
                                doit=TRUE;
                        }
                        else {
                            if (!PatchIsLoaded(ptch))
                                doit=TRUE;
                        }

                        // if percussive or melodic needs one, do it

                        if (doit) {

                            // get the patch & get the size

                            VESAPreLoadPatch( hMIDI, ptch, msg[0]&0x0F );

                        }
                    }
                    OutputMidiMsg (msg,2);
                    m->mt.flen--;               // adjust the length
                    m->p++;
                    break;

                case 0xD0:  // Channel Pressure
                    PrintMsg (6,msg,2);
                    OutputMidiMsg (msg,2);
                    m->mt.flen--;               // adjust the length
                    m->p++;
                    break;

                case 0xF0:
                    PrintMsg (6,msg,5);
                    SystemMessage (status,m);
                    break;

                default:
                    PrintMsg (1,msg,5);
                    printf ("Broken MIDI stream! (no status/running status!)\n");
                    DoExit(1);
            }
        }

    // toss out these tracks

        else {

            switch (status & 0xF0) {

                case 0x80:  // Note Off
                case 0x90:  // Note On
                case 0xA0:  // Poly Key Pressure
                case 0xB0:  // Control Change
                case 0xE0:  // Pitch Wheel change

                    if (VerboseMode >= 9)
                        printf ("   ");

                    PrintMsg (9,msg,3);
                    m->mt.flen -= 2;            // adjust the length
                    m->p += 2;
                    break;

                case 0xC0:  // Program Change
                case 0xD0:  // Channel Pressure

                    if (VerboseMode >= 9)
                        printf ("   ");

                    PrintMsg (9,msg,2);
                    m->mt.flen--;               // adjust the length
                    m->p++;
                    break;

                case 0xF0:  // always do these...

                    PrintMsg (9,msg,5);
                    SystemMessage (status,m);
                    break;

                default:

                    if (VerboseMode >= 9)
                        printf ("   ");

                    PrintMsg (9,msg,5);
                    printf ("Broken MIDI stream! (no status/running status!)\n");
                    DoExit(1);
            }
        }

    // get the next midi msg, or bail out

        if (m->mt.flen)
            m->delta += ReadVariable(m); // read the 1st delta from the file

    // return the adjusted track pointer

        return(-1);
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
            // make sure it's midi

                if (gdc.gdclassid != MIDDEVICE) {
                    printf ("The VESA find device query returned a NON MIDI device!\n");
                    DoExit(-1);
                }

            // make sure it's matches the beta version #

                if (gdc.gdvbever != VersionControl) {
                    printf ("The VESA device version # does not match, cannot continue!\n");
                    DoExit(-1);
                }

            // get the drivers user preference level

                driverpref = gdc.u.gdmi.midevpref;

            // if the caller is not expressing a preference, then use this one

                if (pref == -1)
                    break;

        } while (driverpref != pref);

    // get the memory needed by the device

        if (!(memptr = AllocateBuffer(gdc.u.gdmi.mimemreq))) {
            printf ("We don't have memory for the device!\n");
            DoExit(-1);
        }

        if (!(midptr = AllocateBuffer(0xffff))) {
            printf ("We don't have memory for the loading the MIDI file!\n");
            DoExit(-1);
        }

    // if the MIDI device doesn't open, bomb out...

        if ((msv = (fpMIDServ)VESAOpenADevice(hMIDI,0,memptr)) == 0) {
            printf ("Cannot Open the installed devices!\n");
            DoExit(-1);
        }

    // if there is a patch library, load it now...

        if (gdc.u.gdmi.milibrary[0])
            if (VESALoadPatchBank(hMIDI, msv, &gdc.u.gdmi.milibrary[0]) == 0)
                DoExit(-1);

    // callbacks, reset & other things...

        msv->msApplMIDIIn = &OurMIDIReceiver;
        (msv->msGlobalReset)();
        maxtones = gdc.u.gdmi.miactivetones;

        return(hMIDI);
}


;   /*\
;---|*|----====< PatchIsLoaded >====----
;---|*|
;---|*| Test the driver's bit table to determine if the patch is preloaded
;---|*|
;   \*/
int PatchIsLoaded(int ptch)
{
int far *p = &msv->mspatches[0];

    _asm {
        push    es
        les     bx,[p]
        mov     ax,[ptch]
        and     ax,0FFh
        mov     cx,ax
        shr     ax,3
        add     bx,ax

        and     cx,007h
        mov     ax,0080h
        shr     ax,cl

        and     ax,es:[bx]
        pop     es
    }
}


;   /*\
;---|*|----====< PrintMsg >====----
;---|*|
;---|*| Print the MIDI messages
;---|*|
;   \*/

PrintMsg (level,msg,len)
    int level;
    char far *msg;
    int len;
{
int n;
int cmd;
char far *m;

    // verbose mode must be active to print

        if (!VerboseMode)
            return(0);

    // the priority level must be higher (0=highest, 5=lowest)

        if (level > VerboseMode)
            return(0);

    // get the command. change zero velocity NOTE ON to NOTE OFF

        cmd = (*msg & 0xF0) >> 4;
        if (cmd == 9)
            if (msg[2] == 0)
                cmd = 8;

    // print each byte in the string as a hex pair

        m = msg;
        for (n=len;n;n--)
            printf ("%02x ",*m++ & 0xFF);

    // print the command name and a CR/LF

        printf("%s\n",msgnames[cmd-8]);
}


;   /*\
;---|*|----====< ReadVariable >====----
;---|*|
;---|*| Read a variable length 32 bit number
;---|*|
;   \*/
long ReadVariable(m)
    MIDItrack *m;
{
long result = 0;
register char c;

        while (m->mt.flen) {
            result = (result << 7) + ((c=*m->p++) & 0x7F);
            m->mt.flen--;
            if (!(c & 0x80)) break;
        }
        return(result);
}


;   /*\
;---|*|----====< SystemMessage >====----
;---|*|
;---|*| Process a system message
;---|*|
;   \*/
SystemMessage (status,mtk)
    unsigned int status;
    MIDItrack *mtk;
{
long melen;
long us;
char metype;
int  n;

    // parse the SYSTEM mesage

        switch (status) {

            case 0xF7:  // sysex scan
            case 0xF0:  // sysex scan for the eot
                melen = ReadVariable(mtk);
                mtk->p += melen;            // skip over the sysex
                mtk->mt.flen -= melen;
                break;

            case 0xF2:  // state

                mtk->mt.flen--;             // eat two parameters by
                mtk->p++;                   // falling through

            case 0xF1:  // MTC Quarter-Frame
            case 0xF3:  // continue

                mtk->mt.flen--;             // eat one parameter...
                mtk->p++;

            case 0xF4:  // undefined - no parameters
            case 0xF5:  // undefined - no parameters
            case 0xF6:  // active sensing
            case 0xF8:  // timing clock
            case 0xF9:  // undefined
            case 0xFA:  // state
            case 0xFB:  // continue
            case 0xFC:  // stop
            case 0xFD:  // undefined
            case 0xFE:  // active sensing
                break;

            case 0xFF:  // meta event.

                metype = *mtk->p++;         // get the type
                mtk->mt.flen--;

                melen  = ReadVariable(mtk); // get the length

                switch (metype) {

                    case 0x01:
                    case 0x02:
                    case 0x03:
                    case 0x04:
                    case 0x05:
                    case 0x06:
                    case 0x07:

                        // text messages

                        mtk->mt.flen -= melen;

                        // print all characters under 128

                        for (n=melen;n;n--) {
                            if ((*mtk->p & 0x80) == 0)
                                putchar(*mtk->p);
                            mtk->p++;
                        }
                        printf ("\n");
                        break;

                    case 0x51:

                        // microseconds per division

                        us  = (long)(*mtk->p++ & 0xff) << 16;
                        us += (long)(*mtk->p++ & 0xff) <<  8;
                        us += (long)(*mtk->p++ & 0xff);

                        mtk->mt.flen -= 3;

                        SetSongTempo ( (micros=us), sigdenom );

                        break;

                    case 0x58:

                        sigdenom = mtk->p[1];       // get the sign. denom

                        mtk->mt.flen -= melen;
                        mtk->p       += melen;

                    ////SetSongTempo ( micros, sigdenom );
                        break;

                    default:
                        mtk->p += melen;            // just blow it off
                        mtk->mt.flen -= melen;
                        if (metype == 0x2F) {
                            if (VerboseMode)
                                printf ("End of Track #%d\n",mtk->tnum);
                            mtk->mt.flen = 0;
                            mtk->delta = deltacount;
                        }
                    break;
                }
                break;

            default:
                break;
        }
}


;   /*\
;---|*|----====< ProcessNote >====----
;---|*|
;---|*| Perfrom a ramp of notes up and down
;---|*|
;   \*/
ProcessNote()
{
int n;
int p;

#define BASENOTE    30
static int direction = ON;
static int note  = BASENOTE;

char midimsg[3]  = { 0x90, 0x30, 0x7f };

    // turn the voice on, or off

        if (direction == ON) {

            // if out of voices, start turning them off

            if ((msv->msDeviceCheck) (MIDITONES, 0) == 0) {
                direction = OFF;
                note = BASENOTE;
            }
            else {

                // build a message for NOTEON

                midimsg[1] += note++;   // make each note different
                midimsg[2]  = 0x7f;     // full velocity

                if (note > 90)
                    note = 90;

                // go play the note

                OutputMidiMsg (&midimsg[0],3);

            }
        }

        if (direction == OFF) {

            if ((msv->msDeviceCheck) (MIDITONES, 0) == maxtones) {
                direction = ON;
                note = BASENOTE;
            }
            else {

                // build a message for NOTEON

                midimsg[1] += note++;   // make each note different
                midimsg[2]  = 0x00;     // zero velocity

                if (note > 90)
                    note = 90;

                // go stop the note

                OutputMidiMsg (&midimsg[0],3);
            }
        }
}


;   /*\
;---|*|----------------------=======================----------------------
;---|*|----------------------====< Subroutines >====----------------------
;---|*|----------------------=======================----------------------
;   \*/

;   /*\
;---|*|----====< DumpSomeInfo >====----
;---|*|
;---|*| Do direct screen writes to display our status information. Dumps
;---|*| 256 characters indicating the preloaded bits
;---|*|
;   \*/
DumpSomeInfo()
{
int far *vp = (int far *)0xB8000320;    // starting on video row 5
int x,y,v,m;

    // display the preloaded bits in the services table

        for (y=0;y<16;y++) {

            v = msv->mspatches[y];
            m = 0x0001;

            for (x=0;x<16;x++) {

                *vp++ = 0x3f30 + ((v & m) ? 1 : 0);
                m <<= 1;
            }
        }
}

;
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

        printf ("\nTo Use:  DOS>pmidi [song] [A] [H] [L] [M] [K{+|-xx}] [R{+|-xx}] [V{xx}]\n\n");

        printf ("Where: [song]     is the .MID file to be played\n");
        printf ("       [A{-|+}]   Active Sense Enable(+) or Disable(-)\n");
        printf ("       [H]        High end synth playing on channels 1-10.\n");
        printf ("       [I]        to receive only.\n");
        printf ("       [K{+|-}xx] shifts all key #s up|down by xx count (cheap transpose).\n");
        printf ("       [L]        Low end synth playing on channels 11-16.\n");
        printf ("       [Mxxxx]    patch Map file name (256 bytes for 2 tables).\n");
        printf ("       [Pxx]      selects a VBE device at this (xx) user preference level.\n");
        printf ("       [R{+|-}xx] shifts all patch #s up|down by xx count.\n");
        printf ("       [T+|-xx]   Tempo shift faster (+) or (-) slower, in percent.\n");
        printf ("       [V{xx}]    verbose mode to dump events as they play. xx can be 1-5.\n");

        printf ("\n");

}


;   /*\
;---|*|----====< LoadMIDIFile >====----
;---|*|
;---|*| Load the MIDI file
;---|*|
;   \*/
LoadMIDIFile()
{
int n;
int fhan;
long len;
char str[80];

        if ((rfile = fopen (MIDFileName,"rb")) == 0) {

            strcpy (str,MIDFileName);
            strcat (str,".MID");

            if ((rfile = fopen (str,"rb")) == 0) {
                printf ("cannot open the MIDI file!\n");
                DoExit(1);
            }
        }

        fhan = fileno(rfile);

        readblock (fhan,(char huge *)&mhdr,sizeof(MIDIheader));

    // intel fixup

        mhdr.flen    = swapdd (mhdr.flen   );
        mhdr.ftype   = swapdw (mhdr.ftype  );
        mhdr.ftracks = swapdw (mhdr.ftracks);
        mhdr.fdiv    = swapdw (mhdr.fdiv   );

        if (VerboseMode) {
            printf ("mhdr.flen    = %ld\n",mhdr.flen    );
            printf ("mhdr.ftype   = %d\n", mhdr.ftype   );
            printf ("mhdr.ftracks = %d\n", mhdr.ftracks );
            printf ("mhdr.fdiv    = %d\n", mhdr.fdiv    );
        }

        if (mhdr.fdiv < 0) {
            printf ("This MIDI file is not based upon PPQN time!\n");
            return(0);
        }

        if (mhdr.ftracks > MAXTRACKS) {
            printf ("WARNING: This MIDI file has too many tracks!\n");
            mhdr.ftracks = MAXTRACKS;   // limit it to MAXTRACKS tracks
        }

        return(1);

}


;   /*\
;---|*|----====< LoadMIDITrack >====----
;---|*|
;---|*| Load just one MIDI Track into memory
;---|*|
;   \*/
LoadMIDITrack(m)
    MIDItrack *m;
{
int n;
int fhan;

static int thistrk = 0;

    // go direct to DOS using handles

        fhan = fileno(rfile);

    // get the header

        readblock (fhan,(char huge *)&m->mt,sizeof(MDtrack));

        if (strcmp (&m->mt.fileid,"MTrk")) {
            printf ("Not a MIDI Track!\n");
            DoExit(1);
        }

    // intel fixup

        m->mt.flen = swapdd (m->mt.flen);

    // if there is data, read it into the buffer

        if (m->mt.flen)
            readblock (fhan,(char huge *)m->p,(int)m->mt.flen);

    // flush the delta time to start everything immediately, give a trk #

        printf ("MIDI Track %d length = %ld\n",thistrk,m->mt.flen);
        m->tnum  = thistrk++;
}


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
}


;   /*\
;---|*|----====< OurSystemMSGCallBack >====----
;---|*|
;---|*| Timer tick callback for processing 0xFE messages. NOTE:
;---|*| No assumptions can be made about
;---|*| the segment registers! (DS,ES,GS,FS)
;---|*|
;   \*/

void far pascal OurSystemMSGCallBack()
{

    // setup our data segment

        _asm {

            push    ds
            mov     ax,seg SystemMsgTick
            mov     ds,ax

        }

    // save for later reporting...

        SystemMsgTick++;

    // we're done here...

        _asm {
            pop     ds
        }
}


;   /*\
;---|*|----====< OurTimerCallBack >====----
;---|*|
;---|*| Timer tick callback. NOTE: No assumptions can be made about
;---|*| the segment registers! (DS,ES,GS,FS)
;---|*|
;   \*/

void far pascal OurTimerCallBack()
{

    // setup our data segment

        _asm {

            push    ds
            mov     ax,seg CallBackOccured
            mov     ds,ax

        }

    // save for later reporting...

        CallBackOccured++;

    // we're done here...

        _asm {
            pop     ds
        }
}


;
;   /*\
;---|*|----====< KeepActive >====----
;---|*|
;---|*| Check to see if we need to send active messages
;---|*|
;   \*/
KeepActive()
{
static char activemsg = 0xFE;

    // bail if not doing active sensing

        if (!ActiveSensing)
            return(0);

    // if any message has been sent since the last system msg tick,
    // we don't have to send the active msg

        if (msgsent)
            return(msgsent = SystemMsgTick = 0);

    // send the message to keep the system from going idle

        if (SystemMsgTick) {
            PrintMsg (9,&activemsg,1);
            OutputMidiMsg (&activemsg,1);
            SystemMsgTick = 0;
        }
}


;
;   /*\
;---|*|----====< outputMIDIMsg >====----
;---|*|
;---|*| This routine is not necessary for normal midi, but is added here
;---|*| to thrash the drivers harder by making the data stream closer
;---|*| to "real world" occurances of running status, and incomplete
;---|*| message, etc. The incomplete messages are valid, but are sent to the
;---|*| driver, one byte at a time, to test it's handling of broken up
;---|*| messages. This routine will alternate between sending a whole block
;---|*| of msgs, and sending a block, one byte at a time. Remember, the
;---|*| MIDI driver is supposed to handle messages that span calls.
;---|*|
;   \*/

void OutputMidiMsg (m,l)
    char far *m;
    int l;
{
int n;
char far *s;

static int toggle = 0;
static char lastmsg = 0;
static int col = 0;

    // if in a verbose mode, then print the messages

        if (VerboseMode == 1) {
            s = m;
            for (n=l;n;n--) {
                if ((col++ & 0x0F) == 0)
                    printf ("\n");
                printf ("%02x ",*s++ & 0xFF);
            }
        }

    // test the drivers running status capaiblities

        if (lastmsg == *m) {    // if the same msg, drop the msg byte

            if (l > 1) {        // if the whole msg length is over one
                m++;            // one byte we will do this, else
                l--;            // let the whole thing pass to the driver
            }
        }
        else
            lastmsg = *m;       // not the same, so make it our next victim

    // test the drivers ability to receive incomplete messages by
    // alternately sending a full message, incremental messages, full, etc...

        if (ScatteredMessages) {

            toggle ^= 1;    // toggle the full/incremental flag

            if (toggle) {   // send it off a single byte at a time

                for (;l;l--)
                    (msv->msMIDImsg) (m++,1);

            }
            else            // send off the data as a block

                (msv->msMIDImsg) (m,l);
        }

    // scattered messages are disabled in favor of full msgs so low
    // level debugging can be done easier.

        else
            (msv->msMIDImsg) (m,l);

        msgsent = TRUE;     // we sent something...
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

    // create the string

        sprintf (sstr,"%5d ",cbcnt++);

    // blast it out...

        s = sstr;
        while (*s) {
            *vidptr++ = *s++;
            *vidptr++ = 0x3F;
        }

}


;   /*\
;---|*|----====< SetSongTempo >====----
;---|*|
;---|*| Set the song tempo from the MIDI file tempo meta event
;---|*|
;   \*/
void SetSongTempo(us,denom)
    long us;    // microseconds per quarter note beat
    int denom;  // denominator - which partial qnote gets the beat
{
long cbrate;
int sign;
long cboff;

    // calculate the callback rate

        cbrate   = (micros=us) / mhdr.fdiv; // # of us between timer callbacks
        cbrate   = 1000000/cbrate;          // calc the # of ints at this rate

        if (tempooffset) {

            sign = 0;

            if (tempooffset < 0) {
                sign = -1;
                tempooffset = -tempooffset;
            }

            cboff = cbrate * tempooffset / 100;

            if (sign)
                cboff = -cboff;

            cbrate += cboff;

        }

    // report the true rate

        printf ("Callback rate is %ld times per second\n",cbrate);
        printf ("Tempo = %ldus\n",micros);

    // reduce to a reasonable rate

        if (ScatteredMessages) {
            deltastep = 1;
            if (cbrate > 300) {

                while (cbrate > 300) {
                    deltastep <<= 1;
                    cbrate >>= 1;
                }
                printf("The callback rate is being adjusted to %ld callbacks per second\n",cbrate);
            }
        }

    ////if (denom != 2)
    ////////printf ("DENOMINATOR IS NOT A QUARTER NOTE!(%d)\n",denom);

    // make sure we get callbacks too!

        VESARegisterTimer( 255, &OurTimerCallBack, VESARateToDivisor((int)cbrate) );

}


;   /*\
;---|*|----====< swapdw >====----
;---|*|
;---|*| swap the bytes in the words
;---|*|
;   \*/

int  swapdw ( val )
    int val;
{
    _asm {
        mov     ax,[val]
        xchg    ah,al
    }
}


;   /*\
;---|*|----====< swapdd >====----
;---|*|
;---|*| swap the bytes in the long
;---|*|
;   \*/

long swapdd ( val )
    long val;
{
    _asm {
        mov     ax,word ptr [val+0]
        mov     dx,word ptr [val+2]
        xchg    ah,al
        xchg    dh,dl
        xchg    ax,dx
    }
}

;   /*\
;---|*| end of PMIDI.C
;   \*/

