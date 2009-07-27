//----------------------------------------------------------------------------
//  DDF Parsing code
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2009  The EDGE Team.
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

#include "local.h"

#include <limits.h>
#include <vector>

#include "epi/path.h"
#include "epi/str_format.h"

#include "colormap.h"
#include "attack.h"
#include "thing.h"

#include "src/p_action.h"


int engine_version;
static std::string ddf_where;

bool strict_errors = false;
bool lax_errors = false;
bool no_warnings = false;


//
// DDF_Error
//
// -AJA- 1999/10/27: written.
//
int cur_ddf_line_num;
std::string cur_ddf_filename;
std::string cur_ddf_entryname;
std::string cur_ddf_linedata;

void DDF_Error(const char *err, ...)
{
	va_list argptr;
	char buffer[2048];
	char *pos;

	buffer[2047] = 0;

	// put actual error message on first line
	va_start(argptr, err);
	vsprintf(buffer, err, argptr);
	va_end(argptr);
 
	pos = buffer + strlen(buffer);

	if (!cur_ddf_filename.empty())
	{
		sprintf(pos, "Error occurred near line %d of %s\n", 
				cur_ddf_line_num, cur_ddf_filename.c_str());
				
		pos += strlen(pos);
	}

	if (!cur_ddf_entryname.empty())
	{
		sprintf(pos, "Error occurred in entry: %s\n", 
				cur_ddf_entryname.c_str());
				
		pos += strlen(pos);
	}

	if (!cur_ddf_linedata.empty())
	{
		sprintf(pos, "Line contents: %s\n", 
				cur_ddf_linedata.c_str());
				
		pos += strlen(pos);
	}

	// check for buffer overflow
	if (buffer[2047] != 0)
		I_Error("Buffer overflow in DDF_Error\n");
  
	// add a blank line for readability under DOS/Linux.
	I_Printf("\n");
 
	I_Error("%s", buffer);
}

void DDF_Warning(const char *err, ...)
{
	va_list argptr;
	char buffer[1024];

	if (no_warnings)
		return;

	va_start(argptr, err);
	vsprintf(buffer, err, argptr);
	va_end(argptr);

	I_Warning("%s", buffer);

	if (!cur_ddf_filename.empty())
	{
		I_Printf("  problem occurred near line %d of %s\n", 
				  cur_ddf_line_num, cur_ddf_filename.c_str());
	}

	if (!cur_ddf_entryname.empty())
	{
		I_Printf("  problem occurred in entry: %s\n", 
				  cur_ddf_entryname.c_str());
	}
	
	if (!cur_ddf_linedata.empty())
	{
		I_Printf("  with line contents: %s\n", 
				  cur_ddf_linedata.c_str());
	}

///	I_Printf("\n");
}

void DDF_WarnError(const char *err, ...)
{
	va_list argptr;
	char buffer[1024];

	va_start(argptr, err);
	vsprintf(buffer, err, argptr);
	va_end(argptr);

	if (strict_errors)
		DDF_Error("%s", buffer);
	else
		DDF_Warning("%s", buffer);
}


void DDF_SetWhere(const std::string& dir)
{
	ddf_where = dir;
}


class define_c
{
public:
	// Note: these pointers only point inside the currently
	//       loaded memfile.  Hence there is no need to
	//       explicitly free them.
	char *name;
	char *value;

public:
	define_c() : name(NULL), value(NULL)
	{ }

	define_c(char *_N, char *_V) : name(_N), value(_V)
	{ }

	~define_c()
	{ }
};

// -AJA- 1999/09/12: Made these static.  The variable 'defines' was
//       clashing with the one in rad_trig.c -- Ugh.

static std::vector<define_c> defines;

static void DDF_MainAddDefine(char *name, char *value)
{
	for (int i = 0; i < (int)defines.size(); i++)
	{
		if (stricmp(defines[i].name, name) == 0)
		{
			DDF_Error("Redefinition of '%s'\n", name);
			return;
		}
	}

	defines.push_back(define_c(name, value));
}

