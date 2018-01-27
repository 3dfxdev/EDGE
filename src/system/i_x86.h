#ifndef X86_H
#define X86_H

#include "../../epi/types.h"

struct CPUInfo	// 92 bytes
{
	union
	{
		char VendorID[16];
		u32_t dwVendorID[4];
	};
	union
	{
		char CPUString[48];
		u32_t dwCPUString[12];
	};

	u8_t Stepping;
	u8_t Model;
	u8_t Family;
	u8_t Type;

	union
	{
		struct
		{
			u8_t BrandIndex;
			u8_t CLFlush;
			u8_t CPUCount;
			u8_t APICID;

			u32_t bSSE3:1;
			u32_t DontCare1:8;
			u32_t bSSSE3:1;
			u32_t DontCare1a:9;
			u32_t bSSE41:1;
			u32_t bSSE42:1;
			u32_t DontCare2a:11;

			u32_t bFPU:1;
			u32_t bVME:1;
			u32_t bDE:1;
			u32_t bPSE:1;
			u32_t bRDTSC:1;
			u32_t bMSR:1;
			u32_t bPAE:1;
			u32_t bMCE:1;
			u32_t bCX8:1;
			u32_t bAPIC:1;
			u32_t bReserved1:1;
			u32_t bSEP:1;
			u32_t bMTRR:1;
			u32_t bPGE:1;
			u32_t bMCA:1;
			u32_t bCMOV:1;
			u32_t bPAT:1;
			u32_t bPSE36:1;
			u32_t bPSN:1;
			u32_t bCFLUSH:1;
			u32_t bReserved2:1;
			u32_t bDS:1;
			u32_t bACPI:1;
			u32_t bMMX:1;
			u32_t bFXSR:1;
			u32_t bSSE:1;
			u32_t bSSE2:1;
			u32_t bSS:1;
			u32_t bHTT:1;
			u32_t bTM:1;
			u32_t bReserved3:1;
			u32_t bPBE:1;

			u32_t DontCare2:22;
			u32_t bMMXPlus:1;		// AMD's MMX extensions
			u32_t bMMXAgain:1;		// Just a copy of bMMX above
			u32_t DontCare3:6;
			u32_t b3DNowPlus:1;
			u32_t b3DNow:1;
		};
		u32_t FeatureFlags[4];
	};

	u8_t AMDStepping;
	u8_t AMDModel;
	u8_t AMDFamily;
	u8_t bIsAMD;

	union
	{
		struct
		{
			u8_t DataL1LineSize;
			u8_t DataL1LinesPerTag;
			u8_t DataL1Associativity;
			u8_t DataL1SizeKB;
		};
		u32_t AMD_DataL1Info;
	};
};


extern CPUInfo CPU;
struct PalEntry;

void CheckCPUID (CPUInfo *cpu);
void DumpCPUInfo (const CPUInfo *cpu);
void DoBlending_SSE2(const PalEntry *from, PalEntry *to, int count, int r, int g, int b, int a);

#endif

