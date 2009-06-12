
char *PF_VarString (int	first)
{
	int		i;
	static char out[256];
	
	out[0] = 0;
	for (i=first ; i<pr_argc ; i++)
	{
		strcat (out, G_STRING((OFS_PARM0+i*3)));
	}
	return out;
}


static void PF_Dummy(void)
{
    Error("unimplemented builtin");
}

static void PF_Print(void)
{
    printf("%s\n", PF_VarString(0));
}

static void PF_PrintNum(void)
{
    printf("%1.5f\n", G_FLOAT(OFS_PARM0+0));
}


builtin_t pr_builtin[] =
{
  PF_Dummy,
  
  PF_Print,
  PF_PrintNum,
};

builtin_t *pr_builtins = pr_builtin;
int pr_numbuiltins = sizeof(pr_builtin) / sizeof(pr_builtin[0]);

