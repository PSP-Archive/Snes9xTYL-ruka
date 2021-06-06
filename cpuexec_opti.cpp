/*
 * Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
 *
 * (c) Copyright 1996 - 2001 Gary Henderson (gary.henderson@ntlworld.com) and
 *                           Jerremy Koot (jkoot@snes9x.com)
 *
 * Super FX C emulator code 
 * (c) Copyright 1997 - 1999 Ivar (ivar@snes9x.com) and
 *                           Gary Henderson.
 * Super FX     assembler emulator code (c) Copyright 1998 zsKnight and _Demo_.
 *
 * DSP1 emulator code (c) Copyright 1998 Ivar, _Demo_ and Gary Henderson.
 * C4 asm and some C emulation code (c) Copyright 2000 zsKnight and _Demo_.
 * C4 C code (c) Copyright 2001 Gary Henderson (gary.henderson@ntlworld.com).
 *
 * DOS port code contains the works of other authors. See headers in
 * individual files.
 *
 * Snes9x homepage: http://www.snes9x.com
 *
 * Permission to use, copy, modify and distribute Snes9x in both binary and
 * source form, for non-commercial purposes, is hereby granted without fee,
 * providing that this license information and copyright notice appear with
 * all copies and any derived work.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event shall the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Snes9x is freeware for PERSONAL USE only. Commercial users should
 * seek permission of the copyright holders first. Commercial use includes
 * charging money for Snes9x or software derived from Snes9x.
 *
 * The copyright holders request that bug fixes and improvements to the code
 * should be forwarded to them so everyone can benefit from the modifications
 * in future versions.
 *
 * Super NES and Super Nintendo Entertainment System are trademarks of
 * Nintendo Co., Limited and its subsidiary companies.
 */

#include "snes9x.h"

#include "memmap.h"
#include "cpuops.h"
#include "ppu.h"
#include "cpuexec.h"
#include "debug.h"
#include "snapshot.h"
#include "gfx.h"
#include "missing.h"
#include "apu.h"
#include "dma.h"
#include "fxemu.h"
#include "sa1.h"

extern char str_tmp[256];

#define FLAGS_NMI() \
				if (--CPU.NMICycleCount == 0) {\
		  		CPU.Flags &= ~NMI_FLAG;\
		  		if (CPU.WaitingForInterrupt) {\
		      	CPU.WaitingForInterrupt = FALSE;\
		      	CPU.PC++;\
		    	}\
		  		S9xOpcode_NMI ();\
				}
#define FLAGS_IRQ() \
				if (CPU.IRQCycleCount == 0) {\
		  		if (CPU.WaitingForInterrupt) {\
		      	CPU.WaitingForInterrupt = FALSE;\
		      	CPU.PC++;\
		    	}\
		  		if (CPU.IRQActive && !Settings.DisableIRQ) {\
		      	if (!CheckFlag (IRQ)) S9xOpcode_IRQ ();\
		    	} else CPU.Flags &= ~IRQ_PENDING_FLAG;\
				} else CPU.IRQCycleCount--;
#define FLAGS_SCAN_KEYS_FLAG() return
			


void (*S9x_Current_HBlank_Event)();

void S9xMainLoop_SA1_APU (void) {
	for (;;) { 
		//APU_EXECUTE ();
		if (IAPU->APUExecuting) {			
			cpu_glob_cycles += CPU.Cycles-old_cpu_cycles;
			old_cpu_cycles=CPU.Cycles;
			*apu_glob_cycles=cpu_glob_cycles;
			
			if (cpu_glob_cycles>=0x00700000) {		
				APU_EXECUTE2 ();
			}
		}
		
    if (CPU.Flags) {
	  	if (CPU.Flags & NMI_FLAG) {
	      if (--CPU.NMICycleCount == 0) {
		  		CPU.Flags &= ~NMI_FLAG;
		  		if (CPU.WaitingForInterrupt) {
		      	CPU.WaitingForInterrupt = FALSE;
		      	CPU.PC++;
		    	}
		  		S9xOpcode_NMI ();
				}
	    }

	  	if (CPU.Flags & IRQ_PENDING_FLAG) {
	      if (CPU.IRQCycleCount == 0) {
		  		if (CPU.WaitingForInterrupt) {
		      	CPU.WaitingForInterrupt = FALSE;
		      	CPU.PC++;
		    	}
		  		if (CPU.IRQActive && !Settings.DisableIRQ) {
		      	if (!CheckFlag (IRQ)) S9xOpcode_IRQ ();
		    	} else CPU.Flags &= ~IRQ_PENDING_FLAG;
				} else CPU.IRQCycleCount--;
	    }
	  	if (CPU.Flags & SCAN_KEYS_FLAG) break;
		}

		#ifdef CPU_SHUTDOWN
    CPU.PCAtOpcodeStart = CPU.PC;
		#endif
				
    CPU.Cycles += CPU.MemSpeed;

    (*ICPU.S9xOpcodes[*CPU.PC++].S9xOpcode) ();

    //S9xUpdateAPUTimer ();

    if (SA1.Executing) S9xSA1MainLoop ();
      
    DO_HBLANK_CHECK ();
  }
}



