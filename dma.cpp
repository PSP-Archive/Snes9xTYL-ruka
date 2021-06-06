/*
 * Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
 *
 * (c) Copyright 1996 - 2001 Gary Henderson (gary.henderson@ntlworld.com) and
 *                           Jerremy Koot (jkoot@snes9x.com)
 *
 * Super FX C emulator code 
 * (c) Copyright 1997 - 1999 Ivar (ivar@snes9x.com) and
 *                           Gary Henderson.
 * Super FX assembler emulator code (c) Copyright 1998 zsKnight and _Demo_.
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
#include "ppu.h"
#include "cpuexec.h"
#include "missing.h"
#include "dma.h"
#include "apu.h"
#include "gfx.h"
#include "sa1.h"




#ifdef SDD1_DECOMP
#include "sdd1emu.h"
#endif

#ifdef SDD1_DECOMP
uint32 __yo_for_alignement;
uint8 *sdd1_buffer;//[0x10000];
#endif


extern int HDMA_ModeByteCounts [8];
extern uint8 *HDMAMemPointers [8];
extern uint8 *HDMABasePointers [8];

// #define SETA010_HDMA_FROM_CART

#ifdef SETA010_HDMA_FROM_CART
uint32 HDMARawPointers[8];	// Cart address space pointer
#endif

#if defined(__linux__) || defined(__WIN32__) || defined(__MACOSX__)
static int S9xCompareSDD1IndexEntries (const void *p1, const void *p2)
{
    return (*(uint32 *) p1 - *(uint32 *) p2);
}
#endif

/**********************************************************************************************/
/* S9xDoDMA()                                                                                   */
/* This function preforms the general dma transfer                                            */
/**********************************************************************************************/

