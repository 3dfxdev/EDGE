#pragma once

struct DATA_STREAM
{
	unsigned long cbStream;
	unsigned char* pbStream;
};

struct TAGGED_DATA
{
	const char* pcszTag;	// Data tag (e.g. "MAPHEAD" or "MAPDATA")
	DATA_STREAM ds;
};

enum MAP_OPTION_TYPE
{
	MAP_OPTION_BOOL,
	MAP_OPTION_INT,
	MAP_OPTION_STRING
};

struct MAP_OPTION
{
	char* pszName;
	MAP_OPTION_TYPE eType;
	union
	{
		bool fValue;
		int nValue;
		char* pszValue;
	};
};

struct STREAM_FACILITY
{
	// pfnReallocate() either allocates a new stream (if pds->pbStream is NULL) or reallocates the memory.
	// If the memory is reallocated successfully, the old memory has been copied to the new memory block.
	bool (__stdcall* pfnReallocate)(DATA_STREAM* pds, unsigned long cb);

	// pfnDelete() deletes pds->pbStream and sets it to NULL.  It also sets pds->cbStream to zero.
	void (__stdcall* pfnDelete)(DATA_STREAM* pds);
};

struct LOADER_HOST
{
	void* pvContext;
	bool (__stdcall* pfnQuery)(void* pvContext, const char* pcszFacility, void** ppvFacility);
};

bool __stdcall Loader_Create (LOADER_HOST* pHost, const char* pcszName, void** ppvLoader);
bool __stdcall Loader_GenerateMap (void* pvLoader, MAP_OPTION* prgOptions, INT cOptions, TAGGED_DATA* prgSources, INT cSources, DATA_STREAM* pdsTarget, char* pszCompanion, int cchMaxCompanion);
void __stdcall Loader_DeleteLoader (void* pvLoader);
