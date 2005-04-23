//------------------------------------------------------------------------
//  CONFIG : loading config data
//------------------------------------------------------------------------
//
//  Editor for DDF   (C) 2005  The EDGE Team
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

#ifndef __EDITDDF_CONFIG_H__
#define __EDITDDF_CONFIG_H__

class word_group_c
{
private:
	static const int MAX_WORDS = 5;

	const char * words[MAX_KEYWORDS];
	int num_words;

	// void *extra_data;

public:
	word_group_c();
	word_group_c(const char *W);
	word_group_c(const char *W1, const char *W2);
	word_group_c(const word_group_c& other);
	~word_group_c();

	int Size() const;
	// return the size of this group

	const char *Get(int index) const;
	// retrieve a given word from the group

	/* ---- modifying methods ---- */

	void Append(const char *W);
	// append the given word onto the end of this group
};

class keyword_box_c
{
public:
	enum
	P
		TP_General = 0,  

		TP_Files,
		TP_Keywords,
		TP_Commands,
		TP_States,
		TP_Actions
	};

private:
	int type;

	const char *name;

	class nd_c
	{
	public:
		nd_c *next;
		word_group_c group;

		nd_c();
		~nd_c();
	};

	nd_c *head;
	nd_c *tail;

	// FIXME: fast lookup for keywords

public:
	keyword_box_c(int _type, const char *_name);
	~keyword_box_c();

	bool MatchName(const char *other) const;
	// check if this keyword box matches the given name.

	int Size() const;
	// return the size of the keyword box.

	keyword_set_c *Get(int index) const;
	// retrieve a given word-group from the keyword box.

	bool HasKeyword(const char *W);
	// check if the given keyword exists.  Only checks the first
	// element of each word_group_c.

	// FIXME: auto-complete API

	/* ---- modifying methods ---- */

	void BeginNew(const char *W);
	void AddToCurrent(const char *W);
	// build a new word-group in the keyword box.
};

class kbox_container_c
{
private:
	static const int MAX_DEFBOX = 200;

	keyword_box_c *boxes[MAX_DEFBOX];
	int num_boxes;

public:
	kbox_container_c();
	~kbox_container_c();

	int Size() const;
	// return the size of the container.

	keyword_box_c *Get(int index) const;
	// retrieve a given keyword box from the container.

	keyword_box_c *FindByName(const char *name) const;
	// find the keyword box with a matching name.
	
	/* ---- modifying methods ---- */

	void Append(keyword_box_c *B);
	// add a new keyword box into the container

	bool ReadFile(const char *filename);
	bool WriteFile(const char *filename);
	// functions to read and write from/to the filesystem.
	// Return false on error (e.g. file not found).  The
	// ReadFile() method clears any previous data.
};

#endif /* __EDITDDF_CONFIG_H__ */