void S9xDoDMA (uint8 Channel)
{
    uint8 Work;
	
    if (/*Channel > 7 || */CPUPack.CPU.InDMA)
		return;
	
    CPUPack.CPU.InDMA = TRUE;
    bool8 in_sa1_dma = FALSE;
    uint8 *in_sdd1_dma = NULL;
	uint8 *spc7110_dma=NULL;
	bool s7_wrap=false;
    SDMA *d = &DMA[Channel];
	

    int count = d->TransferBytes;
	
    if (count == 0)
		count = 0x10000;
	
    int inc = d->AAddressFixed ? 0 : (!d->AAddressDecrement ? 1 : -1);

	if((d->ABank==0x7E||d->ABank==0x7F)&&d->BAddress==0x80)
	{
		d->AAddress+= d->TransferBytes;
		//does an invalid DMA actually take time?
		// I'd say yes, since 'invalid' is probably just the WRAM chip
		// not being able to read and write itself at the same time
		CPUPack.CPU.Cycles+=(d->TransferBytes+1)*SLOW_ONE_CYCLE;
//		S9xUpdateAPUTimer();
		goto update_address;
	}
    switch (d->BAddress)
    {
    case 0x18:
    case 0x19:
		if (IPPU.RenderThisFrame)
			FLUSH_REDRAW ();
		break;
    }
    if (Settings.SDD1)
    {
		if (d->AAddressFixed && FillRAM [0x4801] > 0)
		{
			// Hacky support for pre-decompressed S-DD1 data
			inc = !d->AAddressDecrement ? 1 : -1;
			uint32 address = (((d->ABank << 16) | d->AAddress) & 0xfffff) << 4;
			
			address |= FillRAM [0x4804 + ((d->ABank - 0xc0) >> 4)];

#ifdef SDD1_DECOMP
			if(Settings.SDD1Pack)
			{
				uint8* in_ptr=GetBasePointer(((d->ABank << 16) | d->AAddress));
				in_ptr+=d->AAddress;

				SDD1_decompress(sdd1_buffer,in_ptr,d->TransferBytes);
				in_sdd1_dma=sdd1_buffer;
#ifdef SDD1_VERIFY
				void *ptr = bsearch (&address, Memory.SDD1Index, 
					Memory.SDD1Entries, 12, S9xCompareSDD1IndexEntries);
				if(memcmp(sdd1_buffer, ptr, d->TransferBytes))
				{
					uint8 *p = Memory.SDD1LoggedData;
					bool8 found = FALSE;
					uint8 SDD1Bank = FillRAM [0x4804 + ((d->ABank - 0xc0) >> 4)] | 0xf0;
					
					for (uint32 i = 0; i < Memory.SDD1LoggedDataCount; i++, p += 8)
					{
						if (*p == d->ABank ||
							*(p + 1) == (d->AAddress >> 8) &&
							*(p + 2) == (d->AAddress & 0xff) &&
							*(p + 3) == (count >> 8) &&
							*(p + 4) == (count & 0xff) &&
							*(p + 7) == SDD1Bank)
						{
							found = TRUE;
						}
					}
					if (!found && Memory.SDD1LoggedDataCount < MEMMAP_MAX_SDD1_LOGGED_ENTRIES)
					{
						int j=0;
						while(ptr[j]==sdd1_buffer[j])
							j++;
	
						*p = d->ABank;
						*(p + 1) = d->AAddress >> 8;
						*(p + 2) = d->AAddress & 0xff;
						*(p + 3) = j&0xFF;
						*(p + 4) = (j>>8)&0xFF;
						*(p + 7) = SDD1Bank;
						Memory.SDD1LoggedDataCount += 1;
					}
				}
#endif
			}

			else
			{
#endif
#if defined(__linux__) || defined (__WIN32__) || defined(__MACOSX__)
			void *ptr = bsearch (&address, Memory.SDD1Index, 
				Memory.SDD1Entries, 12, S9xCompareSDD1IndexEntries);
			if (ptr)
				in_sdd1_dma = *(uint32 *) ((uint8 *) ptr + 4) + Memory.SDD1Data;
#else
			uint8 *ptr = Memory.SDD1Index;
			
			for (uint32 e = 0; e < Memory.SDD1Entries; e++, ptr += 12)
			{
				if (address == *(uint32 *) ptr)
				{
					in_sdd1_dma = *(uint32 *) (ptr + 4) + Memory.SDD1Data;
					break;
				}
			}
#endif
			
			if (!in_sdd1_dma)
			{
				// No matching decompressed data found. Must be some new 
				// graphics not encountered before. Log it if it hasn't been
				// already.
				uint8 *p = Memory.SDD1LoggedData;
				bool8 found = FALSE;
				uint8 SDD1Bank = FillRAM [0x4804 + ((d->ABank - 0xc0) >> 4)] | 0xf0;
				
				for (uint32 i = 0; i < Memory.SDD1LoggedDataCount; i++, p += 8)
				{
					if (*p == d->ABank ||
						*(p + 1) == (d->AAddress >> 8) &&
						*(p + 2) == (d->AAddress & 0xff) &&
						*(p + 3) == (count >> 8) &&
						*(p + 4) == (count & 0xff) &&
						*(p + 7) == SDD1Bank)
					{
						found = TRUE;
						break;
					}
				}
				if (!found && Memory.SDD1LoggedDataCount < MEMMAP_MAX_SDD1_LOGGED_ENTRIES)
				{
					*p = d->ABank;
					*(p + 1) = d->AAddress >> 8;
					*(p + 2) = d->AAddress & 0xff;
					*(p + 3) = count >> 8;
					*(p + 4) = count & 0xff;
					*(p + 7) = SDD1Bank;
					Memory.SDD1LoggedDataCount += 1;
				}
			}
		}
#ifdef SDD1_DECOMP
		}
#endif

		FillRAM [0x4801] = 0;
    }
    if (d->BAddress == 0x18 && SA1Pack.SA1.in_char_dma && (d->ABank & 0xf0) == 0x40)
    {
		// Perform packed bitmap to PPU character format conversion on the
		// data before transmitting it to V-RAM via-DMA.
		int num_chars = 1 << ((FillRAM [0x2231] >> 2) & 7);
		int depth = (FillRAM [0x2231] & 3) == 0 ? 8 :
		(FillRAM [0x2231] & 3) == 1 ? 4 : 2;
		
		int bytes_per_char = 8 * depth;
		int bytes_per_line = depth * num_chars;
		int char_line_bytes = bytes_per_char * num_chars;
		uint32 addr = (d->AAddress / char_line_bytes) * char_line_bytes;
		uint8 *base = GetBasePointer ((d->ABank << 16) + addr) + addr;
		uint8 *buffer = &ROM [CMemory::MAX_ROM_SIZE - 0x10000];
		uint8 *p = buffer;
		uint32 inc = char_line_bytes - (d->AAddress % char_line_bytes);
		uint32 char_count = inc / bytes_per_char;
		
		in_sa1_dma = TRUE;
		
		//printf ("%08x,", base); fflush (stdout);
		//printf ("depth = %d, count = %d, bytes_per_char = %d, bytes_per_line = %d, num_chars = %d, char_line_bytes = %d\n",
		//depth, count, bytes_per_char, bytes_per_line, num_chars, char_line_bytes);
		int i;
		
		switch (depth)
		{
		case 2:
			for (i = 0; i < count; i += inc, base += char_line_bytes, 
				inc = char_line_bytes, char_count = num_chars)
			{
				uint8 *line = base + (num_chars - char_count) * 2;
				for (uint32 j = 0; j < char_count && p - buffer < count; 
				j++, line += 2)
				{
					uint8 *q = line;
					for (int l = 0; l < 8; l++, q += bytes_per_line)
					{
						for (int b = 0; b < 2; b++)
						{
							uint8 r = *(q + b);
							*(p + 0) = (*(p + 0) << 1) | ((r >> 0) & 1);
							*(p + 1) = (*(p + 1) << 1) | ((r >> 1) & 1);
							*(p + 0) = (*(p + 0) << 1) | ((r >> 2) & 1);
							*(p + 1) = (*(p + 1) << 1) | ((r >> 3) & 1);
							*(p + 0) = (*(p + 0) << 1) | ((r >> 4) & 1);
							*(p + 1) = (*(p + 1) << 1) | ((r >> 5) & 1);
							*(p + 0) = (*(p + 0) << 1) | ((r >> 6) & 1);
							*(p + 1) = (*(p + 1) << 1) | ((r >> 7) & 1);
						}
						p += 2;
					}
				}
			}
			break;
		case 4:
			for (i = 0; i < count; i += inc, base += char_line_bytes, 
				inc = char_line_bytes, char_count = num_chars)
			{
				uint8 *line = base + (num_chars - char_count) * 4;
				for (uint32 j = 0; j < char_count && p - buffer < count; 
				j++, line += 4)
				{
					uint8 *q = line;
					for (int l = 0; l < 8; l++, q += bytes_per_line)
					{
						for (int b = 0; b < 4; b++)
						{
							uint8 r = *(q + b);
							*(p +  0) = (*(p +  0) << 1) | ((r >> 0) & 1);
							*(p +  1) = (*(p +  1) << 1) | ((r >> 1) & 1);
							*(p + 16) = (*(p + 16) << 1) | ((r >> 2) & 1);
							*(p + 17) = (*(p + 17) << 1) | ((r >> 3) & 1);
							*(p +  0) = (*(p +  0) << 1) | ((r >> 4) & 1);
							*(p +  1) = (*(p +  1) << 1) | ((r >> 5) & 1);
							*(p + 16) = (*(p + 16) << 1) | ((r >> 6) & 1);
							*(p + 17) = (*(p + 17) << 1) | ((r >> 7) & 1);
						}
						p += 2;
					}
					p += 32 - 16;
				}
			}
			break;
		case 8:
			for (i = 0; i < count; i += inc, base += char_line_bytes, 
				inc = char_line_bytes, char_count = num_chars)
			{
				uint8 *line = base + (num_chars - char_count) * 8;
				for (uint32 j = 0; j < char_count && p - buffer < count; 
				j++, line += 8)
				{
					uint8 *q = line;
					for (int l = 0; l < 8; l++, q += bytes_per_line)
					{
						for (int b = 0; b < 8; b++)
						{
							uint8 r = *(q + b);
							*(p +  0) = (*(p +  0) << 1) | ((r >> 0) & 1);
							*(p +  1) = (*(p +  1) << 1) | ((r >> 1) & 1);
							*(p + 16) = (*(p + 16) << 1) | ((r >> 2) & 1);
							*(p + 17) = (*(p + 17) << 1) | ((r >> 3) & 1);
							*(p + 32) = (*(p + 32) << 1) | ((r >> 4) & 1);
							*(p + 33) = (*(p + 33) << 1) | ((r >> 5) & 1);
							*(p + 48) = (*(p + 48) << 1) | ((r >> 6) & 1);
							*(p + 49) = (*(p + 49) << 1) | ((r >> 7) & 1);
						}
						p += 2;
					}
					p += 64 - 16;
				}
			}
			break;
		}
    }
	
    if (!d->TransferDirection)
    {
		/* XXX: DMA is potentially broken here for cases where we DMA across
		 * XXX: memmap boundries. A possible solution would be to re-call
		 * XXX: GetBasePointer whenever we cross a boundry, and when
		 * XXX: GetBasePointer returns (0) to take the 'slow path' and use
		 * XXX: S9xGetByte instead of *base. GetBasePointer() would want to
		 * XXX: return (0) for MAP_PPU and whatever else is a register range
		 * XXX: rather than a RAM/ROM block, and we'd want to detect MAP_PPU
		 * XXX: (or specifically, Address Bus B addresses $2100-$21FF in
		 * XXX: banks $00-$3F) specially and treat it as MAP_NONE (since
		 * XXX: PPU->PPU transfers don't work).
		 */

		//reflects extra cycle used by DMA
		CPUPack.CPU.Cycles += SLOW_ONE_CYCLE * (count+1);
//		S9xUpdateAPUTimer();

		uint8 *readptr = GetBasePointer ((d->ABank << 16) + d->AAddress) + d->AAddress;
		
		if (!readptr) {
			readptr = ROM + d->AAddress;
		}
		
		if (in_sa1_dma)
		{
			readptr = &ROM [CMemory::MAX_ROM_SIZE - 0x10000];
		}
		
		if (in_sdd1_dma)
		{
			readptr = in_sdd1_dma;
		}
		if(spc7110_dma)
		{
			readptr=spc7110_dma;
		}
		if (inc > 0)
			d->AAddress += count;
		else
			if (inc < 0)
				d->AAddress -= count;
			
			if (d->TransferMode == 0 || d->TransferMode == 2 || d->TransferMode == 6)
			{
				switch (d->BAddress)
				{
				case 0x04:
					do
					{
						REGISTER_2104(*readptr);
						readptr += inc;
						CHECK_SOUND();
					} while (--count > 0);
					break;
				case 0x18:
#ifndef CORRECT_VRAM_READS
					IPPU.FirstVRAMRead = TRUE;
#endif
					if (!PPU.VMA.FullGraphicCount)
					{
						do
						{
							REGISTER_2118_linear(*readptr);
							readptr += inc;
							CHECK_SOUND();
						} while (--count > 0);
					}
					else
					{
						do
						{
							REGISTER_2118_tile(*readptr);
							readptr += inc;
							CHECK_SOUND();
						} while (--count > 0);
					}
					break;
				case 0x19:
#ifndef CORRECT_VRAM_READS
					IPPU.FirstVRAMRead = TRUE;
#endif
					if (!PPU.VMA.FullGraphicCount)
					{
						do
						{
							REGISTER_2119_linear(*readptr);
							readptr += inc;
							CHECK_SOUND();
						} while (--count > 0);
					}
					else
					{
						do
						{
							REGISTER_2119_tile(*readptr);
							readptr += inc;
							CHECK_SOUND();
						} while (--count > 0);
					}
					break;
				case 0x22:
					do
					{
						REGISTER_2122(*readptr);
						readptr += inc;
						CHECK_SOUND();
					} while (--count > 0);
					break;
				case 0x80:
					do
					{
						REGISTER_2180(*readptr);
						readptr += inc;
						CHECK_SOUND();
					} while (--count > 0);
					break;
				default:
					do
					{
						S9xSetPPU (*readptr, 0x2100 + d->BAddress);
						readptr += inc;
						CHECK_SOUND();
					} while (--count > 0);
					break;
				}
			}
			else
				if (d->TransferMode == 1 || d->TransferMode == 5)
				{
					if (d->BAddress == 0x18)
					{
						// Write to V-RAM
#ifndef CORRECT_VRAM_READS
						IPPU.FirstVRAMRead = TRUE;
#endif
						if (!PPU.VMA.FullGraphicCount)
						{
							while (count > 1)
							{
								REGISTER_2118_linear(*readptr);
								readptr += inc;
								
								REGISTER_2119_linear(*readptr);
								readptr += inc;
								CHECK_SOUND();
								count -= 2;
							}
							if (count == 1)
							{
								REGISTER_2118_linear(*readptr);
								readptr += inc;
							}
						}
						else
						{
							while (count > 1)
							{
								REGISTER_2118_tile(*readptr);
								readptr += inc;
								
								REGISTER_2119_tile(*readptr);
								readptr += inc;
								CHECK_SOUND();
								count -= 2;
							}
							if (count == 1)
							{
								REGISTER_2118_tile(*readptr);
								readptr += inc;
							}
						}
					}
					else
					{
						// DMA mode 1 general case
						while (count > 1)
						{
							S9xSetPPU (*readptr, 0x2100 + d->BAddress);
							readptr += inc;
							
							S9xSetPPU (*readptr, 0x2101 + d->BAddress);
							readptr += inc;
							CHECK_SOUND();
							count -= 2;
						}
						if (count == 1)
						{
							S9xSetPPU (*readptr, 0x2100 + d->BAddress);
							readptr += inc;
						}
					}
				}
				else
					if (d->TransferMode == 3 || d->TransferMode == 7)
					{
						do
						{
							S9xSetPPU (*readptr, 0x2100 + d->BAddress);
							readptr += inc;
							if (count <= 1)
								break;
							
							S9xSetPPU (*readptr, 0x2100 + d->BAddress);
							readptr += inc;
							if (count <= 2)
								break;
							
							S9xSetPPU (*readptr, 0x2101 + d->BAddress);
							readptr += inc;
							if (count <= 3)
								break;
							
							S9xSetPPU (*readptr, 0x2101 + d->BAddress);
							readptr += inc;
							CHECK_SOUND();
							count -= 4;
						} while (count > 0);
					}
					else
						if (d->TransferMode == 4)
						{
							do
							{
								S9xSetPPU (*readptr, 0x2100 + d->BAddress);
								readptr += inc;
								if (count <= 1)
									break;
								
								S9xSetPPU (*readptr, 0x2101 + d->BAddress);
								readptr += inc;
								if (count <= 2)
									break;
								
								S9xSetPPU (*readptr, 0x2102 + d->BAddress);
								readptr += inc;
								if (count <= 3)
									break;
								
								S9xSetPPU (*readptr, 0x2103 + d->BAddress);
								readptr += inc;
								CHECK_SOUND();
								count -= 4;
							} while (count > 0);
						}
						else
						{
#ifdef DEBUGGER
							//	    if (Settings.TraceDMA)
							{
								sprintf (String, "Unknown DMA transfer mode: %d on channel %d\n",
									d->TransferMode, Channel);
								S9xMessage (S9X_TRACE, S9X_DMA_TRACE, String);
							}
#endif
						}
    }
    else
    {
		/* XXX: DMA is potentially broken here for cases where the dest is
		 * XXX: in the Address Bus B range. Note that this bad dest may not
		 * XXX: cover the whole range of the DMA though, if we transfer
		 * XXX: 65536 bytes only 256 of them may be Address Bus B.
		 */
		do
		{
			switch (d->TransferMode)
			{
			case 0:
			case 2:
			case 6:
				Work = S9xGetPPU (0x2100 + d->BAddress);
				S9xSetByte (Work, (d->ABank << 16) + d->AAddress);
				d->AAddress += inc;
				--count;
				break;
				
			case 1:
			case 5:
				Work = S9xGetPPU (0x2100 + d->BAddress);
				S9xSetByte (Work, (d->ABank << 16) + d->AAddress);
				d->AAddress += inc;
				if (!--count)
					break;
				
				Work = S9xGetPPU (0x2101 + d->BAddress);
				S9xSetByte (Work, (d->ABank << 16) + d->AAddress);
				d->AAddress += inc;
				count--;
				break;
				
			case 3:
			case 7:
				Work = S9xGetPPU (0x2100 + d->BAddress);
				S9xSetByte (Work, (d->ABank << 16) + d->AAddress);
				d->AAddress += inc;
				if (!--count)
					break;
				
				Work = S9xGetPPU (0x2100 + d->BAddress);
				S9xSetByte (Work, (d->ABank << 16) + d->AAddress);
				d->AAddress += inc;
				if (!--count)
					break;
				
				Work = S9xGetPPU (0x2101 + d->BAddress);
				S9xSetByte (Work, (d->ABank << 16) + d->AAddress);
				d->AAddress += inc;
				if (!--count)
					break;
				
				Work = S9xGetPPU (0x2101 + d->BAddress);
				S9xSetByte (Work, (d->ABank << 16) + d->AAddress);
				d->AAddress += inc;
				count--;
				break;
				
			case 4:
				Work = S9xGetPPU (0x2100 + d->BAddress);
				S9xSetByte (Work, (d->ABank << 16) + d->AAddress);
				d->AAddress += inc;
				if (!--count)
					break;
				
				Work = S9xGetPPU (0x2101 + d->BAddress);
				S9xSetByte (Work, (d->ABank << 16) + d->AAddress);
				d->AAddress += inc;
				if (!--count)
					break;
				
				Work = S9xGetPPU (0x2102 + d->BAddress);
				S9xSetByte (Work, (d->ABank << 16) + d->AAddress);
				d->AAddress += inc;
				if (!--count)
					break;
				
				Work = S9xGetPPU (0x2103 + d->BAddress);
				S9xSetByte (Work, (d->ABank << 16) + d->AAddress);
				d->AAddress += inc;
				count--;
				break;
				
			default:
#ifdef DEBUGGER
				if (1) //Settings.TraceDMA)
				{
					sprintf (String, "Unknown DMA transfer mode: %d on channel %d\n",
						d->TransferMode, Channel);
					S9xMessage (S9X_TRACE, S9X_DMA_TRACE, String);
				}
#endif
				count = 0;
				break;
			}
			CHECK_SOUND();
		} while (count);
    }
    
#ifdef SPC700_C

    SAPUEVENTS *pEvent = (SAPUEVENTS *)UNCACHE_PTR(&stAPUEvents);
    pEvent->IAPU_APUExecuting = Settings.APUEnabled;    
    if (Settings.APUEnabled) {
    	/*if (CPUPack.CPU.Cycles-old_cpu_cycles<0) msgBoxLines("2",60);
			else */
		cpu_glob_cycles += CPUPack.CPU.Cycles-old_cpu_cycles;
		old_cpu_cycles=CPUPack.CPU.Cycles;
		pEvent->apu_glob_cycles=cpu_glob_cycles;
		if (cpu_glob_cycles>=0x00700000) {
				APU_EXECUTE2 ();
		}
    }
    //APU_EXECUTE ();
#endif
    while (CPUPack.CPU.Cycles > CPUPack.CPU.NextEvent) S9xDoHBlankProcessing ();
//	S9xUpdateAPUTimer();

/*	if(Settings.SPC7110&&spc7110_dma)
	{
		if(spc7110_dma&&s7_wrap) {
#ifdef PSP
		        free (spc7110_dma);
#else			
			delete [] spc7110_dma;
#endif
		}
	}*/

update_address:
    // Super Punch-Out requires that the A-BUS address be updated after the
    // DMA transfer.
    *((uint16*)(FillRAM + 0x4302 + (Channel << 4))) = d->AAddress;
	
    // Secret of the Mana requires that the DMA bytes transfer count be set to
    // zero when DMA has completed.
    FillRAM [0x4305 + (Channel << 4)] = 0;
    FillRAM [0x4306 + (Channel << 4)] = 0;
	
    DMA[Channel].IndirectAddress = 0;
    d->TransferBytes = 0;
    
    CPUPack.CPU.InDMA = FALSE;


}

