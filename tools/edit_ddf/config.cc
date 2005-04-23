//------------------------------------------------------------------------
//  CONFIG: reading config data
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

#include "defs.h"
 
word_group_c::word_group_c() : num_words(0)
{ }

word_group_c::word_group_c(const char *W) : num_words(0)
{
	Append(W);
}

word_group_c::word_group_c(const char *W1, const char *W2) : num_words(0)
{
	Append(W1);
	Append(W2);
}

word_group_c::word_group_c(const word_group_c& other)
{
	num_words = other.num_words;

	for (int i=0; i < num_words; i++)
		words[i] = strdup(other.words[i]);
}

word_group_c::~word_group_c()
{
	Clear();
}

const char *word_group_c::Get(int index) const
{
	SYS_ASSERT(index >= 0);
	SYS_ASSERT(index < num_words);

	return words[index];
}

void word_group_c::Clear()
{
	for (int w = 0; w < num_words; w++)
		delete words[w];

	num_words = 0;
}

void word_group_c::Append(const char *W)
{
	SYS_ASSERT(num_words < MAX_WORDS);

	words[num_words++] = strdup(W);
}

void word_group_c::WriteToFile(FILE *fp)
{
	for (int i = 0; i < Size(); i++)
	{
		fprintf(fp, "%s", words[i]);

		if (i+1 < Size())
			fprintf(fp, ":");
	}
}

//------------------------------------------------------------------------

keyword_box_c::keyword_box_c(int _type, const char *_name) :
	type(type), head(NULL), tail(NULL)
{
	name = strdup(_name);
}

keyword_box_c::~keyword_box_c()
{
	free((void*)name);

	while (head)
	{
		nd_c *cur = head;
		head = head->next;

		delete cur;
	}
}

keyword_box_c::nd_c::nd_c() : next(NULL), group()
{ }

keyword_box_c::nd_c::~nd_c()
{ }

bool keyword_box_c::MatchName(const char *req_name) const
{
	return (UtilStrCaseCmp(name, req_name) == 0);
}

int keyword_box_c::Size() const
{
	int size = 0;

	for (nd_c *cur = head; cur; cur = cur->next)
		size++;
	
	return size;
}

word_group_c *keyword_box_c::Get(int index) const
{
	SYS_ASSERT(index >= 0);

	nd_c *cur = head;

	for (; index > 0; index--)
	{
		SYS_NULL_CHECK(cur);
		cur = cur->next;
	}

	SYS_NULL_CHECK(cur);
	return &cur->group;
}

bool keyword_box_c::HasKeyword(const char *W)
{
	for (nd_c *cur = head; cur; cur = cur->next)
	{
		SYS_ASSERT(cur->group.Size() > 0);

		const char *cur_word = cur->group.Get(0);

		if (UtilStrCmpDDF(cur_word, W) == 0)
			return true;
	}

	return false; // not found
}

void keyword_box_c::BeginNew(const char *W)
{
	nd_c *cur = new nd_c();

	cur->group.Append(W);

	// link it in
	if (tail)
		tail->next = cur;
	else
		head = cur;
	
	tail = cur;
}

void keyword_box_c::AddToCurrent(const char *W)
{
	SYS_NULL_CHECK(tail);

	tail->group.Append(W);
}

void keyword_box_c::WriteToFile(FILE *fp)
{
	switch (type)
	{
		case TP_General:  fprintf(fp, "GENERAL "); break;
		case TP_Files:    fprintf(fp, "FILES ");   break;
		case TP_Keywords: fprintf(fp, "KEYWORDS "); break;
		case TP_Commands: fprintf(fp, "COMMANDS "); break;
		case TP_States:   fprintf(fp, "STATES ");  break;
		case TP_Actions:  fprintf(fp, "ACTIONS "); break;

		default:
			AssertFail("Illegal keyword-box type %d\n", type);
			return;
	};

	fprintf(fp, "{");

	for (nd_c *cur = head; cur; cur = cur->next)
	{
		cur->group.WriteToFile(fp);

		if (cur->next)
			fprintf(fp, ",");

		fprintf(fp, "\n");
	}

	fprintf(fp, "}\n\n");
}

//------------------------------------------------------------------------

kb_container_c::kb_container_c() : num_boxes(0)
{ }

kb_container_c::~kb_container_c()
{
	Clear();
}

keyword_box_c *kb_container_c::Get(int index) const
{
	SYS_ASSERT(index >= 0);
	SYS_ASSERT(index < num_boxes);

	return boxes[index];
}

keyword_box_c *kb_container_c::Find(int type, const char *name)
{
	for (int i = 0; i < Size(); i++)
	{
		keyword_box_c *box = Get(i);

		if (box->MatchType(type) && box->MatchName(name))
			return box;
	}

	return NULL;  // not found
}

void kb_container_c::Clear()
{
	for (int i = 0; i < num_boxes; i++)
		delete boxes[i];
	
	num_boxes = 0;
}

void kb_container_c::Append(keyword_box_c *B)
{
	SYS_ASSERT(num_boxes < MAX_BOXES);

	boxes[num_boxes++] = B;
}

//
// Config Reading
//
bool kb_container_c::ReadFile(const char *filename)
{
	FILE *fp = fopen(filename, "r");

	if (! fp)
	{
		PrintWarn("Unable to open redd file: %s\n", strerror(errno));
		return false;
	}

	// clear previous stuff
	Clear();

	// !!!! FIXME:

	fclose(fp);
	return true;
}

bool kb_container_c::WriteFile(const char *filename, const char *comment)
{
	FILE *fp = fopen(filename, "w");

	if (! fp)
	{
		PrintWarn("Unable to open write file: %s\n", strerror(errno));
		return false;
	}

	if (comment)
		fprintf(fp, "// %s\n\n", comment);

	for (int i = 0; i < Size(); i++)
	{
		Get(i)->WriteToFile(fp);
	}

	fclose(fp);
	return true;
}