static const char *DDF_MainGetDefine(const char *name)
{
	for (int i = 0; i < (int)defines.size(); i++)
	{
		if (stricmp(defines[i].name, name) == 0)
			return defines[i].value;
	}
	return name; // un-defined.
}


static void DDF_ParseVersion(const char *str, int len)
{
	if (len <= 0 || ! isspace(*str))
		DDF_Error("Badly formed #VERSION directive.\n");
	
	for (; isspace(*str) && len > 0; str++, len--)
	{ }

	if (len < 4 || ! isdigit(str[0]) || str[1] != '.' ||
		! isdigit(str[2]) || ! isdigit(str[3]))
	{
		DDF_Error("Badly formed #VERSION directive.\n");
	}

	int ddf_version = ((str[0]-'0') * 100) +
	                  ((str[2]-'0') *  10) +
			           (str[3]-'0');

	if (ddf_version < 123)
		DDF_Error("Illegal #VERSION number: %s\n", str);

	if (ddf_version > engine_version)
		DDF_Error("This version of EDGE cannot handle this DDF.\n");
}


//
// Description of the DDF Parser:
//
// The DDF Parser is a simple reader that is very limited in error checking,
// however it can adapt to most tasks, as is required for the variety of stuff
// need to be loaded in order to configure the EDGE Engine.
//
// The parser will read an ascii file, character by character an interpret each
// depending in which mode it is in; Unless an error is encountered or a called
// procedure stops the parser, it will read everything until EOF is encountered.
//
// When the parser function is called, a pointer to a readinfo_t is passed and
// contains all the info needed, it contains:
//
// * filename              - filename to be read, returns error if NULL
// * DDF_MainCheckName     - function called when a def has been just been started
// * DDF_MainCheckCmd      - function called when we need to check a command
// * DDF_MainCreateEntry   - function called when a def has been completed
// * DDF_MainFinishingCode - function called when EOF is read
// * currentcmdlist        - Current list of commands
//
// Also when commands are referenced, they use currentcmdlist, which is a pointer
// to a list of entries, the entries are formatted like this:
//
// * name - name of command
// * routine - function called to interpret info
// * numeric - void pointer to an value (possibly used by routine)
//
// name is compared with the read command, to see if it matchs.
// routine called to interpret info, if command name matches read command.
// numeric is used if a numeric value needs to be changed, by routine.
//
// The different parser modes are:
//  waiting_newdef
//  reading_newdef
//  reading_command
//  reading_data
//  reading_remark
//  reading_string
//
// 'waiting_newdef' is only set at the start of the code, At this point every
// character with the exception of DEFSTART is ignored. When DEFSTART is
// encounted, the parser will switch to reading_newdef. DEFSTART the parser
// will only switches modes and sets firstgo to false.
//
// 'reading_newdef' reads all alphanumeric characters and the '_' character - which
// substitudes for a space character (whitespace is ignored) - until DEFSTOP is read.
// DEFSTOP passes the read string to DDF_MainCheckName and then clears the string.
// Mode reading_command is now set. All read stuff is passed to char *buffer.
//
// 'reading_command' picks out all the alphabetic characters and passes them to
// buffer as soon as COMMANDREAD is encountered; DDF_MainReadCmd looks through
// for a matching command, if none is found a fatal error is returned. If a matching
// command is found, this function returns a command reference number to command ref
// and sets the mode to reading_data. if DEFSTART is encountered the procedure will
// clear the buffer, run DDF_MainCreateEntry (called this as it reflects that in Items
// & Scenery if starts a new mobj type, in truth it can do anything procedure wise) and
// then switch mode to reading_newdef.
//
// 'reading_data' passes alphanumeric characters, plus a few other characters that
// are also needed. It continues to feed buffer until a SEPARATOR or a TERMINATOR is
// found. The difference between SEPARATOR and TERMINATOR is that a TERMINATOR refs
// the cmdlist to find the routine to use and then sets the mode to reading_command,
// whereas SEPARATOR refs the cmdlist to find the routine and a looks for more data
// on the same command. This is how the multiple states and specials are defined.
//
// 'reading_remark' does not process any chars except REMARKSTOP, everything else is
// ignored. This mode is only set when REMARKSTART is found, when this happens the
// current mode is held in formerstatus, which is restored when REMARKSTOP is found.
//
// 'reading_string' is set when the parser is going through data (reading_data) and
// encounters STRINGSTART and only stops on a STRINGSTOP. When reading_string,
// everything that is an ASCII char is read (which the exception of STRINGSTOP) and
// passed to the buffer. REMARKS are ignored in when reading_string and the case is
// take notice of here.
//
// The maximum size of BUFFER is set in the BUFFERSIZE define.
//
// DDF_MainReadFile & DDF_MainProcessChar handle the main processing of the file, all
// the procedures in the other DDF files (which the exceptions of the Inits) are
// called directly or indirectly. DDF_MainReadFile handles to opening, closing and
// calling of procedures, DDF_MainProcessChar makes sense from the character read
// from the file.
//

