/*******************************************************************************
  Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
 
  (c) Copyright 1996 - 2002 Gary Henderson (gary.henderson@ntlworld.com) and
                            Jerremy Koot (jkoot@snes9x.com)

  (c) Copyright 2001 - 2004 John Weidman (jweidman@slip.net)

  (c) Copyright 2002 - 2004 Brad Jorsch (anomie@users.sourceforge.net),
                            funkyass (funkyass@spam.shaw.ca),
                            Joel Yliluoma (http://iki.fi/bisqwit/)
                            Kris Bleakley (codeviolation@hotmail.com),
                            Matthew Kendora,
                            Nach (n-a-c-h@users.sourceforge.net),
                            Peter Bortas (peter@bortas.org) and
                            zones (kasumitokoduck@yahoo.com)

  C4 x86 assembler and some C emulation code
  (c) Copyright 2000 - 2003 zsKnight (zsknight@zsnes.com),
                            _Demo_ (_demo_@zsnes.com), and Nach

  C4 C++ code
  (c) Copyright 2003 Brad Jorsch

  DSP-1 emulator code
  (c) Copyright 1998 - 2004 Ivar (ivar@snes9x.com), _Demo_, Gary Henderson,
                            John Weidman, neviksti (neviksti@hotmail.com),
                            Kris Bleakley, Andreas Naive

  DSP-2 emulator code
  (c) Copyright 2003 Kris Bleakley, John Weidman, neviksti, Matthew Kendora, and
                     Lord Nightmare (lord_nightmare@users.sourceforge.net

  OBC1 emulator code
  (c) Copyright 2001 - 2004 zsKnight, pagefault (pagefault@zsnes.com) and
                            Kris Bleakley
  Ported from x86 assembler to C by sanmaiwashi

  SPC7110 and RTC C++ emulator code
  (c) Copyright 2002 Matthew Kendora with research by
                     zsKnight, John Weidman, and Dark Force

  S-DD1 C emulator code
  (c) Copyright 2003 Brad Jorsch with research by
                     Andreas Naive and John Weidman
 
  S-RTC C emulator code
  (c) Copyright 2001 John Weidman
  
  ST010 C++ emulator code
  (c) Copyright 2003 Feather, Kris Bleakley, John Weidman and Matthew Kendora

  Super FX x86 assembler emulator code 
  (c) Copyright 1998 - 2003 zsKnight, _Demo_, and pagefault 

  Super FX C emulator code 
  (c) Copyright 1997 - 1999 Ivar, Gary Henderson and John Weidman


  SH assembler code partly based on x86 assembler code
  (c) Copyright 2002 - 2004 Marcus Comstedt (marcus@mc.pp.se) 

 
  Specific ports contains the works of other authors. See headers in
  individual files.
 
  Snes9x homepage: http://www.snes9x.com
 
  Permission to use, copy, modify and distribute Snes9x in both binary and
  source form, for non-commercial purposes, is hereby granted without fee,
  providing that this license information and copyright notice appear with
  all copies and any derived work.
 
  This software is provided 'as-is', without any express or implied
  warranty. In no event shall the authors be held liable for any damages
  arising from the use of this software.
 
  Snes9x is freeware for PERSONAL USE only. Commercial users should
  seek permission of the copyright holders first. Commercial use includes
  charging money for Snes9x or software derived from Snes9x.
 
  The copyright holders request that bug fixes and improvements to the code
  should be forwarded to them so everyone can benefit from the modifications
  in future versions.
 
  Super NES and Super Nintendo Entertainment System are trademarks of
  Nintendo Co., Limited and its subsidiary companies.
*******************************************************************************/
#ifndef _SPC700_H_
#define _SPC700_H_

#define Carry       1
#define Zero        2
#define Interrupt   4
#define HalfCarry   8
#define BreakFlag  16
#define DirectPageFlag 32
#define Overflow   64
#define Negative  128

#define APUClearCarry() (APUPack.IAPU._Carry = 0)
#define APUSetCarry() (APUPack.IAPU._Carry = 1)
#define APUSetInterrupt() (APUPack.APURegisters.P |= Interrupt)
#define APUClearInterrupt() (APUPack.APURegisters.P &= ~Interrupt)
#define APUSetHalfCarry() (APUPack.APURegisters.P |= HalfCarry)
#define APUClearHalfCarry() (APUPack.APURegisters.P &= ~HalfCarry)
#define APUSetBreak() (APUPack.APURegisters.P |= BreakFlag)
#define APUClearBreak() (APUPack.APURegisters.P &= ~BreakFlag)
#define APUSetDirectPage() (APUPack.APURegisters.P |= DirectPageFlag)
#define APUClearDirectPage() (APUPack.APURegisters.P &= ~DirectPageFlag)
#define APUSetOverflow() (APUPack.IAPU._Overflow = 1)
#define APUClearOverflow() (APUPack.IAPU._Overflow = 0)

