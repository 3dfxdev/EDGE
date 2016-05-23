#ifdef	ROQ_SUPPORT
	// Cinematic information
	cinHandle_t		cinematicHandle;
#endif // ROQ_SUPPORT

#ifdef ROQ_SUPPORT
void	R_DrawStretchRaw (int x, int y, int w, int h, const byte *raw, int rawWidth, int rawHeight);
#else
void	R_DrawStretchRaw (int x, int y, int w, int h, int cols, int rows, byte *data);
#endif // ROQ_SUPPORT