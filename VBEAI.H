
;   /*\
;---|*|--------------------====< VESA Sound Include >====--------------------
;---|*|
;---|*| Copyright (c) 1993,1994  V.E.S.A, Inc. All Rights Reserved.
;---|*|
;---|*| VBE/AI 1.0 Specification
;---|*|    February 2, 1994. 1.00 release
;---|*|
;---|*| Additional Changes:
;---|*|   03/03 - Added the full list of Volume feature bit as #defines
;---|*|
;   \*/

#define VBEAI_VERSION   0x10h     // first VBE/AI version

;   /*\
;---|*|----====< VESA Services Definitions >====----
;   \*/

#define INTHOOK     0x0010

;   // VAI query interface functions

#define VESAFUNCID  0x4f13    // function ID

#define VESAFUNC0   0x0000    // VESA Function #0 -- reserved
#define VESAFUNC1   0x0001    // VESA Function #1 -- Locate the device
#define VESAFUNC2   0x0002    // VESA Function #2 -- Query the device
#define VESAFUNC3   0x0003    // VESA Function #3 -- Open the device
#define VESAFUNC4   0x0004    // VESA Function #4 -- Close the device
#define VESAFUNC5   0x0005    // VESA Function #5 -- Driver Unload
#define VESAFUNC6   0x0006    // VESA Function #6 -- Driver Chain/Unchain

;   // VAI function 2 queries

#define VESAQUERY1  0x0001    // get length of GeneralDeviceClass
#define VESAQUERY2  0x0002    // get copy   of GeneralDeviceClass
#define VESAQUERY3  0x0003    // get length of Volume Info Structure
#define VESAQUERY4  0x0004    // get copy   of Volume Info Structure
#define VESAQUERY5  0x0005    // get length of Volume Services Structure
#define VESAQUERY6  0x0006    // get copy   of Volume Services Structure
#define VESAENDQUERY    0x000f    // subfunctions 6 - 15 are reserved
                  // subfunctions 16+ are used by the devices
;   // VAI Device types

#define WAVDEVICE   0x0001
#define MIDDEVICE   0x0002
#define VOLDEVICE   0x0003

#define WAVESERVICES    0x57415653 // ascii "WAVS"
#define MIDISERVICES    0x4D494453 // ascii "MIDS"


;   /*\
;---|*|----====< Volume Services >====----
;   \*/

    typedef struct {

        // housekeeping

        char viname[4];     // name of the structure
        long vilength;      // structure length

        // hardware vendor name

        long viversion;     // driver software version
        char vivname[32];       // vendor name, etc.
        char viprod[32];        // vendor product name
        char vichip[32];        // vendor chip/hardware description
        char viboardid;     // installed board #
        char viunused[3];       // unused data

        char vicname[24];       // mixer channel name

        long vifeatures;

        int  vimin;         // minimum volume setting
        int  vimax;         // maximum volume setting
        int  vicross;       // attenuation/gain crossover

    } VolumeInfo, far *fpVolInfo;

;  // Volume feature bits

#define VOLSTEREO   0x0001      // Stereo Volume control available
#define VOLLOPASS   0x0004      // Low Pass Filter is available
#define VOLHIPASS   0x0008      // High Pass Filter is available
#define VOLPARATC   0x0010      // Parametric Tone Control is available
#define VOLSELOUT   0x0020      // selectable output paths
#define VOLAZIMUTH  0x0100      // Azimuth Field positioning supported
#define VOLPHIMUTH  0x0200      // Phi Field positioning supported
#define VOLMASTER   0x80000000  // Master Volume device

;  // VOL device check messages

#define VOLFILTERRANGE        0x0011
#define VOLFILTERSETTING      0x0012
#define VOLFILTERCURRENT      0x0013
#define VOLTONERANGE          0x0014
#define VOLTONESETTING        0x0015
#define VOLTONECURRENT        0x0016
#define VOLPATH           0x0017
#define VOLGETIOADDRESS       0x0018
#define VOLOEMSTARTINGDC      0x0080    // vendors can add DevChks above 0x80


;   /*\
;---|*|----====< Volume services Structure >====----
;   \*/

    typedef struct {

        // housekeeping

        char vsname[4];     // name of the structure
        long vslength;      // structure length

        char vsfuture[16];      // 16 bytes for future expansion

        long (pascal far *vsDeviceCheck  ) ( int, long );
        long (pascal far *vsSetVolume    ) ( int, int, int );
        long (pascal far *vsSetFieldVol  ) ( int, int, int );
        int  (pascal far *vsToneControl  ) ( int, int, int, int );
        long (pascal far *vsFilterControl) ( int, int, int );
        void (pascal far *vsOutputPath   ) ( int );
        void (pascal far *vsResetChannel ) ( void );
        int  (pascal far *vsGetLastError ) ( void );

    } VolumeService, far *fpVolServ;

