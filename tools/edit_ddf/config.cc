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

//
// Grammar of the config file:
//
// FILE  = BOX { BOX }.
// BOX   = section WORD "{" GROUP { "," GROUP } [","] "}".
// GROUP = WORD { ":" (WORD | NUMBER | STRING) }.
//
// Lexical structures:
//
// COMMENT = "//" until the end of line.
// WORD    = letter { letter | digit | "_" }.
// NUMBER  = digit  { digit | "." }.
// STRING  = '"' { str-char } '"'.
//
 
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
		if (i > 0) fputc('"', fp);

		fprintf(fp, "%s", words[i]);

		if (i > 0) fputc('"', fp);

		if (i+1 < Size())
			fprintf(fp, ":");
	}
}

//------------------------------------------------------------------------

keyword_box_c::keyword_box_c(int _type, const char *_name) :
	type(_type), head(NULL), tail(NULL)
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

const char *keyword_box_c::GetTypeName() const
{
	switch (type)
	{
		case TP_General:  return "GENERAL";
		case TP_Files:    return "FILES";
		case TP_Keywords: return "KEYWORDS";
		case TP_Commands: return "COMMANDS";
		case TP_States:   return "STATES";
		case TP_Actions:  return "ACTIONS";

		default:
			AssertFail("Illegal keyword-box type %d", type);
			return ""; /* NOT REACHED */
	};
}

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