#define SPRITE_CHARS  "_-:.[]\\!#%+@?"

//
// 1998/08/10 Added String reading code.
//
static readchar_t DDF_MainProcessString(char ch, std::string& token)
{
	// -ACB- 1998/08/11 Used for detecting formatting in a string
	static bool formatchar = false;

	if (!formatchar && ch == '\\')
	{
		formatchar = true;
		return nothing;
	}

	// -ACB- 1998/08/10 New string handling
	// -KM- 1999/01/29 Fixed nasty bug where \" would be recognised as
	//  string end quote mark.  One of the level text used this.
	if (formatchar)
	{
		formatchar = false;

		// -ACB- 1998/08/11 Formatting check: Carriage-return.
		if (ch == 'n')
		{
			token += '\n';
			return ok_char;
		}
		else if (ch == '\"')    // -KM- 1998/10/29 Also recognise quote
		{
			token += '\"';
			return ok_char;
		}
		else if (ch == '\\') // -ACB- 1999/11/24 Double backslash means directory
		{
			token += '\\';
			return ok_char;
		}

		// -ACB- 1999/11/24 Any other characters are treated in the norm
		token += ch;
		return ok_char;
	}
	else if (ch == '\"')
	{
		return string_stop;
	}
	else if (ch == '\n')
	{
		cur_ddf_line_num--;
		DDF_WarnError("Unclosed string detected.\n");

		cur_ddf_line_num++;
		return nothing;
	}
	// -KM- 1998/10/29 Removed ascii check, allow foreign characters (?)
	// -ES- HEY! Swedish is not foreign!

	token += ch;
	return ok_char;
}


static readchar_t DDF_MainProcessChar(char ch, std::string& token, int status)
{
	if (status == reading_string)
		return DDF_MainProcessString(ch, token);

	// With the exception of reading_string, whitespace is ignored.
	if (isspace(ch))
		return nothing;

	// -AJA- 1999/09/26: Handle unmatched '}' better.
	if (status == reading_remark)
	{
		if (ch == '}')
			return remark_stop;

		return nothing;
	}

	if (ch == '{')
		return remark_start;

	if (ch == '}')
		DDF_Error("DDF: Encountered '}' without previous '{'.\n");
	

	switch (status)
	{
		case waiting_newdef:
			if (ch == '[')
				return def_start;
			else
				return nothing;

		case reading_newdef:
			if (ch == ']')
			{
				return def_stop;
			}
			else if (isalnum(ch) || ch == '_' || ch == ':' || ch == '+')
			{
				token += toupper(ch);
				return ok_char;
			}
			return nothing;

		case reading_command:
			if (ch == '=')
			{
				return command_read;
			}
			else if (ch == ';')
			{
				return property_read;
			}
			else if (ch == '[')
			{
				return def_start;
			}
			else if (isalnum(ch) || ch == '_' || ch == '(' || ch == ')' || ch == '.')
			{
				token += toupper(ch);
				return ok_char;
			}
			return nothing;

			// -ACB- 1998/08/10 Check for string start
		case reading_data:
			if (ch == '\"')
				return string_start;
      
			if (ch == ';')
				return terminator;
      
			if (ch == ',')
				return separator;
      
			if (ch == '(')
			{
				token += ch;
				return group_start;
			}
      
			if (ch == ')')
			{
				token += ch;
				return group_stop;
			}
      
			// Sprite Data - more than a few exceptions....
			if (isalnum(ch) || strchr(SPRITE_CHARS, ch))
			{
				token += toupper(ch);
				return ok_char;
			}

			if (isprint(ch))
				DDF_WarnError("DDF: Illegal character '%c' found.\n", ch);
			break;

		default:  // doh!
			I_Error("DDF_MainProcessChar: Bad status value %d !\n", status);
			break;
	}

	return nothing;
}