void S9xStartHDMA () {
	if (Settings.DisableHDMA)	IPPU.HDMA = 0;
  else IPPU.HDMA = FillRAM [0x420c];
//		missing.hdma_this_frame = IPPU.HDMA = FillRAM [0x420c];
	
	//per anomie timing post
	if(IPPU.HDMA!=0) {
		CPUPack.CPU.Cycles+=ONE_CYCLE*3;
//		S9xUpdateAPUTimer();
	}
    
	IPPU.HDMAStarted = TRUE;

	for (uint8 i = 0; i < 8; i++) {
		if (IPPU.HDMA & (1 << i)) {
			CPUPack.CPU.Cycles+=SLOW_ONE_CYCLE ;
//			S9xUpdateAPUTimer();
			DMA [i].LineCount = 0;
			DMA [i].FirstLine = TRUE;
			DMA [i].Address = DMA [i].AAddress;
			if(DMA[i].HDMAIndirectAddressing) {
				CPUPack.CPU.Cycles+=(SLOW_ONE_CYCLE <<2);
//				S9xUpdateAPUTimer();
			}
		}
		HDMAMemPointers [i] = NULL;
#ifdef SETA010_HDMA_FROM_CART
		HDMARawPointers [i] = 0;
#endif
  }
}

#ifdef DEBUGGER
void S9xTraceSoundDSP (const char *s, int i1 = 0, int i2 = 0, int i3 = 0,
					   int i4 = 0, int i5 = 0, int i6 = 0, int i7 = 0);
