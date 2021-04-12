
;   /*\
;---|*|----====< build.c >====----
;---|*|
;---|*| This program displays the contents of the selected bank patches,
;---|*| then dumps it to disk
;---|*|
;---|*| Copyright (c) 1993,1994  V.E.S.A, Inc. All Rights Reserved.
;---|*|
;---|*| VBE/AI 1.0 Specification
;---|*|    February 2, 1994. 1.00 release
;---|*|
;   \*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "bank.h"
#include "..\..\..\vbeai.h"

;   /*\
;---|*|----====< global constants >====----
;   \*/

#define TRUE    -1
#define FALSE   0

        FILE *fn;                       // pointer to the stream
        FILE *ouf;                      // output file
		FILE *oufc; 					// output file compressed patches

        BANKHEADER  Header;
        TIMBRE      Instr;

        INSHEADER   InstrList[1000];    // array of 1000 instrument headers
        int         InstrIndex;         // current index to the instrument array
        int         InstrChanged;       // changes have occured flag
        int         NewFreq = 60;       // percussive patch frequencies

#define INSTMAX 256
        int     instidx    = 0;
        long    instruments[256] = {0};

        long    vaipoffset = 0;     // offset to the patch chunk header

;   /*\
;---|*|---====< VBE/AI OPL2 Patch Structure >====----
;   \*/

        typedef struct {
            char    opl2ksl;       // ksl
            char    opl2freqMult;  // freqMult
            char    opl2feedBack;  // feedBack  // used by operator 0 only
            char    opl2attack;    // attack
            char    opl2sustLevel ;// sustLevel
            char    opl2sustain;   // sustain
            char    opl2decay;     // decay
            char    opl2release;   // release
            char    opl2output;    // output
            char    opl2am;        // am
            char    opl2vib;       // vib
            char    opl2ksr;       // ksr
            char    opl2fm;        // fm        // used by operator 0 only
        } opl2opr;

        typedef struct {
            int     opl2type;      // patch type
            char    opl2mode;      // mode:  0 = melodic, 1 = percussive
            char    opl2percVoice; // percVoice: if mode == 1, voice number to be used
            opl2opr opl2op0;       // operator 0 parameters;
            opl2opr opl2op1;       // operator 1 parameters;
            char    opl2wave0;     // waveform for operator 0
            char    opl2wave1;     // waveform for operator 1
        } opl2patch;

        opl2patch opl2inst =
            {
              0,0,
              0,0,0,0,0,0,0,0,0,0,0,0,0,
              0,0,0,0,0,0,0,0,0,0,0,0,0,
              0,0
            };

    // the following structures are listed with indentation
    // to indicate nesting levels of the VBE/AI patch library

        RIFFhdr riffhdr = { "RIFF", 0L };
            VAILhdr vailhdr = { "vail", 0L };
                ZSTRhdr zstrhdr = { "ZSTR", 0L };
                VAIPhdr vaiphdr = { "vaip", 0L };
                    VAIIhdr vaiihdr = { "vaii", 8L, 0L, 0L };
                    VAIDhdr vaidhdr = { "vaid", 0L };


;   /*\
;---|*|----====< prototypes >====----
;   \*/

        void    DisplayInstrument   ( FILE *,int, int );
        void    DoExit              ( int );
        int     GetEntry            ( );
        void    OpenBankFile        ( int, char * );
        int     CopyToNew           ( opl2patch *, TIMBRE *, int );
        void    DumpInstrument      ( int );

        void    LoadTextStrings     ( );
                WriteRIFFHeader     ( FILE *, long );
				WriteInstrument 	( opl2patch far *);

// code necessary for linking PATCH.ASM

		char driverdata[4];
		wrtIdxDta()  { };
		wrtIdxDta0() { };
		wrtIdxDta2() { };
		char far * xlatpatch		( );	// ONLY A NEAR FUNCTION!