static char * DDF_MainProcessNewLine(readinfo_t *readinfo, char *pos, bool seen_entry)
{
	// -AJA- 2000/03/21: determine linedata
	int len = 0;

	while (pos[len] && pos[len] != '\n' && pos[len] != '\r')
		 len++;

	cur_ddf_linedata = std::string(pos, len);

	// -AJA- 2001/05/21: handle directives (lines beginning with #).
	// This code is more hackitude -- to be fixed when the whole
	// parsing code gets the overhaul it needs.

	while (*pos == ' ' || *pos == '\t')
		pos++;

	// -KM- 1998/12/16 Added #define command to ddf files.
	if (strnicmp(pos, "#DEFINE", 7) == 0)
	{
		bool line = false;

		pos += 8;
		char *name = pos;

		while (*pos && *pos != ' ')
			pos++;

		if (! *pos)
			DDF_Error("#DEFINE used without a value\n");

		*pos++ = 0;
		char *value = pos;

		while (*pos)
		{
			if (*pos == '\r')
				*pos = ' ';
			
			if (*pos == '\\')
				line = true;

			if (*pos == '\n' && !line)
				break;
			pos++;
		}

		if (*pos == '\n')
			cur_ddf_line_num++;

		*pos++ = 0;

		DDF_MainAddDefine(name, value);

		return pos;
	}

	if (strnicmp(pos, "#CLEARALL", 9) == 0)
	{
		if (seen_entry)
			DDF_Error("#CLEARALL cannot be used inside an entry !\n");

		(* readinfo->clear_all)();

		return pos + len;
	}

	if (strnicmp(pos, "#VERSION", 8) == 0)
	{
		if (seen_entry)
			DDF_Error("#VERSION cannot be used inside an entry !\n");

		DDF_ParseVersion(pos + 8, len - 8);

		return pos + len;
	}

	return NULL;  // situation normal
}


