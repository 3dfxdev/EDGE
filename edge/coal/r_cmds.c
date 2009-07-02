
char *PF_VarString (int	first)
{
	int		i;
	static char out[256];

	out[0] = 0;
	for (i=first ; i < pr_argc ; i++)
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
printf("STRING\n");
//    printf("%s\n", strings + (int)localstack[stack_base]);
}

static void PF_PrintNum(void)
{
printf("NUMBER\n");
//    printf("%1.5f\n", localstack[stack_base]); // FIXME
}

static void PF_PrintVector(void)
{
	printf("VECTOR\n");
//	double *vec = G_VECTOR(OFS_PARM0);

//   printf("'%1.3f %1.3f %1.3f'\n", vec[0], vec[1], vec[2]);
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