;   /*\
;---|*|------------------------==============================---------------------
;---|*|---------------------====< Start of Execution >====---------------------
;---|*|---------------------==============================---------------------
;   \*/

main(argc,argv)
    int argc;
    char *argv[];
{
int index;
fpos_t len;

    // get the bank file name

        OpenBankFile(argc,argv[1]);     // will not return if bad file read

    // open the target bank file

		ouf  = fopen ("output.bnk","wb+");
		oufc = fopen ("patch.inc","w+");

    // process the list till done

        WriteRIFFHeader (ouf,0);

        while (1) {

            if ((index = GetEntry()) <= -1)
                break;

            DumpInstrument(index);

        }

        fgetpos (ouf,&len);
        WriteRIFFHeader (ouf,(long)len);    // len = total file length
        DoExit(0);
}


;   /*\
;---|*|------------------------=======================------------------------
;---|*|------------------------====< Subroutines >====------------------------
;---|*|------------------------=======================------------------------
;   \*/


;   /*\
;---|*|----====< DisplayInstrument ( int, int ) >====----
;---|*|
;---|*| This displays the instrument contents, thats all...
;---|*|
;---|*| Entry Conditions:
;---|*|    None
;---|*|
;---|*| Exit Conditions:
;---|*|    None
;---|*|
    \*/
void DisplayInstrument(f,index,cls)
    FILE *f;
    int index;
    int cls;
{
TIMBRE *i;

    // print each element

        i = &Instr;

////    fprintf (f,"Index=%-3d Instrument Name \"%s\" Offset=%-3d\n",index+1,InstrList[index].name,InstrList[index].index);
////    fprintf (f,"============================================\n");
////    fprintf (f,"              1)mode  : %2u\n",i->mode );
////    fprintf (f,"              2)Voice : %2u\n",i->percVoice );
////    fprintf (f,"==== OPERATOR 0 ====    ==== OPERATOR 1 ====\n");
////    fprintf (f,"  3) ksl      = %4u    17) ksl      = %4u  \n",i->op0.ksl      , i->op1.ksl      );
////    fprintf (f,"  4) freqMult = %4u    18) freqMult = %4u  \n",i->op0.freqMult , i->op1.freqMult );
////    fprintf (f,"  5) feedBack = %4u    19) N/U (PAN)  %4u  \n",i->op0.feedBack , i->op1.feedBack );
////    fprintf (f,"  6) attack   = %4u    20) attack   = %4u  \n",i->op0.attack   , i->op1.attack   );
////    fprintf (f,"  7) sustLevel= %4u    21) sustLevel= %4u  \n",i->op0.sustLevel, i->op1.sustLevel);
////    fprintf (f,"  8) sustain  = %4u    22) sustain  = %4u  \n",i->op0.sustain  , i->op1.sustain  );
////    fprintf (f," 10) decay    = %4u    23) decay    = %4u  \n",i->op0.decay    , i->op1.decay    );
////    fprintf (f," 11) release  = %4u    24) release  = %4u  \n",i->op0.release  , i->op1.release  );
////    fprintf (f," 12) output   = %4u    25) output   = %4u  \n",i->op0.output   , i->op1.output   );
////    fprintf (f," 13) am       = %4u    26) am       = %4u  \n",i->op0.am       , i->op1.am       );
////    fprintf (f," 14) vib      = %4u    27) vib      = %4u  \n",i->op0.vib      , i->op1.vib      );
////    fprintf (f," 15) ksr      = %4u    28) ksr      = %4u  \n",i->op0.ksr      , i->op1.ksr      );
////    fprintf (f," 16) fm       = %4u    29) N/U        %4u  \n",i->op0.fm       , i->op1.fm       );
////    fprintf (f,"             30)op0 wave : %2u\n", i->wave0 );
////    fprintf (f,"             31)op1 wave : %2u\n", i->wave1 );
////

}