#define APUCheckZero() (APUPack.IAPU._Zero == 0)
#define APUCheckCarry() (APUPack.IAPU._Carry)
#define APUCheckInterrupt() (APUPack.APURegisters.P & Interrupt)
#define APUCheckHalfCarry() (APUPack.APURegisters.P & HalfCarry)
#define APUCheckBreak() (APUPack.APURegisters.P & BreakFlag)
#define APUCheckDirectPage() (APUPack.APURegisters.P & DirectPageFlag)
#define APUCheckOverflow() (APUPack.IAPU._Overflow)
#define APUCheckNegative() (APUPack.IAPU._Zero & 0x80)

#define APUClearFlags(f) (APUPack.APURegisters.P &= ~(f))
#define APUSetFlags(f)   (APUPack.APURegisters.P |=  (f))
#define APUCheckFlag(f)  (APUPack.APURegisters.P &   (f))

typedef union
{
#ifdef LSB_FIRST
    struct { uint8 A, Y; } B;
#else
    struct { uint8 Y, A; } B;
#endif
    uint16 W;
} YAndA;

struct SAPURegisters{
    uint8  P;
    YAndA YA;
    uint8  X;
    uint8  S;
    uint16  PC;
};

// Needed by ILLUSION OF GAIA
//#define ONE_APU_CYCLE 14
#define ONE_APU_CYCLE 21

// Needed by all games written by the software company called Human
//#define ONE_APU_CYCLE_HUMAN 17
#define ONE_APU_CYCLE_HUMAN 21

// 1.953us := 1.024065.54MHz

#define APU_SETAPURAM() {\
  SAPUEVENTS *pEvent = (SAPUEVENTS *)UNCACHE_PTR(&stAPUEvents);\
  int apu_ram_write_cpt1_,apu_ram_write_cpt2_; \
  apu_ram_write_cpt1_=pEvent->apu_ram_write_cpt1;\
  apu_ram_write_cpt2_=pEvent->apu_ram_write_cpt2;\
  if (apu_ram_write_cpt1_<apu_ram_write_cpt2_) { \
    int cpt;\
    for (cpt=apu_ram_write_cpt1_;cpt<apu_ram_write_cpt2_;cpt++) {\
        unsigned short dwValue = pEvent->apu_ram_write_log[cpt & 0xFFFF];\
        APUPack.IAPU.RAM[((dwValue>>8) & 3) | 0xF4] = dwValue & 0xFF;\
    }\
    pEvent->apu_ram_write_cpt1=apu_ram_write_cpt2_;\
  }\
}

#define APU_EXECUTE3() { \
 volatile SAPUEVENTS *pEvent = (SAPUEVENTS *)UNCACHE_PTR(&stAPUEvents);\
 int apu_target_cycles_=pEvent->apu_glob_cycles;\
 if (pEvent->APU_Cycles<=apu_target_cycles_) {\
  int apu_event1_cpt1_,apu_event1_cpt2_;\
  APUPack.APU.Cycles=pEvent->APU_Cycles; \
  apu_event1_cpt1_=pEvent->apu_event1_cpt1;\
  apu_event1_cpt2_=pEvent->apu_event1_cpt2;\
	while (APUPack.APU.Cycles<=apu_target_cycles_) {\
		APUPack.APU.Cycles+=S9xAPUCycles [*(APUPack.IAPU.PC)];\
		S9xApuOpcodes[*(APUPack.IAPU.PC)](); \
		while (apu_event1_cpt1_<apu_event1_cpt2_) {\
		    uint32 EventVal = pEvent->apu_event1[apu_event1_cpt1_ & 0xFFFF];\
			uint32 V_Counter = EventVal & 0x80000000;\
			EventVal &= 0x7FFFFFFF;\
			if (APUPack.APU.Cycles>=EventVal) {\
				apu_event1_cpt1_++;\
				if ((APUPack.APU.TimerEnabled) [2]) {\
					(APUPack.APU.Timer) [2] += 4;\
					while (APUPack.APU.Timer[2] >= APUPack.APU.TimerTarget[2]) {\
		  			APUPack.IAPU.RAM[0xff] = (APUPack.IAPU.RAM[0xff] + 1) & 0xf;\
		  			APUPack.APU.Timer[2] -= APUPack.APU.TimerTarget[2];\
					}\
				}\
				if (V_Counter) {\
					if (APUPack.APU.TimerEnabled[0]) {\
		  			APUPack.APU.Timer[0]++;\
		  			if (APUPack.APU.Timer[0] >= APUPack.APU.TimerTarget[0]) {\
							APUPack.IAPU.RAM[0xfd] = (APUPack.IAPU.RAM[0xfd] + 1) & 0xf;\
							APUPack.APU.Timer[0] = 0;\
					  }\
					}\
					if (APUPack.APU.TimerEnabled[1]) {\
		  			APUPack.APU.Timer[1]++;\
		  			if (APUPack.APU.Timer[1] >= APUPack.APU.TimerTarget[1]) {\
							APUPack.IAPU.RAM[0xfe] = (APUPack.IAPU.RAM[0xfe] + 1) & 0xf;\
							APUPack.APU.Timer[1] = 0;\
			  		}\
					}\
				}\
			} else break;\
		}\
	}\
 *((int*)(pEvent->APU_OutPorts))=*((int*)(APUPack.APU.OutPorts)); \
 pEvent->apu_event1_cpt1=apu_event1_cpt1_;\
 pEvent->APU_Cycles=APUPack.APU.Cycles; \
} \
}


