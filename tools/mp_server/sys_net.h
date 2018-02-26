//----------------------------------------------------------------------------
//  Platform-specific NET code
//----------------------------------------------------------------------------
//
//  Edge MultiPlayer Server (C) 2005  Andrew Apted
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
//----------------------------------------------------------------------------

#ifndef __SYS_NET__
#define __SYS_NET__

// LINUX
const char * I_LocalIPAddrString(const char *iface_name);

// LINUX and MACOSX
void I_IgnoreBrokenPipes(void);

#endif  /* __SYS_NET__ */
