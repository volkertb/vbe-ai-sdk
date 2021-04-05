
;   /*\
;---|*|----====< VESA >====----
;---|*|
;---|*| high level support routines for the VESA AI interface
;---|*|
;---|*| Copyright (c) 1993,1994  V.E.S.A, Inc. All Rights Reserved.
;---|*|
;---|*| Portions Copyright (c) 1993 Jason Blochowiak, Argo Games. Used
;---|*| with permission.
;---|*|
;---|*| VBE/AI 1.0 Specification
;---|*|    February 2, 1994. 1.00 release
;---|*|
;   \*/

#include <stdio.h>
#include <stdlib.h>
#include <direct.h>
#include <string.h>
#include <dos.h>
#include <io.h>
#include <conio.h>

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
;---|*| Global Variables
;   \*/

        // nada...

;   /*\
;---|*| Local Variables
;   \*/

#define CALLBACKS       16  // Maximum number of callbacks that can be installed
#define MAXNESTEDCBS    3   // Maximum nesting of timer interrupts

        typedef struct {
            VESAHANDLE  handle;                         // The device handle
            void        (far pascal *callBack)(void);   // The callback function
            short       semaphore;                      // Our lockout semaphore for this callback
            long        divisor;                        // The timer divisor this callback wants
            long        divCount;                       // Overflow counter for this callback
        } Timer_t;

    // timer callback control table

        static volatile Timer_t timers[CALLBACKS];

    // a table to store data from device info blocks

        static int  doreg[CALLBACKS][2]    =      // do give ticks to this dev
            {{0,0},{0,0},{0,0},{0,0},
             {0,0},{0,0},{0,0},{0,0},
             {0,0},{0,0},{0,0},{0,0},
             {0,0},{0,0},{0,0},{0,0}};

        static int  denom      = 18;            // decimation denominator
        static int  isopen     = 0;             // number of devices opened

        static int  timerhooked = FALSE;        // TRUE if timer is currently hooked
        static int  curnestedtimer = 0;         // Number of nested timer interrupts
        static long curtimerdiv = 0x10000L;     // Current hardware timer divisor
        static long curtimerslop = 0;           // Number of timer counts that we've missed, due to excessive nesting

    // general prototypes

        static int  cmpstr                      ( char far *, char far *, int );
        static int  readblock                   ( int, char huge *, int );

    // timer prototypes

        static int  findhandleindex ( VESAHANDLE han );
        static void setuptimer      ( void );
        static void programtimer    ( unsigned );
        static void interrupt ourintvect ( void );

    // patch handling prototypes

        static void pascal FreePatchBlock       ( void far * );
        static void far * pascal GetPatchBlock  ( int );
        static VAIDhdr far *pascal GetPatch     ( VESAHANDLE, int );
        static long pascal GetPatchLength       ( VESAHANDLE, int );
        static void far pascal OurFreeCBCallBack( VESAHANDLE, int, void far *, long );

    // patch handler data structures

        typedef struct {
            int  han;                   // VBE/AI handle
            fpMIDServ ms;               // far pointer to the services structure
            long    poffset;            // offset into the file to VAIP
            FILE   *fp;                 // pointer to the patch file
            VAIIhdr (far *fpVAII)[];    // far pointer to the index portion
            VAIDhdr (far *fpVAID)[];    // far pointer to the data portion, if loaded
        } PatchCTL;

#define MAXLIBS     4
#define VAIIPLEN    (sizeof(VAIIhdr)*256)
        static PatchCTL PatchLibs[MAXLIBS] = // static of libraries can be
            { 0,0,0,0,0,0,
              0,0,0,0,0,0,
              0,0,0,0,0,0,
              0,0,0,0,0,0
            };

    // the table of Patch blocks for use by the MIDI drivers. This is a
    // global pool for all MIDI drivers.

#define MAXFREE     64                      // maximum free blocks we save
        static int       FreeMax = 0;       // last free position
        static void far *PatchPtrs[MAXFREE];
        static long      PatchLength[MAXFREE];
        static int       PatchState[MAXFREE] =  // TRUE=inuse, FALSE=available
            { 0 };

;
;   /*\
;---|*|------------------==============================-------------------
;---|*|------------------====< Start of execution >====-------------------
;---|*|------------------==============================-------------------
;   \*/