;   // VOL SetVolume message

#define VOL_USERSETTING 0x01        // User master volume setting
#define VOL_APPSETTING  0x02        // application master volume setting

;   // VOL error message

#define VOL_NOSUPPORT   0x01        // unsupported feature/function
#define VOL_BADVALUE    0x02        // out of range parameter value
#define VOL_HWFAILURE   0x80        // vendors specific errors


;   /*\
;---|*|----====< WAVE Info Structure >====----
;   \*/

        typedef struct {

            // housekeeping

        char winame[4];     // name of the structure
        long wilength;      // structure length

        // hardware vendor name

        long wiversion;     // driver software version
        char wivname[32];       // vendor name, etc. (ASCIIZ string)
        char wiprod[32];        // vendor product name
        char wichip[32];        // vendor chip/hardware description
        char wiboardid;     // installed board #
        char wiunused[3];       // unused data

            // device specific information

        long wifeatures;        // feature bits
        int  widevpref;     // user determined preference field
            int  wimemreq;              // memory required for driver use.
        int  witimerticks;      // # of timer tick callbacks per second

        int  wiChannels;        // 1 = mono, 2 = stereo. Stereo is
                    // assumed to be interleaved data.
        int  wiSampleSize;      // Bit field of max sample sizes

    } WAVEInfo, far *fpWAVInfo;

;   // WAVE feature bit definitions

#define WAVEMP8K    0x00000001  // 8000hz Mono Playback.
#define WAVEMR8K    0x00000002  // 8000hz Mono Record.
#define WAVESR8K    0x00000004  // 8000hz Stereo Record.
#define WAVESP8K    0x00000008  // 8000hz Stereo Playback.
#define WAVEFD8K    0x00000010  // 8000hz Full Duplex Play/Record.

#define WAVEMP11K   0x00000020  // 11025hz Mono Playback.
#define WAVEMR11K   0x00000040  // 11025hz Mono Record.
#define WAVESR11K   0x00000080  // 11025hz Stereo Record.
#define WAVESP11K   0x00000100  // 11025hz Stereo Playback.
#define WAVEFD11K   0x00000200  // 11025hz Full Duplex Play/Record.

#define WAVEMP22K   0x00000400  // 22050hz Mono Playback.
#define WAVEMR22K   0x00000800  // 22050hz Mono Record.
#define WAVESR22K   0x00001000  // 22050hz Stereo Record.
#define WAVESP22K   0x00002000  // 22050hz Stereo Playback.
#define WAVEFD22K   0x00004000  // 22050hz Full Duplex Play/Record.

#define WAVEMP44K   0x00008000  // 44100hz Mono Playback.
#define WAVEMR44K   0x00010000  // 44100hz Mono Record.
#define WAVESR44K   0x00020000  // 44100hz Stereo Record.
#define WAVESP44K   0x00040000  // 44100hz Stereo Playback.
#define WAVEFD44K   0x00080000  // 44100hz Full Duplex Play/Record.

#define WAVEPREPARE 0x08000000  // driver must pre handle the data
#define WAVEVARIPMONO   0x10000000  // Variable Sample mono   playback
#define WAVEVARIPSTER   0x20000000  // Variable Sample stereo playback
#define WAVEVARIRMONO   0x40000000  // Variable Sample mono   record
#define WAVEVARIRSTER   0x80000000  // Variable Sample stereo record

;   // WAVE device check messages

#define WAVECOMPRESSION       0x0011
#define WAVEDRIVERSTATE       0x0012
#define WAVEGETCURRENTPOS     0x0013
#define WAVESAMPLERATE        0x0014
#define WAVESETPREFERENCE     0x0015
#define WAVEGETDMAIRQ         0x0016
#define WAVEGETIOADDRESS      0x0017
#define WAVEGETMEMADDRESS     0x0018
#define WAVEGETMEMFREE        0x0019
#define WAVEFULLDUPLEX        0x001A
#define WAVEGETBLOCKSIZE      0x001B
#define WAVEGETPCMFORMAT      0x001C
#define WAVEENAPCMFORMAT      0x001D
#define WAVEOEMSTARTINGDC     0x0080    // vendors can add DevChks above 0x80

;   // types of compression

