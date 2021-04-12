
#
# BANK.EXE instrument build editor
#

AS = /DDEBUG=0			# Assembler static switches
CS = /Zi /Zp1 /Ox /c		# C compiler static switches
LS =				# Linker static switches
AO =				# Assembler optional switches
CO =				# C compiler optional switches
LO = /Co /Ma			# Linker optional switches

build.obj: build.c
    cl $(CS) $(CO) build.c

..\patch.obj: ..\patch.asm ..\opl2.inc
    cd ..\..\..
    masm $(AS) $(AO) midi\opl2\patch,midi\opl2\patch;
    cd midi\opl2\tools

build.exe: build.obj ..\patch.obj
    link $(LS) $(LO) build+..\patch;

