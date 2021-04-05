
;   /*\
;---|*|-----------------====< VBE/AI MIDI INCLUDE >====----------------------
;---|*|
;---|*| Copyright (c) 1993,1994  V.E.S.A, Inc. All Rights Reserved.
;---|*|
;---|*| VBE/AI 1.0 Specification
;---|*|    February 2, 1994. 1.00 release
;---|*|
;   \*/

    //
    // OPL2 Registered Patch
    //

    typedef struct {        // nested structure within the
        char opl2ksl;       // patch structure
        char opl2freqMult;
        char opl2feedBack;      // used by operator 0 only
        char opl2attack;
        char opl2sustLevel;
        char opl2sustain;
        char opl2decay;
        char opl2release;
        char opl2output;
        char opl2am;
        char opl2vib;
        char opl2ksr;
        char opl2fm;        // used by operator 0 only
    } OPL2opr;

    //
    // The following structure represents the format of whole patch
    // passed in from the application. This whole structure makes up
    // a single OPL2 patch.
    //

        typedef struct {
        int     opl2type;       // patch type
            char    opl2mode;           // 0 = melodic, 1 = percussive
        char    opl2percVoice;  // if mode == 1, voice number to be used
        OPL2opr opl2op0;
            OPL2opr opl2op1;
        char    opl2wave0;      // waveform for operator 0
        char    opl2wave1;      // waveform for operator 1
    } OPL2native;


    //
    // The following structure makes up the complete VBE/AI 'vaid' patch
    // in the patch librayr. It includes the VESA patch header wrapper.
    //

    typedef struct {
        VAIDhdr vbehdr;     // the 'vaid' header
        OPL2native  native;     // native patch data
    } OPL2patch;

    //
    // OPL3 Registered Patch
    //

    // OPL3 Patch Structure

    typedef struct {        // register values from the
        int  opl3type;      // patch type
        char reg20[4];      // four operators
        char reg40[4];      // patch structure
        char reg60[4];      // patch structure
        char reg80[4];      // patch structure
        char regC0[2];      // patch structure
        char regE0[2];      // patch structure
    } OPL3native;

        //
    // The following structure makes up the complete VBE/AI 'vaid' patch
    // in the patch library. It includes the VESA patch header wrapper.
    //

    typedef struct {
        VAIDhdr vbehdr;     // the 'vaid' header
        OPL3native  native;     // native patch data
    } OPL3patch;


;   /*\
;---|*| end of MIDI.INC
;   \*/


