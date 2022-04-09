//----------------------------------------------------------------------------
//  EDGE Path Handling Methods for Win32
//----------------------------------------------------------------------------
//
//  Copyright (c) 2003-2008  The EDGE Team.
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
//
#include "epi.h"
#include "path.h"
#include <filesystem>

namespace epi
{

// Path Manipulation Functions
std::string PATH_GetDir(const char *path)
{
	SYS_ASSERT(path);

	return std::filesystem::path(path).remove_filename().string();
}


std::string PATH_GetFilename(const char *path)
{
	SYS_ASSERT(path);

	return std::filesystem::path(path).filename().string();
}


std::string PATH_GetExtension(const char *path)
{
	SYS_ASSERT(path);

	return std::filesystem::path(path).extension().string();
}


std::string PATH_GetBasename(const char *path) 
{
	SYS_ASSERT(path);

	return std::filesystem::path(path).stem().string();
}



bool PATH_IsAbsolute(const char *path)
{
	SYS_ASSERT(path);

	return std::filesystem::path(path).is_absolute();
}


bool PATH_IsDirSep(const char c)
{
#ifdef WIN32
    return (c == '\\' || c == '/' || c == ':'); // Kester added ':'
#else // LINUX
    return (c == '\\' || c == '/');
#endif
}


std::string PATH_Join(const char *lhs, const char *rhs)
{
	SYS_ASSERT(lhs && rhs);

	if (PATH_IsAbsolute(rhs))
		return std::string(rhs);

    std::string result(lhs);

	if (result.size() > 0)
	{
		const char c = result[result.size()-1];

#ifdef WIN32
		if (c != '\\' && c != '/')
			result += DIRSEPARATOR;
#else // LINUX
		if (c != '/')
			result += DIRSEPARATOR;
#endif
	}

	result += rhs;

    return result;
}


} // namespace epi

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