;
;   /*\
;---|*|----====< AllocateBuffer >====----
;---|*|
;---|*| Allocate some huge memory from the DOS pool
;---|*|
;   \*/
void far * pascal AllocateBuffer(siz)
    long siz;
{
void far *f;

    // go to DOS to get the memory block size

        _asm {

        // get the # of paragraphs into BX

            mov     bx,word ptr [siz+0] // 0xLLLL
            mov     dx,word ptr [siz+2] // 0x000H
            shr     bx,4            // 0x0LLL
            shl     dx,12           // 0xH000
            add     bx,dx           // 0xHLLL: bx = 16 bits out of 20 bits
            inc     bx              // get one more 16 byte block
            inc     bx              // get one more 16 byte block

        // call DOS for a memory block

            mov ah,48h
            int 21h

        // carry is set if in error.

            cmc                     // set carry if no error
            sbb     bx,bx
            and     ax,bx           // ax = 0 if in error, else a segment
            sub     dx,dx
            xchg    ax,dx           // dx:ax holds the pointer

            mov     word ptr [f+0],ax
            mov     word ptr [f+2],dx
        }

    // return the modified, or empty pointer

        return(f);
}

;
;   /*\
;---|*|----====< FreeBuffer >====----
;---|*|
;---|*| Allocate some huge memory from the DOS pool
;---|*|
;   \*/
void pascal FreeBuffer(void far *p)
{
void far *f;

    // go to DOS to release the memory block size

        _asm {

        // call DOS for a memory block

            mov es,word ptr [p+2]
            mov ah,49h
            int 21h

        }
}

;
;   /*\
;---|*|----====< VESAFindADevice >====----
;---|*|
;---|*| This function returns the next available device handle
;---|*| from the driver list.
;---|*|
;   \*/

VESAHANDLE pascal VESAFindADevice ( class )
    int class;
{
static VESAHANDLE lh = 0;

    // if this is a reset call, do it...

        if (class == -1)
            return(lh = 0); // start over...

    // call the driver directly

        _asm {
            mov     ax,VESAFUNCID
            mov     bx,VESAFUNC1
            mov     cx,[lh]
            mov     dx,[class]
            int     INTHOOK
            mov     [lh],cx
        }

    // return the finding

        return (lh);

}

;
;   /*\
;---|*|----====< VESAQueryDevice >====----
;---|*|
;---|*| Query the driver for general or device specific data.
;---|*|
;   \*/
long pascal VESAQueryDevice ( han, query, ptr )
    VESAHANDLE han;
    int        query;
    void far  *ptr;
{
long cc;
int ticks,n;

    // ask the driver for some specific information

        cc = (long) ptr; // we have to do this due to MS bugs in inline code!

        _asm {
            push    si
            push    di

            mov     ax,VESAFUNCID
            mov     bx,VESAFUNC2
            mov     cx,[han]
            mov     dx,[query]

            mov     si,word ptr [cc+2]
            mov     di,word ptr [cc+0]

            int     INTHOOK

            sub     ax,004Fh        // considered a good return
            neg     ax              // null out SI:DI if AX != 0x004F
            sbb     ax,ax
            not     ax
            and     si,ax
            and     di,ax

            mov     word ptr [cc+2],si
            mov     word ptr [cc+0],di

            pop     di
            pop     si
        }

    // if just a length query, return the value now

        if (query == VESAQUERY1)
            return (cc);
        if (query == VESAQUERY3)
            return (cc);
        if (query == VESAQUERY5)
            return (cc);

    // if this is returning the info block, grab the timer tick count
    // since this is the only time we may see the INFO block.

        if (query == VESAQUERY2) {

            switch (((fpGDC)cc)->gdclassid) {

                case WAVDEVICE:
                    ticks = ((fpGDC)cc)->u.gdwi.witimerticks;
                    break;

                case MIDDEVICE:
                    ticks = ((fpGDC)cc)->u.gdmi.mitimerticks;
                    break;

                default:
                case VOLDEVICE:
                    ticks = 0;  // no timer ticks for these types
                    break;
            }

            // see if the handle is already registered, if so, we're done

            for (n=1;n<CALLBACKS;n++) {
                if (doreg[n][0] == han) {
                    ticks = 0;
                    break;
                }
            }

            // if timer ticks needed, save that fact for a later open

            if (ticks) {
                for (n=1;n<CALLBACKS;n++) {
                    if (!doreg[n][0]) {
                        doreg[n][0] = han;
                        doreg[n][1] = ticks;
                        break;
                    }
                }
            }
        }

    // return the completion code

        return (cc);

}

