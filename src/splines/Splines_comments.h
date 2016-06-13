///C_MAIN.C
#if 0
// cmaster.matso's camera info code
cvar_t	*cl_camerainfo;

// cmaster.matso
extern qboolean loadCamera(int camNum, const char *name);
extern qboolean saveCamera(int camNum, const char *name);
extern void startCamera(int camNum, int time);
extern void togglePause(int camNum);
extern void toggleTracking(int camNum);
extern void toggleFollowing(int camNum);
extern void stopCamera(int camNum);
extern void setCurrentCam(int camNum);
extern qboolean loadCameraTrigger(int trigNum, const char *name);
extern void toggleCameraTrigger(int trigNum);
// cmaster.matso: END


// cmaster.matso
/*
=================
CL_PrintCameraInfo_f

Prints current camera information.
=================
*/
void CL_PrintCameraInfo_f (void)
{
	vec3_t origin;
	origin[0] = cl.refdef.vieworg[0];
	origin[1] = cl.refdef.vieworg[1];
	origin[2] = cl.refdef.vieworg[2];
	Com_Printf("Currnet camera position: {%g, %g, %g}\n", origin[0], origin[1], origin[2]);
}

/*
=================
CL_ToggleCameraInfoDraw_f

Toggles drawing camera information on the screen.
=================
*/
void CL_ToggleCameraInfoDraw_f (void)
{
	r_speeds->value = !r_speeds->value;
	cl_camerainfo->value = !cl_camerainfo->value;
}

/*
=================
CL_ScriptCameraLoad_f

Loads scripted camera from the file given as commend argument.
=================
*/
void CL_ScriptCameraLoad_f (void)
{
	int camNum = atoi(Cmd_Argv(1));
	const char *cameraName = Cmd_Argv(2);
	if (!strlen(cameraName)) {
		Com_Printf("usage: %s {0-15} [scripted-camera-filename]\n", Cmd_Argv(0));
	} else {
		Com_Printf("loading scripted camera...");
		if (!loadCamera(camNum, cameraName)) Com_Printf("'%s' camera load failed!\n", cameraName);
		else Com_Printf("Done!\n");
	}
}

/*
=================
CL_ScriptCameraSave_f
=================
*/
//void CL_ScriptCameraSave_f (void)
//{
//	const char *cameraName = Cmd_Argv(1);
//	if (!strlen(cameraName)) {
//		Com_Printf("usage: %s [scripted-camera-filename]\n", Cmd_Argv(0));
//	} else {
//		Com_Printf("saving scripted camera...");
//		saveCamera(cameraName);
//		Com_Printf("Done!\n");
//	}
//}

/*
=================
CL_ScriptCameraStart_f

Starts scripted camera.
=================
*/
void CL_ScriptCameraStart_f (void)
{
	int camNum = atoi(Cmd_Argv(1));
	startCamera(camNum, getCurrentRendererTime());
}

/*
=================
CL_ScriptCameraTogglePause_f

Toggles scripted camera pause.
=================
*/
void CL_ScriptCameraTogglePause_f (void)
{
	int camNum = atoi(Cmd_Argv(1));
	togglePause(camNum);
}

/*
=================
CL_ScriptCameraStart_f

Stops scripted camera.
=================
*/
void CL_ScriptCameraStop_f (void)
{
	int camNum = atoi(Cmd_Argv(1));
	stopCamera(camNum);
}

/*
=================
CL_ScriptCameraSwitchTo_f

Switches current scripted camera to the one of given index.
=================
*/
void CL_ScriptCameraSwitchTo_f (void)
{
	int camNum = atoi(Cmd_Argv(1));
	setCurrentCam(camNum);
}

/*
=================
CL_ScriptCameraToggleTracking_f

Toggle camera tracking mode.
=================
*/
void CL_ScriptCameraToggleTracking_f (void)
{
	int camNum = atoi(Cmd_Argv(1));
	toggleTracking(camNum);
}

