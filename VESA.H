
;   /*\
;---|*|--------------------====< VESA Sound Include >====--------------------
;---|*|
;---|*| Copyright (c) 1993,1994  V.E.S.A, Inc. All Rights Reserved.
;---|*|
;---|*| VBE/AI 1.0 Specification
;---|*|    February 2, 1994. 1.00 release
;---|*|
;   \*/

;   /*\
;---|*|----====< VESA data types >====----
;   \*/

    // handle to the device

    typedef int VESAHANDLE;
    typedef int BLOCKHANDLE;

;   /*\
;---|*| VESA high level calls
;   \*/

    void far *  pascal AllocateBuffer    ( long );
    void    pascal FreeBuffer    ( void far * );

    VESAHANDLE  pascal VESAFindADevice   ( int );
    long    pascal VESAQueryDevice   ( VESAHANDLE, int, void far * );
    void far *  pascal VESAOpenADevice   ( VESAHANDLE, int, void far * );
    void    pascal VESACloseDevice   ( VESAHANDLE );
    int     pascal VESARegisterTimer ( VESAHANDLE,void (far pascal *)(),long);
    long    pascal VESARateToDivisor ( long );

    int     pascal VESAFreePatchBank ( VESAHANDLE );
    int     pascal VESAPreLoadPatch  ( VESAHANDLE,int,int);
    int     pascal VESALoadPatchBank ( VESAHANDLE,fpMIDServ,char far *);


;   /*\
;---|*| end of VESA.H
;   \*/


