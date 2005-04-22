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

class keyword_set_c
{
private:
	static const int MAX_KEYWORDS = 5;

	const char ** keywords;
	int num_keywords;

public:
	keyword_set_c();
	keyword_set_c(const char *K);
	keyword_set_c(const char *K1, const char *K2);
	~keyword_set_c();

	void Append(const char *K);
	// append the given keyword onto the end of this set

	int Size() const;
	// return the size of this set

	const char *Get(int index) const;
	// retrieve a given keyword from the set
};

class defin_box_c
{
private:
	class nd_c
	{
	public:
		nd_c *next;
		const char *line;

		nd_c(keyword_set_c *K);
		~nd_c();
	};

	nd_c *head;

	// FIXME: binary lookup for keywords

public:
	defin_box_c();
	~defin_box_c();

	void Append(keyword_set_c *K);
	// add a new keyword_set into the definition box

	int Size() const;
	// return the size of the definition box

	keyword_set_c *Get(int index) const;
	// retrieve a given keyword_set from the definition box

	bool HasKeyword(const char *K);
	// check if the given keyword exists.
};

class defbox_container_c
{
private:
	static const int MAX_DEFBOX = 200;

	defin_box_c *boxes;

	int num_boxes;

public:
	defbox_container_c();
	~defbox_container_c();

	void Append(defin_box_c *B);
	// add a new definition box into the container

	int Size() const;
	// return the size of the container

	defin_box_c *Get(int index) const;
	// retrieve a given definition box from the container

	bool ReadFile(const char *filename);
	bool WriteFile(const char *filename);
	// functions to read and write from/to the filesystem.
	// Return false on error (e.g. file not found).  The
	// ReadFile() method clears any previous data.
};

#endif /* __EDITDDF_CONFIG_H__ */