bool keyword_box_c::MatchSection(const char *word, int *sec_type)
{
	if (UtilStrCaseCmp(word, "GENERAL") == 0)
		*sec_type = TP_General;
	else if (UtilStrCaseCmp(word, "FILES") == 0)
		*sec_type = TP_Files;
	else if (UtilStrCaseCmp(word, "KEYWORDS") == 0)
		*sec_type = TP_Keywords;
	else if (UtilStrCaseCmp(word, "COMMANDS") == 0)
		*sec_type = TP_Commands;
	else if (UtilStrCaseCmp(word, "STATES") == 0)
		*sec_type = TP_States;
	else if (UtilStrCaseCmp(word, "ACTIONS") == 0)
		*sec_type = TP_Actions;
	else
		return false;
	
	return true;
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
	fprintf(fp, "%s %s\n", GetTypeName(), GetName());
	fprintf(fp, "{\n");

	for (nd_c *cur = head; cur; cur = cur->next)
	{
		fprintf(fp, "    ");

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
	if (num_boxes >= MAX_BOXES)
		FatalError("Too many sections in config file!\n");

	boxes[num_boxes++] = B;
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
		fprintf(fp, "# %s\n\n", comment);

	for (int i = 0; i < Size(); i++)
	{
		Get(i)->WriteToFile(fp);
	}

	fclose(fp);
	return true;
}

//------------------------------------------------------------------------
//  Config Reading
//------------------------------------------------------------------------

char kb_container_c::error_buf[1024];

const char * kb_container_c::ParseToken(FILE *fp)
{
	static char buffer[256];
	int buf_len = 0;

	enum { PAR_Unknown, PAR_Word, PAR_Number, PAR_String, PAR_Comment };

	int state = PAR_Unknown;

	for (;;)
	{
		if (buf_len+4 > (int)sizeof(buffer))
		{
			Error("Extremely long token found!\n");
			break;
		}

		int c = fgetc(fp);

		if (c == EOF || ferror(fp))
			break;

		if (state == PAR_Unknown)
		{
			// skip whitespace
			if (isspace(c) || ! isprint(c))
			{
				if (c == '\n')
					parse_line++;
				
				continue;
			}

			if (isalpha(c))
			{
				buffer[buf_len++] = TOK_WORD;
				buffer[buf_len++] = c;

				state = PAR_Word;
				continue;
			}
			else if (isdigit(c) || c == '-' || c == '+')
			{
				buffer[buf_len++] = TOK_NUMBER;
				buffer[buf_len++] = c;

				state = PAR_Word;
				continue;
			}
			else if (c == '"')
			{
				buffer[buf_len++] = TOK_STRING;

				state = PAR_String;
				continue;
			}
			else if (c == '#')
			{
				state = PAR_Comment;
				continue;
			}

			// found a symbol
			buffer[buf_len++] = TOK_SYMBOL;
			buffer[buf_len++] = c;
			break;
		}
		else if (state == PAR_Comment)
		{
			if (c == '\n')
			{
				state = PAR_Unknown;
				parse_line++;
		    }

			continue;
		}
		else if (state == PAR_String)
		{
			if (c != '"' && c != '\n')
			{
				buffer[buf_len++] = c;
				continue;
			}

			// prevent unterminated string error
			if (c == '"')
				state = PAR_Unknown;

			// found end of string
			break;
		}
		else if (state == PAR_Word)
		{
			if (isalnum(c) || c == '_')
			{
				buffer[buf_len++] = c;
				continue;
			}

			// found end of identifier
			ungetc(c, fp);
			break;
		}
		else if (state == PAR_Number)
		{
			if (isdigit(c) || c == '.')
			{
				buffer[buf_len++] = c;
				continue;
			}

			// found end of number
			ungetc(c, fp);
			break;
		}
		else
		{
			AssertFail("Illegal parse state %d", state);
			return NULL; /* NOT REACHED */
		}
	}

	if (state == PAR_String)
	{
		parse_line--;
		Error("Unterminated string!\n");
	}

	// special handling for '-' and '+' symbols
	if (buf_len == 2 && buffer[0] == TOK_NUMBER)
		buffer[0] = TOK_SYMBOL;

	buffer[buf_len] = 0;

	if (buffer[0] == TOK_STRING)
		return buffer;

	if (buf_len >= 2) 
		return buffer;

	return NULL;  // EOF
}

const char *kb_container_c::ParseGROUP(FILE *fp, keyword_box_c *box)
{
	const char *tok = ParseToken(fp);

	if (! tok || (tok[0] == TOK_SYMBOL && (tok[1] == ',' || tok[1] == '}')))
		return tok;

	if (tok[0] != TOK_WORD)
		Error("Bad word group (expected a word, got: %s)\n", tok+1);

	box->BeginNew(tok+1);

	for (;;)
	{
		tok = ParseToken(fp);

		if (! tok || (tok[0] == TOK_SYMBOL && (tok[1] == ',' || tok[1] == '}')))
			break;

		if (tok[0] != TOK_SYMBOL || tok[1] != ':')
			Error("Missing ':' for word group (got: %s)\n", tok+1);

		tok = ParseToken(fp);
		
		if (! tok || (tok[0] == TOK_SYMBOL && (tok[1] == ',' || tok[1] == '}')))
			Error("Bad word group (missing word after ':')\n");

		if (tok[0] == TOK_SYMBOL && tok[1] == ':')
			Error("Bad word group (two ':' in a row)\n");

		box->AddToCurrent(tok+1);
	}

	return tok;
}

keyword_box_c *kb_container_c::ParseBOX(FILE *fp)
{
	const char *tok = ParseToken(fp);

	if (! tok)
		return NULL;

	if (tok[0] != TOK_WORD)
		Error("Missing section type (got: %s)\n", tok+1);

	int sec_type;

	if (! keyword_box_c::MatchSection(tok+1, &sec_type))
		Error("Bad section type: %s\n", tok+1);

	tok = ParseToken(fp);

	if (! tok)
		Error("Missing section name\n");
	else if (tok[0] != TOK_WORD)
		Error("Bad section name: %s\n", tok+1);

	keyword_box_c *box = new keyword_box_c(sec_type, tok+1);

	parse_box = box;  // error information

	tok = ParseToken(fp);

	if (! tok || tok[0] != TOK_SYMBOL || tok[1] != '{')
		Error("Missing '{' for section\n");
	
	for (;;)
	{
		tok = ParseGROUP(fp, box);

		if (! tok || (tok[0] == TOK_SYMBOL && tok[1] == '}'))
			break;

		if (tok[0] != TOK_SYMBOL || tok[1] != ',')
			Error("Bad word group (expected a comma, got: %s)\n", tok[1]);
	}

	parse_box = NULL;

	return box;
}

bool kb_container_c::ReadFile(const char *filename)
{
	FILE *fp = fopen(filename, "r");

	if (! fp)
	{
		PrintWarn("Unable to open read file: %s\n", strerror(errno));
		return false;
	}

	// clear previous stuff
	Clear();

	// setup information for error messages
	parse_line = 1;
	parse_box = NULL;
	parse_file = filename;

	for (;;)
	{
		keyword_box_c *box = ParseBOX(fp);

		if (! box)
			break;

		Append(box);
	}

	fclose(fp);

	parse_box  = NULL;
	parse_file = NULL;

	return true;
}

void kb_container_c::Error(const char *str, ...)
{
	va_list args;

	va_start(args, str);
	vsprintf(error_buf, str, args);
	va_end(args);

	SYS_ASSERT(parse_file);

	if (parse_box)
		FatalError("ERROR: %sLine: %d, section: %s %s, file: %s\n",
			error_buf, parse_line, parse_box->GetTypeName(),
			parse_box->GetName(), parse_file);
	else
		FatalError("ERROR: %sLine: %d, file: %s\n",
			error_buf, parse_line, parse_file);
}