;   /*\
;---|*|----====< DisplayNewInstrument ( int, int ) >====----
;---|*|
;---|*| This displays the instrument contents, thats all...
;---|*|
;---|*| Entry Conditions:
;---|*|    None
;---|*|
;---|*| Exit Conditions:
;---|*|    None
;---|*|
    \*/
void DisplayNewInstrument(f,index,d)
    FILE *f;
    int index;
    opl2patch *d;
{

    // print each element

////    fprintf (f,"============================================\n");
////    fprintf (f,"              1)mode  : %2u\n",d->opl2mode );
////    fprintf (f,"              2)Voice : %2u\n",d->opl2percVoice );
////    fprintf (f,"==== OPERATOR 0 ====    ==== OPERATOR 1 ====\n");
////    fprintf (f,"  3) ksl      = %4u    17) ksl      = %4u  \n",d->opl2op0.opl2ksl      , d->opl2op1.opl2ksl      );
////    fprintf (f,"  4) freqMult = %4u    18) freqMult = %4u  \n",d->opl2op0.opl2freqMult , d->opl2op1.opl2freqMult );
////    fprintf (f,"  5) feedBack = %4u    19) N/U (PAN)  %4u  \n",d->opl2op0.opl2feedBack , d->opl2op1.opl2feedBack );
////    fprintf (f,"  6) attack   = %4u    20) attack   = %4u  \n",d->opl2op0.opl2attack   , d->opl2op1.opl2attack   );
////    fprintf (f,"  7) sustLevel= %4u    21) sustLevel= %4u  \n",d->opl2op0.opl2sustLevel, d->opl2op1.opl2sustLevel);
////    fprintf (f,"  8) sustain  = %4u    22) sustain  = %4u  \n",d->opl2op0.opl2sustain  , d->opl2op1.opl2sustain  );
////    fprintf (f," 10) decay    = %4u    23) decay    = %4u  \n",d->opl2op0.opl2decay    , d->opl2op1.opl2decay    );
////    fprintf (f," 11) release  = %4u    24) release  = %4u  \n",d->opl2op0.opl2release  , d->opl2op1.opl2release  );
////    fprintf (f," 12) output   = %4u    25) output   = %4u  \n",d->opl2op0.opl2output   , d->opl2op1.opl2output   );
////    fprintf (f," 13) am       = %4u    26) am       = %4u  \n",d->opl2op0.opl2am       , d->opl2op1.opl2am       );
////    fprintf (f," 14) vib      = %4u    27) vib      = %4u  \n",d->opl2op0.opl2vib      , d->opl2op1.opl2vib      );
////    fprintf (f," 15) ksr      = %4u    28) ksr      = %4u  \n",d->opl2op0.opl2ksr      , d->opl2op1.opl2ksr      );
////    fprintf (f," 16) fm       = %4u    29) N/U        %4u  \n",d->opl2op0.opl2fm       , d->opl2op1.opl2fm       );
////    fprintf (f,"             30)op0 wave : %2u\n", d->opl2wave0 );
////    fprintf (f,"             31)op1 wave : %2u\n", d->opl2wave1 );
////

}


;   /*\
;---|*|----====< CopyToNew >====----
;---|*|
;---|*| Copy the instrument to the VBE/AI format
;---|*|
;---|*| Entry Conditions:
;---|*|    None
;---|*|
;---|*| Exit Conditions:
;---|*|    None
;---|*|
    \*/