void S9xMainLoop_NoSA1_APU (void) {
	for (;;) {
		//APU_EXECUTE ();				
		if (IAPU->APUExecuting) {
			if (CPU.Cycles-old_cpu_cycles<0) msgBoxLines("1",60);
			else cpu_glob_cycles += CPU.Cycles-old_cpu_cycles;
			old_cpu_cycles=CPU.Cycles;
			*apu_glob_cycles=cpu_glob_cycles;
			
			if (cpu_glob_cycles>=0x00700000) {		
				APU_EXECUTE2 ();
			}
		}
		
    if (CPU.Flags) {
	  	if (CPU.Flags & NMI_FLAG) {
	      if (--CPU.NMICycleCount == 0) {
		  		CPU.Flags &= ~NMI_FLAG;
		  		if (CPU.WaitingForInterrupt) {
		      	CPU.WaitingForInterrupt = FALSE;
		      	CPU.PC++;
		    	}
		  		S9xOpcode_NMI ();
				}
	    }

	  	if (CPU.Flags & IRQ_PENDING_FLAG) {
	      if (CPU.IRQCycleCount == 0) {
		  		if (CPU.WaitingForInterrupt) {
		      	CPU.WaitingForInterrupt = FALSE;
		      	CPU.PC++;
		    	}
		  		if (CPU.IRQActive && !Settings.DisableIRQ) {
		      	if (!CheckFlag (IRQ)) S9xOpcode_IRQ ();
		    	} else CPU.Flags &= ~IRQ_PENDING_FLAG;
				} else CPU.IRQCycleCount--;
	    }
	  	if (CPU.Flags & SCAN_KEYS_FLAG) break;
		}

		#ifdef CPU_SHUTDOWN
    CPU.PCAtOpcodeStart = CPU.PC;
		#endif
    CPU.Cycles += CPU.MemSpeed;

    (*ICPU.S9xOpcodes[*CPU.PC++].S9xOpcode) ();

    DO_HBLANK_CHECK ();
  } 
  
}


void S9xMainLoop_SA1_NoAPU (void) {
	for (;;) {		
    if (CPU.Flags) {
	  	if (CPU.Flags & NMI_FLAG) {
	      if (--CPU.NMICycleCount == 0) {
		  		CPU.Flags &= ~NMI_FLAG;
		  		if (CPU.WaitingForInterrupt) {
		      	CPU.WaitingForInterrupt = FALSE;
		      	CPU.PC++;
		    	}
		  		S9xOpcode_NMI ();
				}
	    }

	  	if (CPU.Flags & IRQ_PENDING_FLAG) {
	      if (CPU.IRQCycleCount == 0) {
		  		if (CPU.WaitingForInterrupt) {
		      	CPU.WaitingForInterrupt = FALSE;
		      	CPU.PC++;
		    	}
		  		if (CPU.IRQActive && !Settings.DisableIRQ) {
		      	if (!CheckFlag (IRQ)) S9xOpcode_IRQ ();
		    	} else CPU.Flags &= ~IRQ_PENDING_FLAG;
				} else CPU.IRQCycleCount--;
	    }
	  	if (CPU.Flags & SCAN_KEYS_FLAG) break;
		}

		#ifdef CPU_SHUTDOWN
    CPU.PCAtOpcodeStart = CPU.PC;
		#endif
    CPU.Cycles += CPU.MemSpeed;

    (*ICPU.S9xOpcodes[*CPU.PC++].S9xOpcode) ();
    if (SA1.Executing) S9xSA1MainLoop ();
      
    DO_HBLANK_CHECK ();
  }
}