;
;   /*\
;---|*|----====< VESAOpenADevice >====----
;---|*|
;---|*| This function returns a pointer to the driver
;---|*| services table if the driver opens successfully.
;---|*|
;   \*/
void far *pascal VESAOpenADevice ( han, apiset, memptr )
    VESAHANDLE han;
    int apiset;
    void far *memptr;
{
void far *cc = 0;
int n;

    // attempt to open the device

        _asm {
            cmp     word ptr [memptr+0],0   // offset must be null
            jnz     voad_05

            mov     ax,VESAFUNCID           // open function
            mov     bx,VESAFUNC3
            mov     cx,[han]                // device handle is required
            mov     dx,[apiset]             // select 16/32 bit interface
            mov     si,word ptr [memptr+2]  // pass in the segment
            int     INTHOOK                 // do the interrupt

            mov     word ptr [cc+2],si
            mov     word ptr [cc+0],cx
        }
        voad_05:;

    // if successful, check to see if the device needs timer ticks

        if (cc) {

            for (n=1;n<CALLBACKS;n++) {

                // if this is it, setup for timer ticks

                if (doreg[n][0] == han) {

                    // register wave audio

                    if ( _fstrncmp(((fpWAVServ)cc)->wsname,"WAVS",4) == 0 )

                        VESARegisterTimer
                          (
                            han,                            // handle
                            ((fpWAVServ)cc)->wsTimerTick,   // address
                            VESARateToDivisor(doreg[n][1])  // tick count
                          );

                    // register midi audio

                    if ( _fstrncmp(((fpMIDServ)cc)->msname,"MIDS",4) == 0 )

                        VESARegisterTimer
                          (
                            han,                            // handle
                            ((fpMIDServ)cc)->msTimerTick,   // address
                            VESARateToDivisor(doreg[n][1])  // tick count
                          );

                    break;
                }
            }
        }

    // return the pointer to the services table

        return(cc);

}

;
;   /*\
;---|*|----====< VESACloseDevice >====----
;---|*|
;---|*| This function closes an opened device.
;---|*|
;   \*/
void  pascal VESACloseDevice ( han )
    VESAHANDLE han;
{
int n,gd;

    // remove the timer callbacks for this handle

        VESARegisterTimer( han, 0, 0 );

    // make sure MIDI devices free up bank memory

        VESAFreePatchBank ( han );

    // go close the device.

        _asm {
            mov     ax,VESAFUNCID
            mov     bx,VESAFUNC4
            mov     cx,[han]            // if the handle exists, the
            int     INTHOOK             // device will close
        }
}

;
;   /*\
;---|*|----====< VESAFreePatchBank >====----
;---|*|
;---|*| This function will release the patch bank data memory and
;---|*| remove the VBE device from the internal tables.
;---|*|
;   \*/
int  pascal VESAFreePatchBank ( han )
    VESAHANDLE han; // device handle
{
int n;
int inuse = 0;

    for (n=0;n<MAXLIBS;n++) {

        // only one will match. Free up the memory, then flush the entry

        if (han == PatchLibs[n].han) {

            // free the patch libs memory

            if (PatchLibs[n].fpVAII)
                FreeBuffer((void far *)PatchLibs[n].fpVAII);

            // free the patch data memory

            if (PatchLibs[n].fpVAID);
                FreeBuffer((void far *)PatchLibs[n].fpVAID);

            // close the file

            if (PatchLibs[n].fp)
                fclose(PatchLibs[n].fp);

            PatchLibs[n].han    = 0;        // clear out this entry
            PatchLibs[n].ms     = 0;
            PatchLibs[n].fp     = 0;
            PatchLibs[n].fpVAII = 0;
            PatchLibs[n].fpVAID = 0;
            PatchLibs[n].poffset= 0;

            break;                          // leave the loop
        }
    }

    // see if all banks are unloaded. "inuse" == 0 if no more banks loaded

        for (n=0;n<MAXLIBS;n++)
            if (PatchLibs[n].han)
                inuse++;

    // if all patch banks are freed, then flush the MIDI patch blocks

        if ((!inuse) && (FreeMax)) {

            for (n=0;n<FreeMax;n++) {

                FreeBuffer(PatchPtrs[n]);

                PatchPtrs[n]   = 0;
                PatchLength[n] = 0;
                PatchState[n]  = FALSE;

            }
        }
}

