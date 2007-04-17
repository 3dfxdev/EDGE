//------------------------------------------------------------------------
//  Path to Executable
//------------------------------------------------------------------------
// 
//  Copyright (c) 2006-2007  The EDGE Team.
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

#include "epi.h"
#include "exe_path.h"
#include "path.h"

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>      // access()
#else
#include <unistd.h>  // access(), readlink()
#endif

#ifdef MACOSX
#include <sys/param.h>
#include <mach-o/dyld.h> // _NSGetExecutablePath
#endif

#include <limits.h>  // PATH_MAX

#ifndef PATH_MAX
#define PATH_MAX  2048
#endif

namespace epi
{

const char *GetExecutablePath(const char *argv0)
{
  // NOTE: there are a few memory leaks here.  Because this
  //       function is only called once, I don't care.

  char *dir;

#ifdef WIN32
  dir = new char[PATH_MAX+2];

  int length = GetModuleFileName(GetModuleHandle(NULL), dir, PATH_MAX);

  if (length > 0 && length < PATH_MAX)
  {
    if (access(dir, 0) == 0)  // sanity check
    {
      return path::GetDir(dir).GetString();
    }
  }

  // didn't work, free the memory
  delete[] dir;
#endif

#ifdef LINUX
  dir = new char[PATH_MAX+2];

  int length = readlink("/proc/self/exe", dir, PATH_MAX);

  if (length > 0)
  {
    dir[length] = 0; // add the missing NUL

    if (access(dir, 0) == 0)  // sanity check
    {
      return path::GetDir(dir).GetString();
    }
  }

  // didn't work, free the memory
  delete[] dir;
#endif

#ifdef MACOSX
/*
  from http://www.hmug.org/man/3/NSModule.html

  extern int _NSGetExecutablePath(char *buf, unsigned long *bufsize);

  _NSGetExecutablePath copies the path of the executable
  into the buffer and returns 0 if the path was successfully
  copied in the provided buffer. If the buffer is not large
  enough, -1 is returned and the expected buffer size is
  copied in *bufsize.
*/
  int pathlen = PATH_MAX * 2;

  dir = new char [pathlen+2];

  if (0 == _NSGetExecutablePath(dir, &pathlen))
  {
    // FIXME: will this be _inside_ the .app folder???
    return path::GetDir(dir).GetString();
  }

  // didn't work, free the memory
  delete[] dir;
#endif

  // fallback method: use argv[0]

#ifdef MACOSX
  // FIXME: check if _inside_ the .app folder
#endif
  
  return path::GetDir(argv0).GetString();
}

};  // namespace epi

