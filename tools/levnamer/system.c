//------------------------------------------------------------------------
// SYSTEM : System specific code (memory, I/O, etc).
//------------------------------------------------------------------------
//
//  LevNamer (C) 2001 Andrew Apted
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//------------------------------------------------------------------------

#include "system.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>
#include <assert.h>

#if defined(MSDOS) || defined(__MSDOS__)
#include <dos.h>
#endif

#if defined(__TURBOC__)
#include <alloc.h>
#endif

#if defined(UNIX) || defined(__UNIX__)
#include <unistd.h>  // Unix: isatty()
#endif


#define DEBUG_ENABLED   0
#define DEBUG_COREDUMP  0


int no_progress = 0;

static int progress_target;
static int progress_current;
static int progress_shown;


//
// PrintMsg
//
void PrintMsg(const char *str, ...)
{
  va_list args;

  va_start(args, str);
  vprintf(str, args);
  va_end(args);

  fflush(stdout);
}

//
// PrintWarn
//
void PrintWarn(const char *str, ...)
{
  va_list args;

  printf("Warning: ");

  va_start(args, str);
  vprintf(str, args);
  va_end(args);

  fflush(stdout);
}

//
// PrintDebug
//
void PrintDebug(const char *str, ...)
{
#if DEBUG_ENABLED
  va_list args;

  fprintf(stderr, "Debug: ");

  va_start(args, str);
  vfprintf(stderr, str, args);
  va_end(args);
#else
  (void) str;
#endif
}

//
// FatalError
//
// Terminate the program reporting an error.
//
void FatalError(const char *str, ...)
{
  va_list args;

  fprintf(stderr, " \nProblem: *** ");

  va_start(args, str);
  vfprintf(stderr, str, args);
  va_end(args);

  fprintf(stderr, " ***\n\n");

  #if DEBUG_COREDUMP
  raise(11);
  #endif

  exit(5);
}

//
// InternalError
//
// Terminate the program reporting an internal error.
//
void InternalError(const char *str, ...)
{
  va_list args;

  fprintf(stderr, " \nINTERNAL ERROR: *** ");

  va_start(args, str);
  vfprintf(stderr, str, args);
  va_end(args);

  fprintf(stderr, " ***\n\n");

  #if DEBUG_COREDUMP
  raise(11);
  #endif

  exit(6);
}

//
// SysCalloc
//
// Allocate memory with error checking.  Zeros the memory.
//
void *SysCalloc(int size)
{
  void *ret = calloc(1, size);
  
  if (!ret)
    FatalError("Out of memory (cannot allocate %d bytes)", size);

  return ret;
}

//
// SysRealloc
//
// Reallocate memory with error checking.
//
void *SysRealloc(void *old, int size)
{
  void *ret = realloc(old, size);

  if (!ret)
    FatalError("Out of memory (cannot reallocate %d bytes)", size);

  return ret;
}

//
// SysFree
//
// Free the memory with error checking.
//
void SysFree(void *data)
{
  if (data == NULL)
    InternalError("Trying to free a NULL pointer");
  
  free(data);
}

//
// SysStrdup
//
// Duplicate a string with error checking.
//
char *SysStrdup(const char *str)
{
  char *result;
  int len = strlen(str);

  result = SysCalloc(len+1);

  if (len > 0)
    memcpy(result, str, len);
  
  result[len] = 0;

  return result;
}

//
// SysStrndup
//
// Duplicate a limited length string.
//
char *SysStrndup(const char *str, int size)
{
  char *result;
  int len;

  for (len=0; len < size && str[len]; len++)
  { }

  result = SysCalloc(len+1);

  if (len > 0)
    memcpy(result, str, len);
  
  result[len] = 0;

  return result;
}

//
// StrCaseCmp
//
int StrCaseCmp(const char *A, const char *B)
{
  for (; *A || *B; A++, B++)
  {
    if (toupper(*A) != toupper(*B))
      return (toupper(*A) - toupper(*B));
  }

  // strings are equal
  return 0;
}

//
// StrUpperCase
// 
void StrUpperCase(char *str)
{
  assert(str);

  for (; *str; str++)
    *str = toupper(*str);
}


//
// InitProgress
//
void InitProgress(void)
{
  setbuf(stdout, NULL);

#ifdef UNIX  
  // Unix: no whirling baton if stderr is redirected
  if (! isatty(2))
    no_progress=1;
#endif
}

//
// TermProgress
//
void TermProgress(void)
{
  /* nothing to do */
}

//
// StartProgress
//
void StartProgress(int target)
{
  progress_target  = target;
  progress_current = 0;
  progress_shown   = -1;
}

//
// ShowProgress
//
void ShowProgress(int step)
{
  int perc;
  
  if (no_progress)
    return;
 
  // whirling baton ?
  if (progress_target == 0)
  {
    progress_current += step ? 128 : 1;

    if (step || (progress_current & 127) == 0)
      fprintf(stderr, "%c\b", "/-\\|"[((progress_current)>>7) & 3]);

    return;
  }
 
  progress_current += step;

  if (progress_current > progress_target)
    InternalError("Progress went past target !");
 
  perc = progress_current * 100 / progress_target;

  if (perc == progress_shown)
    return;

  fprintf(stderr, "--%3d%%--\b\b\b\b\b\b\b\b", perc);

  progress_shown = perc;
}

//
// ClearProgress
//
void ClearProgress(void)
{
  if (no_progress)
    return;

  // whirling baton ?
  if (progress_target == 0)
  {
    fprintf(stderr, " \b");
    return;
  }

  fprintf(stderr, "                \b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
}

//
// RoundPOW2
//
int RoundPOW2(int x)
{
  int tmp;

  if (x <= 2)
    return x;

  x--;
  
  for (tmp=x / 2; tmp; tmp /= 2)
    x |= tmp;
  
  return (x + 1);
}