#endif


uint8 S9xDoHDMA (uint8 byte) {
	struct SDMA *p = &DMA [0];    
	int d = 0;

	CPUPack.CPU.InDMA = TRUE;
	CPUPack.CPU.Cycles+=ONE_CYCLE*3;
//	S9xUpdateAPUTimer();
  for (uint8 mask = 1; mask; mask <<= 1, p++, d++) {
		if (byte & mask) {
			if (!p->LineCount) {
				//remember, InDMA is set.
				//Get/Set incur no charges!
				CPUPack.CPU.Cycles+=SLOW_ONE_CYCLE;
//				S9xUpdateAPUTimer();
				uint8 line = S9xGetByte ((p->ABank << 16) + p->Address);
				if (line == 0x80) {
					p->Repeat = TRUE;
					p->LineCount = 128;
				} else {
					p->Repeat = !(line & 0x80);
					p->LineCount = line & 0x7f;
				}

				// Disable H-DMA'ing into V-RAM (register 2118) for Hook
				/* XXX: instead of p->BAddress == 0x18, make S9xSetPPU fail
				 * XXX: writes to $2118/9 when appropriate
				 */
#ifdef SETA010_HDMA_FROM_CART
				if (!p->LineCount) {
#else
				if (!p->LineCount || p->BAddress == 0x18) {
#endif				
					byte &= ~mask;
					p->IndirectAddress += HDMAMemPointers [d] - HDMABasePointers [d];
					FillRAM [0x4305 + (d << 4)] = (uint8) p->IndirectAddress;
					FillRAM [0x4306 + (d << 4)] = p->IndirectAddress >> 8;
					continue;
				}

				p->Address++;
				p->FirstLine = 1;
				if (p->HDMAIndirectAddressing) {
					p->IndirectBank = FillRAM [0x4307 + (d << 4)];
					//again, no cycle charges while InDMA is set!
					CPUPack.CPU.Cycles+=SLOW_ONE_CYCLE<<2;
//					S9xUpdateAPUTimer();
					p->IndirectAddress = S9xGetWord ((p->ABank << 16) + p->Address);
					p->Address += 2;
				} else {
					p->IndirectBank = p->ABank;
					p->IndirectAddress = p->Address;
				}
				HDMABasePointers [d] = HDMAMemPointers [d] = S9xGetMemPointer ((p->IndirectBank << 16) + p->IndirectAddress);
#ifdef SETA010_HDMA_FROM_CART
				HDMARawPointers [d] = (p->IndirectBank << 16) + p->IndirectAddress;
#endif
			} else {
				CPUPack.CPU.Cycles += SLOW_ONE_CYCLE;
//				S9xUpdateAPUTimer();
			}
			if (!HDMAMemPointers [d]) {
				if (!p->HDMAIndirectAddressing) {
					p->IndirectBank = p->ABank;
					p->IndirectAddress = p->Address;
				}
#ifdef SETA010_HDMA_FROM_CART
				HDMARawPointers [d] = (p->IndirectBank << 16) + p->IndirectAddress;
#endif
				if (!(HDMABasePointers [d] = HDMAMemPointers [d] = S9xGetMemPointer ((p->IndirectBank << 16) + p->IndirectAddress))) {
					/* XXX: Instead of this, goto a slow path that first
					 * XXX: verifies src!=Address Bus B, then uses
					 * XXX: S9xGetByte(). Or make S9xGetByte return OpenBus
					 * XXX: (probably?) for Address Bus B while inDMA.
					 */
					byte &= ~mask;
					continue;
				}
				// Uncommenting the following line breaks Punchout - it starts
				// H-DMA during the frame.
				//p->FirstLine = TRUE;
			}
			if (p->Repeat && !p->FirstLine) {
				p->LineCount--;
				continue;
			}
			if (p->BAddress == 0x04) {
				if(SNESGameFixes.Uniracers) {
					PPU.OAMAddr = 0x10c;
					PPU.OAMFlip=0;
				}
			}
			switch (p->TransferMode) {
				case 0:
					CPUPack.CPU.Cycles += SLOW_ONE_CYCLE;
//					S9xUpdateAPUTimer();
#ifdef SETA010_HDMA_FROM_CART
					S9xSetPPU (S9xGetByte (HDMARawPointers [d]++), 0x2100 + p->BAddress);
					HDMAMemPointers [d]++;
#else
					S9xSetPPU (*HDMAMemPointers [d]++, 0x2100 + p->BAddress);
#endif
					break;
				case 5:
					CPUPack.CPU.Cycles += 2*SLOW_ONE_CYCLE;
//					S9xUpdateAPUTimer();
#ifdef SETA010_HDMA_FROM_CART
					S9xSetPPU (S9xGetByte (HDMARawPointers [d]), 0x2100 + p->BAddress);
					S9xSetPPU (S9xGetByte (HDMARawPointers [d] + 1), 0x2101 + p->BAddress);
					HDMARawPointers [d] += 2;
#else
					S9xSetPPU (*(HDMAMemPointers [d] + 0), 0x2100 + p->BAddress);
					S9xSetPPU (*(HDMAMemPointers [d] + 1), 0x2101 + p->BAddress);
#endif
					HDMAMemPointers [d] += 2;
					/* fall through */
				case 1:
					CPUPack.CPU.Cycles += 2*SLOW_ONE_CYCLE;
//					S9xUpdateAPUTimer();
#ifdef SETA010_HDMA_FROM_CART
					S9xSetPPU (S9xGetByte (HDMARawPointers [d]), 0x2100 + p->BAddress);
					S9xSetPPU (S9xGetByte (HDMARawPointers [d] + 1), 0x2101 + p->BAddress);
					HDMARawPointers [d] += 2;
#else
					S9xSetPPU (*(HDMAMemPointers [d] + 0), 0x2100 + p->BAddress);
					S9xSetPPU (*(HDMAMemPointers [d] + 1), 0x2101 + p->BAddress);
#endif
					HDMAMemPointers [d] += 2;
					break;
				case 2:
				case 6:
					CPUPack.CPU.Cycles += 2*SLOW_ONE_CYCLE;
//					S9xUpdateAPUTimer();
#ifdef SETA010_HDMA_FROM_CART
					S9xSetPPU (S9xGetByte (HDMARawPointers [d]), 0x2100 + p->BAddress);
					S9xSetPPU (S9xGetByte (HDMARawPointers [d] + 1), 0x2100 + p->BAddress);
					HDMARawPointers [d] += 2;
#else
					S9xSetPPU (*(HDMAMemPointers [d] + 0), 0x2100 + p->BAddress);
					S9xSetPPU (*(HDMAMemPointers [d] + 1), 0x2100 + p->BAddress);
#endif
					HDMAMemPointers [d] += 2;
					break;
				case 3:
				case 7:
					CPUPack.CPU.Cycles += 4*SLOW_ONE_CYCLE;
//					S9xUpdateAPUTimer();
#ifdef SETA010_HDMA_FROM_CART
					S9xSetPPU (S9xGetByte (HDMARawPointers [d]), 0x2100 + p->BAddress);
					S9xSetPPU (S9xGetByte (HDMARawPointers [d] + 1), 0x2100 + p->BAddress);
					S9xSetPPU (S9xGetByte (HDMARawPointers [d] + 2), 0x2101 + p->BAddress);
					S9xSetPPU (S9xGetByte (HDMARawPointers [d] + 3), 0x2101 + p->BAddress);
					HDMARawPointers [d] += 4;
#else
					S9xSetPPU (*(HDMAMemPointers [d] + 0), 0x2100 + p->BAddress);
					S9xSetPPU (*(HDMAMemPointers [d] + 1), 0x2100 + p->BAddress);
					S9xSetPPU (*(HDMAMemPointers [d] + 2), 0x2101 + p->BAddress);
					S9xSetPPU (*(HDMAMemPointers [d] + 3), 0x2101 + p->BAddress);
#endif
					HDMAMemPointers [d] += 4;
					break;
				case 4:
					CPUPack.CPU.Cycles += 4*SLOW_ONE_CYCLE;
//					S9xUpdateAPUTimer();
#ifdef SETA010_HDMA_FROM_CART
					S9xSetPPU (S9xGetByte (HDMARawPointers [d]), 0x2100 + p->BAddress);
					S9xSetPPU (S9xGetByte (HDMARawPointers [d] + 1), 0x2101 + p->BAddress);
					S9xSetPPU (S9xGetByte (HDMARawPointers [d] + 2), 0x2102 + p->BAddress);
					S9xSetPPU (S9xGetByte (HDMARawPointers [d] + 3), 0x2103 + p->BAddress);
					HDMARawPointers [d] += 4;
#else
					S9xSetPPU (*(HDMAMemPointers [d] + 0), 0x2100 + p->BAddress);
					S9xSetPPU (*(HDMAMemPointers [d] + 1), 0x2101 + p->BAddress);
					S9xSetPPU (*(HDMAMemPointers [d] + 2), 0x2102 + p->BAddress);
					S9xSetPPU (*(HDMAMemPointers [d] + 3), 0x2103 + p->BAddress);
#endif
					HDMAMemPointers [d] += 4;
					break;
			}
			if (!p->HDMAIndirectAddressing) p->Address += HDMA_ModeByteCounts [p->TransferMode];
			p->IndirectAddress += HDMA_ModeByteCounts [p->TransferMode];
			/* XXX: Check for p->IndirectAddress crossing a mapping boundry,
			 * XXX: and invalidate HDMAMemPointers[d]
			 */
			p->FirstLine = FALSE;
			p->LineCount--;
		}
	}
	CPUPack.CPU.InDMA=FALSE;
	return (byte);
}

void S9xResetDMA ()
{
    int d;
    for (d = 0; d < 8; d++)
    {
		DMA [d].TransferDirection = FALSE;
		DMA [d].HDMAIndirectAddressing = FALSE;
		DMA [d].AAddressFixed = TRUE;
		DMA [d].AAddressDecrement = FALSE;
		DMA [d].TransferMode = 0xff;
		DMA [d].ABank = 0xff;
		DMA [d].AAddress = 0xffff;
		DMA [d].Address = 0xffff;
		DMA [d].BAddress = 0xff;
		DMA [d].TransferBytes = 0xffff;
    }
    for (int c = 0x4300; c < 0x4380; c += 0x10)
    {
		for (d = c; d < c + 12; d++)
			FillRAM [d] = 0xff;
		
		FillRAM [c + 0xf] = 0xff;
    }
}