/*
=================
CL_ScriptCameraToggleFollowing_f

Toggle camera following mode.
=================
*/
void CL_ScriptCameraToggleFollowing_f (void)
{
	int camNum = atoi(Cmd_Argv(1));
	toggleFollowing(camNum);
}

/*
=================
CL_ScriptCameraLoadTrigger_f

Loads a scripted camera trigger.
=================
*/
void CL_ScriptCameraLoadTrigger_f (void)
{
	int trigNum = atoi(Cmd_Argv(1));
	const char *trigName = Cmd_Argv(2);
	if (!strlen(trigName)) {
		Com_Printf("usage: %s {0-15} [scripted-camera-trigger-filename]\n", Cmd_Argv(0));
	} else {
		Com_Printf("loading scripted camera trigger...");
		if (!loadCameraTrigger(trigNum, trigName)) Com_Printf("'%s' trigger load failed!\n", trigName);
		else Com_Printf("Done!\n");
	}
}

/*
=================
CL_ScriptCameraToggleTriggering_f

Toggle scripted camera trigger of index provided.
=================
*/
void CL_ScriptCameraToggleTriggering_f (void)
{
	int trigNum = atoi(Cmd_Argv(1));
	toggleCameraTrigger(trigNum);
}

// cmaster.matso: END

	// cmaster.matso
	//
	// camera info commands
	//
	Cmd_AddCommand ("caminfo", CL_PrintCameraInfo_f);
	Cmd_AddCommand ("tgcaminfo", CL_ToggleCameraInfoDraw_f);

	cl_camerainfo = Cvar_Get ("cl_camerainfo", "0", 0);

	//
	// scripted camera commands
	//
	Cmd_AddCommand ("scload", CL_ScriptCameraLoad_f);
	//Cmd_AddCommand ("scsave", CL_ScriptCameraSave_f);
	Cmd_AddCommand ("scstart", CL_ScriptCameraStart_f);
	Cmd_AddCommand ("scpause", CL_ScriptCameraTogglePause_f);
	Cmd_AddCommand ("sctrack", CL_ScriptCameraToggleTracking_f);
	Cmd_AddCommand ("scfollow", CL_ScriptCameraToggleFollowing_f);
	Cmd_AddCommand ("scplay", CL_ScriptCameraTogglePause_f);
	Cmd_AddCommand ("scstop", CL_ScriptCameraStop_f);
	Cmd_AddCommand ("scswitch", CL_ScriptCameraSwitchTo_f);
	Cmd_AddCommand ("sctgload", CL_ScriptCameraLoadTrigger_f);
	Cmd_AddCommand ("sctrigger", CL_ScriptCameraToggleTriggering_f);
	// cmaster.matso: END
====================================================================================================================================================================================================================================
====================================================================================================================================================================================================================================
====================================================================================================================================================================================================================================
====================================================================================================================================================================================================================================
====================================================================================================================================================================================================================================
====================================================================================================================================================================================================================================
#endif

///CL_VIEW.C
#if 0
		// cmaster.matso: Camera modes toggler
		V_ToggleCameraMode(&cl.refdef);
		// cmaster.matso: END
		void V_Viewpos_f (void)// cmaster.matso: Interesting!
#endif

///ref.h
#if 0
// cmaster.matso
extern qboolean getCameraInfo(int camNum, int time, vec3_t *origin, vec3_t *angles, float *fov);
extern qboolean getCameraInfoTarget(int camNum, int time, vec3_t *target, vec3_t *origin, vec3_t *angles, float *fov);
extern int getCurrentCamera(vec3_t *target);
extern long getCurrentRendererTime(void);

extern vec3_t player_position;

// camera info code
extern  cvar_t	*r_speeds;
extern  cvar_t	*cl_camerainfo;
extern  refdef_t	r_newrefdef;
// cmaster.matso: END
#endif

///R_MAIN.C
#if 0
/////////////////////////////////////////////////////////////////////////////
// cmaster.matso
long getCurrentRendererTime(void) {
	return (long) (r_newrefdef.time * 1000);
}
// cmaster.matso: END

