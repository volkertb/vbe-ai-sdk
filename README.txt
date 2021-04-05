
vaisdk.zip    SDK for VESA audio interface BIOS extension

Suggested directory: /pub/msdos/demos/code/sound

I uploaded the file vaisdk.zip.  This file contains an SDK for the
VESA audio interface BIOS extensions.  The file was downloaded 
from MediaVision's BBS (one of the companies involved in defining
the new standard).  The original file was a self-extracting zip
file (vaisdk.exe).  I converted this to a standard zip file.  
Unzip with the -d option to create subdirectories.

Philip VanBaren
phillipv@eecs.umich.edu

----------------------------------------------------------------------
note: the above text was uploaded with a file that was only about
90k, and wouldn't unzip. So I found this file on ftp.leidenuniv.nl
to replace it. I extracted the following description from the file
SDK.DOC which is in the zip file. (The docs say there's an OPL3
driver included, but I couldn't find what they were talking about.
So maybe this version isn't complete either?)

   - Dave Boynton, dboynton@clark.net

----------------------------------------------------------------------

             ----====<   VBE/AI Driver Sample Source Code   >====----
             ----====<  Copyright (c) 1993,1994 VESA, Inc.  >====----
             ----====<          All Rights Reserved         >====----
             ----====<        Version 1.03   04/09/94       >====----


    INTRODUCTION:

        This document describes the VESA VBE/AI Software Developers
        Kit as found on the VBE/AI SDK release diskette. Included
        are sample WAVE drivers for the Pro Audio Spectrum, Disney
        Sound Source, and Sound Blaster cards.  Also included are
        sample MIDI drivers for the OPL2, OPL3 FM chip, and Roland
        MPU-401 MIDI transmitter/receiver.

        The VBE/AI specification may be purchased directly from the
	VESA office. To order a copy, call (408)435-0333.

        Each body of code is as complete as possible for the 1.00
        revision of the VBE/AI specification. The code may still
        change to match any changes when the specification is
        adopted, or to fix bugs.

[snip snip snip]

    DRIVER DISKETTE CONTENTS:

        Disk Contents:

        REV.DOC         Revision changes for the SDK/DDK

        SDK.DOC         Intro documentation to the SDK
        SDK.BAT         Batch file to build the release floppy
        MKSDK.BAT       Batch file to build the example programs

        PASWAVE.COM     Pro Audio Spectrum WAVE VBE/AI driver
        SSWAVE.COM      Disney Sound source WAVE VBE/AI driver
        SBWAVE.COM      Sound Blaster & Compatibles WAVE VBE/AI driver
        OPL2.COM        OPL2 VBE/AI MIDI driver
        MPU.COM         MPU-401 VBE/AI MIDI driver

        ALFRE.MID       Sample MIDI file
        GUPPY.WAV       Sample WAVE file

        OPL2.BNK        Instrument bank for the OPL2 MIDI driver
[note: above file is from The Fatman(tm) himself!]

        MT32            Patch map from GM to an MT32

        PLAY            Make file for the WAVE PLAY.EXE program
        PLAY.C          Source to PLAY.EXE
        PLAY.EXE        PLAY.EXE executable for playback testing of WAVE audio

        TESTW           Make file for the WAVE TESTW.EXE program
        TESTW.C         Source to TESTW.EXE
        TESTW.EXE       TESTW.EXE executable for playback testing of WAVE audio

        NOTE            Make file for the single MIDI note test program
        NOTE.C          Source code for the single MIDI note test program
        NOTE.EXE        Executable MIDI note test program

        PMIDI.C         Source to PMIDI.EXE midi file player
        PMIDI           Make file for the MIDI PMIDI.EXE program
        PMIDI.EXE       PMIDI.EXE executable for MIDI playback
        PDATA.C         Static data for PMIDI.C

        SETPREF         Make file for setting user preferences & general query
        SETPREF.C       Source to SETPREF.EXE executable
        SETPREF.EXE     SETPREF.EXE executable to set user preference levels

        VOLUME          Make file for the volume test program
        VOLUME.C        Tests the Volume control of a given device
        VOLUME.EXE      Executable volume test program

        VESA.C          Mid level helper code for VBE/AI driver support

        MIDI.H          Miscellaneous header files
        VBEAI.H
        VESA.H
        MIDI.INC
        VBEAI.INC
        WAVE.INC

        MIDI\OPL2\TOOLS A directory with s/w for building the OPL2.BNK and
                        MT32 patch map.

