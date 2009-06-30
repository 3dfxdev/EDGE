
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

static void PF_PrintStr(void)
{
    printf("%s\n", PF_VarString(0));
}

static void PF_PrintNum(void)
{
    printf("%1.5f\n", G_FLOAT(OFS_PARM0+0));
}

static void PF_PrintVector(void)
{
	double *vec = G_VECTOR(OFS_PARM0);

    printf("'%1.3f %1.3f %1.3f'\n", vec[0], vec[1], vec[2]);
}


builtin_t pr_builtin[] =
{
  PF_Dummy,

  PF_PrintStr,
  PF_PrintNum,
  PF_PrintVector,
};

builtin_t *pr_builtins = pr_builtin;
int pr_numbuiltins = sizeof(pr_builtin) / sizeof(pr_builtin[0]);


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