CopyToNew (d,s,p)
    opl2patch *d;
    TIMBRE *s;
    int p;  // patch #
{

    // place the specified frequency in the old percussive field

        d->opl2percVoice        = NewFreq | 0x80;

        if (s->mode)
            printf ("\aThere is a non-melodic mode specified in patch #%d!\n",p);

        d->opl2type             = MIDI_PATCH_OPL2;
        d->opl2mode             = s->mode;

        d->opl2op0.opl2ksl      = s->op0.ksl;
        d->opl2op0.opl2freqMult = s->op0.freqMult;
        d->opl2op0.opl2feedBack = s->op0.feedBack;
        d->opl2op0.opl2attack   = s->op0.attack;
        d->opl2op0.opl2sustLevel= s->op0.sustLevel;
        d->opl2op0.opl2sustain  = s->op0.sustain;
        d->opl2op0.opl2decay    = s->op0.decay;
        d->opl2op0.opl2release  = s->op0.release;
        d->opl2op0.opl2output   = s->op0.output;
        d->opl2op0.opl2am       = s->op0.am;
        d->opl2op0.opl2vib      = s->op0.vib;
        d->opl2op0.opl2ksr      = s->op0.ksr;
        d->opl2op0.opl2fm       = s->op0.fm;

        d->opl2op1.opl2ksl      = s->op1.ksl;
        d->opl2op1.opl2freqMult = s->op1.freqMult;
        d->opl2op1.opl2feedBack = s->op1.feedBack;
        d->opl2op1.opl2attack   = s->op1.attack;
        d->opl2op1.opl2sustLevel= s->op1.sustLevel;
        d->opl2op1.opl2sustain  = s->op1.sustain;
        d->opl2op1.opl2decay    = s->op1.decay;
        d->opl2op1.opl2release  = s->op1.release;
        d->opl2op1.opl2output   = s->op1.output;
        d->opl2op1.opl2am       = s->op1.am;
        d->opl2op1.opl2vib      = s->op1.vib;
        d->opl2op1.opl2ksr      = s->op1.ksr;
        d->opl2op1.opl2fm       = s->op1.fm;

        d->opl2wave0            = s->wave0;
        d->opl2wave1            = s->wave1;
}


;   /*\
;---|*|----====< DoExit(int) >====----
;---|*|
;---|*| Shutdown and exit to DOS
;---|*|
;---|*| Entry Conditions:
;---|*|    int = completion code
;---|*|
;---|*| Exit Conditions:
;---|*|    None
;---|*|
;   \*/
void DoExit(cc)
   int cc;
{

    // if the instrument bank is opened, close to save all...

        if (fn)
            fclose (fn);

        if (ouf)
            fclose (ouf);

		if (oufc) {
			fputs ("\n\n",oufc);
			fclose (oufc);
		}

    // bye...

        exit(cc);
}


;   /*\
;---|*|----====< DumpInstrument() >====----
;---|*|
;---|*| This displays the instrument contents, then Dumps the
;---|*| operator to the output file.
;---|*|
;---|*| Entry Conditions:
;---|*|     int = the index of the instrument
;---|*|
;---|*| Exit Conditions:
;---|*|    None
;---|*|
;   \*/
void DumpInstrument(index)
    int index;
{
long pos;
char buff[11];

    // position the file pointer to that record

        pos = (InstrList[index].index * sizeof(TIMBRE)) + Header.offsetTimbre;
        fseek ( fn, pos, SEEK_SET );

    // read in the instrument record

        if (fread(&Instr,sizeof(TIMBRE),1,fn) != 1) {
            printf ("Cannot read that instrument!\n");
            return;
        }

        if (Instr.mode)
            printf ("===============Percussive mode patch!=================\n");



    // display and edit, display and edit, display and edit...

        DisplayInstrument(stdout,index,1);

        CopyToNew (&opl2inst,&Instr,index);

        DisplayNewInstrument(stdout,index,&opl2inst);

        WriteInstrument(&opl2inst);

}

;   /*\
;---|*|----====< GetEntry >====----
;---|*|
;---|*| Get the next bank index #
;---|*|
;---|*| Entry Conditions:
;---|*|    none
;---|*|
;---|*| Exit Conditions:
;---|*|    -1 if done, else 0+ for the index
;---|*|
;   \*/

