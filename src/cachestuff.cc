
//
// W_CacheLumpName
//
const void *W_CacheLumpName2(const char *name)
{
	return W_CacheLumpNum2(W_GetNumForName2(name));
}

//
// W_PreCacheLumpNum
//
// Attempts to load lump into the cache, if it isn't already there
//
void W_PreCacheLumpNum(int lump)
{
	W_DoneWithLump(W_CacheLumpNum(lump));
}

//
// W_PreCacheLumpName
//
void W_PreCacheLumpName(const char *name)
{
	W_DoneWithLump(W_CacheLumpName(name));
}

//
// W_CacheInfo
//
int W_CacheInfo(int level)
{
	lumpheader_t *h;
	int value = 0;

	for (h = lumphead.next; h != &lumphead; h = h->next)
	{
		if ((level & 1) && h->users)
			value += W_LumpLength(h->lumpindex);
		if ((level & 2) && !h->users)
			value += W_LumpLength(h->lumpindex);
	}
	return value;
}

//
// W_LoadLumpNum
//
// Returns a copy of the lump (it is your responsibility to free it)
//
void *W_LoadLumpNum(int lump)
{
	void *p;
	const void *cached;
	int length = W_LumpLength(lump);
	p = (void *) Z_Malloc(length);
	cached = W_CacheLumpNum2(lump);
	memcpy(p, cached, length);
	W_DoneWithLump(cached);
	return p;
}

//
// W_LoadLumpName
//
void *W_LoadLumpName(const char *name)
{
	return W_LoadLumpNum(W_GetNumForName2(name));
}

//
// W_GetLumpName
//
const char *W_GetLumpName(int lump)
{
	return lumpinfo[lump].name;
}
