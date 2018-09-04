#ifndef _PROFILE_GUARD
#define _PROFILE_GUARD

#ifdef __cplusplus
extern "C" {
#endif

#include <kos.h>
#include <time.h>

#define PM_CR_BASE	((volatile int16_t*)0xff000084)
#define PM_CTR_BASE	((volatile int32_t*)0xff100004)
#define PMCR(n)         (PM_CR_BASE[n*2])
#define PMCTRH(n)       (PM_CTR_BASE[n*2])
#define PMCTRL(n)       (PM_CTR_BASE[n*2+1])

#define PMCR_PMM_MASK   0x0000003f

#define PMCR_CLKF       0x00000100
#define PMCR_PMCLR      0x00002000
#define PMCR_PMST       0x00004000
#define PMCR_PMENABLE   0x00008000

typedef uint64 pfCounter;

typedef enum {
	PE_OC_READ_HIT = 0x1,
	PE_OC_WRITE_HIT = 0x2,
	PE_OC_ALL_HIT = 0xe, //?
	PE_OC_READ_MISS = 0x4,
	PE_OC_WRITE_MISS = 0x5,
	PE_OC_ALL_MISS = 0xf,
	PE_OC_ALL_ACCESS = 0x9, //?
	PE_OC_LINE_READ_CYCLES = 0x22,
	
	PE_OCRAM_ACCESS = 0xb,
	PE_CHIPIO_ACCESS = 0xd,
	
	PE_IC_READ_HIT = 0x6,
	PE_IC_READ_MISS = 0x8,
	PE_IC_ALL_ACCESS = 0xa,
	PE_IC_LINE_READ_CYCLES = 0x21,
	
	PE_UTLB_MISS = 0x3,
	PE_ITLB_MISS = 0x7,
	
	PE_BRANCH_COUNT = 0x10,
	PE_BRANCH_TAKEN = 0x11,
	PE_SUBROUTINE_CALLS = 0x12,
	
	PE_SINGLE_INSTRUCTIONS_EXECUTED = 0x13,
	PE_DUAL_INSTRUCTIONS_EXECUTED = 0x14,
	PE_FPU_INSTRUCTIONS_EXECUTED = 0x15,
	
	PE_IRQ_COUNT = 0x16,
	PE_NMI_COUNT = 0x17,
	PE_TRAPA_COUNT = 0x18,
	PE_UBCA_COUNT = 0x19,
	PE_UBCB_COUNT = 0x1a,
	
	PE_CYCLE_COUNT = 0x23,
	PE_STALL_IC_MISS_CYCLES = 0x24,
	PE_STALL_OC_MISS_CYCLES = 0x25,
	PE_STALL_BRANCH_CYCLES = 0x27,
	PE_STALL_CPU_CYCLES = 0x28,
	PE_STALL_FPU_CYCLES = 0x29,
} PERF_EVENT;

static inline pfCounter pfcRead(int n) {
	pfCounter result = (pfCounter)(PMCTRH(n) & 0xffff) << 32;
	result |= PMCTRL(n);
	return result;
}

static inline void pfcClearMaybe(int idx) {
	PMCR(idx) = PMCR(idx) | PMCR_PMCLR;
}

static inline void pfcPauseMaybe(int idx) {
	PMCR(idx) &= ~(PMCR_PMENABLE);
}

static inline void pfcEnable(int idx, PERF_EVENT event) {
	PMCR(idx) = PMCR(idx) | PMCR_PMCLR;
	PMCR(idx) = event | PMCR_PMENABLE | PMCR_PMST;
}

static inline void pfcDisable(int idx) {
	PMCR(idx) &= ~(PMCR_PMM_MASK | PMCR_PMENABLE);
}

typedef uint64 profile_t;

static inline profile_t PfStart() {
	return timer_us_gettime64();
}

static inline profile_t PfEnd(profile_t start) {
	profile_t end = timer_us_gettime64();
	return end-start;
}

float PfSecs(profile_t time);
float PfMillisecs(profile_t time);
float PfCycles(profile_t time);
float PfCycLoop(profile_t time, int loops);

void ProfileColorPoint(int red, int green, int blue);

void profile_init(size_t size);
void profile_reset(void);
void profile_shutdown(void);
void profile_go(void);
void profile_stop(void);
int profile_running(void);
int profile_save(const char *fname, int append);
void profile_record_pc(void);

#define MAX_PROFILE_POINTS 20
typedef enum {
	PFP_SUBMIT,
	PFP_COLLISION,
} profile_point;

void profile_point_start(profile_point profile_idx);
void profile_point_end(profile_point profile_idx);
void profile_point_start_colored(profile_point profile_idx);
profile_t profile_point_get_last_time(profile_point profile_idx);
profile_t profile_point_get_highwater(profile_point profile_idx);
void profile_point_string(profile_point *pfp_idxs, int points, char *dst);

#ifdef __cplusplus
}
#endif

#endif