;
;   /*\
;---|*|----====< VESALoadPatchBank >====----
;---|*|
;---|*| This function will load the patch bank index structure into memory.
;---|*| If the data portion is small, then it too will be loaded.
;---|*|
;   \*/
int  pascal VESALoadPatchBank ( han, ms, bnk )
    VESAHANDLE han; // device handle
    fpMIDServ ms;   // fp to services structure
    char far *bnk;
{
FILE *fp;
long len,l;
int  n,idx=-1;
char far *pt,far *opt, far *patchbank;
char fname[100],*s,*b;
char *env,c;

    // make sure we don't already have this device's bank or are already
    // filled with patch banks

        for (c=n=0;n<MAXLIBS;n++) {

            if (PatchLibs[n].han == han)    // bail if already here
                return(FALSE);

            if (PatchLibs[n].han != 0)      // count the used slots
                c++;

            if (PatchLibs[n].han == 0)      // record the empty slots
                if (idx == -1)
                    idx = n;
        }

        if (c == MAXLIBS)                   // if all slots full, bail
            return(FALSE);

    // open the patch bank in the current directory, PATH or root

        _fstrcpy (fname,bnk);

        if ((fp = fopen (fname,"rb")) == 0) {

            // if there is a path statement, process it...

            if ((env = getenv ("PATH")) != 0) {

                s = fname;      // setup to process each string

                while (s) {

                    // pull the next string from the path

                        c = 0;
                        while (*env)
                            if ((*s++ = c = *env++) == ';')
                                break;

                    // if a semicolon stopped us, point to it...

                        if (c == ';') s--;

                        *s = 0;             // set a terminator

                    // if there is no other string, break out now...

                        if (s == fname) {
                            s = 0;
                            continue;
                        }

                    // if the prior character is not a backslash, append one

                        if (*(s-1) != '\\') {
                            *s++ = '\\';
                            *s = 0;
                        }

                    // append the library name now

                        b = bnk;
                        while (*s++ = *b++) ;

                    // attempt to open the bank now

                        if ((fp = fopen (fname,"rb")))
                            break;

                        s = fname;      // reset for the next iteration

                }

            }

            // if no library after the path search, try the root directory

            if (!fp) {

                // try the root directory of the current drive

                fname[0] = '@' + _getdrive();   // get the current drive
                fname[1] = ':';
                fname[2] = '\\';
                s = &fname[3];

                b = bnk;
                while (*s++ = *b++) ;

                fp = fopen (fname,"rb");

            }

            // if after all that, if not opened, it's not there!

            if (!fp)
                return(FALSE);

        }

    // allocate memory for the index block

        if ((patchbank = AllocateBuffer(VAIIPLEN)) == 0) {
            fclose (fp);
            return(FALSE);
        }

    // load in the first chunk. The first "RIFF" block must be here!

        len = readblock (fileno(fp),(pt=patchbank),VAIIPLEN) & 0xffff;

        if (cmpstr(&((VAILhdr far *)pt)->type[0], "RIFF",4) != 0) {
            fclose (fp);
            return(FALSE);
        }
        pt += sizeof(RIFFhdr);          // skip the header

    // load in the first chunk. The first "vail" block must be here!

        if (cmpstr(&((VAILhdr far *)pt)->type[0], "vail",4) != 0) {
            fclose (fp);
            return(FALSE);
        }
        pt += sizeof(VAILhdr);          // skip the header

    // loop through till the vaii header is located

        while (1) {

            opt = pt;

            if (cmpstr(&((ZSTRhdr far *)pt)->type[0], "ZSTR",4) == 0) {
                l = ((ZSTRhdr far *)pt)->tlen;
                pt += sizeof(ZSTRhdr)+l; // skip the ASCIIZ block
            }

            if (cmpstr(&((VAIPhdr far *)pt)->type[0], "vaip",4) == 0) {
                pt += sizeof (VAIPhdr); // move past this block
                break;                  // we're done, save this pointer!
            }

            if (opt == pt) {
                fclose (fp);
                return(FALSE);          // invalid file format
            }
        }

    // we have the start of the VAII record. reposition the file pointer,
    // then pull in the entire index.

        len = (int)pt - (int)patchbank;     // get the starting offset
        fseek (fp,len,SEEK_SET);

    // save the starting position of the VAIP record

        PatchLibs[idx].poffset= ftell(fp) - sizeof (VAIPhdr);

        if (readblock (fileno(fp),patchbank,VAIIPLEN & 0xffff) != VAIIPLEN) {
            fclose (fp);
            return(FALSE);
        }

    // register this VESA device in our system

        PatchLibs[idx].han    = han;    // record this new entry
        PatchLibs[idx].ms     = ms;
        (void far *)PatchLibs[idx].fpVAII = (void far *)patchbank;

    // grab the callback since we're doing the patch handling

        ms->msApplFreeCB = &OurFreeCBCallBack;

    // get the unused file length to determine if we can preload the patch data

        len  = filelength (fileno(fp)); // patch size = file len. - index
        len -= ftell(fp);

    // just load banks less than 64K

        if (len < 65535) {

            // if not enough memory, skip it...

                if ((patchbank = AllocateBuffer(len)) == 0)
                    return(TRUE);

            // read in the patch data now. If not, we'll do the slow method

                if (((long)readblock(fileno(fp),patchbank,(int)len) & 0xffff) != len)
                    return(TRUE);

            // record the patch data buffer now

                (void far *)PatchLibs[idx].fpVAID = (void far *)patchbank;
                fclose (fp);
        }

    // if >64K, then we need to go to disk for each patch.

        else {

            // save the file pointer for later access when we need the data

            PatchLibs[n].fp = fp;
        }

    // return the length of the structure

        return(TRUE);

}