================
R_ShowSpeeds

Knightmare- draws r_speeds (modified from Echon's tutorial)
================
*/
void R_ShowSpeeds (refdef_t *fd)
{
	char	buf[128];
	int		i, /*j,*/ lines, x, y, n = 0;
	float	textSize, textScale;
	
	vec3_t	origin, angles, dir;
	float time = (float) getCurrentRendererTime();
	float fov, speed = 0.0f;

	static vec3_t last_pos;
	static float last_time;

	if (!r_speeds->value || r_newrefdef.rdflags & RDF_NOWORLDMODEL)	// don't do this for options menu
		return;

	// cmaster.matso: draw only if the camera info flag is set
	if (cl_camerainfo->value) {// NOTE: why the values are lower then expected?...
		fov = fd->fov_y;
		angles[0] = fd->viewangles[0];
		angles[1] = fd->viewangles[1];
		origin[0] = fd->vieworg[0];
		origin[1] = fd->vieworg[1];
		origin[2] = fd->vieworg[2];

		// caluculating current camera speed
		dir[0] = origin[0] - last_pos[0];
		dir[1] = origin[1] - last_pos[1];
		dir[2] = origin[2] - last_pos[2];
		
		speed = VectorLength(dir) / (time - last_time);

		last_pos[0] = origin[0];
		last_pos[1] = origin[1];
		last_pos[2] = origin[2];
		last_time = time;

		lines = 5;
	}
	else lines = 5; // 7

	textScale = min( ((float)r_newrefdef.width / SCREEN_WIDTH), ((float)r_newrefdef.height / SCREEN_HEIGHT) );
	textSize = 8.0 * textScale;

	for (i=0; i<lines; i++)
	{
		if (!cl_camerainfo->value) {
			switch (i) {
				case 0:	n = sprintf (buf, "%5i wcall", c_brush_calls); break;
				case 1:	n = sprintf (buf, "%5i wsurf", c_brush_surfs); break;
				case 2:	n = sprintf (buf, "%5i wpoly", c_brush_polys); break;
				case 3: n = sprintf (buf, "%5i epoly", c_alias_polys); break;
				case 4: n = sprintf (buf, "%5i ppoly", c_part_polys); break;
			//	case 5: n = sprintf (buf, "%5i tex  ", c_visible_textures); break;
			//	case 6: n = sprintf (buf, "%5i lmaps", c_visible_lightmaps); break;
				default: break;
			}
		} else {
			VectorNormalize(dir);
			switch (i) {
				case 0: n = sprintf (buf, "speed: %2.3f", speed * 10); break;	// speed is showed 10x bigger then calculated
				case 1: n = sprintf (buf, "view: {%2.3f, %2.3f}", -angles[0], angles[1]); break;
				case 2: n = sprintf (buf, "position: {%2.3f, %2.3f, %2.3f}", origin[0], origin[1], origin[2]); break;
				case 3: n = sprintf (buf, "direction: {%2.3f, %2.3f, %2.3f}", dir[0], dir[1], dir[2]); break;
				case 4: n = sprintf (buf, "fov: %2.3f", fov); break;
				default: break;
			}
		}
		if (scr_netgraph_pos->value)
			x = r_newrefdef.width - (n*textSize + textSize*0.5);
		else
			x = textSize*0.5;
		y = r_newrefdef.height-(lines-i)*(textSize+2);

		R_DrawString (x, y, buf, FONT_SCREEN, textScale, 255, 255, 255, 255, false, true);
	/*	for (j=0; j<n; j++)
			R_DrawChar ( (x + j*textSize + textSize*0.25), (y + textSize*0.125), 
					buf[j], FONT_SCREEN, textScale, 0, 0, 0, 255, false, false );
		for (j=0; j<n; j++)
			R_DrawChar ( (x + j*textSize), y,
					buf[j], FONT_SCREEN, textScale, 255, 255, 255, 255, false, (j==(n-1)) );
	*/
	}
}
#endif

