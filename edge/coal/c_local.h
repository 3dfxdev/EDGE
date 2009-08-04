//----------------------------------------------------------------------
//  COAL LOCAL DEFS
//----------------------------------------------------------------------
// 
//  Copyright (C)      2009  Andrew Apted
//  Copyright (C) 1996-1997  Id Software, Inc.
//
//  Coal is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as
//  published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  Coal is distributed in the hope that it will be useful, but
//  WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
//  the GNU General Public License for more details.
//
//----------------------------------------------------------------------
//
//  Based on QCC (the Quake-C Compiler) and the corresponding
//  execution engine from the Quake source code.
//
//----------------------------------------------------------------------

#ifndef __COAL_LOCAL_DEFS_H__
#define __COAL_LOCAL_DEFS_H__

#include "c_memory.h"

typedef unsigned char byte;

typedef int	func_t;
typedef int	string_t;

typedef enum
{
	ev_INVALID = -1,

	ev_void = 0,
	ev_string,
	ev_float,
	ev_vector,
	ev_entity,
	ev_field,
	ev_function,
	ev_pointer
}
etype_t;


typedef struct
{
	short op;
	short line;  // offset from start of function

	int a, b, c;
}
statement_t;


enum
{
	OP_DONE,
	OP_DONE_V,

	OP_NOT_F,
	OP_NOT_V,
	OP_NOT_S,
	OP_NOT_FNC,

	OP_POWER_F,
	OP_MUL_F,
	OP_MUL_V,
	OP_MUL_FV,
	OP_MUL_VF,

	OP_DIV_F,
	OP_DIV_V,
	OP_MOD_F,

	OP_ADD_F,
	OP_ADD_V,
	OP_SUB_F,
	OP_SUB_V,

	OP_EQ_F,
	OP_EQ_V,
	OP_EQ_S,
	OP_EQ_FNC,

	OP_NE_F,
	OP_NE_V,
	OP_NE_S,
	OP_NE_FNC,

	OP_LE,
	OP_GE,
	OP_LT,
	OP_GT,

	OP_MOVE_F,
	OP_MOVE_V,
	OP_MOVE_S,
	OP_MOVE_FNC,

	OP_CALL,

	OP_IF,
	OP_IFNOT,

	OP_GOTO,
	OP_AND,
	OP_OR,

	OP_BITAND,
	OP_BITOR,

	OP_PARM_F,
	OP_PARM_V
};


#define	MAX_PARMS	16

typedef struct
{
	const char *name;

	// where it was defined (last)
	const char *source_file;
	int source_line;

	int		return_size;

	int		parm_num;
	short	parm_ofs[MAX_PARMS];
	short	parm_size[MAX_PARMS];

	int		locals_ofs;
	int		locals_size;
	int		locals_end;

	int		first_statement;	// negative numbers are builtins
}
function_t;

// offset in global data block (if > 0)
// when < 0, it is offset into local stack frame
typedef int	gofs_t;


#define	MAX_NAME		64


typedef struct type_s
{
	etype_t			type;

// function types are more complex
	struct type_s	*aux_type;	// return type or field type

	int				parm_num;	// -1 = variable args
	struct type_s	*parm_types[MAX_PARMS];	// only [parm_num] allocated

	struct type_s	*next;
}
type_t;

typedef struct def_s
{
	type_t		*type;
	char		*name;

	gofs_t		ofs;
	struct def_s	*scope;		// function the var was defined in, or NULL

	int			flags;

	struct def_s	*next;
}
def_t;

typedef enum
{
	DF_Initialized = (1 << 0),	// when a declaration included "= immediate"
	DF_Constant    = (1 << 1),
	DF_Temporary   = (1 << 2),
	DF_FreeTemp    = (1 << 3),  // temporary is free to re-use
}
def_flag_e;


#if 0  // prototyping.....
class scope_c
{
public:
	std::map<std::string, def_t *> defs;

public:
	 scope_c() : defs() { }
	~scope_c() { }
};

extern scope_c global_scope;
#endif


//=============================================================================

#define	MAX_DATA_PATH	64

#define	MAX_REGS		16384
#define	MAX_GLOBALS		16384
#define	MAX_FIELDS		1024
#define	MAX_STATEMENTS	65536
#define	MAX_FUNCTIONS	8192

#define OFS_NULL		0
#define OFS_RETURN		1
#define OFS_DEFAULT		4

#define RESERVED_OFS	10

extern	statement_t	statements[MAX_STATEMENTS];
extern	int			numstatements;

extern	function_t  functions[MAX_FUNCTIONS];
extern	int			numfunctions;

extern	double		pr_globals[MAX_REGS];
extern	int			numpr_globals;