;
;   /*\
;---|*|----====< VESAPreloadPatch >====----
;---|*|
;---|*| Download the VESA patch to the driver.
;---|*|
;---|*| NOTE: This routine may call DOS, so use it only in the foreground!
;---|*|
;   \*/
int  pascal VESAPreLoadPatch ( han, patch, ch )
    VESAHANDLE han; // device handle
    int patch;      // patch # to be downloaded
    int ch;         // channel # for download
{
long isdata;
int n,trim;
char far *pptr;

    // percussive patches reside in the second half of the table

        if (ch == 9)
            patch |= 0x80;

    // get the patch and size of patch

        for (n=0;n<MAXLIBS;n++) {

            // if a device is found, try to load it's patch

            if (PatchLibs[n].han == han) {

                // if the patch is already loaded, we can just do it...

                if (PatchLibs[n].fpVAID) {

                    // send off the patch - just the data portion, no VAIDhdr

                       (*PatchLibs[n].ms->msPreLoadPatch)
                          (
                            ch,
                            patch & 0x7F,
                            GetPatch       ( han, patch ), // sizeof(VAIDhdr),
                            GetPatchLength ( han, patch )
                          );

                        break;  // exit the for loop
                }

                else {

                    // get the length needed

                        isdata = GetPatchLength ( han, patch );
                        trim   = sizeof (VAIDhdr);

                        while (isdata) {

                            // find the size needed

                            n = (*PatchLibs[n].ms->msPreLoadPatch)(ch,patch&0x7F,0,0);

                            // if we get a block, process the data

                            if (pptr = GetPatchBlock(n)) {

                                // read it from disk

                                fseek
                                  (
                                    PatchLibs[n].fp,
                                    PatchLibs[n].poffset+
                                        (*PatchLibs[n].fpVAII)[patch].poffset,
                                    SEEK_SET
                                  );

                                n = readblock (fileno(PatchLibs[n].fp),pptr,n);

                                // send it to the driver, and maybe free it up

                                if (!(*PatchLibs[n].ms->msPreLoadPatch)
                                  (
                                    ch,
                                    patch&0x7F,
                                    pptr+trim,
                                    n
                                  ))
                                    FreePatchBlock(pptr);

                                // remove our portion

                                trim    = 0;
                                isdata -= n;
                            }

                            // oops, no data, let's pull back...

                            else {
                                (*PatchLibs[n].ms->msUnloadPatch) (ch,patch&0x7F);
                                break;
                            }

                        }

                    break;  // exit the for loop
                }
            }
        }
}

;
;   /*\
;---|*|----====< VESARateToDivisor >====----
;---|*|
;---|*| Converts an integral rate (in Hz) to a timer divisor value, which
;---|*| can then be passed to VESARegisterTimer().
;---|*|
;---|*| Copyright (c) 1993 Jason Blochowiak, Argo Games. Used with permission.
;---|*|
;   \*/
long pascal VESARateToDivisor(rate)
    long rate;
{
long result;

    if (rate)
        result = 1193180L / rate;
    else
        result = 0x10000L;

    return(result);
}

;
;   /*\
;---|*|----====< VESARegisterTimer >====----
;---|*|
;---|*| Add/Remove a far call from the timer registration.
;---|*|
;---|*| Copyright (c) 1993 Jason Blochowiak, Argo Games. Used with permission.
;---|*|
;   \*/
int   pascal VESARegisterTimer( han, addr, divisor )
    VESAHANDLE han;
    void (far pascal *addr)();
    long divisor;
{
int n,i,max;
int retval = 0;
int needsProgram = FALSE;
int index;
Timer_t *t;

    // Puke if we have a zero handle

        if (!han)
            return(retval);

    // Make sure no interrupts come in while we're messing with the table

        _asm cli ;

    // Make sure that the special entry doesn't get used by something else

        timers[0].handle = -1;

    // If address in non-0, register it

        if (addr) {

        // See if an entry with this handle already exists

            index = findhandleindex(han);

        // If it returned -1, no entry with this handle exists, so find
        // an entry with no handle

            if (index == -1)
                index = findhandleindex(0);

        // If we found an entry with a matching handle, or an empty entry,
        // fill it in.

            if (index != -1) {

            // Get pointer to timer table, and cast to eliminate the volatile
            // modifier (interrupts are disabled, so there's no chance of the
            // structure being modified from an interrupt)

                t = (Timer_t *)(&timers[index]);

                t->handle = han;        // Store the handle
                t->divisor = divisor;   // Store our divisor
                t->divCount = 0;        // Clear the overflow counter
                t->callBack = addr;     // Store the callback address
                t->semaphore = 0;       // Initialize the semaphore

                retval = -1;            // Mark as successful
                needsProgram = TRUE;    // We'll need to mess with the hardware timer
            }
        }
        else {

        // We've got a 0 address - remove the timer entry

        // Find the entry with a matching handle

            index = findhandleindex(han);
            if (index != -1) {

            // We found the entry - kill it

                t = (Timer_t *)(&timers[index]);

                t->handle = 0;
                t->divisor = 0;
                t->divCount = 0;
                t->callBack = 0;

                retval = -1;            // Mark as successful
                needsProgram = TRUE;    // We'll need to mess with the hardware timer
            }
        }

    // If we need to, go mess with the hardware timer

        if (needsProgram)
            setuptimer();

        _asm sti ;

    // Return success or failure

        return(retval);
}

