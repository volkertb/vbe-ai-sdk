
    /*\
    |*|----====< bank.h >====----
    \*/

    /*\
    |*| global constants
    \*/

#define BYTE        char

#define DEF_PORT    0x388   // default port address

#define NUMVOICES   16  // number of voices we are working with
#define NUMMELODIC  6   // number of voices we can allocate
#define NUMCHANNELS 16  // number of MIDI channels

#define MAXPATCH    150 // max patch number allowed (>127 for drums)

    // default patch numbers

#define PATCHaDRUMKIT   90  // our acoustic drum kit patch
#define PATCHeDRUMKIT   91  // our electronic drum kit patch
#define PATCHPIANO  0

#define FIRSTDRUMNOTE   35
#define LASTDRUMNOTE    75
#define NUMDRUMNOTES    (LASTDRUMNOTE - FIRSTDRUMNOTE + 1)

#define MAX_VOLUME  0x7f
#define MAX_PITCH   0x3fff
#define MID_PITCH   0x2000

#define MID_C       60  // MIDI standard mid C
#define CHIP_MID_C  48  // sound chip mid C

#define TOM_PITCH   24  // best frequency, in range of 0 to 95
#define TOM_TO_SD   7   // 7 half-tones between voice 7 & 8
#define SD_PITCH    (TOM_PITCH + TOM_TO_SD)
#define NUMLOCPARAM 14  // number of loc params we keep
#define NR_STEP_PITCH   25  // 25 steps within a half-tone for pitch bend


    /*\
    |*| definitions of sound chip parameters
    \*/

    // parameters of each voice

#define prmKsl          0
#define prmMulti        1
#define prmFeedBack     2           // use for operator 0 only
#define prmAttack       3
#define prmSustain      4
#define prmStaining     5           // sustaining...
#define prmDecay        6
#define prmRelease      7
#define prmLevel        8
#define prmAm           9
#define prmVib          10
#define prmKsr          11
#define prmFm           12          // use for operator 0 only
#define prmWaveSel      13          // wave select

    // global parameters

#define prmAmDepth      14
#define prmVibDepth     15
#define prmNoteSel      16
#define prmPercussion   17

    // percussive voice numbers:

#define BD              6
#define SD              7
#define TOM             8
#define CYMB            9
#define HIHAT           10


    /*\
    |*| bank file support
    \*/

#define BANK_SIG_LEN        6
#define BANK_FILLER_SIZE    8

    // instrument bank file header

    typedef struct {
        char      majorVersion;
        char      minorVersion;
        char      sig[BANK_SIG_LEN];        // signature: "ADLIB-"
        unsigned  nrDefined;            // number of list entries used
        unsigned  nrEntry;              // number of total entries in list
        long      offsetIndex;          // offset of start of list of names
        long      offsetTimbre;         // offset of start of data
        char      filler[BANK_FILLER_SIZE];     // must be zero
    } BANKHEADER;

    // instrument name structure

    typedef struct {
        unsigned  index;        // index into data section
        char      used;     // used TRUE(1)/FALSE(0)
        char      name[9];      // instrument name
    } INSHEADER;

    // operator structure

    typedef struct {
        BYTE ksl;
        BYTE freqMult;
        BYTE feedBack;      // used by operator 0 only
        BYTE attack;
        BYTE sustLevel;
        BYTE sustain;
        BYTE decay;
        BYTE release;
        BYTE output;
        BYTE am;
        BYTE vib;
        BYTE ksr;
        BYTE fm;            // used by operator 0 only
    } OPERATOR;

    typedef struct {
        BYTE      mode;     // 0 = melodic, 1 = percussive
        BYTE      percVoice;    // if mode == 1, voice number to be used
        OPERATOR  op0;
        OPERATOR  op1;
        BYTE      wave0;        // waveform for operator 0
        BYTE      wave1;        // waveform for operator 1
    } TIMBRE;


    /*\
    |*| end of bank.h
    \*/