void S9xMainLoop_NoSA1_NoAPU (void) {
	for (;;) {
    if (CPU.Flags) {
	  	if (CPU.Flags & NMI_FLAG) {
	      if (--CPU.NMICycleCount == 0) {
		  		CPU.Flags &= ~NMI_FLAG;
		  		if (CPU.WaitingForInterrupt) {
		      	CPU.WaitingForInterrupt = FALSE;
		      	CPU.PC++;
		    	}
		  		S9xOpcode_NMI ();
				}
	    }

	  	if (CPU.Flags & IRQ_PENDING_FLAG) {
	      if (CPU.IRQCycleCount == 0) {
		  		if (CPU.WaitingForInterrupt) {
		      	CPU.WaitingForInterrupt = FALSE;
		      	CPU.PC++;
		    	}
		  		if (CPU.IRQActive && !Settings.DisableIRQ) {
		      	if (!CheckFlag (IRQ)) S9xOpcode_IRQ ();
		    	} else CPU.Flags &= ~IRQ_PENDING_FLAG;
				} else CPU.IRQCycleCount--;
	    }
	  	if (CPU.Flags & SCAN_KEYS_FLAG) break;
		}

		#ifdef CPU_SHUTDOWN
    CPU.PCAtOpcodeStart = CPU.PC;
		#endif
    CPU.Cycles += CPU.MemSpeed;

    (*ICPU.S9xOpcodes[*CPU.PC++].S9xOpcode) ();
      
    DO_HBLANK_CHECK ();
  }
  return;
  
}

void S9xMainLoop (void)
{
	START_PROFILE_FUNC (S9xMainLoop);
	
	if (Settings.APUEnabled) {
		if (Settings.SA1) S9xMainLoop_SA1_APU();
		else {
			S9xMainLoop_NoSA1_APU();
		}				
	} else {
		if (Settings.SA1) S9xMainLoop_SA1_NoAPU();
		else S9xMainLoop_NoSA1_NoAPU();
	}
	
#ifndef ME_SOUND	
	if (cpu_glob_cycles>=0x00000000) {		
			APU_EXECUTE2 ();
	}		
#endif	
	
  Registers.PC = CPU.PC - CPU.PCBase;
    
  S9xPackStatus ();
      
  if (CPU.Flags & SCAN_KEYS_FLAG) {
    FINISH_PROFILE_FUNC (S9xMainLoop);
    S9xSyncSpeed ();
    CPU.Flags &= ~SCAN_KEYS_FLAG;
  }
  if (CPU.BRKTriggered && Settings.SuperFX && !CPU.TriedInterleavedMode2) {
    CPU.TriedInterleavedMode2 = TRUE;
    CPU.BRKTriggered = FALSE;
    S9xDeinterleaveMode2 ();
  }

  /*(APURegistersUncached->PC) = (IAPUuncached->PC) - (IAPUuncached->RAM);
  S9xAPUPackStatusUncached ();*/
}

void
S9xSetIRQ (uint32 source)
{
  CPU.IRQActive |= source;
  CPU.Flags |= IRQ_PENDING_FLAG;
  CPU.IRQCycleCount = 3;
  if (CPU.WaitingForInterrupt)
    {
      // Force IRQ to trigger immediately after WAI - 
      // Final Fantasy Mystic Quest crashes without this.
      CPU.IRQCycleCount = 0;
      CPU.WaitingForInterrupt = FALSE;
      CPU.PC++;
    }
}

void
S9xClearIRQ (uint32 source)
{
  CLEAR_IRQ_SOURCE (source);
}