//
// DDF_MainReadFile
//
// -ACB- 1998/08/10 Added the string reading code
// -ACB- 1998/09/28 DDF_ReadFunction Localised here
// -AJA- 1999/10/02 Recursive { } comments.
// -ES- 2000/02/29 Added
//
void DDF_MainReadFile(readinfo_t * readinfo, char *pos)
{
	std::string token;
	std::string current_cmd;

	int status = waiting_newdef;
	int formerstatus = readstatus_invalid;

	int current_index = 0;
	int comment_level = 0;
	int bracket_level = 0;

	bool seen_entry = false;
	bool seen_command = false;
	bool extending;

	DDF_MainProcessNewLine(readinfo, pos, seen_entry);

	while (* pos)
	{
		// -AJA- 1999/10/27: detect // comments and ignore them
		if (pos[0] == '/' && pos[1] == '/' &&
			comment_level == 0 && status != reading_string)
		{
			while (*pos && *pos != '\n')
				pos++;

			if (! *pos)
				break;

			// fall through to '\n' handling code...
		}
    
		char ch = *pos++;

		if (ch == '\n')
		{
			char *new_pos = DDF_MainProcessNewLine(readinfo, pos, seen_entry);

			cur_ddf_line_num++;

			if (new_pos)
			{
				pos = new_pos;
				continue;
			}
		}

		int response = DDF_MainProcessChar(ch, token, status);

		switch (response)
		{
			case ok_char:
				break;

			case remark_start:
				if (comment_level == 0)
				{
					formerstatus = status;
					status = reading_remark;
				}
				comment_level++;
				break;

			case remark_stop:
				comment_level--;
				if (comment_level == 0)
				{
					status = formerstatus;
				}
				break;

			// -AJA- 2000/10/02: support for () brackets
			case group_start:
				if (status == reading_data || status == reading_command)
					bracket_level++;
				break;

			case group_stop:
				if (status == reading_data || status == reading_command)
				{
					bracket_level--;
					if (bracket_level < 0)
						DDF_Error("Unexpected ')' bracket.\n");
				}
				break;

			// -ACB- 1998/08/10 String Handling
			case string_start:
				status = reading_string;
				break;

			// -ACB- 1998/08/10 String Handling
			case string_stop:
				status = reading_data;
				break;

			case def_start:
				if (bracket_level > 0)
					DDF_Error("Unclosed () brackets detected.\n");
         
				if (seen_entry)
				{
					cur_ddf_linedata.clear();

					// finish off previous entry
					(* readinfo->finish_entry)();
				}

				token.clear();
				cur_ddf_entryname.clear();

				seen_entry = true;
				status = reading_newdef;
				break;

			case def_stop:
				cur_ddf_entryname = epi::STR_Format("[%s]", token.c_str());

				extending = false; // TODO

				(* readinfo->start_entry)(token.c_str(), extending);
         
				token.clear();
				seen_command = false;
				status = reading_command;
				break;

			case command_read:
				if (! token.empty())
					current_cmd = token.c_str();
				else
					current_cmd.clear();

				token.clear();
				current_index = 0;
				status = reading_data;
				break;

			case property_read:
				DDF_WarnError("Badly formed command: expected '='\n");
				break;

			case separator:
				if (bracket_level > 0)
				{
					token += ',';
					break;
				}

				if (current_cmd.empty())
					DDF_Error("Unexpected comma ','.\n");

				if (! seen_entry)
					DDF_WarnError("Command %s used outside of any entry\n",
								   current_cmd.c_str());
				else
				{ 
					(* readinfo->parse_field)(current_cmd.c_str(), 
						  DDF_MainGetDefine(token.c_str()), current_index, false);

					current_index++;
				}

				token.clear();
				break;

			case terminator:
				if (current_cmd.empty())
					DDF_Error("Unexpected semicolon ';'.\n");

				if (bracket_level > 0)
					DDF_Error("Missing ')' bracket in ddf command.\n");

				(* readinfo->parse_field)(current_cmd.c_str(), 
					  DDF_MainGetDefine(token.c_str()), current_index, true);

				token.clear();
				seen_command = true;
				status = reading_command;
				break;

			case nothing:
			default:
				break;
		}
	}

	current_cmd.clear();
	cur_ddf_linedata.clear();

	// -AJA- 1999/10/21: check for unclosed comments
	if (comment_level > 0)
		DDF_Error("Unclosed comments detected.\n");

	if (bracket_level > 0)
		DDF_Error("Unclosed () brackets detected.\n");

	if (status == reading_newdef)
		DDF_Error("Unclosed [] brackets detected.\n");
	
	if (status == reading_data || status == reading_string)
		DDF_WarnError("Unfinished DDF command on last line.\n");

	// make sure we finish any in-progress entry
	if (seen_entry)
		(* readinfo->finish_entry)();

	defines.clear();

	cur_ddf_entryname.clear();
	cur_ddf_filename.clear();
	cur_ddf_linedata.clear();
}


//
// Check if the command exists, and call the parser function if it
// does (and return true), otherwise return false.
//
bool DDF_MainParseField(char *object, const commandlist_t *commands, 
						const char *field, const char *value)
{
	for (int i=0; commands[i].name; i++)
	{
		bool obsolete = false;

		const char *name = commands[i].name;

		if (name[0] == '!')
		{
			name++;
			obsolete = true;
		}
    
		// handle subfields
		if (name[0] == '*')
		{
			name++;

			int len = strlen(name);
			SYS_ASSERT(len > 0);

			if (strncmp(field, name, len) == 0 && field[len] == '.' && 
				isalnum(field[len+1]))
			{
				// recursively parse the sub-field
				return DDF_MainParseField(object + commands[i].offset,
							commands[i].sub_comms,
							field + len + 1, value);
			}
      
			continue;
		}

		if (DDF_CompareName(field, name) != 0)
			continue;

		// found it, so call parse routine
		if (obsolete)
			DDF_WarnError("The ddf %s command is obsolete !\n", name);

		SYS_ASSERT(commands[i].parse_command);

		(* commands[i].parse_command)(value, object + commands[i].offset);

		return true;
	}

	return false;
}


