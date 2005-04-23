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

	const char * words[MAX_WORDS];
	int num_words;

	// void *extra_data;

public:
	word_group_c();
	word_group_c(const char *W);
	word_group_c(const char *W1, const char *W2);
	word_group_c(const word_group_c& other);
	~word_group_c();

	int Size() const { return num_words; }
	// return the size of this group

	const char *Get(int index) const;
	// retrieve a given word from the group

	/* ---- modifying methods ---- */

	void Clear();

	void Append(const char *W);
	// append the given word onto the end of this group

	void WriteToFile(FILE *fp);
	// write this word group into the given file.
};

class keyword_box_c
{
public:
	enum
	{
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

	bool MatchType(int req_type) const { return req_type == type; }
	bool MatchName(const char *req_name) const;
	// check if this keyword box matches the given name.

	int Size() const;
	// return the size of the keyword box.

	word_group_c *Get(int index) const;
	// retrieve a given word-group from the keyword box.

	bool HasKeyword(const char *W);
	// check if the given keyword exists.  Only checks the first
	// element of each word_group_c.

	// FIXME: auto-complete API

	/* ---- modifying methods ---- */

	void BeginNew(const char *W);
	void AddToCurrent(const char *W);
	// build a new word-group in the keyword box.

	void WriteToFile(FILE *fp);
	// write this keyword box into the given file.
};

class kb_container_c
{
private:
	static const int MAX_BOXES = 200;

	keyword_box_c *boxes[MAX_BOXES];
	int num_boxes;

public:
	kb_container_c();
	~kb_container_c();

	int Size() const { return num_boxes; }
	// return the size of the container.

	keyword_box_c *Get(int index) const;
	// retrieve a given keyword box from the container.

	keyword_box_c *Find(int type, const char *name);
	// find the keyword box with a matching name.  Returns
	// NULL when not found.

	/* ---- modifying methods ---- */

	void Clear();
	// remove existing contents.

	void Append(keyword_box_c *B);
	// add a new keyword box into the container.  This pointer is merely
	// copied (not the whole keyword box), and will be freed by the
	// destructor of this class.

	bool ReadFile(const char *filename);
	bool WriteFile(const char *filename, const char *comment = NULL);
	// functions to read and write from/to the filesystem.
	// Return false on error (e.g. file not found).  The
	// ReadFile() method merely appends the new boxes
	// (even if the types/names match).

private:
	enum
	{
		TOK_SYMBOL = 1, TOK_WORD, TOK_NUMBER, TOK_STRING
	};

	const char * ParseToken(FILE *fp);
	// read a single token from the file, and return it.  The
	// returned pointer is into a static buffer.  The first
	// character is one of the TOK_XXX values.  Returns NULL
	// upon EOF.
};

#endif /* __EDITDDF_CONFIG_H__ */