;
;   /*\
;---|*|------------------=============================-------------------
;---|*|------------------====< internal routines >====-------------------
;---|*|------------------=============================-------------------
;   \*/

;
;   /*\
;---|*|----====< cmpstr >====----
;---|*|
;---|*| compare strings at the end of far pointers
;---|*|
;   \*/

static int cmpstr(t,s,l)
    char far *t;
    char far *s;
    int l;
{
char c;

    while (l--)
        if ((c = *t++ - *s++) != 0)
            return(c);

    return(0);
}

;
;   /*\
;---|*|----====< FreePatchBlock >====----
;---|*|
;---|*| mark the block as free
;---|*|
;   \*/
static void pascal FreePatchBlock(void far *pptr)
{
int n;

    // if already full, send it back to DOS

        for (n=0;n<MAXFREE;n++) {

            // find it, and mark it free

            if (pptr == PatchPtrs[n]) {
                PatchState[n] = FALSE;
                break;

            }
        }
}

;
;   /*\
;---|*|----====< GetPatchBlock >====----
;---|*|
;---|*| Get a block from the pool, or from DOS
;---|*|
;   \*/
static void far * pascal GetPatchBlock(len)
    int  len;
{
int n;
void far *vp;

    // scan the global pool before allocating from DOS

        for (n=0;n<FreeMax;n++) {

            if (len <= PatchLength[n])

                return(PatchPtrs[n]);

        }

    // we didn't find anything, so allocate it from DOS

        if (FreeMax < MAXFREE) {

            if ((PatchPtrs[FreeMax] = vp = AllocateBuffer(len)) != 0) {

                PatchLength[FreeMax] = len;
                PatchState[FreeMax++]= TRUE;

            }
            return(vp);     // returns the valid pointer
        }

    // nothing

        return(FALSE);
}

;
;   /*\
;---|*|----====< GetPatch >====----
;---|*|
;---|*| This function returns a far pointer to the patch block
;---|*|
;   \*/

static VAIDhdr far *pascal GetPatch ( han, patch )
    VESAHANDLE han;
    int  patch;
{
long off=-1;
int n;
VAIIhdr (far *p)[];

    // if an invalid patch, bomb out

        if ((patch > 256) || (patch < 0))
            return(FALSE);

        for (n=0;n<MAXLIBS;n++) {

            if (PatchLibs[n].han == han) {
                off = n;
                break;
            }
        }

        if (off == -1)
            return(FALSE);

    // if the patch is memory, return a pointer to it

        if (PatchLibs[n].fpVAID) {

            p   =  PatchLibs[n].fpVAII;
            off =  (*p)[patch].poffset - VAIIPLEN;

            return((VAIDhdr far *)((char far *)PatchLibs[n].fpVAID + off));

        }

    // we have to read it from disk!

        return(FALSE);

}

;
;   /*\
;---|*|----====< GetPatchLength >====----
;---|*|
;---|*| Returns the length of the patches data portion (include VAIDhdr)
;---|*|
;   \*/
static long pascal GetPatchLength ( han, patch )
    VESAHANDLE han;
    int  patch;
{
VAIIhdr (far *p)[];
long off;
int n;

    // if an invalid patch, bomb out

        if ((patch > 256) || (patch < 0))
            return(0L);

    // find the patch table, then return the length of the patch data

        for (n=0;n<MAXLIBS;n++) {

            if (PatchLibs[n].han == han) {
                p = PatchLibs[n].fpVAII;
                return ((*p)[patch].vaidln);
            }
        }

    // this handle was'nt registered

        return(FALSE);
}

;   /*\
;---|*|----====< ourintvect >====----
;---|*|
;---|*| Process timer interrupts.
;---|*|
;---|*| Copyright (c) 1993 Jason Blochowiak, Argo Games. Used with permission.
;---|*|
;   \*/

#if __BORLANDC__
static interrupt void ourintvect(void)
#else
static void interrupt ourintvect(void)
#endif

