/*
 *  dialog.h
 *  Dialog boxes
 *  AYM 1998-11-30
 */


bool Confirm (int, int, const char *, const char *);
int Confirm2 (int x0, int y0, confirm_t *confirm_flag,
   const char *prompt1, const char *prompt2);
void Notify (int, int, const char *, const char *);
void NotImplemented (void);


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
