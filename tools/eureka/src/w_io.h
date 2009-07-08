/*
 *  wads.h
 *  AYM 1998-11-25
 */


#ifndef YH_WADS  /* DO NOT INSERT ANYTHING BEFORE THIS LINE */
#define YH_WADS


int  file_read_i16    (FILE *,  s16_t *buf, long count = 1);
int  file_read_i32    (FILE *,  s32_t *buf, long count = 1);
long file_read_vbytes (FILE *, void *buf, long count);
int  file_read_bytes  (FILE *, void *buf, long count);
void file_write_i16   (FILE *,  s16_t buf);
void file_write_i32   (FILE *,  s32_t buf, long count = 1);
void file_write_name  (FILE *, const char *name);
void WriteBytes       (FILE *, const void *, long);
int  copy_bytes       (FILE *dest, FILE *source, long size);



#if 0
/*
 *  flat_name_cmp
 *  Compare two flat names like strcmp() compares two strings.
 */
inline int flat_name_cmp (const char *name1, const char *name2)
{
}


/*
 *  tex_name_cmp
 *  Compare two texture names like strcmp() compares two strings.
 *  T
 */
inline int tex_name_cmp (const char *name1, const char *name2)
{
}


/*
 *  sprite_name_cmp
 *  Compare two sprite names like strcmp() compares two strings.
 */
inline int sprite_name_cmp (const char *name1, const char *name2)
{
}
#endif


#endif  /* DO NOT ADD ANYTHING AFTER THIS LINE */