{
int     i;
int     didEOI = FALSE;
long    timercount;
Timer_t *t;

    // We want to process any previously unaccounted for timer divisor
    // counts, plus the number of counts that the hardware did before
    // triggering this interrupt

        curtimerslop += curtimerdiv;

    // If we don't have too many nested timer interrupts, process the
    // individual callbacks.

        if (++curnestedtimer < MAXNESTEDCBS) {

        // Clear any accumulated slop, and use for count to advance the
        // individual callbacks' overflow values

            timercount = curtimerslop;
            curtimerslop = 0;

            t = (Timer_t *)timers;
            for (i = 0;i < CALLBACKS;i++,t++) {

            // If this isn't a valid entry, skip it

                if (!(t->handle && t->callBack))
                    continue;

            // It's valid - advance its overflow value

                t->divCount += timercount;

            // If we're past the first (special) entry, and the we didn't
            // call the old timer ISR, do the EOI now.

                if (i && !didEOI) {
                    outp(0x20,0x20);
                    didEOI = TRUE;
                }

            // If we're not already in a call to this one, and its overflow
            // value equals or exceeds its timer count value, set its
            // semaphore and call it.

                if ((t->semaphore == 0) && (t->divCount >= t->divisor)) {

                    t->divCount -= t->divisor;
                    t->semaphore++;

                // If it's the special entry, push the flags for the old
                // timer ISR's IRET, and flag the EOI as having happened,
                // as the old ISR will have issued it. If it's not the
                // special entry, enable interrupts before the call.

                if (i) {
                    _asm { sti };
                }
                else {
                    didEOI = TRUE;
                    _asm { pushf };
                }

                t->callBack();

                // Re-disable interrupts, and flag entry as not currently
                // being processed.

                    _asm { cli };
                    t->semaphore--;

            }
        }
    }

    // Decrement nested interrupt call count

        curnestedtimer--;

    // If we didn't get to the EOI before, do it now

        if (!didEOI)
            outp(0x20,0x20);
}

;
;   /*\
;---|*|----====< OurFreeCBCallBack >====----
;---|*|
;---|*| The patch block is now free. NOTE: No assumptions can be made about
;---|*| the segment registers! (DS,ES,GS,FS)
;---|*|
;   \*/

static void far pascal OurFreeCBCallBack( han, patch, fptr, filler )
    VESAHANDLE han; // the caller's handle
    int  patch;     // the actual patch #
    void far *fptr; // buffer that just played
    long filler;    // reserved
{

    // point to our data segment

        _asm {
            pusha
            push    ds
            push    es
            mov     ax,seg FreeMax
            mov     ds,ax

        }

    // free up the block for other patchs to use

        FreePatchBlock(fptr);

    // restore the segment & return home

        _asm {
            pop     es
            pop     ds
            popa
        }
}


;
;   /*\
;---|*|----====< programtimer >====----
;---|*|
;---|*| Program the timer chip for the appropriate interrupt rate. The
;---|*| parameter is the actual timer value, so it's 1193180/rate
;---|*|
;---|*| Copyright (c) 1993 Jason Blochowiak, Argo Games. Used with permission.
;---|*|
;   \*/

void programtimer (max)
    unsigned max;
{
        _asm    pushf
        _asm    cli

        _asm    mov al,0x36
        _asm    out 0x43,al
        _asm    jmp delay1

    delay1:;

        _asm    jmp delay2

    delay2:;

        _asm    mov ax,[max]
        _asm    out 0x40,al
        _asm    jmp delay3

    delay3:;

        _asm    jmp delay4

    delay4:;

        _asm    xchg ah,al
        _asm    out 0x40,al
        _asm    popf
}

;
;   /*\
;---|*|----====< readblock >====----
;---|*|
;---|*| read a chunk of the PCM file into the huge buffer
;---|*|
;   \*/
static int readblock (han,tptr,len)
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

        } rdbl05: _asm {

            mov     ah,03fh
            mov     bx,[han]
            mov     cx,[len]

            jcxz    rdbldone

            lds     dx,[tptr]
            int     21h

            add     [siz],ax            // we moved this much

        } rdbldone: _asm {

            pop     ds

        }

    // return the amount read

        return(siz);
}