int GetEntry()
{
char buff[100];
int result;

    while (1) {

        if (feof(stdin)) {
            printf ("end of input\n");
            return(-1);
        }

        printf ("\nEnter an Index, (0 to exit, [ENTER] for next page) :");

        fgets(buff,99,stdin);
        printf ("%s\n",buff);

        if (isdigit(buff[0]))
            if (sscanf(buff,"%d %d",&result,&NewFreq) == 2)
                return (--result);
    }
}


;   /*\
;---|*|----====< OpenBankFile >====----
;---|*|
;---|*| Open the bank file, and read in the header block.
;---|*|
;---|*| Entry Conditions:
;---|*|
;---|*|    char *f is the possible file name
;---|*|
;---|*| Exit Conditions:
;---|*|    if in error, the routine returns directly to DOS.
;---|*|
;   \*/

void OpenBankFile (argc,f)
    int argc;
    char *f;
{
INSHEADER Iname; // current instrment header

    // if no arguments, just give helps

        if (argc < 2) {
            printf ("\nBANK -- By Media Vision, Inc.\n\n");
            printf ("To Use: DOS>BANK [FILENAME.BNK] [CREDITS]\n\n",f);
            exit(0);
        }

    // attemp the open

        if ((fn = fopen (f,"r+b")) == 0) {

            printf ("Cannot open the bank file: \"%s\"\n",f);
            DoExit (-1);
        }

    // read in the header

        if (fread(&Header,sizeof(BANKHEADER),1,fn) != 1) {

            printf ("Cannot read the bank's header record!");
            DoExit (-1);
        }

    // exit if no index entries are used

        if (Header.nrDefined == 0) {

            printf ("There are no valid entries in this bank!");
            DoExit (-1);
        }

    // load the instrument index into memory starting with the first record

        InstrIndex = 0;

    // read each header till done...

            do {

                if (fread(&Iname,sizeof(INSHEADER),1,fn) != 1) {
                    printf ("Unexpected end of file read the index!\n");
                    DoExit(-1);
                }

            // save only if the data is valid

                if (Iname.used) {

                    // save the instrument index data

                        memcpy
                        (
                            (void *)&InstrList[InstrIndex++],
                            (void *)&Iname,
                            sizeof (INSHEADER)
                        );
                }

        } while (InstrIndex != Header.nrDefined);

    // InstrIndex will point to the last good entry

        InstrIndex--;

}


;   /*\
;---|*|----====< WriteInstrument >====----
;---|*|
;---|*| Write the instrument out to disk
;---|*|
;---|*| Entry Conditions:
;---|*|    none
;---|*|
;---|*| Exit Conditions:
;---|*|    none
;---|*|
;   \*/

WriteInstrument(d)
	opl2patch far *d;
{
int n;
fpos_t p;
char *s = (char *) &vaidhdr;
char far *np;

static int firstpass = 1;
static int patchnum;

    // build the header

        vaidhdr.vaidlen = sizeof(opl2patch);

    // if we get the file position, save it in the table

        if (!fgetpos (ouf,&p))
            instruments[instidx++] = (long)p - vaipoffset;

    // write the patch header to disk

        for (n=sizeof(VAIDhdr);n;n--)
            fputc (*s++,ouf);

    // write the patch to disk

        s = (char *)d;
        for (n=sizeof(opl2patch);n;n--)
            fputc (*s++,ouf);

	// write out the compressed form of the patch

		if (firstpass) {

            firstpass = 0;

			fputs ("\tpage    63,131\n",oufc);
			fputs ("\tSubttl  Copyright (c) 1993,1994  V.E.S.A, Inc. All Rights Reserved.\n",oufc);
			fputs ("\n",oufc);
			fputs (";   /*\\ \n",oufc);
			fputs (";---|*|----------------------====< PATCH DATA >====-------------------------\n",oufc);
			fputs (";   \\*/\n",oufc);
            fputs ("\n",oufc);
			fputs ("\tpublic\tpatchtable\n",oufc);
            fputs ("patchtable label    byte\n",oufc);

			patchnum = 0;
		}

	// first translate it to the compressed format

		_asm {

			push	es
			push	si
			les 	si,[d]
			mov 	bx,1
			mov 	cx,[patchnum]
			cmp 	cl,128
			jb		_xxxx
			mov 	ch,9
		_xxxx:
			call	xlatpatch
			mov 	[np+2],dx
			mov 	[np+0],ax
		}

	// now, write it out...

		fputs ("  db  ",oufc);
		for (n=10;n;n--) {
			fprintf (oufc,"0%02xh",*np++ & 0xFF);
			if (n!=1)
				fprintf (oufc,",");
		}
		fprintf (oufc,"\t; patch #%d\n",patchnum++);

}