#ifdef ME_SOUND
#define APU_EXECUTE2() {\
volatile SAPUEVENTS *pEvent = (SAPUEVENTS *)UNCACHE_PTR(&stAPUEvents);\
int nApuCycles = pEvent->apu_glob_cycles;\
if ((pEvent->APU_Cycles)<=nApuCycles) {\
int dummy=0;\
int nCounter = 0;\
while (1) {\
	if (pEvent->APU_Cycles>nApuCycles) break; \
	dummy=rand()+dummy;\
	nCounter++;\
if (nCounter > 10000000) {\
	while (1) {\
		char st[108];\
		pgPrintBG(0,7,0xFFFF,"maybe deadlock(in APU_EXECUTE2)");\
		sprintf(st,"param MAIN%08X ME%08X", pEvent->dwDeadlockTestMain,pEvent->dwDeadlockTestMe);\
		pgPrintBG(0,8,0xFFFF,st);\
		sprintf(st,"APU%X", pEvent->IAPU_APUExecuting);\
		pgPrintBG(0,3,0xFFFF,st);\
		sprintf(st,"E1[%04X,%04X] E2[%04X,%04X], RW[%04X,%04X]",\
			pEvent->apu_event1_cpt1&0xFFFF, pEvent->apu_event1_cpt2&0xFFFF,\
			pEvent->apu_ram_write_cpt1&0xFFFF, pEvent->apu_ram_write_cpt2&0xFFFF);\
		pgPrintBG(0,4,0xFFFF,st);\
		sprintf(st,"APUCycles%08X, %08X, %08X",\
			pEvent->APU_Cycles, pEvent->apu_glob_cycles, cpu_glob_cycles);\
		pgPrintBG(0,5,0xFFFF,st);\
		sprintf(st,"%08X,%08X,%08X,%08X",\
			pEvent->adwParam[0], pEvent->adwParam[1], pEvent->adwParam[2], pEvent->adwParam[3]);\
		pgPrintBG(0,6,0xFFFF,st);\
		pgScreenFlipV();\
	}\
}\
}\
pEvent->apu_glob_cycles = nApuCycles - cpu_glob_cycles;\
pEvent->APU_Cycles-=cpu_glob_cycles;\
cpu_glob_cycles=0;\
if (pEvent->apu_event1_cpt1<pEvent->apu_event1_cpt2) { \
	int i,j;\
	j=pEvent->apu_event1_cpt2;\
	for (i=pEvent->apu_event1_cpt1;i<j;i++) pEvent->apu_event1[i & 0xFFFF]=0;\
}\
}\
}
#else
#define APU_EXECUTE2() { \
volatile SAPUEVENTS *pEvent = (SAPUEVENTS *)UNCACHE_PTR(&stAPUEvents);\
if ((pEvent->APU_Cycles)<=pEvent->apu_glob_cycles) {\
	int apu_target_cycles_=pEvent->apu_glob_cycles;\
  int apu_event1_cpt1_,apu_event2_cpt1_;\
  int apu_event1_cpt2_,apu_event2_cpt2_;\
  int apu_ram_write_cpt1_,apu_ram_write_cpt2_; \
  APUPack.APU.Cycles=pEvent->APU_Cycles; \
  apu_event1_cpt1_=pEvent->apu_event1_cpt1;\
  apu_event2_cpt1_=pEvent->apu_event2_cpt1;\
  apu_event1_cpt2_=pEvent->apu_event1_cpt2;\
  apu_event2_cpt2_=pEvent->apu_event2_cpt2;\
  apu_ram_write_cpt1_=pEvent->apu_ram_write_cpt1;\
  apu_ram_write_cpt2_=pEvent->apu_ram_write_cpt2;\
  if (apu_ram_write_cpt1_<apu_ram_write_cpt2_) { \
  	int cpt;\
  	for (cpt=apu_ram_write_cpt1_;cpt<apu_ram_write_cpt2_;cpt++) (APUPack.IAPU.RAM)[(pEvent->apu_ram_write_log[cpt & 0xFFFF]>>8)|0xF4]=pEvent->apu_ram_write_log[cpt&(65536*2-1)]&0xFF;\
		pEvent->apu_ram_write_cpt1=apu_ram_write_cpt2_;\
	}  \
	while (APUPack.APU.Cycles<=apu_target_cycles_) {\
		(APUPack.APU.Cycles)+=S9xAPUCycles [*(APUPack.IAPU.PC)];\
		(*S9xApuOpcodes[*(APUPack.IAPU.PC)]) (); \
		while (apu_event1_cpt1_<apu_event1_cpt2_) {\
			if (APUPack.APU.Cycles>=pEvent->apu_event1[apu_event1_cpt1_ & 0xFFFF]) {\
				apu_event1_cpt1_++;\
				if (APUPack.APU.TimerEnabled[2]) {\
					APUPack.APU.Timer[2] += 4;\
					while (PUPack.APU.Timer[2] >= APUPack.APU.TimerTarget[2]) {\
		  			APUPack.IAPU.RAM [0xff] = (APUPack.IAPU.RAM[0xff] + 1) & 0xf;\
		  			APUPack.APU.Timer [2] -= APUPack.APU.TimerTarget[2];\
					}\
				}\
			} else break;\
		}\
		while (apu_event2_cpt1_<apu_event2_cpt2_) {\
			if (APUPack.APU.Cycles>=pEvent->apu_event2[apu_event2_cpt1_ & 0xFFFF]) {\
				apu_event2_cpt1_++;\
				if (APUPack.APU.TimerEnabled[0]) {\
		  		APUPack.APU.Timer[0]++;\
		  		if (APUPack.APU.Timer[0] >= APUPack.APU.TimerTarget[0]) {\
						APUPack.IAPU.RAM[0xfd] = (APUPack.IAPU.RAM[0xfd] + 1) & 0xf;\
						APUPack.APU.Timer[0] = 0;\
				  }\
				}\
				if (APUPack.APU.TimerEnabled[1]) {\
		  		APUPack.APU.Timer[1]++;\
		  		if (APUPack.APU.Timer[1] >= APUPack.APU.TimerTarget[1]) {\
						APUPack.IAPU.RAM[0xfe] = (APUPack.IAPU.RAM [0xfe] + 1) & 0xf;\
						APUPack.APU.Timer[1] = 0;\
			  	}\
				}\
			} else break;\
		}\
}\
*((int*)(pEvent->APU_OutPorts))=*((int*)(APUPack.APU.OutPorts)); \
pEvent->apu_event1_cpt1=apu_event1_cpt1_;\
pEvent->apu_event2_cpt1=apu_event2_cpt1_;\
 pEvent->APU_Cycles=APUPack.APU.Cycles; \
pEvent->apu_glob_cycles-=cpu_glob_cycles;\
pEvent->APU_Cycles-=cpu_glob_cycles;\
cpu_glob_cycles=0;\
if (pEvent->apu_event1_cpt1<pEvent->apu_event1_cpt2) { \
	int i,j;\
	j=pEvent->apu_event1_cpt2;\
	for (i=pEvent->apu_event1_cpt1;i<j;i++) pEvent->apu_event1[i & 0xFFFF]=0;\
}\
if (pEvent->apu_event2_cpt1<pEvent->apu_event2_cpt2) { \
	int i,j;\
	j=pEvent->apu_event2_cpt2;\
	for (i=pEvent->apu_event2_cpt1;i<j;i++) pEvent->apu_event2[i & 0xFFFF]=0;\
}\
} \
}
#endif

#endif
