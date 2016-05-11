e_input:

extern int I_JoyGetAxis(int n);


int joy_axis[6] = { 0, 0, 0, 0, 0, 0 };

static int joy_last_raw[6];

// The last one is ignored (AXIS_DISABLE)
static float ball_deltas[6] = {0, 0, 0, 0, 0, 0};
static float  joy_forces[6] = {0, 0, 0, 0, 0, 0};


cvar_c joy_dead;
cvar_c joy_peak;
cvar_c joy_tuning;

cvar_c debug_joyaxis;

float JoyAxisFromRaw(int raw)
{
	SYS_ASSERT(abs(raw) <= 32768);

	float v = raw / 32768.0f;
	
	if (fabs(v) <= joy_dead.f + 0.01)
		return 0;

	if (fabs(v) >= joy_peak.f - 0.01)
		return (v < 0) ? -1.0f : +1.0f;

	SYS_ASSERT(joy_peak.f > joy_dead.f);

	float t = CLAMP(0.2f, joy_tuning.f, 5.0f);

	if (v >= 0)
	{
		v = (v - joy_dead.f) / (joy_peak.f - joy_dead.f);
		return pow(v, 1.0f / t);
	}
	else
	{
		v = (-v - joy_dead.f) / (joy_peak.f - joy_dead.f);
		return - pow(v, 1.0f / t);
	}
}

static void UpdateJoyAxis(int n)
{
	if (joy_axis[n] == AXIS_DISABLE)
		return;

	int raw = I_JoyGetAxis(n);
	int old = joy_last_raw[n];

	joy_last_raw[n] = raw;

	// cooked value = average of last two raw samples
	int cooked = (raw + old) >> 1;

	float force = JoyAxisFromRaw(cooked);

	// perform inversion
	if ((joy_axis[n]+1) & 1)
		force = -force;

	if (debug_joyaxis.d == n+1)
	{
		I_Printf("Axis%d : raw %+05d --> %+7.3f\n", n+1, raw, force);
	}

	int axis = (joy_axis[n]+1) >> 1;

	joy_forces[axis] += force;
}


static inline void AddKeyForce(int axis, int upkeys, int downkeys, float qty = 1.0f)
{
	//let movement keys cancel each other out
	if (E_IsKeyPressed(upkeys))
	{
		///joy_forces[axis] += qty;
	}
	if (E_IsKeyPressed(downkeys))
	{
		///joy_forces[axis] -= qty;
	}
}

static void UpdateForces(void)
{
	for (int k = 0; k < 6; k++)
		joy_forces[k] = 0;

	// ---Keyboard---

	AddKeyForce(AXIS_TURN,    key_right,  key_left);
	AddKeyForce(AXIS_MLOOK,   key_lookup, key_lookdown);
	AddKeyForce(AXIS_FORWARD, key_up,     key_down);
	// -MH- 1998/08/18 Fly down
	AddKeyForce(AXIS_FLY,     key_flyup,  key_flydown);
	AddKeyForce(AXIS_STRAFE,  key_straferight, key_strafeleft);

	// ---Joystick---

	for (int j = 0; j < 6; j++)
		UpdateJoyAxis(j);
}



/* void I_ShowJoysticks(void)
{
	if (nojoy)
	{
		I_Printf("Joystick system is disabled.\n");
		return;
	}

	if (num_joys == 0)
	{
		I_Printf("No joysticks found.\n");
		return;
	}

	I_Printf("Joysticks:\n");

	for (int i = 0; i < num_joys; i++)
	{
		const char *name = SDL_JoystickName(i);
		if (! name)
			name = "(UNKNOWN)";

		I_Printf("  %2d : %s\n", i+1, name);
	}
} */


/* void I_OpenJoystick(int index)
{
	SYS_ASSERT(1 <= index && index <= num_joys);

	joy_info = SDL_JoystickOpen(index-1);
	if (! joy_info)
	{
		I_Printf("Unable to open joystick %d (SDL error)\n", index);
		return;
	}

	cur_joy = index;

	const char *name = SDL_JoystickName(cur_joy-1);
	if (! name)
		name = "(UNKNOWN)";

	joy_num_axes    = SDL_JoystickNumAxes(joy_info);
	joy_num_buttons = SDL_JoystickNumButtons(joy_info);
	joy_num_hats    = SDL_JoystickNumHats(joy_info);
	joy_num_balls   = SDL_JoystickNumBalls(joy_info);

	I_Printf("Opened joystick %d : %s\n", cur_joy, name);
	I_Printf("Axes:%d buttons:%d hats:%d balls:%d\n",
			 joy_num_axes, joy_num_buttons, joy_num_hats, joy_num_balls);
}
 */

/* void I_StartupJoystick(void)
{
	cur_joy = 0;

	if (M_CheckParm("-nojoy"))
	{
		I_Printf("I_StartupControl: Joystick system disabled.\n");
		nojoy = true;
		return;
	}

	if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) < 0)
	{
		I_Printf("I_StartupControl: Couldn't init SDL JOYSTICK!\n");
		nojoy = true;
		return;
	}

	SDL_JoystickEventState(SDL_ENABLE);

	num_joys = SDL_NumJoysticks();

	I_Printf("I_StartupControl: %d joysticks found.\n", num_joys);

	joystick_device = CLAMP(0, joystick_device, num_joys);

	if (num_joys == 0)
		return;

	if (joystick_device > 0)
		I_OpenJoystick(joystick_device);
}
 */

/* void CheckJoystickChanged(void)
{
	int new_joy = joystick_device;

	if (joystick_device < 0 || joystick_device > num_joys)
		new_joy = 0;

	if (new_joy == cur_joy)
		return;

	if (joy_info)
	{
		SDL_JoystickClose(joy_info);
		joy_info = NULL;

		I_Printf("Closed joystick %d\n", cur_joy);
		cur_joy = 0;
	}

	if (new_joy > 0)
	{
		I_OpenJoystick(new_joy);
	}
} */