;   /*\
;---|*|----====< WriteRIFFHeader >====----
;---|*|
;---|*| Write the instrument out to disk
;---|*|
;---|*| Entry Conditions:
;---|*|    none
;---|*|
;---|*| Exit Conditions:
;---|*|    none
;---|*|
;   \*/

WriteRIFFHeader (ouf,len)
    FILE *ouf;
    long len;
{
char *s;
int n,x;
fpos_t p;
long l;

    // start at the beginning

        fsetpos (ouf,0);

    // write the "RIFF" header

        len -= sizeof (RIFFhdr);    // remove this length

        s = (char *) &riffhdr;
        riffhdr.rcount = len;

        for (n=sizeof(RIFFhdr);n;n--)
            fputc (*s++,ouf);

    // write the "vail" header

        len -= sizeof (VAILhdr);    // remove this length

        s = (char *) &vailhdr;
        vailhdr.pcount = len;

        for (n=sizeof(VAILhdr);n;n--)
            fputc (*s++,ouf);

    // write the "ZSTR" headers

        LoadTextStrings();

    // write the "vaip" header

        if (!fgetpos (ouf,&p))
            vaipoffset = (long)p;

        vaiphdr.vaiplen =   (instidx * sizeof(VAIIhdr))
                          + (instidx * sizeof(VAIDhdr))
                          + (instidx * sizeof(opl2patch));

        s = (char *) &vaiphdr;
        for (n=sizeof(VAIPhdr);n;n--)
            fputc (*s++,ouf);

    // write the "vaii" headers

        for (x=0;x<INSTMAX;x++) {

            // move the patch offset into the array

            vaiihdr.poffset = instruments[x];

            // move the patch length into the structure

            vaiihdr.vaidln = sizeof (opl2patch);

            // write out the array

            s = (char *) &vaiihdr;
            for (n=sizeof(VAIIhdr);n;n--)
                fputc (*s++,ouf);
        }

}


;   /*\
;---|*|----====< LoadTextSTrings >====----
;---|*|
;---|*| Give credit for the bank file
;---|*|
;---|*| Entry Conditions:
;---|*|    none
;---|*|
;---|*| Exit Conditions:
;---|*|    none
;---|*|
;   \*/

void LoadTextStrings()
{
FILE *tfil;
char str[100],*s;
int n;

ZSTRhdr thd;

        thd.type[0] = 'Z';
        thd.type[1] = 'S';
        thd.type[2] = 'T';
        thd.type[3] = 'R';

        if ((tfil = fopen ("credits","r")) ==0)
            return;

        while (fgets(str,99,tfil)) {

            s = str;
            for (n=0;n<99;n++) {
                if (*s == 0x0A)
                    *s == 0;
                if (*s == 0x0D)
                    *s == 0;
                if (!*s++)
                    break;
            }

            if ((thd.tlen = n) == 0)
                break;

            s = (char *)&thd;
            for (n=8;n;n--)
                fputc (*s++,ouf);

            s = str;
            for (n=thd.tlen;n;n--)
                fputc (*s++,ouf);

        }

        fclose (tfil);
}

;   /*\
;---|*| end of build.c
;   \*/