#define WAVE_IMA_PLAY         0x0001
#define WAVE_ALAW_PLAY        0x0002
#define WAVE_ULAW_PLAY        0x0003

#define WAVE_IMA_RECORD       0x0011
#define WAVE_ALAW_RECORD      0x0012
#define WAVE_ULAW_RECORD      0x0013

;   // sample data sizes

#define WAVE08BITPLAY         0x0001
#define WAVE16BITPLAY         0x0002
#define WAVE08BITREC          0x0010
#define WAVE16BITREC          0x0020


;   /*\
;---|*|----====< WAVE Audio Services >====----
;   \*/

;   //  The following is a description of the contents of the WAVE Audio
;   //  Services. For the most part, this structure is read-only to
;   //  the application. The only two fields the application can write
;   //  are the callback fields.

    typedef struct {

        // housekeeping

        char wsname[4];     // name of the structure
        long wslength;      // structure length

        char wsfuture[16];      // 16 bytes for future expansion

        // device driver functions

        long (pascal far *wsDeviceCheck   ) ( int, long );
        long (pascal far *wsPCMInfo       ) ( int, long, int, int, int );
        int  (pascal far *wsPlayBlock     ) ( int, long );
        int  (pascal far *wsPlayCont      ) ( void far *, long, long );

        int  (pascal far *wsRecordBlock   ) ( int, long );
        int  (pascal far *wsRecordCont    ) ( void far *, long, long );
        int  (pascal far *wsPauseIO       ) ( int );
        int  (pascal far *wsResumeIO      ) ( int );

        int  (pascal far *wsStopIO        ) ( int );
        int  (pascal far *wsWavePrepare   ) ( int,int,int,void far *,long );
        int  (pascal far *wsWaveRegister  ) ( void huge *,long );

        int  (pascal far *wsGetLastError  ) ( void );
        void (pascal far *wsTimerTick     ) ( void );

        // callback filled in by the application

        void (pascal far *wsApplPSyncCB ) ( int, void far *, long, long ); //play
        void (pascal far *wsApplRSyncCB ) ( int, void far *, long, long ); //rec

    } WAVEService, far *fpWAVServ;

;   // error messages

#define WAV_NOSUPPORT       1   // unsupported feature/function
#define WAV_BADSAMPLERATE   2
#define WAV_BADBLOCKLENGTH  3
#define WAV_BADBLOCKADDR    4
#define WAV_BADLOSTIRQ      5   // app. missed an IRQ
#define WAV_BADPCMDATA      6   // don't understand the PCM size/format
#define WAV_HWFAILURE          80h  // vendors specific errors


;   /*\
;---|*|----====< MIDI Info Structure >====----
;   \*/

    typedef struct {

        // housekeeping

        char miname[4];     // name of the structure
        long milength;      // structure length

        // hardware vendor name

        long miversion;     // driver software version
        char mivname[32];       // vendor name, etc. (ASCIIZ string)
        char miprod[32];        // vendor product name
        char michip[32];        // vendor chip/hardware description
        char miboardid;     // installed board #
        char miunused[3];       // unused data

        char milibrary[14];     // the patch library name. file name
                    // only - 8 name char, 3 ext char
        // Feature list

        long mifeatures;        // feature bits
        int  midevpref;     // user determined preference field
        int  mimemreq;      // memory required for driver use
        int  mitimerticks;      // # of timer tick callbacks per second

        int  miactivetones;     // max # of tones (voices/partials/etc)

    } MIDIInfo, far *fpMIDInfo;

;   // MIDI feature bits

#define MIDIFRESVD1 0x00000001  // reserved for GM extensions
#define MIDIFRESVD2 0x00000002  // reserved for GM extensions
#define MIDIFRESVD3 0x00000004  // reserved for GM extensions
#define MIDIFRESVD4 0x00000008  // reserved for GM extensions
#define MIDIFXMITR  0x00000010  // Transmitter/Receiver only.
#define MIDIFPRELD  0x00000020  // Patches preloaded.
#define MIDIFTIMEST     0x00000040      // MIDI receive has time stamp.
#define MIDIINTR    0x00000100  // MIDI interrupt driven input supported
#define MIDIPOLL    0x00000200  // MIDI polled input supported
#define MIDIREMOTELD    0x00000400  // MIDI remote patches supported

;   // MIDI device check messages

