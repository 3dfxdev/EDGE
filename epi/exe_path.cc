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

#include "exe_path.h"

#include <string.h>

namespace epi
{

const char *GetExecutablePath(const char *argv0)
{
  char *path;

#ifdef WIN32
  path = StringNew(PATH_MAX+2);

  int length = GetModuleFileName(GetModuleHandle(NULL), path, PATH_MAX);

  if (length > 0 && length < PATH_MAX)
  {
    if (access(path, 0) == 0)  // sanity check
    {
      FilenameStripBase(path);
      return path;
    }
  }

  // didn't work, free the memory
  StringFree(path);
#endif

#ifdef LINUX
  path = StringNew(PATH_MAX+2);

  int length = readlink("/proc/self/exe", path, PATH_MAX);

  if (length > 0)
  {
    path[length] = 0; // add the missing NUL

    if (access(path, 0) == 0)  // sanity check
    {
      FilenameStripBase(path);
      return path;
    }
  }

  // didn't work, free the memory
  StringFree(path);
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

  path = StringNew(pathlen+2);

  if (0 == _NSGetExecutablePath(path, &pathlen))
  {
    // FIXME: will this be _inside_ the .app folder???
    FilenameStripBase(path);
    return path;
  }
  
  // didn't work, free the memory
  StringFree(path);
#endif

  // fallback method: use argv[0]
  path = StringDup(argv0);

#ifdef MACOSX
  // FIXME: check if _inside_ the .app folder
#endif
  
  FilenameStripBase(path);
  return path;
}

};  // namespace epi

