
static void PF_Dummy(void)
{
    PR_RunError("unimplemented builtin");
}

static void PF_PrintStr(void)
{
	double * p = PR_Parameter(0);

    printf("%s\n", strings + (int)*p);
}

static void PF_PrintNum(void)
{
	double * p = PR_Parameter(0);

    printf("%1.5f\n", *p);
}

static void PF_PrintVector(void)
{
	double * vec = PR_Parameter(0);

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