void
S9xDoHBlankProcessing_HBLANK_START_EVENT ()
{
	//START_PROFILE_FUNC (S9xDoHBlankProcessing);
#ifdef CPU_SHUTDOWN
  CPU.WaitCounter++;
#endif

  if (IPPU.HDMA && CPU.V_Counter <= PPU.ScreenHeight) IPPU.HDMA = S9xDoHDMA (IPPU.HDMA);
  S9xReschedule ();
  //FINISH_PROFILE_FUNC (S9xDoHBlankProcessing); 
}

void
S9xDoHBlankProcessing_HBLANK_END_EVENT () {
	//START_PROFILE_FUNC (S9xDoHBlankProcessing);			
#ifdef CPU_SHUTDOWN
  CPU.WaitCounter++;
#endif
  if (Settings.SuperFX) S9xSuperFXExec ();
  	  

	cpu_glob_cycles += CPU.Cycles-old_cpu_cycles;		
	CPU.Cycles -= Settings.H_Max;	
	old_cpu_cycles=CPU.Cycles;
	
			
  //(IAPUuncached->NextAPUTimerPos) -= (Settings.H_Max * 10000L);      
  if (  (IAPUuncached->APUExecuting)) {
  	//(APUuncached->Cycles) -= Settings.H_Max;
		*apu_glob_cycles=cpu_glob_cycles;
#ifdef ME_SOUND		
		if (cpu_glob_cycles>=0x00700000) {		
			APU_EXECUTE2 ();
		}
#else
//		if (cpu_glob_cycles>=0x00000000) {		
//			APU_EXECUTE2 ();
//		}
#endif		
  }
  else {
  	//(APUuncached->Cycles) = 0;
  	*apu_glob_cycles=0;
  	APUuncached->Cycles = 0;
  }
  
        
  CPU.NextEvent = -1;
  ICPU.Scanline++;

  if (++CPU.V_Counter > (Settings.PAL ? SNES_MAX_PAL_VCOUNTER : SNES_MAX_NTSC_VCOUNTER)) {
  	//PPU.OAMAddr = PPU.SavedOAMAddr;
    //PPU.OAMFlip = 0;            
    CPU.V_Counter = 0;
    FillRAM[0x213F]^=0x80;
    CPU.NMIActive = FALSE;
    ICPU.Frame++;
    PPU.HVBeamCounterLatched = 0;
    CPU.Flags |= SCAN_KEYS_FLAG;
    S9xStartHDMA ();
  }

  if (PPU.VTimerEnabled && !PPU.HTimerEnabled && CPU.V_Counter == PPU.IRQVBeamPos) S9xSetIRQ (PPU_V_BEAM_IRQ_SOURCE);
    
  apu_event1[(*apu_event1_cpt2)&(65536*2-1)]=cpu_glob_cycles * os9x_apu_ratio / 256;
  (*apu_event1_cpt2)++;
  
  //APU_EXECUTE2 ();
    
  /*if ((APUuncached->TimerEnabled) [2]) {
		(APUuncached->Timer) [2] += 4;
		while ((APUuncached->Timer) [2] >= (APUuncached->TimerTarget) [2]) {
		  (IAPUuncached->RAM) [0xff] = ((IAPUuncached->RAM) [0xff] + 1) & 0xf;
		  (APUuncached->Timer) [2] -= (APUuncached->TimerTarget) [2];
#ifdef SPC700_SHUTDOWN
		  (IAPUuncached->WaitCounter)++;
		  (IAPUuncached->APUExecuting)= TRUE;
#endif		
		}
	}*/
	if (CPU.V_Counter & 1) {		
		apu_event2[(*apu_event2_cpt2)&(65536*2-1)]=cpu_glob_cycles * os9x_apu_ratio / 256;  
  	(*apu_event2_cpt2)++;
		/*if ((APUuncached->TimerEnabled) [0]) {
		  (APUuncached->Timer) [0]++;
		  if ((APUuncached->Timer) [0] >= (APUuncached->TimerTarget) [0]) {
				(IAPUuncached->RAM) [0xfd] = ((IAPUuncached->RAM) [0xfd] + 1) & 0xf;
				(APUuncached->Timer) [0] = 0;
#ifdef SPC700_SHUTDOWN		
				(IAPUuncached->WaitCounter)++;
				(IAPUuncached->APUExecuting)= TRUE;
#endif		    
		  }
		}
		if ((APUuncached->TimerEnabled) [1]) {
		  (APUuncached->Timer) [1]++;
		  if ((APUuncached->Timer) [1] >= (APUuncached->TimerTarget) [1]) {
				(IAPUuncached->RAM) [0xfe] = ((IAPUuncached->RAM) [0xfe] + 1) & 0xf;
				(APUuncached->Timer) [1] = 0;
#ifdef SPC700_SHUTDOWN		
				(IAPUuncached->WaitCounter)++;
				(IAPUuncached->APUExecuting) = TRUE;
#endif		    
		  }
		}*/		
	}	  
	
  if (CPU.V_Counter == FIRST_VISIBLE_LINE)
    {
      FillRAM[0x4210] = 0;
      CPU.Flags &= ~NMI_FLAG;
      S9xStartScreenRefresh ();
    }
  if (CPU.V_Counter >= FIRST_VISIBLE_LINE &&
      CPU.V_Counter < PPU.ScreenHeight + FIRST_VISIBLE_LINE)
    {
      RenderLine (CPU.V_Counter - FIRST_VISIBLE_LINE);
      S9xReschedule ();                  
  		//FINISH_PROFILE_FUNC (S9xDoHBlankProcessing); 
  		return;
    }

  if (CPU.V_Counter == PPU.ScreenHeight + FIRST_VISIBLE_LINE)
    {
      // Start of V-blank
      S9xEndScreenRefresh ();
      //PPU.FirstSprite = 0;
      IPPU.HDMA = 0;
      // Bits 7 and 6 of $4212 are computed when read in S9xGetPPU.
      missing.dma_this_frame = 0;
      IPPU.MaxBrightness = PPU.Brightness;
      PPU.ForcedBlanking = (FillRAM[0x2100] >> 7) & 1;
      
      if(!PPU.ForcedBlanking){
				PPU.OAMAddr = PPU.SavedOAMAddr;			
				uint8 tmp = 0;
				if(PPU.OAMPriorityRotation)
					tmp = (PPU.OAMAddr&0xFE)>>1;
				if((PPU.OAMFlip&1) || PPU.FirstSprite!=tmp){
					PPU.FirstSprite=tmp;
					IPPU.OBJChanged=TRUE;
				}			
				PPU.OAMFlip = 0;
			}

      FillRAM[0x4210] = 0x80;
      if (FillRAM[0x4200] & 0x80) {
	  		CPU.NMIActive = TRUE;
	  		CPU.Flags |= NMI_FLAG;
	  		CPU.NMICycleCount = CPU.NMITriggerPoint;
			}

			S9xReschedule ();
  		//FINISH_PROFILE_FUNC (S9xDoHBlankProcessing); 
  		return;
    }

  if (CPU.V_Counter == PPU.ScreenHeight + 3)
    S9xUpdateJoypads ();

  
  S9xReschedule ();
  //FINISH_PROFILE_FUNC (S9xDoHBlankProcessing); 
}

void
S9xDoHBlankProcessing_HTIMER_BEFORE_EVENT ()
{
	//START_PROFILE_FUNC (S9xDoHBlankProcessing);
#ifdef CPU_SHUTDOWN
  CPU.WaitCounter++;
#endif
  if (PPU.HTimerEnabled && (!PPU.VTimerEnabled || CPU.V_Counter == PPU.IRQVBeamPos)){
    S9xSetIRQ (PPU_H_BEAM_IRQ_SOURCE);
  }
  S9xReschedule ();
  //FINISH_PROFILE_FUNC (S9xDoHBlankProcessing); 
}

void
S9xDoHBlankProcessing_HTIMER_AFTER_EVENT ()
{
	//START_PROFILE_FUNC (S9xDoHBlankProcessing);
#ifdef CPU_SHUTDOWN
  CPU.WaitCounter++;
#endif
  if (PPU.HTimerEnabled && (!PPU.VTimerEnabled || CPU.V_Counter == PPU.IRQVBeamPos)) {
    S9xSetIRQ (PPU_H_BEAM_IRQ_SOURCE);
  }
  S9xReschedule ();
  //FINISH_PROFILE_FUNC (S9xDoHBlankProcessing); 
}