#define MIDITONES         0x0011    // return available tones
#define MIDIPATCHTYPE         0x0012    // return TRUE/FALSE if patch is understood
#define MIDISETPREFERENCE     0x0013    // set the preference
#define MIDIVOICESTEAL        0x0014    // allow/disallow voice stealing
#define MIDIGETFIFOSIZES      0x0015
#define MIDIGETDMAIRQ         0x0016
#define MIDIGETIOADDRESS      0x0017
#define MIDIGETMEMADDRESS     0x0018
#define MIDIGETMEMFREE        0x0019
#define MIDIOEMSTARTINGDC     0x0080h   // vendors can add DevChks above 0x80

;   // MIDI Registered Patch Types

#define MIDI_PATCH_OPL2       0x0010
#define MIDI_PATCH_OPL3       0x0011


;   /*\
;---|*|----====< MIDI Audio Services Structure >====----
;   \*/

    // Synthesizer Services Structure

    typedef struct {

         // housekeeping

         char msname[4];        // name of the structure
         long mslength;     // structure length

         // runtime data

         int  mspatches[16];    // patches loaded table bit field
         char msfuture[16];     // 16 bytes for future expansion

         // device driver functions

         long ( pascal far *msDeviceCheck  ) ( int, long );
         int  ( pascal far *msGlobalReset  ) ( void );
         int  ( pascal far *msMIDImsg      ) ( char far *, int );
         void ( pascal far *msPollMIDI     ) ( int );
         int  ( pascal far *msPreLoadPatch ) ( int, int, void far *, long );
         int  ( pascal far *msUnloadPatch  ) ( int, int );
         void ( pascal far *msTimerTick    ) ( void );
         int  ( pascal far *msGetLastError ) ( void );

         // callbacks filled in by the application

         void ( pascal far *msApplFreeCB ) ( int, int, void far *, long );
         void ( pascal far *msApplMIDIIn ) ( int, int, char, long );

    } MIDIService, far *fpMIDServ;

#define MID_NOSUPPORT       0x01    // unsupported feature/function
#define MID_UNKNOWNPATCH    0x02    // unknown patch type
#define MID_ALLTONESUSED    0x03    // all tones are used
#define MID_BADMIDIMSG      0x04    // messages are out of sync
#define MID_PATCHINCOMP     0x05    // an incoming patch was incomplete
#define MID_PATCHINFULL     0x06    // incoming patch list is full
#define MID_BADLOSTIRQ      0x07    // had to drop an incoming byte
#define MID_PATCHINFAIL     0x08    // driver is failing a patch download
#define MID_HWFAILURE       0x80    // vendors specific errors


;   /*\
;---|*|----====< General Device Class structure for all devices >====----
;   \*/

;   //  When a device is queried via function #1, it will return the
;   //  following structure. The entire structure contents are read-only
;   //  for the application. There are no fields that the application
;   //  may modify.

    typedef struct {

        // housekeeping...

        char gdname[4];     // name of the structure
        long gdlength;      // structure length

        // generalities...

        int  gdclassid;     // type of device
        int  gdvbever;      // version of VESA driver support

        union {
        WAVEInfo   gdwi;
        MIDIInfo   gdmi;
        VolumeInfo gdvi;
        } u;

    } GeneralDeviceClass, far *fpGDC;


;   /*\
;---|*|----====< VBE/AI data structures >====----
;   \*/

    //
    // patch file format structures
    //

        typedef struct {
        char type[4];   // structure type holds "RIFF"
        long rcount;    // length of the RIFF block
    } RIFFhdr;

        typedef struct {
        char type[4];   // structure type holds "vail"
        long pcount;    // count of patches in the file
    } VAILhdr;

        typedef struct {
        char type[4];   // structure type holds "ZSTR"
        long tlen;      // length of text following this field
    } ZSTRhdr;

        typedef struct {
        char type[4];   // structure type holds "vaip"
        long vaiplen;   // VBE/AI Patch data table
    } VAIPhdr;

        typedef struct {
        char type[4];   // structure type holds "vaii", index
        long vaiilen;   // vaii structure length
        long poffset;   // holds offset to "vaid" from "vaip"
        long vaidln;    // holds length of the corresponding patch
    } VAIIhdr;

        typedef struct {
        char type[4];   // structure type holds "vaid"
        long vaidlen;   // holds length of this patch
    } VAIDhdr;


;   /*\
;---|*|----====< PC Specific Hardware Defintions >====----
;   \*/

// Interrupt Controller #1 Port Addresses and Interrupt Masks

#define IRQ1MASKREG 0x21        // 8259 mask register
#define IRQ1ACKREG  0x20        // 8259 INT acknowledge register