//----------------------------------------------------------------------------


extern readinfo_t anim_readinfo;
extern readinfo_t attack_readinfo;
extern readinfo_t colormap_readinfo;
extern readinfo_t font_readinfo;
extern readinfo_t game_readinfo;
extern readinfo_t image_readinfo;
extern readinfo_t language_readinfo;
extern readinfo_t level_readinfo;
extern readinfo_t line_readinfo;
extern readinfo_t playlist_readinfo;
extern readinfo_t sector_readinfo;
extern readinfo_t sound_readinfo;
extern readinfo_t switch_readinfo;
extern readinfo_t thing_readinfo;
extern readinfo_t weapon_readinfo;


static readinfo_t * all_readinfos[] =
{
	&anim_readinfo,
	&attack_readinfo,
	&colormap_readinfo,
	&font_readinfo,
	&game_readinfo,
	&image_readinfo,
	&language_readinfo,
	&level_readinfo,
	&line_readinfo,
	&playlist_readinfo,
	&sector_readinfo,
	&sound_readinfo,
	&switch_readinfo,
	&thing_readinfo,
	&weapon_readinfo,

	NULL  // end of list
};


void DDF_ParseSection(char *buffer)
{
	SYS_ASSERT(*buffer == '<');
	buffer++;

	char *memfileptr = strchr(buffer, '>');
	*memfileptr++ = 0;

	I_Debugf("DDF_ParseSection : <%s>\n", buffer);

///	while (*memfileptr && *memfileptr != '\n')
///		memfileptr++;
///	while (*memfileptr == '\n' || *memfileptr == '\r')
///		memfileptr++;
	
	// find the readinfo for the tag
	readinfo_t *info = NULL;

	for (int i = 0; all_readinfos[i]; i++)
	{
		info = all_readinfos[i];

		if (stricmp(buffer, info->tag) == 0)
			break;

		info = NULL;
	}

	if (info)
		DDF_MainReadFile(info, memfileptr);
	else
		DDF_Error("Unknown DDF tag: <%s>\n", buffer);
}


static char *FindTag(char *pos, bool skip_to_eol, int *line_num)
{
	while (*pos)
	{
		if (skip_to_eol)
		{
			skip_to_eol = false;

			while (*pos && *pos != '\n')
				pos++;

			if (*pos)
			{
				pos++; *line_num += 1;
			}
		}

		// the format must be <LETTERS> with the '<' at the
		// start of the line, and containing no spaces.

		if (*pos != '<')
		{
			skip_to_eol = true;
			continue;
		}

		char *found = pos;

		pos++;
		while (*pos && isalpha(*pos))
			pos++;

		if (*pos != '>')
		{
			// no match
			skip_to_eol = true;
			continue;
		}

		return found;
	}

	return NULL;
}


void DDF_Parse(void *data, int length, const char *name)
{
	// NOTE WELL: we assume buffer is NUL terminated !
	
	// this function chops the text buffer into sections, where
	// each section begins with a tag of the form: <XXXX>.

	cur_ddf_line_num = 1;
	cur_ddf_filename = std::string(name);

	char *pos = FindTag((char *) data, false, &cur_ddf_line_num);
	if (! pos)
		return;

	for (;;)
	{
		int ln_next = cur_ddf_line_num;
		char *p_next = FindTag(pos+1, true, &ln_next);

		// NUL terminate the section beginning at 'pos'
		if (p_next)
			p_next[-1] = 0;

		DDF_ParseSection(pos);

		if (! p_next)
			break;

		pos = p_next;
		cur_ddf_line_num = ln_next;
	}
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
