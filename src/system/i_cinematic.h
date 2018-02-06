 /*==============================================================================

 CINEMATICS

 ==============================================================================
*/

#define MAX_CINEMATICS						16
#define BIT(num)							(1 << (num))

typedef int							cinHandle_t;

cinHandle_t	cinematicHandle;

typedef enum 
{
	CIN_SYSTEM						= BIT(0),
	CIN_LOOPING						= BIT(1),
	CIN_SILENT						= BIT(2)
} cinFlags_t;

typedef struct 
{
	const byte *			image;
	bool					dirty;

	int						width;
	int						height;
} cinData_t;

// Plays a cinematic
cinHandle_t		CIN_PlayCinematic (const char *name, int flags);

// Runs a cinematic frame
cinData_t		CIN_UpdateCinematic (cinHandle_t handle, int time);

// Resets a cinematic
void			CIN_ResetCinematic (cinHandle_t handle);

// Stops a cinematic
void			CIN_StopCinematic (cinHandle_t handle);

// Initializes the cinematic module
void			CIN_Init (void);

// Shuts down the cinematic module
void			CIN_Shutdown (void);

void E_PlayMovie(const char *name, int flags);

#define ROQ_ID							0x1084

#define ROQ_CHUNK_HEADER_SIZE			8
#define ROQ_CHUNK_MAX_DATA_SIZE			131072

#define ROQ_QUAD_INFO					0x1001
#define ROQ_QUAD_CODEBOOK				0x1002
#define ROQ_QUAD_VQ						0x1011
#define ROQ_QUAD_JPEG					0x1012
#define ROQ_QUAD_HANG					0x1013

#define ROQ_SOUND_MONO_22				0x1020
#define ROQ_SOUND_STEREO_22				0x1021
#define ROQ_SOUND_MONO_48				0x1022
#define ROQ_SOUND_STEREO_48				0x1023

#define ROQ_VQ_MOT						0x0000
#define ROQ_VQ_FCC						0x4000
#define ROQ_VQ_SLD						0x8000
#define ROQ_VQ_CCC						0xC000

typedef struct 
{
	u16_t				id;
	u32_t				size;
	u16_t				args;
} 
roqChunkHeader_t;
