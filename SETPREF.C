
;   /*\
;---|*|----====< Setpref.exe >====----
;---|*|
;---|*| Sets the preference levels of the different drivers
;---|*|
;---|*| Copyright (c) 1993,1994  V.E.S.A, Inc. All Rights Reserved.
;---|*|
;---|*| VBE/AI 1.0 Specification
;---|*|    February 2, 1994. 1.00 release
;---|*|
;---|*| Additional changes:
;---|*|    04/20 - Improved channel stealing reporting on MIDI devices.
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
        int  IncrVal = 0;
        int  DecVal  = 0;
        int  DevHan  = 0;
        int  Listing = FALSE;   // amout reported
        int  Verbose = -1;      // amout reported

#define SERVICESIZE 4096
        char services[SERVICESIZE] = {0};

    // tables for reporting attached volume info and services

#define VOLTABLESIZE 1024
        char volinf[VOLTABLESIZE] = {0};
        char volsrv[VOLTABLESIZE] = {0};


;   /*\
;---|*| prototypes
;   \*/

        long GetValue               ( char *, long );

        void far *NewAddress        ( char far * );

        PrintMIDIServices           ( fpMIDServ );
        PrintWaveServices           ( fpWAVServ );
        PrintSpecificDeviceInfo     ( int, void far * );
        PrintGeneralDeviceInfo      ( GeneralDeviceClass *,VESAHANDLE );
        PrintAttachedVolumes        ( VESAHANDLE );

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

    // process each driver

        while (-1) {

            // Find each device and adjust each

                if ((hVESA = VESAFindADevice(0)) == 0)
                    break;

                if (VESAQueryDevice(hVESA, VESAQUERY2 ,&gdc) == 0) {
                    printf ("Cannot query the installed VBE/AI devices!\n");
                    DoExit(-1);
                }

            // if we want to do just one, then skip the rest

                if (DevHan)
                    if (DevHan != hVESA)
                        continue;

            // make sure it's matches the beta version #

                if (gdc.gdvbever != VersionControl)
                    continue;

                PrintGeneralDeviceInfo(&gdc,hVESA);

                if (IncrVal||DecVal) {

                    switch (gdc.gdclassid) {

                        case WAVDEVICE:
                            printf ("    Old setting = %d  ",pref = gdc.u.gdwi.widevpref);

                            pref += IncrVal;
                            pref -= DecVal;
                            if (pref < 0)
                                pref = 0;
                            query = WAVESETPREFERENCE;

                            printf ("New setting = %d\n",pref);
                            break;

                        case MIDDEVICE:
                            printf ("    Old setting = %d  ",pref = gdc.u.gdmi.midevpref);

                            pref += IncrVal;
                            pref -= DecVal;
                            if (pref < 0)
                                pref = 0;
                            query = MIDISETPREFERENCE;

                            printf ("New setting = %d\n",pref);
                            break;

                        case VOLDEVICE:
                            break;

                        default:
                            printf ("Invalid, or unknown device class! (handle=%x)\n",hVESA);
                            query = 0;
                            break;
                    }
                }

            // if this is a WAVE/MIDI device, set the preference now

                if (query)
                    VESAQueryDevice(hVESA, query, (void far *)(pref));

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
int n,x;
char *s,c;
int vh,vl;

        vh = VersionControl >> 8;
        vl = VersionControl &  0xFF;

    // intro...

        printf ("\nVESA VBE/AI Preference setting Program, %02x.%02x\n",vh,vl);
        printf ("Copyright (c) 1993,1994  VESA, Inc. All Rights Reserved.\n\n");

    // exit if no other parameters

        if (argc < 2) {
            GiveHelps();
            return(0);
        }

    // process all the switches

        n = 1;
        while (n<argc) {

            s = argv[n++];

            switch (*s) {

                case '?':
                case 'H':
                case 'h':
                    GiveHelps();
                    DoExit(0);

                case '+':
                    IncrVal = 1;
                    DevHan  = GetValue (++s,DevHan);
                    if (DevHan)
                        printf ("Will increment %d's preference\n\n",DevHan);
                    else
                        printf ("Will increment all preferences\n\n");
                    break;

                case '-':
                    DecVal  = 1;
                    DevHan  = GetValue (++s,DevHan);
                    if (DevHan)
                        printf ("Will Decrement %d's preference\n\n",DevHan);
                    else
                        printf ("Will Decrement all preferences\n\n");
                    break;

                case 'L':   // list  all drivers
                case 'l':
                    Listing = TRUE;
                    break;

                case 'V':
                case 'v':
                    Verbose = GetValue (++s,Verbose);
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

        printf ("To Use: DOS>SETPREF [H|?] [L] [+] [+xx] [-] [-xxx] [Vxxx]\n\n");

        printf ("Where:     [L]        lists all devices.\n");
        printf ("           [+]        increments all preference levels by 1\n");
        printf ("           [+xxx]     increments a specific devices's preference.\n");
        printf ("           [-]        decrements all preference levels by 1\n");
        printf ("           [-xxx]     decrements a specific devices's preference.\n");
        printf ("           [Vxxx]     verbose listing of this devices's configuration.\n");
        printf ("\n");

        printf ("NOTE: The highest preference level is zero (0), so incrementing the\n");
        printf ("      preference actually lowers the user preference, and visa versa.\n");
        printf ("\n");

}

;
;   /*\
;---|*|----====< PrintGeneralDeviceInfo >====----
;---|*|
;---|*| Printout the table entries
;---|*|
;   \*/
PrintGeneralDeviceInfo(p,h)
    GeneralDeviceClass *p;
    VESAHANDLE h;
{
void far *vfp;


    // if verbose, give a better heading

        if (Verbose==h) {
            printf ("\n   %s General Device Class:\n\n", &p->u.gdwi.wivname);
            printf ("    char gdname[4]  = %lx\n", (long)*(long*)&p->gdname);
            printf ("    long gdlength   = %lx\n",  p->gdlength   );
            printf ("    int  gdclassid  = %x\n",   p->gdclassid  );
            printf ("    int  gdvbever   = %x\n",   p->gdvbever  );
            printf ("    ");

            vfp = NewAddress(services);

        }

        switch (p->gdclassid) {

            case WAVDEVICE:

                if ((Listing)||(Verbose==h)) {

                    printf ("WAVE device Class (handle=%x)\n",h);
                    printf ("    %s\n",  &p->u.gdwi.wivname   );
                    printf ("    %s\n",  &p->u.gdwi.wiprod    );
                    printf ("    %s\n",  &p->u.gdwi.wichip    );
                    printf ("    %lx\n",  p->u.gdwi.wiboardid  );
                    printf ("    %d preference\n\n",   p->u.gdwi.widevpref  );

                    if (Verbose==h) {

                        PrintSpecificDeviceInfo(WAVDEVICE,(void far *) &p->u.gdwi );

                        if (VESAOpenADevice ( h, 0, vfp )) {
                            PrintWaveServices((fpWAVServ)vfp);
                            VESACloseDevice ( h );
                        }
                        else
                            printf ("This WAVE device is opened, thus cannot be displayed.\n");
                    }

                    PrintAttachedVolumes(h); // print any volume services

                }
                else
                    printf ("WAVE device Class (handle=%x,pref=%d) %s\n",h,p->u.gdwi.widevpref,&p->u.gdwi.wiprod);

                break;

            case MIDDEVICE:

                if ((Listing)||(Verbose==h)) {
                    printf ("MIDI device Class (handle=%x)\n",h);
                    printf ("    %s\n",  &p->u.gdmi.mivname   );
                    printf ("    %s\n",  &p->u.gdmi.miprod    );
                    printf ("    %s\n",  &p->u.gdmi.michip    );
                    printf ("    %lx\n\n",  p->u.gdmi.miboardid );
                    printf ("    %d preference\n\n", p->u.gdmi.midevpref  );

                    if (Verbose==h) {

                        PrintSpecificDeviceInfo(MIDDEVICE,(void far *) &p->u.gdmi );

                        if (VESAOpenADevice ( h, 0, vfp )) {
                            PrintMIDIServices((fpMIDServ)vfp);
                            VESACloseDevice ( h );
                        }
                        else
                            printf ("This MIDI device is opened, thus cannot be displayed.\n");
                    }

                    PrintAttachedVolumes(h); // print any volume services
                }
                else
                    printf ("MIDI device Class (handle=%x,pref=%d) %s\n",h,p->u.gdmi.midevpref,&p->u.gdmi.miprod);

                break;

            case VOLDEVICE:

                if ((Listing)||(Verbose==h)) {
                    printf ("VOLUME device Class (handle=%x)\n",h);
                    printf ("    %s\n",  &p->u.gdvi.vivname   );
                    printf ("    %s\n",  &p->u.gdvi.viprod    );
                    printf ("    %s\n",  &p->u.gdvi.vichip    );
                    printf ("    %lx\n\n",  p->u.gdvi.viboardid );

                    if (Verbose==h)
                        PrintSpecificDeviceInfo(VOLDEVICE,(void far *) &p->u.gdvi );

                }
                else
                    printf ("VOLUME device Class (handle=%x,no preferences) %s\n",h,&p->u.gdvi.viprod);
                break;

            default:
                printf ("Invalid, or unknown device class! (handle=%x)\n",h);
                break;
        }

}

;
;   /*\
;---|*|----====< NewAddress >====----
;---|*|
;---|*| Build a far address with an offset of zero
;---|*|
;   \*/
void far *NewAddress(char far *cfp)
{
    _asm {

        mov     ax,word ptr [cfp+0];
        mov     dx,word ptr [cfp+2];
        shr     ax,4
        add     dx,ax
        inc     dx
        sub     ax,ax
    }
}

;
;   /*\
;---|*|----====< PrintAttachedVolumes >====----
;---|*|
;---|*| locate and print any volume info and services structures for the
;---|*| device handle
;---|*|
;   \*/
PrintAttachedVolumes(han)
    VESAHANDLE han;
{
fpVolInfo vi;
fpVolServ vs;

    // if there is a valid volume info structure, print it...

        if (VESAQueryDevice ( han, VESAQUERY3, 0 )) {

            if (vi = (fpVolInfo)VESAQueryDevice ( han, VESAQUERY4, (fpVolInfo)&volinf ))

                PrintSpecificDeviceInfo(VOLDEVICE,(void far *)vi);

        }

    // if there is a valid volume info structure, print it...


        if (VESAQueryDevice ( han, VESAQUERY5, 0 )) {

            if (vs = (fpVolServ)VESAQueryDevice ( han, VESAQUERY6, (fpVolServ)&volsrv ))

                PrintVolServices(vs);

        }
}

;
;   /*\
;---|*|----====< PrintSpecificDeviceInfo >====----
;---|*|
;---|*| Printout the table entries
;---|*|
;   \*/
PrintSpecificDeviceInfo(id,p)
    int id;
    void far *p;
{
fpMIDInfo mp;
fpVolInfo vp;
fpWAVInfo wp;

    // print the specific info structure

        switch (id) {

            case WAVDEVICE:

                wp = (fpWAVInfo) p;

                printf ("    char winame[4]   = %4c\n", &wp->winame    );
                printf ("    long wilength    = %lx\n",  wp->wilength  );

                printf ("    long wifeatures  = %lx\n",  wp->wifeatures);

                printf ("    int  wimemreq    = %x\n",   wp->wimemreq    );
                printf ("    int  witimerticks= %x\n",   wp->witimerticks);
                printf ("    int  wiChannels  = %x\n",   wp->wiChannels  );
                printf ("    int  wiSampleSize= %x\n\n", wp->wiSampleSize);

                // print a list of supported features

                if (wp->wifeatures & WAVEMP8K)
                    printf ("    Supports 8000hz Mono Playback.\n");

                if (wp->wifeatures & WAVEMR8K)
                    printf ("    Supports 8000hz Mono Record.\n");

                if (wp->wifeatures & WAVESR8K)
                    printf ("    Supports 8000hz Stereo Record.\n");

                if (wp->wifeatures & WAVESP8K)
                    printf ("    Supports 8000hz Stereo Playback.\n");

                if (wp->wifeatures & WAVEFD8K)
                    printf ("    Supports 8000hz Full Duplex Play/Record.\n");

                if (wp->wifeatures & WAVEMP11K)
                    printf ("    Supports 11025hz Mono Playback.\n");

                if (wp->wifeatures & WAVEMR11K)
                    printf ("    Supports 11025hz Mono Record.\n");

                if (wp->wifeatures & WAVESR11K)
                    printf ("    Supports 11025hz Stereo Record.\n");

                if (wp->wifeatures & WAVESP11K)
                    printf ("    Supports 11025hz Stereo Playback.\n");

                if (wp->wifeatures & WAVEFD11K)
                    printf ("    Supports 11025hz Full Duplex Play/Record.\n");

                if (wp->wifeatures & WAVEMP22K)
                    printf ("    Supports 22050hz Mono Playback.\n");

                if (wp->wifeatures & WAVEMR22K)
                    printf ("    Supports 22050hz Mono Record.\n");

                if (wp->wifeatures & WAVESR22K)
                    printf ("    Supports 22050hz Stereo Record.\n");

                if (wp->wifeatures & WAVESP22K)
                    printf ("    Supports 22050hz Stereo Playback.\n");

                if (wp->wifeatures & WAVEFD22K)
                    printf ("    Supports 22050hz Full Duplex Play/Record.\n");

                if (wp->wifeatures & WAVEMP44K)
                    printf ("    Supports 44100hz Mono Playback.\n");

                if (wp->wifeatures & WAVEMR44K)
                    printf ("    Supports 44100hz Mono Record.\n");

                if (wp->wifeatures & WAVESR44K)
                    printf ("    Supports 44100hz Stereo Record.\n");

                if (wp->wifeatures & WAVESP44K)
                    printf ("    Supports 44100hz Stereo Playback.\n");

                if (wp->wifeatures & WAVEFD44K)
                    printf ("    Supports 44100hz Full Duplex Play/Record.\n");

                if (wp->wifeatures &
                    (WAVEVARIPMONO+WAVEVARIPSTER+WAVEVARIRMONO+WAVEVARIRSTER))
                    printf ("    Supports Variable Sample Rates\n");

                // print the different sample sizes

                if (wp->wiSampleSize & WAVE08BITPLAY)
                    printf ("    Supports 8 bit PCM playback\n");

                if (wp->wiSampleSize & WAVE08BITREC )
                    printf ("    Supports 8 bit PCM record\n");

                if (wp->wiSampleSize & WAVE16BITPLAY)
                    printf ("    Supports 16 bit PCM playback\n");

                if (wp->wiSampleSize & WAVE16BITREC )
                    printf ("    Supports 16 bit PCM record\n");
                break;

            case MIDDEVICE:

                mp = (fpMIDInfo) p;

                printf ("    char miname[4]   = %4c\n", &mp->miname    );
                printf ("    long milength    = %lx\n",  mp->milength  );

                printf ("    long milibrary   = %s\n",  &mp->milibrary );

                printf ("    long mifeatures  = %lx\n",  mp->mifeatures);

                printf ("    int  mimemreq    = %x\n",   mp->mimemreq     );
                printf ("    int  mitimerticks= %x\n",   mp->mitimerticks );
                printf ("    int  miactivetons= %x\n",   mp->miactivetones);

                break;

            case VOLDEVICE:

                vp = (fpVolInfo) p;

                printf ("    char viname[4]   = %4c\n", &vp->viname    );
                printf ("    long vilength    = %lx\n",  vp->vilength  );

                printf ("    long vicname     = %s\n",  &vp->vicname   );

                printf ("    long vifeatures  = %lx\n",  vp->vifeatures);
                printf ("    int  vimin       = %x\n",   vp->vimin     );
                printf ("    int  vimax       = %x\n",   vp->vimax     );
                printf ("    int  vicross     = %x\n",   vp->vicross   );

                break;

            default:
                printf ("Invalid, or unknown device class!\n");
                break;
        }
        printf ("\n");

}

;
;   /*\
;---|*|----====< PrintMIDIServices >====----
;---|*|
;---|*| Printout the table entries
;---|*|
;   \*/
PrintMIDIServices(msv)
fpMIDServ msv;
{
        printf ("    MIDI Services:\n");
        printf ("    char msname[4]  = %4Fc\n", &msv->msname   );
        printf ("    long mslength   = %lx\n",   msv->mslength );

        // device driver supplied function

        printf ("    msDeviceCheck   = %x:%x\n",(int)(((long)msv->msDeviceCheck) >>16), (int)msv->msDeviceCheck  );
        printf ("    msGlobalReset   = %x:%x\n",(int)(((long)msv->msGlobalReset) >>16), (int)msv->msGlobalReset  );
        printf ("    msMIDImsg       = %x:%x\n",(int)(((long)msv->msMIDImsg)     >>16), (int)msv->msMIDImsg      );
        printf ("    msPreLoadPatch  = %x:%x\n",(int)(((long)msv->msPreLoadPatch)>>16), (int)msv->msPreLoadPatch );
        printf ("    msTimerTick     = %x:%x\n",(int)(((long)msv->msTimerTick)   >>16), (int)msv->msTimerTick    );
        printf ("    msGetLastError  = %x:%x\n",(int)(((long)msv->msGetLastError)>>16), (int)msv->msGetLastError );

        // device driver run-information time data

        printf ("    msApplFreeCB    = %x:%x\n",(int)(((long)msv->msApplFreeCB)>>16),(int)msv->msApplFreeCB );
        printf ("    msApplMIDIIn    = %x:%x\n",(int)(((long)msv->msApplMIDIIn)>>16),(int)msv->msApplMIDIIn );

        printf ("    mspatches[0]     = %x\n",msv->mspatches[0]    );
        printf ("    mspatches[1]     = %x\n",msv->mspatches[1]    );
        printf ("    mspatches[2]     = %x\n",msv->mspatches[2]    );
        printf ("    mspatches[3]     = %x\n",msv->mspatches[3]    );
        printf ("    mspatches[4]     = %x\n",msv->mspatches[4]    );
        printf ("    mspatches[5]     = %x\n",msv->mspatches[5]    );
        printf ("    mspatches[6]     = %x\n",msv->mspatches[6]    );
        printf ("    mspatches[7]     = %x\n",msv->mspatches[7]    );
        printf ("    mspatches[8]     = %x\n",msv->mspatches[8]    );
        printf ("    mspatches[9]     = %x\n",msv->mspatches[9]    );
        printf ("    mspatches[A]     = %x\n",msv->mspatches[0xA]    );
        printf ("    mspatches[B]     = %x\n",msv->mspatches[0xB]    );
        printf ("    mspatches[C]     = %x\n",msv->mspatches[0xC]    );
        printf ("    mspatches[D]     = %x\n",msv->mspatches[0xD]    );
        printf ("    mspatches[E]     = %x\n",msv->mspatches[0xE]    );
        printf ("    mspatches[F]     = %x\n",msv->mspatches[0xF]    );

        printf ("\n");

		printf ("    DC: tones      = %lx\n",
			(msv->msDeviceCheck)(MIDITONES, 		0));
		printf ("    DC: patch type = %lx\n",
			(msv->msDeviceCheck)(MIDIPATCHTYPE, 	0));
		printf ("    DC: preference = %lx\n",
			(msv->msDeviceCheck)(MIDISETPREFERENCE,-1));
		printf ("    DC: ch stealing= %lx\n",
			(msv->msDeviceCheck)(MIDIVOICESTEAL,   0xFFFF0002));
		printf ("    DC: fifo sizes = %lx\n",
			(msv->msDeviceCheck)(MIDIGETFIFOSIZES,	0));
		printf ("    DC: DMA/IRQ    = %lx\n",
			(msv->msDeviceCheck)(MIDIGETDMAIRQ, 	0));
		printf ("    DC: I/O address= %lx\n",
			(msv->msDeviceCheck)(MIDIGETIOADDRESS,	0));
		printf ("    DC: mem address= %lx\n",
			(msv->msDeviceCheck)(MIDIGETMEMADDRESS, 0));
		printf ("    DC: mem free   = %lx\n",
			(msv->msDeviceCheck)(MIDIGETMEMFREE,	0));
        printf ("\n");

}

;
;   /*\
;---|*|----====< PrintVolServices >====----
;---|*|
;---|*| Printout the table entries
;---|*|
;   \*/
PrintVolServices(vfn)
fpVolServ vfn;
{
        printf ("    Volume Services:\n");
        printf ("    char vsname[4]  = %4c\n", &vfn->vsname   );
        printf ("    long vslength   = %lx\n",  vfn->vslength );

        // device driver supplied function

        printf ("    vsDeviceCheck   = %x:%x\n",(int)(((long)vfn->vsDeviceCheck) >>16), (int)vfn->vsDeviceCheck  );
        printf ("    vsSetVolume     = %x:%x\n",(int)(((long)vfn->vsSetVolume)   >>16), (int)vfn->vsSetVolume    );
        printf ("    vsSetFieldVol   = %x:%x\n",(int)(((long)vfn->vsSetFieldVol) >>16), (int)vfn->vsSetFieldVol  );
        printf ("    vsToneControl   = %x:%x\n",(int)(((long)vfn->vsToneControl) >>16), (int)vfn->vsToneControl  );
        printf ("    vsFilterControl = %x:%x\n",(int)(((long)vfn->vsFilterControl)>>16),(int)vfn->vsFilterControl);
        printf ("    vsOutputPath    = %x:%x\n",(int)(((long)vfn->vsOutputPath)  >>16), (int)vfn->vsOutputPath   );
        printf ("    vsResetChannel  = %x:%x\n",(int)(((long)vfn->vsResetChannel)>>16), (int)vfn->vsResetChannel );
        printf ("    vsGetLastError  = %x:%x\n",(int)(((long)vfn->vsGetLastError)>>16), (int)vfn->vsGetLastError );

        printf ("\n");

}

;
;   /*\
;---|*|----====< PrintWaveServices >====----
;---|*|
;---|*| Printout the table entries
;---|*|
;   \*/
PrintWaveServices(wfn)
fpWAVServ wfn;
{
        printf ("    Wave Services:\n");
        printf ("    char wsname[4]  = %4c\n", &wfn->wsname   );
        printf ("    long wslength   = %lx\n",  wfn->wslength );

        // device driver supplied function

        printf ("    wsDeviceCheck   = %x:%x\n",(int)(((long)wfn->wsDeviceCheck) >>16), (int)wfn->wsDeviceCheck  );
        printf ("    wsPCMInfo       = %x:%x\n",(int)(((long)wfn->wsPCMInfo)     >>16), (int)wfn->wsPCMInfo      );
        printf ("    wsPlayBlock     = %x:%x\n",(int)(((long)wfn->wsPlayBlock)   >>16), (int)wfn->wsPlayBlock    );
        printf ("    wsPlayCont      = %x:%x\n",(int)(((long)wfn->wsPlayCont)    >>16), (int)wfn->wsPlayCont     );
        printf ("    wsRecordBlock   = %x:%x\n",(int)(((long)wfn->wsRecordBlock) >>16), (int)wfn->wsRecordBlock  );
        printf ("    wsRecordCont    = %x:%x\n",(int)(((long)wfn->wsRecordCont)  >>16), (int)wfn->wsRecordCont   );
        printf ("    wsPauseIO       = %x:%x\n",(int)(((long)wfn->wsPauseIO)     >>16), (int)wfn->wsPauseIO      );
        printf ("    wsResumeIO      = %x:%x\n",(int)(((long)wfn->wsResumeIO)    >>16), (int)wfn->wsResumeIO     );
        printf ("    wsStopIO        = %x:%x\n",(int)(((long)wfn->wsStopIO)      >>16), (int)wfn->wsStopIO       );
        printf ("    wsWavePrepare   = %x:%x\n",(int)(((long)wfn->wsWavePrepare) >>16), (int)wfn->wsWavePrepare  );
        printf ("    wsTimerTick     = %x:%x\n",(int)(((long)wfn->wsTimerTick)   >>16), (int)wfn->wsTimerTick    );
        printf ("    wsGetLastError  = %x:%x\n",(int)(((long)wfn->wsGetLastError)>>16), (int)wfn->wsGetLastError );

        // device driver run-information time data

        printf ("    wsApplPSyncCB   = %x:%x\n",(int)(((long)wfn->wsApplPSyncCB)>>16),(int)wfn->wsApplPSyncCB );
        printf ("    wsApplRSyncCB   = %x:%x\n",(int)(((long)wfn->wsApplRSyncCB)>>16),(int)wfn->wsApplRSyncCB );

        printf ("    DC: DMA/IRQ    = %lx\n",(wfn->wsDeviceCheck)(WAVEGETDMAIRQ,     0));
        printf ("    DC: IO address = %lx\n",(wfn->wsDeviceCheck)(WAVEGETIOADDRESS,  0));
        printf ("    DC: MEM address= %lx\n",(wfn->wsDeviceCheck)(WAVEGETMEMADDRESS, 0));
        printf ("    DC: MEM free   = %lx\n",(wfn->wsDeviceCheck)(WAVEGETMEMFREE,    0));
        printf ("    DC: block size = %lx\n",(wfn->wsDeviceCheck)(WAVEGETBLOCKSIZE,  0));

        printf ("\n");

}