#define INT0MSK     0x01        // interrupt 0 mask
#define INT1MSK     0x02        // interrupt 1 mask
#define INT2MSK     0x04        // interrupt 2 mask
#define INT3MSK     0x08        // interrupt 3 mask
#define INT4MSK     0x10        // interrupt 4 mask
#define INT5MSK     0x20        // interrupt 5 mask
#define INT6MSK     0x40        // interrupt 6 mask
#define INT7MSK     0x80        // interrupt 7 mask

// Interrupt Controller #2 Port Addresses and Interrupt Masks

#define IRQ2MASKREG 0xA1        // 8259 mask register
#define IRQ2ACKREG  0xA0        // 8259 INT acknowledge register

#define INT8MSK     0x01        // interrupt 8 mask
#define INT9MSK     0x02        // interrupt 9 mask
#define INTAMSK     0x04        // interrupt A mask
#define INTBMSK     0x08        // interrupt B mask
#define INTCMSK     0x10        // interrupt C mask
#define INTDMSK     0x20        // interrupt D mask
#define INTEMSK     0x40        // interrupt E mask
#define INTFMSK     0x80        // interrupt F mask

#define EOI     020h        // non specific end of interrupt

// dma controller #1 port addresses

#define DMAC0ADDR   0x00        // DMA channel 0 Base & Address
#define DMAC0COUNT  0x01        // DMA channel 0 Base & Count
#define DMAC1ADDR   0x02        // DMA channel 1 Base & Address
#define DMAC1COUNT  0x03        // DMA channel 1 Base & Count
#define DMAC2ADDR   0x04        // DMA channel 2 Base & Address
#define DMAC2COUNT  0x05        // DMA channel 2 Base & Count
#define DMAC3ADDR   0x06        // DMA channel 3 Base & Address
#define DMAC3COUNT  0x07        // DMA channel 3 Base & Count

#define DMA2C4ADDR  0xC0        // DMA channel 4 Base & Current Address
#define DMA2C4COUNT 0xC2        // DMA channel 4 Base & Current Count
#define DMA2C5ADDR  0xC4        // DMA channel 5 Base & Current Address
#define DMA2C5COUNT 0xC6        // DMA channel 5 Base & Current Count
#define DMA2C6ADDR  0xC8        // DMA channel 6 Base & Current Address
#define DMA2C6COUNT 0xCA        // DMA channel 6 Base & Current Count
#define DMA2C7ADDR  0xCC        // DMA channel 7 Base & Current Address
#define DMA2C7COUNT 0xCE        // DMA channel 7 Base & Current Count

#define DMARDSTAT   0x08        // DMA read status
#define DMAWRCNTRL  0x08        // DMA write command register
#define DMAWREQ     0x09        // DMA write request register
#define DMAWRSMR    0x0A        // DMA write single mask reg.
#define DMAWRMODE   0x0B        // DMA write mode register
#define DMACLEAR    0x0C        // DMA clear low/high flip-flop
#define DMARDTEMP   0x0D        // DMA read temp register
#define DMAWRCLR    0x0D        // DMA write master clear
#define DMACLRMSK   0x0E        // DMA clear mask register
#define DMAWRALL    0x0F        // DMA write all mask reg. bits

#define DMA2RDSTAT  0xD0        // DMA read status
#define DMA2WRCNTRL 0xD0        // DMA write command register
#define DMA2WREQ    0xD2        // DMA write request register
#define DMA2WRSMR   0xD4        // DMA write single mask register
#define DMA2WRMODE  0xD6        // DMA write mode register
#define DMA2CLEAR   0xD8        // DMA clear low/high flip-flop
#define DMA2RDTEMP  0xDA        // DMA read temp register
#define DMA2WRCLR   0xDA        // DMA write master clear
#define DMA2CLRMSK  0xDC        // DMA clear mask register
#define DMA2WRALL   0xDE        // DMA write all mask register bits

#define CH0PAGEREG  0x87        // Channel 0 Page Register
#define CH1PAGEREG  0x83        // Channel 1 Page Register
#define CH2PAGEREG  0x81        // Channel 2 Page Register
#define CH3PAGEREG  0x82        // Channel 3 Page Register

#define CH5PAGEREG  0x8B        // Channel 5 Page Register
#define CH6PAGEREG  0x89        // Channel 6 Page Register
#define CH7PAGEREG  0x8A        // Channel 7 Page Register

#define SYSSPKRTMR  0x0042      // System Speaker Timer Address
#define SYSTMRCTLR  0x0043      // System Timer Control Register
#define SYSSPKRREG  0x0061      // System Speaker Register
#define JOYSTICK    0x0201      // Joystick Register

;   /*\
;---|*| end of VBEAI.H
;   \*/