;   /*\
;---|*|----====< setuptimer >====----
;---|*|
;---|*| Something has changed the timer structures, so we need to figure
;---|*| out what to do with the hardware timer and the hardware timer ISR
;---|*|
;---|*| This is only called by VESARegisterTimer(), so we assume that
;---|*| interrupts are disabled.
;---|*|
;---|*| Copyright (c) 1993 Jason Blochowiak, Argo Games. Used with permission.
;---|*|
;   \*/
void
setuptimer(void)
{
int     i,j;
int     otherCount;
long    minDiv,minDivDelta,timerVal;
Timer_t *t,*t2;

    // If we haven't filled out the special slot 0 entry, do so now

        if (!timers[0].callBack) {
            timers[0].callBack = (void(far pascal *)(void))_dos_getvect(8);
            timers[0].divisor  = 0x10000L;
            timers[0].divCount = 0;
        }

    // Count the number of non-special entries, and find 1) The entry
    // with the smallest timer divisor, and 2) The smallest difference
    // between divisors.

        minDiv = 0x10000L;
        minDivDelta = 0x10000L;
        otherCount = 0;
        t = (Timer_t *)(&timers[1]);
        for (i = 1;i < CALLBACKS;i++,t++) {

        // Only pay attention to valid entries

            if (t->handle) {

            // Count this non-special entry

                otherCount++;

            // If this entry's divisor is smaller than our previous minimum,
            // use it for our new minimum.

                if (t->divisor && (t->divisor < minDiv))
                    minDiv = t->divisor;

            // Go through the table again, finding the smallest non-zero
            // difference between two divisors

                t2 = (Timer_t *)(&timers[0]);
                for (j = 0;j < CALLBACKS;j++,t2++) {

                    if ((j != i) && (t2->handle)) {

                        long    delta;

                    // Get absolute value of difference between the two
                    // entries' divisors

                        delta = t2->divisor - t->divisor;

                        if (delta < 0)
                            delta = -delta;

                    // If non-zero, and smaller than previous smallest
                    // difference, hold on to it

                        if (delta && (delta < minDivDelta))
                            minDivDelta = delta;
                    }
                }
            }
        }

    // If we no longer need to own the vector, and our ISR is installed,
    // remove it

        if ((otherCount == 0) && timerhooked) {

#if __BORLANDC__ //DSC
        _dos_setvect(8,(void interrupt (*)())(timers[0].callBack));
#else
        _dos_setvect(8,(void (interrupt far *)())(timers[0].callBack));
#endif
        timerhooked = FALSE;

        }

    // If we need to own the vector, and our ISR isn't installed,
    // install it

        if (otherCount && !timerhooked) {
            _dos_setvect(8,ourintvect);
            timerhooked = TRUE;
        }

    // Set timer divisor to minimum divisor

        timerVal = minDiv;

    //  This next bit is here to help out in situations where there are a
    // couple of fairly slow callbacks set up, and no fast callbacks. If the
    // only callbacks set up are at 40Hz and 50Hz, and this code weren't
    // here, the hardware would be set up to generate 50Hz interrupts.
    // Starting at a time of 0.0, the interrupts would be called at:
    // Time     40Hz    50Hz
    // ----     ----    ----
    // 0.02             x
    // 0.04     x       x
    // 0.06     x       x
    // 0.08     x       x
    // 0.10     x       x
    // 0.12             x
    // 0.14     x       x
    //
    // etc. In other words, four out of every five of the 40Hz callbacks
    // would be performed with 0.02 seconds in between, and then there
    // would be a 0.04 second delay before the next call to the 40Hz
    // callback. Although the 40Hz callback will get called 40 times a
    // second, the time between calls won't be regular.
    //  Although this isn't a problem under some circumstances, it can be a
    // problem under other circumstances, and a nuisance under still other
    // circumstances.
    //  This code can increase the timer resolution to about 145.6Hz,
    // which translates to about 6.87ms (0.00687 seconds). This means that
    // a particular callback will occur, at most, roughly 3.435ms before
    // or after it "should".
    //  Note that by changing the value 0x2000, you can alter the adjustment
    // frequency that will be used.
    //

        if (timerVal > minDivDelta) {

            if (minDivDelta >= 0x2000)
                timerVal = minDivDelta;
            else if (timerVal > 0x2000)
                timerVal = 0x2000;
        }

    // Set the hardware timer value. It's possible that timerVal will be
    // 0x10000L, more than will fit into a 16-bit word. This is ok, as
    // programming the timer for 0x0000 is treated by the hardware as
    // programming the timer for 0x10000.

        programtimer((unsigned)timerVal);
        curtimerdiv = timerVal;
}

;
;   /*\
;---|*|----====< findhandleindex >====----
;---|*|
;---|*| This finds an entry in the timers[] array with a handle value
;---|*| that matches the parameter. Returns -1 if a matching entry
;---|*| isn't found.
;---|*|
;---|*| Copyright (c) 1993 Jason Blochowiak, Argo Games. Used with permission.
;---|*|
;   \*/
static int findhandleindex(VESAHANDLE han)
{
    int i;

    for (i = 0;i < CALLBACKS;i++)
        if (timers[i].handle == han)
            return(i);
    return(-1);
}

;   /*\
;---|*| end of VESA.C
;   \*/