#define REF_GLOBAL(ofs)  ((double *)global_mem.deref(ofs))
#define REF_STRING(ofs)  ((ofs)==0 ? "" : (char *)string_mem.deref(ofs))
#define REF_OP(ofs)      ((statement_t *)op_mem.deref(ofs))

#define	G_FLOAT(o) (pr_globals[o])
#define	G_VECTOR(o) (&pr_globals[o])
#define	G_STRING(o)  REF_STRING((int)pr_globals[o])
#define	G_FUNCTION(o) (pr_globals[o])

int	CopyString(char *str);


//=== COMPILER STUFF =========================================//

typedef struct
{
	const char *name;
	int op;  // OP_XXX
	int priority;
	type_t *type_a, *type_b, *type_c;
}
opcode_t;

extern const char *opcode_names[];

void PR_Lex(void);

type_t *PR_ParseType(void);
char *PR_ParseName(void);

bool PR_Check(char *string);
void PR_Expect(char *string);
void PR_ParseError(char *error, ...);


class parse_error_x
{
public:
	int foo;

	 parse_error_x() { }
	~parse_error_x() { }
};



//=== EXECUTION STUFF ========================================//


typedef void (*builtin_t) (void);
extern builtin_t *pr_builtins;
extern int pr_numbuiltins;

extern int pr_argc;



class exec_error_x
{
public:
	int foo;

	 exec_error_x() { }
	~exec_error_x() { }
};


char *PR_GetString(int num);

void PR_PrintDefs(void);

int PR_FindNativeFunc(const char *name);


class real_vm_c : public vm_c
{
public:
	/* API functions */

	 real_vm_c();
	~real_vm_c();

	void SetErrorFunc(print_func_t func);
	void SetPrintFunc(print_func_t func);

	void AddNativeModule(const char *name);
	void AddNativeFunction(const char *name, native_func_t func);

	bool CompileFile(char *buffer, const char *filename);
	void ShowStats();

	void SetTrace(bool enable);

	int FindFunction(const char *name);
	int FindVariable(const char *name);

	int Execute(int func_id);

	double     * AccessParam(int p);
	const char * AccessParamString(int p);

private:
	bmaster_c global_mem;
	bmaster_c string_mem;
	bmaster_c op_mem;

	// c_compile.cc
private:
	const char *source_file;
	int source_line;
	int function_line;

	void GLOB_Globals();
	void GLOB_Constant();
	void GLOB_Variable();
	void GLOB_Function();
	int  GLOB_FunctionBody(type_t *type, const char *func_name);

	void STAT_Statement(bool allow_def);
	void STAT_Assignment(def_t *e);
	void STAT_If_Else();
	void STAT_WhileLoop();
	void STAT_RepeatLoop();
	void STAT_Return();

	def_t * EXP_Expression(int priority, bool *lvalue = NULL);
	def_t * EXP_FieldQuery(def_t *e, bool lvalue);
	def_t * EXP_ShortCircuit(def_t *e, opcode_t *op);
	def_t * EXP_Term();
	def_t * EXP_VarValue();
	def_t * EXP_FunctionCall(def_t *func);
	def_t * EXP_Constant();

	def_t * GetDef (type_t *type, char *name, def_t *scope);
	def_t * FindDef(type_t *type, char *name, def_t *scope);

	void StoreConstant(int ofs);
	def_t * FindConstant();

	def_t * NewTemporary(type_t *type);
	void FreeTemporaries();

	def_t * NewGlobal(type_t *type);
	def_t * NewLocal(type_t *type);
	void DefaultValue(gofs_t ofs, type_t *type);

	char *   ParseName();
	type_t * ParseType();
	type_t * FindType(type_t *type);

	void EmitCode(short op, short a=0, short b=0, short c=0);


	void LEX_Next();
	void LEX_Whitespace();
	void LEX_NewLine();
	void LEX_SkipToSemicolon();

	bool LEX_Check (const char *str);
	void LEX_Expect(const char *str);

	void LEX_String();
	float LEX_Number();
	void LEX_Vector();
	void LEX_Name();
	void LEX_Punctuation();

	void CompileError(const char *error, ...);


	// c_execute.cc
private:
	bool trace;

	void DoExecute(int func_id);

	void EnterNative(function_t *newf, int result);

	int  EnterFunction(function_t *f, int result = 0);
	int  LeaveFunction(int *result);

	int	InternaliseString(const char *new_s);

	void RunError(const char *error, ...);

	void StackTrace();
	void PrintStatement(function_t *f, int s);
	const char * RegString(statement_t *st, int who);
};


#endif /* __COAL_LOCAL_DEFS_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
