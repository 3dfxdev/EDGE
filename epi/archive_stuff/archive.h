//------------------------------------------------------------------------
//  EPI Archive interface
//------------------------------------------------------------------------
// 
//  Copyright (c) 2003-2005  The EDGE Team.
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

#ifndef __EPI_ARCHIVE_H__
#define __EPI_ARCHIVE_H__

#include "epi.h"
#include "types.h"

#include "arrays.h"
#include "timestamp.h"

namespace epi
{

class archive_entry_c  /* ABSTRACT BASE CLASS */
{
protected: archive_entry_c() { };
public: virtual ~archive_entry_c() { };

public:
	virtual bool isDir() const = 0;
	virtual const char *getName() const = 0;
	virtual int getSize() const = 0;  /* reading size only */
	virtual bool getTimeStamp(timestamp_c *time) const = 0; /* false if none */

	/* -------- directory operations -------- */

	virtual int NumEntries() const = 0;
	// number of entries in this directory.

	virtual archive_entry_c *operator[](int idx) const = 0;
	// lookup method.  Returns NULL if idx is out of range.

	virtual archive_entry_c *operator[](const char *name) const = 0;
	// name lookup utility, returns NULL if name not found.  Names are
	// matched case SENSITIVELY (foo != Foo != FOO).

	virtual archive_entry_c *Add(const char *name, const timestamp_c& time,
		bool dir = false, bool compress = false) = 0;
	// creates a new directory or normal entry.  No checking is done to
	// ensure the name is unique, and entries are NOT sorted.
	// The 'compress' parameter is a hint only, and may be ignored.
	// Returns NULL on error.

	virtual bool Remove(int idx) = 0;
	// delete entry with the given idx.  All entries after this one will
	// be shifted down (have index values one less).  Therefore if deleting
	// entries in a loop, you should go backwards.  Pointers from previous
	// lookups are invalidated !  Returns false on error.
};

class archive_c  /* ABSTRACT BASE CLASS */
{
protected: archive_c() { };
public: virtual ~archive_c() { };

public:
	/* -------- whole archive operations -------- */

	virtual const char *getFilename() const = 0;
	// get the archive's filename.

	virtual const char *getComment() const = 0;
	virtual bool setComment(const char *s) const = 0;
	// get/set the comment stored in the archive.  If there was no comment
	// then getComment() returns an empty string (never NULL).  The
	// setComment() method returns false if comments aren't supported.

	/* -------- root directory operations -------- */

	// See archive_entry_c above for documentation of these methods.

	virtual int NumEntries() const = 0;

	virtual archive_entry_c *operator[](int idx) const = 0;
	virtual archive_entry_c *operator[](const char *name) const = 0;

	virtual archive_entry_c *Add(const char *name, const timestamp_c& time,
		bool dir = false, bool compress = false) = 0;

	virtual bool Remove(int idx) = 0;

	/* -------- reading and writing entries -------- */

	virtual byte *Read(archive_entry_c *entry) const = 0;
	// read an entry completely.  You need to delete[] the returned data
	// when finished with it.  The data will have an extra zero byte (NUL)
	// added to the end (to make parsing text easier).  Only data from an
	// existing WAD is read (data you have added with the Write method is
	// NOT returned with thie call).  Returns NULL on error.

	virtual bool Truncate(archive_entry_c *entry) = 0;
	// when the archive was opened in "append" mode, then this method is
	// required if you want to remove any previous contents.  Normally
	// in append mode, writes will append data to the end of the existing
	// data.  This call will also clear any previously written data.  You
	// don't need to use this on new entries.  Returns false on error.

	virtual bool Write(archive_entry_c *entry, const byte *data, int length) = 0;
	// write (append) data to an entry.  You can call this multiple
	// times, the new data is appended to the current data.  Actual
	// writing to the destination occurs when the archive is closed.
	// Returns false on error.


	// Static methods that should be available in the subclass:
	//
	//    xxx_archive_c *Open(const char *filename, const char *mode);
	//    bool Close(xxx_archive_c *xxx);
};

//------------------------------------------------------------------------
// IMPLEMENTATION HELPER CLASS (PRIVATE TO SUBCLASSERS)
//------------------------------------------------------------------------

class archive_dir_c : public array_c
{
protected:
    archive_dir_c() : epi::array_c(sizeof(archive_entry_c*)) { }
public:
    virtual ~archive_dir_c() { Clear(); }
 
private:
    void CleanupObject(void *obj) { delete *(archive_entry_c**)obj; }
 
protected:
    // List Management
    int NumEntries() const { return array_entries; }
    int Insert(archive_entry_c *e) { return InsertObject((void*)&e); }

	// Archive Directory Management
    archive_entry_c *operator[](int idx) const;
	archive_entry_c *operator[](const char *name) const;

	bool Delete(int idx);
};

}  // namespace epi

#endif  /* __EPI_ARCHIVE_H__ */
