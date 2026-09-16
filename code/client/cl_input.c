/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// cl.input.c  -- builds an intended movement command to send to the server

#include "client.h"

unsigned	frame_msec;
int			old_com_frameTime;

/*
===============================================================================

KEY BUTTONS

Continuous button event tracking is complicated by the fact that two different
input sources (say, mouse button 1 and the control key) can both press the
same button, but the button should only be released when both of the
pressing key have been released.

When a key event issues a button command (+forward, +attack, etc), it appends
its key number as argv(1) so it can be matched up with the release.

argv(2) will be set to the time the event happened, which allows exact
control even at low framerates when the down and up events may both get qued
at the same time.

===============================================================================
*/


kbutton_t	in_left, in_right, in_forward, in_back;
kbutton_t	in_lookup, in_lookdown, in_moveleft, in_moveright;
kbutton_t	in_strafe, in_speed;
kbutton_t	in_up, in_down;

#ifdef USE_VOIP
kbutton_t	in_voiprecord;
#endif

kbutton_t	in_buttons[16];


qboolean	in_mlooking;


void IN_MLookDown( void ) {
	in_mlooking = qtrue;
}

void IN_MLookUp( void ) {
	in_mlooking = qfalse;
	if ( !cl_freelook->integer ) {
		IN_CenterView ();
	}
}

void IN_KeyDown( kbutton_t *b ) {
	int		k;
	char	*c;
	
	c = Cmd_Argv(1);
	if ( c[0] ) {
		k = atoi(c);
	} else {
		k = -1;		// typed manually at the console for continuous down
	}

	if ( k == b->down[0] || k == b->down[1] ) {
		return;		// repeating key
	}
	
	if ( !b->down[0] ) {
		b->down[0] = k;
	} else if ( !b->down[1] ) {
		b->down[1] = k;
	} else {
		Com_Printf ("Three keys down for a button!\n");
		return;
	}
	
	if ( b->active ) {
		return;		// still down
	}

	// save timestamp for partial frame summing
	c = Cmd_Argv(2);
	b->downtime = atoi(c);

	b->active = qtrue;
	b->wasPressed = qtrue;
}

void IN_KeyUp( kbutton_t *b ) {
	int		k;
	char	*c;
	unsigned	uptime;

	c = Cmd_Argv(1);
	if ( c[0] ) {
		k = atoi(c);
	} else {
		// typed manually at the console, assume for unsticking, so clear all
		b->down[0] = b->down[1] = 0;
		b->active = qfalse;
		return;
	}

	if ( b->down[0] == k ) {
		b->down[0] = 0;
	} else if ( b->down[1] == k ) {
		b->down[1] = 0;
	} else {
		return;		// key up without coresponding down (menu pass through)
	}
	if ( b->down[0] || b->down[1] ) {
		return;		// some other key is still holding it down
	}

	b->active = qfalse;

	// save timestamp for partial frame summing
	c = Cmd_Argv(2);
	uptime = atoi(c);
	if ( uptime ) {
		b->msec += uptime - b->downtime;
	} else {
		b->msec += frame_msec / 2;
	}

	b->active = qfalse;
}



/*
===============
CL_KeyState

Returns the fraction of the frame that the key was down
===============
*/
float CL_KeyState( kbutton_t *key ) {
	float		val;
	int			msec;

	msec = key->msec;
	key->msec = 0;

	if ( key->active ) {
		// still down
		if ( !key->downtime ) {
			msec = com_frameTime;
		} else {
			msec += com_frameTime - key->downtime;
		}
		key->downtime = com_frameTime;
	}

#if 0
	if (msec) {
		Com_Printf ("%i ", msec);
	}
#endif

	val = (float)msec / frame_msec;
	if ( val < 0 ) {
		val = 0;
	}
	if ( val > 1 ) {
		val = 1;
	}

	return val;
}



void IN_UpDown(void) {IN_KeyDown(&in_up);}
void IN_UpUp(void) {IN_KeyUp(&in_up);}
void IN_DownDown(void) {IN_KeyDown(&in_down);}
void IN_DownUp(void) {IN_KeyUp(&in_down);}
void IN_LeftDown(void) {IN_KeyDown(&in_left);}
void IN_LeftUp(void) {IN_KeyUp(&in_left);}
void IN_RightDown(void) {IN_KeyDown(&in_right);}
void IN_RightUp(void) {IN_KeyUp(&in_right);}
void IN_ForwardDown(void) {IN_KeyDown(&in_forward);}
void IN_ForwardUp(void) {IN_KeyUp(&in_forward);}
void IN_BackDown(void) {IN_KeyDown(&in_back);}
void IN_BackUp(void) {IN_KeyUp(&in_back);}
void IN_LookupDown(void) {IN_KeyDown(&in_lookup);}
void IN_LookupUp(void) {IN_KeyUp(&in_lookup);}
void IN_LookdownDown(void) {IN_KeyDown(&in_lookdown);}
void IN_LookdownUp(void) {IN_KeyUp(&in_lookdown);}
void IN_MoveleftDown(void) {IN_KeyDown(&in_moveleft);}
void IN_MoveleftUp(void) {IN_KeyUp(&in_moveleft);}
void IN_MoverightDown(void) {IN_KeyDown(&in_moveright);}
void IN_MoverightUp(void) {IN_KeyUp(&in_moveright);}

void IN_SpeedDown(void) {IN_KeyDown(&in_speed);}
void IN_SpeedUp(void) {IN_KeyUp(&in_speed);}
void IN_StrafeDown(void) {IN_KeyDown(&in_strafe);}
void IN_StrafeUp(void) {IN_KeyUp(&in_strafe);}

#ifdef USE_VOIP
void IN_VoipRecordDown(void)
{
	IN_KeyDown(&in_voiprecord);
	Cvar_Set("cl_voipSend", "1");
}

void IN_VoipRecordUp(void)
{
	IN_KeyUp(&in_voiprecord);
	Cvar_Set("cl_voipSend", "0");
}
#endif

void IN_Button0Down(void) {IN_KeyDown(&in_buttons[0]);}
void IN_Button0Up(void) {IN_KeyUp(&in_buttons[0]);}
void IN_Button1Down(void) {IN_KeyDown(&in_buttons[1]);}
void IN_Button1Up(void) {IN_KeyUp(&in_buttons[1]);}
void IN_Button2Down(void) {IN_KeyDown(&in_buttons[2]);}
void IN_Button2Up(void) {IN_KeyUp(&in_buttons[2]);}
void IN_Button3Down(void) {IN_KeyDown(&in_buttons[3]);}
void IN_Button3Up(void) {IN_KeyUp(&in_buttons[3]);}
void IN_Button4Down(void) {IN_KeyDown(&in_buttons[4]);}
void IN_Button4Up(void) {IN_KeyUp(&in_buttons[4]);}
void IN_Button5Down(void) {IN_KeyDown(&in_buttons[5]);}
void IN_Button5Up(void) {IN_KeyUp(&in_buttons[5]);}
void IN_Button6Down(void) {IN_KeyDown(&in_buttons[6]);}
void IN_Button6Up(void) {IN_KeyUp(&in_buttons[6]);}
void IN_Button7Down(void) {IN_KeyDown(&in_buttons[7]);}
void IN_Button7Up(void) {IN_KeyUp(&in_buttons[7]);}
void IN_Button8Down(void) {IN_KeyDown(&in_buttons[8]);}
void IN_Button8Up(void) {IN_KeyUp(&in_buttons[8]);}
void IN_Button9Down(void) {IN_KeyDown(&in_buttons[9]);}
void IN_Button9Up(void) {IN_KeyUp(&in_buttons[9]);}
void IN_Button10Down(void) {IN_KeyDown(&in_buttons[10]);}
void IN_Button10Up(void) {IN_KeyUp(&in_buttons[10]);}
void IN_Button11Down(void) {IN_KeyDown(&in_buttons[11]);}
void IN_Button11Up(void) {IN_KeyUp(&in_buttons[11]);}
void IN_Button12Down(void) {IN_KeyDown(&in_buttons[12]);}
void IN_Button12Up(void) {IN_KeyUp(&in_buttons[12]);}
void IN_Button13Down(void) {IN_KeyDown(&in_buttons[13]);}
void IN_Button13Up(void) {IN_KeyUp(&in_buttons[13]);}
void IN_Button14Down(void) {IN_KeyDown(&in_buttons[14]);}
void IN_Button14Up(void) {IN_KeyUp(&in_buttons[14]);}
void IN_Button15Down(void) {IN_KeyDown(&in_buttons[15]);}
void IN_Button15Up(void) {IN_KeyUp(&in_buttons[15]);}

void IN_CenterView (void) {
	cl.viewangles[PITCH] = -SHORT2ANGLE(cl.snap.ps.delta_angles[PITCH]);
}


//==========================================================================

cvar_t	*cl_yawspeed;
cvar_t	*cl_pitchspeed;

cvar_t	*cl_run;

cvar_t	*cl_anglespeedkey;


/*
================
CL_AdjustAngles

Moves the local angle positions
================
*/
void CL_AdjustAngles( void ) {
	float	speed;
	
	if ( in_speed.active ) {
		speed = 0.001 * cls.frametime * cl_anglespeedkey->value;
	} else {
		speed = 0.001 * cls.frametime;
	}

	if ( !in_strafe.active ) {
		cl.viewangles[YAW] -= speed*cl_yawspeed->value*CL_KeyState (&in_right);
		cl.viewangles[YAW] += speed*cl_yawspeed->value*CL_KeyState (&in_left);
	}

	cl.viewangles[PITCH] -= speed*cl_pitchspeed->value * CL_KeyState (&in_lookup);
	cl.viewangles[PITCH] += speed*cl_pitchspeed->value * CL_KeyState (&in_lookdown);
}

/*
================
CL_KeyMove

Sets the usercmd_t based on key states
================
*/
void CL_KeyMove( usercmd_t *cmd ) {
	int		movespeed;
	int		forward, side, up;

	//
	// adjust for speed key / running
	// the walking flag is to keep animations consistent
	// even during acceleration and develeration
	//
	if ( in_speed.active ^ cl_run->integer ) {
		movespeed = 127;
		cmd->buttons &= ~BUTTON_WALKING;
	} else {
		cmd->buttons |= BUTTON_WALKING;
		movespeed = 64;
	}

	forward = 0;
	side = 0;
	up = 0;
	if ( in_strafe.active ) {
		side += movespeed * CL_KeyState (&in_right);
		side -= movespeed * CL_KeyState (&in_left);
	}

	side += movespeed * CL_KeyState (&in_moveright);
	side -= movespeed * CL_KeyState (&in_moveleft);


	up += movespeed * CL_KeyState (&in_up);
	up -= movespeed * CL_KeyState (&in_down);

	forward += movespeed * CL_KeyState (&in_forward);
	forward -= movespeed * CL_KeyState (&in_back);

	cmd->forwardmove = ClampChar( forward );
	cmd->rightmove = ClampChar( side );
	cmd->upmove = ClampChar( up );
}

/*
=================
CL_MouseEvent
=================
*/
void CL_MouseEvent( int dx, int dy, int time ) {
	if ( Key_GetCatcher( ) & KEYCATCH_UI ) {
		VM_Call( uivm, UI_MOUSE_EVENT, dx, dy );
	} else if (Key_GetCatcher( ) & KEYCATCH_CGAME) {
		VM_Call (cgvm, CG_MOUSE_EVENT, dx, dy);
	} else {
		cl.mouseDx[cl.mouseIndex] += dx;
		cl.mouseDy[cl.mouseIndex] += dy;
	}
}

/*
=================
CL_JoystickEvent

Joystick values stay set until changed
=================
*/
void CL_JoystickEvent( int axis, int value, int time ) {
	if ( axis < 0 || axis >= MAX_JOYSTICK_AXIS ) {
		Com_Error( ERR_DROP, "CL_JoystickEvent: bad axis %i", axis );
	}
	cl.joystickAxis[axis] = value;
}

/*
=================
CL_JoystickMove
=================
*/
void CL_JoystickMove( usercmd_t *cmd ) {
	float	anglespeed;

	float yaw     = j_yaw->value     * cl.joystickAxis[j_yaw_axis->integer];
	float right   = j_side->value    * cl.joystickAxis[j_side_axis->integer];
	float forward = j_forward->value * cl.joystickAxis[j_forward_axis->integer];
	float pitch   = j_pitch->value   * cl.joystickAxis[j_pitch_axis->integer];
	float up      = j_up->value      * cl.joystickAxis[j_up_axis->integer];

	if ( !(in_speed.active ^ cl_run->integer) ) {
		cmd->buttons |= BUTTON_WALKING;
	}

	if ( in_speed.active ) {
		anglespeed = 0.001 * cls.frametime * cl_anglespeedkey->value;
	} else {
		anglespeed = 0.001 * cls.frametime;
	}

	if ( !in_strafe.active ) {
		cl.viewangles[YAW] += anglespeed * yaw;
		cmd->rightmove = ClampChar( cmd->rightmove + (int)right );
	} else {
		cl.viewangles[YAW] += anglespeed * right;
		cmd->rightmove = ClampChar( cmd->rightmove + (int)yaw );
	}

	if ( in_mlooking ) {
		cl.viewangles[PITCH] += anglespeed * forward;
		cmd->forwardmove = ClampChar( cmd->forwardmove + (int)pitch );
	} else {
		cl.viewangles[PITCH] += anglespeed * pitch;
		cmd->forwardmove = ClampChar( cmd->forwardmove + (int)forward );
	}

	cmd->upmove = ClampChar( cmd->upmove + (int)up );
}

/*
=================
CL_MouseMove
=================
*/

void CL_MouseMove(usercmd_t *cmd)
{
	float mx, my;

	// allow mouse smoothing
	if (m_filter->integer)
	{
		mx = (cl.mouseDx[0] + cl.mouseDx[1]) * 0.5f;
		my = (cl.mouseDy[0] + cl.mouseDy[1]) * 0.5f;
	}
	else
	{
		mx = cl.mouseDx[cl.mouseIndex];
		my = cl.mouseDy[cl.mouseIndex];
	}
	
	cl.mouseIndex ^= 1;
	cl.mouseDx[cl.mouseIndex] = 0;
	cl.mouseDy[cl.mouseIndex] = 0;

	if (mx == 0.0f && my == 0.0f)
		return;
	
	if (cl_mouseAccel->value != 0.0f)
	{
		if(cl_mouseAccelStyle->integer == 0)
		{
			float accelSensitivity;
			float rate;
			
			rate = sqrt(mx * mx + my * my) / (float) frame_msec;

			accelSensitivity = cl_sensitivity->value + rate * cl_mouseAccel->value;
			mx *= accelSensitivity;
			my *= accelSensitivity;
			
			if(cl_showMouseRate->integer)
				Com_Printf("rate: %f, accelSensitivity: %f\n", rate, accelSensitivity);
		}
		else
		{
			float rate[2];
			float power[2];

			// sensitivity remains pretty much unchanged at low speeds
			// cl_mouseAccel is a power value to how the acceleration is shaped
			// cl_mouseAccelOffset is the rate for which the acceleration will have doubled the non accelerated amplification
			// NOTE: decouple the config cvars for independent acceleration setup along X and Y?

			rate[0] = fabs(mx) / (float) frame_msec;
			rate[1] = fabs(my) / (float) frame_msec;
			power[0] = powf(rate[0] / cl_mouseAccelOffset->value, cl_mouseAccel->value);
			power[1] = powf(rate[1] / cl_mouseAccelOffset->value, cl_mouseAccel->value);

			mx = cl_sensitivity->value * (mx + ((mx < 0) ? -power[0] : power[0]) * cl_mouseAccelOffset->value);
			my = cl_sensitivity->value * (my + ((my < 0) ? -power[1] : power[1]) * cl_mouseAccelOffset->value);

			if(cl_showMouseRate->integer)
				Com_Printf("ratex: %f, ratey: %f, powx: %f, powy: %f\n", rate[0], rate[1], power[0], power[1]);
		}
	}
	else
	{
		mx *= cl_sensitivity->value;
		my *= cl_sensitivity->value;
	}

	// ingame FOV
	mx *= cl.cgameSensitivity;
	my *= cl.cgameSensitivity;

	// add mouse X/Y movement to cmd
	if(in_strafe.active)
		cmd->rightmove = ClampChar(cmd->rightmove + m_side->value * mx);
	else
		cl.viewangles[YAW] -= m_yaw->value * mx;

	if ((in_mlooking || cl_freelook->integer) && !in_strafe.active)
		cl.viewangles[PITCH] += m_pitch->value * my;
	else
		cmd->forwardmove = ClampChar(cmd->forwardmove - m_forward->value * my);
}


/*
=================
CL_AimAssistProjectileSpeed

Keep these values in sync with the missile launch speeds in g_missile.c.
Zero means that the current weapon is hitscan and needs no lead.
=================
*/
static float CL_AimAssistProjectileSpeed( int weapon ) {
	switch ( weapon ) {
	case WP_GRENADE_LAUNCHER:
		return 700.0f;
	case WP_ROCKET_LAUNCHER:
		return 900.0f;
	case WP_PLASMAGUN:
	case WP_BFG:
		return 2000.0f;
	case WP_GRAPPLING_HOOK:
		return 800.0f;
#ifdef MISSIONPACK
	case WP_NAILGUN:
		return 1455.0f;	// random 555..2355, use its mean speed
	case WP_PROX_LAUNCHER:
		return 700.0f;
#endif
	default:
		return 0.0f;
	}
}


// Movement constants the game keeps to itself: the height a walking player is
// lifted over (STEPSIZE in bg_local.h) and the head start every missile gets on
// its first frame (MISSILE_PRESTEP_TIME in g_missile.c).
#define STEPSIZE			18
#define MISSILE_PRESTEP		0.05f

/*
=================
CL_AimAssistWeaponName

Short names for the shot log, in the order of weapon_t.
=================
*/
static const char *CL_AimAssistWeaponName( int weapon ) {
	static const char *names[WP_NUM_WEAPONS] = {
		"none", "gauntlet", "machinegun", "shotgun", "grenade", "rocket",
		"lightning", "railgun", "plasma", "bfg", "hook",
#ifdef MISSIONPACK
		"nailgun", "prox", "chaingun",
#endif
	};

	if ( weapon <= WP_NONE || weapon >= WP_NUM_WEAPONS || !names[weapon] ) {
		return "unknown";
	}

	return names[weapon];
}


/*
=================
CL_AimAssistCrouched

Whether a player entity is ducked. The player state's own flag never leaves
the server, but the legs animation does, and the game picks a crouching one
for every ducked player whether it stands, walks or backs up.
=================
*/
qboolean CL_AimAssistCrouched( const entityState_t *entity ) {
	int	anim;

	anim = entity->legsAnim & ~ANIM_TOGGLEBIT;
	return anim == LEGS_WALKCR || anim == LEGS_IDLECR || anim == LEGS_BACKCR;
}


/*
=================
CL_AimAssistHull

The box the server hits for this player: the full one, or the ducked one
that is half as tall.
=================
*/
static void CL_AimAssistHull( const entityState_t *entity, vec3_t mins, vec3_t maxs ) {
	VectorSet( mins, -15, -15, MINS_Z );
	VectorSet( maxs, 15, 15, CL_AimAssistCrouched( entity ) ? CROUCH_HEIGHT : DEFAULT_HEIGHT );
}


/*
=================
CL_AimAssistBodyHeight

Where in the box to aim, above the origin: a little over the middle whether
the target stands or ducks.
=================
*/
static float CL_AimAssistBodyHeight( const entityState_t *entity ) {
	// The box reaches from 24 below the origin to 32 above it standing and
	// 16 above it crouched, so its middle - the point furthest from every
	// face - is 4 above the origin standing and 4 below it crouched.
	return CL_AimAssistCrouched( entity ) ? -4.0f : 4.0f;
}


/*
=================
CL_AimAssistFloats

A target that swims or flies is off the ground without falling, and can turn
at will: neither the gravity nor the sure course of a jump applies to it. The
game has a player swimming once the water reaches the middle of its body,
which is a unit above its origin.
=================
*/
static qboolean CL_AimAssistFloats( const entityState_t *entity ) {
	vec3_t	point;

	if ( entity->powerups & ( 1 << PW_FLIGHT ) ) {
		return qtrue;
	}

	VectorCopy( entity->pos.trBase, point );
	point[2] += 1.0f;
	return ( CM_PointContents( point, 0 ) & MASK_WATER ) != 0;
}


/*
=================
CL_AimAssistFrameTime

The server's frame, in seconds. On a listen server the game runs in this
process, so its cvar is ours to read. The world moves in these steps and
nothing in it moves between them.
=================
*/
static float CL_AimAssistFrameTime( void ) {
	int	fps;

	fps = Cvar_VariableIntegerValue( "sv_fps" );
	if ( fps < 1 ) {
		fps = 20;
	}
	return ( 1000 / fps ) * 0.001f;
}


/*
=================
CL_AimAssistPhase

Where the picture stands between two snapshots, as a zero-mean time.

The snapshot's age, cl.snap.serverTime - cl.serverTime, is a sawtooth: about
55 ms right after a snapshot arrives, sliding down to about 5 ms before the
next one, then up again. Anything read straight from the snapshot steps once
per snapshot. What the steering wants is a lead that changes as smoothly as
the view does, and this is the correction that gives it: the sawtooth less
its own running mean. Added to a lead it cancels the step the target makes
when a snapshot lands, because the target moves one frame forward exactly as
this drops by one frame. Its mean is zero, so on average it changes nothing,
and on the frame a shot is fired it is left out altogether.
=================
*/
static float	aimPhaseMean = 0.03f;		// running mean of the snapshot age, seconds

static void CL_AimAssistPhaseUpdate( void ) {
	float	age;

	age = ( cl.snap.serverTime - cl.serverTime ) * 0.001f;
	aimPhaseMean += ( age - aimPhaseMean ) * Com_Clamp( 0.0f, 1.0f, frame_msec / 1000.0f );
}

static float CL_AimAssistPhase( void ) {
	float	age;

	age = ( cl.snap.serverTime - cl.serverTime ) * 0.001f;
	return Com_Clamp( -0.1f, 0.1f, aimPhaseMean - age );
}


/*
=================
CL_AimAssistEye

Where the shot leaves from: the player's position at the time of the command
being built, which is what the server computes before it fires. The snapshot
holds the position as of its commandTime and the command carries
cl.serverTime; the player has moved between the two by its velocity, and
fallen by gravity if it was in the air. This is continuous across snapshots
- a new one moves both the base and its commandTime forward together - so
the aim does not step while the player runs, and it is where the server puts
the muzzle, so it is also right.

What went wrong before: the base was carried forward by the snapshot's age
instead, which runs the other way - largest right after a snapshot, when the
player has barely moved on from it, and smallest just before the next, when
it has moved the most. That made the eye slide backwards through every
interval and leap two frames forward at each snapshot: a jerk while moving,
and up to a body width of error at running speed.
=================
*/
static void CL_AimAssistEye( vec3_t eye ) {
	float	dt, gravity;

	dt = ( cl.serverTime - cl.snap.ps.commandTime ) * 0.001f;
	dt = Com_Clamp( 0.0f, 0.2f, dt );

	VectorMA( cl.snap.ps.origin, dt, cl.snap.ps.velocity, eye );
	if ( cl.snap.ps.groundEntityNum == ENTITYNUM_NONE ) {
		gravity = cl.snap.ps.gravity > 0 ? cl.snap.ps.gravity : DEFAULT_GRAVITY;
		eye[2] -= 0.5f * gravity * dt * dt;
	}
	eye[2] += cl.snap.ps.viewheight;
}


/*
=================
CL_AimAssistVelocity

What the target is really doing. A player standing on a lift or a platform
reports no velocity of its own while the mover carries it along, so when the
reported velocity is nothing and the position still moved between the last
two snapshots, the movement seen is the one to trust.
=================
*/
static void CL_AimAssistVelocity( const entityState_t *entity, vec3_t velocity ) {
	const clSnapshot_t	*previous;
	const entityState_t	*old;
	vec3_t				seen;
	float				interval, speed;
	int					i;

	VectorCopy( entity->pos.trDelta, velocity );

	previous = &cl.snapshots[( cl.snap.messageNum - 1 ) & PACKET_MASK];
	if ( !previous->valid || previous->serverTime >= cl.snap.serverTime ) {
		return;
	}
	interval = ( cl.snap.serverTime - previous->serverTime ) * 0.001f;

	for ( i = 0; i < previous->numEntities; i++ ) {
		old = &cl.parseEntities[( previous->parseEntitiesNum + i ) & ( MAX_PARSE_ENTITIES - 1 )];
		if ( old->number != entity->number ) {
			continue;
		}

		VectorSubtract( entity->pos.trBase, old->pos.trBase, seen );
		VectorScale( seen, 1.0f / interval, seen );
		speed = VectorLength( seen );

		// carried along: nothing reported, yet it moved - and not by the
		// thousands of units a teleporter would show
		if ( VectorLength( velocity ) < 10.0f && speed > 30.0f && speed < 1500.0f ) {
			VectorCopy( seen, velocity );
		}
		return;
	}
}


/*
=================
CL_AimAssistTrust

How much of the sideways lead is worth using. A straight line is a good guess
for the next tenth of a second and a poor one for the next second and a half,
because a player turns several times on the way. Two things shorten it: the
lead itself, and a target that is already changing its velocity between the
last two snapshots.
=================
*/
static float CL_AimAssistTrust( const entityState_t *entity, float time ) {
	const clSnapshot_t	*previous;
	const entityState_t	*old;
	vec3_t				change, heading;
	float				speed, spread, interval;
	int					i;

	time = fabs( time );

	// A target in the air cannot change where it is going, and the falling part
	// is modelled, so its course is the surest one there is. Swimming and
	// flying are not falling.
	if ( entity->groundEntityNum == ENTITYNUM_NONE && !CL_AimAssistFloats( entity ) ) {
		return 1.0f;
	}

	// A bot running its line holds its velocity to the unit: friction takes
	// exactly as much as the acceleration puts back, so the straight line is
	// not an approximation there and deserves the whole lead. What costs us is
	// a target that is turning, and the longer we look ahead the more of its
	// turn we are guessing at.
	previous = &cl.snapshots[( cl.snap.messageNum - 1 ) & PACKET_MASK];
	if ( !previous->valid || previous->serverTime >= cl.snap.serverTime ) {
		return 1.0f;
	}

	interval = ( cl.snap.serverTime - previous->serverTime ) * 0.001f;

	for ( i = 0; i < previous->numEntities; i++ ) {
		old = &cl.parseEntities[( previous->parseEntitiesNum + i ) & ( MAX_PARSE_ENTITIES - 1 )];
		if ( old->number != entity->number ) {
			continue;
		}

		// Only a change of course counts. Up and down is the gravity above,
		// and speeding up or slowing down along the line it already runs is
		// no reason to doubt the line: a bot setting off keeps its heading.
		VectorSubtract( entity->pos.trDelta, old->pos.trDelta, change );
		change[2] = 0.0f;
		speed = sqrt( entity->pos.trDelta[0] * entity->pos.trDelta[0]
			+ entity->pos.trDelta[1] * entity->pos.trDelta[1] );
		if ( speed < 1.0f ) {
			return 1.0f;
		}
		heading[0] = entity->pos.trDelta[0] / speed;
		heading[1] = entity->pos.trDelta[1] / speed;
		heading[2] = 0.0f;
		VectorMA( change, -DotProduct( change, heading ), heading, change );

		// How far the course would wander over the whole lead. One interval's
		// turn does not go on in the same direction for twenty more: the
		// turns come and go, and what they add up to grows with the square
		// root of the time, as a random walk does.
		spread = VectorLength( change ) / speed * sqrt( time / interval );

		return 1.0f / ( 1.0f + spread );
	}

	return 1.0f;
}


/*
=================
CL_AimAssistSideways

How far along its line a target is expected to get in the given time, as a
time. A straight line is right for a moment and wrong for a second, because
bots change direction: a target expected to hold its course for T seconds
on average gets T(1 - e^(-t/T)) of straight running out of t, which is t for
a short flight and never more than T for a long one. T is cl_aimAssistLead,
in seconds, and the learner tunes it from what the bots really do.
=================
*/
// Die Fassung der Protokollzeilen. Hochzaehlen, sobald ein Feld dazukommt,
// verschwindet oder seine Bedeutung wechselt.
//
// Sieben: die Trefferquoten-Tabelle schreibt mit ("aim rate:", "aim rated:"),
// die Korrekturzeile nennt das Fach vor und nach dem Schuss ("was", "now"),
// und der Abzug der ganzen Tabelle heisst jetzt "aim table:" statt "aim tune:",
// damit eine Korrektur und ein Abzug nicht mehr gleich aussehen.
#define AIM_LOG_VERSION	7

static qboolean	aimLogStamped;			// ob diese Verbindung schon gestempelt ist

/*
=================
CL_AimAssistHold

How long a target is assumed to hold its direction, in seconds. This is the
player's setting, not a learned one: it gives the lead its shape, and the
measured table corrects that shape per weapon and per flight time. It used to
be learned as well, and that was the mistake - one number moved by every shot
carries a correction found at one range into every other.
=================
*/
static float CL_AimAssistHold( void ) {
	return cl_aimAssistLead->value;
}

/*
=================
CL_AimAssistFlush

Called when the connection goes: the measured table is written out, and the
next session stamps the log afresh.
=================
*/
static void CL_AimAssistTuneSave( void );
void CL_AimAssistTuneDump( void );
static void CL_AimAssistRateSave( void );
static void CL_AimAssistRateForget( void );
void CL_AimAssistRateDump( void );
static void CL_AimAssistPriorityDump( void );
void CL_AimAssistPriorityReload( void );

void CL_AimAssistFlush( void ) {
	aimLogStamped = qfalse;
	// the bench writes the per-weapon lists just before it starts the game, so
	// a disconnect is the right moment to look at that file again
	CL_AimAssistPriorityReload();
	// whatever was still in the air when the connection went was never seen
	// land, so it is dropped rather than counted as a miss
	CL_AimAssistRateForget();
	CL_AimAssistTuneSave();
	CL_AimAssistRateSave();
	CL_AimAssistTuneDump();
	CL_AimAssistRateDump();
}

static float CL_AimAssistSideways( float time ) {
	float	hold;

	hold = CL_AimAssistHold();
	if ( time <= 0.0f || hold <= 0.0f ) {
		return time;
	}

	return hold * ( 1.0f - exp( -time / hold ) );
}


/*
=================
The record of what the prediction is really worth: per weapon, per flight
time, and per how fast the target was going.

The lead model says how far a target gets while a shot is on its way. How well
that holds depends on two things, and they are not the same thing. On how long
the shot is on its way - a plasma bolt arrives before anyone can change their
mind, a rocket at the far end of the map gives them all the time in the world.
And on how fast the target is going: one that is walking is manoeuvring and
will turn, one at full speed is committed to its line and mostly keeps it.

So a box is a weapon, a band of flight time and a band of speed, and it keeps
two numbers: the factor the expectation has to be multiplied by to match what
really happened, and how far the result still scatters after that. The factor
tells the prediction how much of the lead to believe; the scatter tells the
target choice which shots are worth steering at all.

The shooter's own speed is deliberately not an axis. The eye is carried to
firing time exactly, so where the shot leaves from is known, and the target's
path does not care how fast the shooter runs. It does change how quickly the
view has to swing, which shows up as the residual error on the shot line - a
different number, measured separately. Both speeds are written to the log so
the question can be settled from data rather than from opinion.

A box fills slowly, a handful of shots an evening, so the table is written
next to the config and read back at the start.
=================
*/
#define AIM_BANDS		4
#define AIM_SPEEDS		2
#define AIM_TUNE_FILE	"aimtune.cfg"
// The shape of the file, not of the log. Three because the scatter column now
// carries the whole miss and not only its along-track half: a two would read
// as a spread a third too narrow, so an older file is dropped rather than
// mixed with what is measured from here on.
#define AIM_TUNE_FORMAT	3
#define AIM_TUNE_PRIOR	4.0f		// weight the untouched factor 1.0 carries
#define AIM_TUNE_DECAY	0.98f		// what a box keeps of its past per sample

typedef struct {
	float	sum;			// weighted sum of actual/expected
	float	weight;			// weight behind it
	float	square;			// weighted mean square of what is left over, in units
	int		samples;
} aimTune_t;

static aimTune_t	aimTune[WP_NUM_WEAPONS][AIM_BANDS][AIM_SPEEDS];
static qboolean		aimTuneLoaded;
static qboolean		aimTuneDirty;
static int			aimTuneWritten;		// when the table last reached the disk

static float CL_AimAssistBandStart( int band ) {
	static const float	start[AIM_BANDS] = { 0.0f, 0.4f, 0.8f, 1.3f };

	return start[band < 0 ? 0 : ( band >= AIM_BANDS ? AIM_BANDS - 1 : band )];
}

static int CL_AimAssistBand( float lead ) {
	if ( lead < 0.4f ) {
		return 0;
	}
	if ( lead < 0.8f ) {
		return 1;
	}
	if ( lead < 1.3f ) {
		return 2;
	}
	return 3;
}

static float CL_AimAssistBandCentre( int band ) {
	static const float	centre[AIM_BANDS] = { 0.2f, 0.6f, 1.05f, 1.8f };

	return centre[band < 0 ? 0 : ( band >= AIM_BANDS ? AIM_BANDS - 1 : band )];
}

/*
=================
CL_AimAssistSpeedBand

Walking or running. A bot at full pace does about three hundred and twenty
units a second; much below two hundred it is turning, stopping or picking
something up, and its line is worth less. Two steps only, because every step
divides the samples, and a box with nothing in it learns nothing.
=================
*/
static int CL_AimAssistSpeedBand( float speed ) {
	return speed < 200.0f ? 0 : 1;
}

static float CL_AimAssistSpeedCentre( int band ) {
	static const float	centre[AIM_SPEEDS] = { 110.0f, 330.0f };

	return centre[band < 0 ? 0 : ( band >= AIM_SPEEDS ? AIM_SPEEDS - 1 : band )];
}

static float CL_AimAssistSpeedStart( int band ) {
	static const float	start[AIM_SPEEDS] = { 0.0f, 200.0f };

	return start[band < 0 ? 0 : ( band >= AIM_SPEEDS ? AIM_SPEEDS - 1 : band )];
}

static void CL_AimAssistTuneLoad( void ) {
	union { char *c; void *v; }	file;
	const char					*line;
	aimTune_t					*t;
	float						sum, weight, square;
	int							weapon, band, pace, samples, format = 0;
	long						length;

	aimTuneLoaded = qtrue;

	length = FS_ReadFile( AIM_TUNE_FILE, &file.v );
	if ( length <= 0 || !file.c ) {
		return;
	}

	line = file.c;
	while ( *line ) {
		if ( sscanf( line, "format %i", &format ) == 1 && format != AIM_TUNE_FORMAT ) {
			break;			// written by an older build, its boxes mean something else
		}
		if ( *line != '/' && sscanf( line, "%i %i %i %f %f %f %i",
				&weapon, &band, &pace, &sum, &weight, &square, &samples ) == 7
			&& weapon > WP_NONE && weapon < WP_NUM_WEAPONS
			&& band >= 0 && band < AIM_BANDS && pace >= 0 && pace < AIM_SPEEDS
			&& weight >= 0.0f && square >= 0.0f && samples >= 0 ) {
			t = &aimTune[weapon][band][pace];
			t->sum = sum;
			t->weight = weight;
			t->square = square;
			t->samples = samples;
		}

		while ( *line && *line != '\n' ) {
			line++;
		}
		while ( *line == '\n' || *line == '\r' ) {
			line++;
		}
	}

	FS_FreeFile( file.v );
}

static void CL_AimAssistTuneSave( void ) {
	char		text[8192];
	aimTune_t	*t;
	int			weapon, band, pace;

	if ( !aimTuneDirty ) {
		return;
	}

	Com_sprintf( text, sizeof( text ),
		"format %i\n// what the aim assist has measured about its own lead.\n"
		"// weapon band pace sum weight square samples\n", AIM_TUNE_FORMAT );

	for ( weapon = WP_NONE + 1; weapon < WP_NUM_WEAPONS; weapon++ ) {
		for ( band = 0; band < AIM_BANDS; band++ ) {
			for ( pace = 0; pace < AIM_SPEEDS; pace++ ) {
				t = &aimTune[weapon][band][pace];
				if ( !t->samples ) {
					continue;
				}
				Q_strcat( text, sizeof( text ), va( "%i %i %i %.4f %.4f %.1f %i\t// %s %.1fs %.0fu/s\n",
					weapon, band, pace, t->sum, t->weight, t->square, t->samples,
					CL_AimAssistWeaponName( weapon ), CL_AimAssistBandStart( band ),
					CL_AimAssistSpeedStart( pace ) ) );
			}
		}
	}

	FS_WriteFile( AIM_TUNE_FILE, text, strlen( text ) );
	aimTuneDirty = qfalse;
}

// What one box on its own says, with its own weight against the untouched one
static float CL_AimAssistBoxFactor( int weapon, int band, int pace ) {
	const aimTune_t	*t = &aimTune[weapon][band][pace];

	return ( t->sum + AIM_TUNE_PRIOR ) / ( t->weight + AIM_TUNE_PRIOR );
}

// Where a value falls between the middles of its own band and the next one,
// and which that next one is. Zero share means the box speaks for itself.
static float CL_AimAssistBlend( float value, float here, float there, int band, int next, int *outNext ) {
	*outNext = next;
	if ( next == band ) {
		return 0.0f;
	}
	return Com_Clamp( 0.0f, 1.0f, ( value - here ) / ( there - here ) );
}

/*
=================
CL_AimAssistTune

How much of the modelled lead to believe for this weapon, at this flight time,
against a target going this fast.

A box is written sharp and read soft. Half a frame either side of a boundary
are the same shot and should not get different answers because one landed in
the next box, so the neighbouring boxes are blended along both axes by where
the shot falls between their middles.
=================
*/
static float CL_AimAssistTune( int weapon, float lead, float speed ) {
	float	overTime, overPace, here, there, low, high;
	int		band, nextBand, pace, nextPace;

	if ( !aimTuneLoaded ) {
		CL_AimAssistTuneLoad();
	}
	if ( weapon <= WP_NONE || weapon >= WP_NUM_WEAPONS || lead <= 0.0f ) {
		return 1.0f;
	}

	band = CL_AimAssistBand( lead );
	here = CL_AimAssistBandCentre( band );
	nextBand = band;
	if ( lead < here && band > 0 ) {
		nextBand = band - 1;
	} else if ( lead > here && band < AIM_BANDS - 1 ) {
		nextBand = band + 1;
	}
	there = CL_AimAssistBandCentre( nextBand );
	overTime = CL_AimAssistBlend( lead, here, there, band, nextBand, &nextBand );

	pace = CL_AimAssistSpeedBand( speed );
	here = CL_AimAssistSpeedCentre( pace );
	nextPace = pace;
	if ( speed < here && pace > 0 ) {
		nextPace = pace - 1;
	} else if ( speed > here && pace < AIM_SPEEDS - 1 ) {
		nextPace = pace + 1;
	}
	there = CL_AimAssistSpeedCentre( nextPace );
	overPace = CL_AimAssistBlend( speed, here, there, pace, nextPace, &nextPace );

	// the two flight-time neighbours at each of the two speeds, then between
	low = CL_AimAssistBoxFactor( weapon, band, pace ) * ( 1.0f - overTime )
		+ CL_AimAssistBoxFactor( weapon, nextBand, pace ) * overTime;
	high = CL_AimAssistBoxFactor( weapon, band, nextPace ) * ( 1.0f - overTime )
		+ CL_AimAssistBoxFactor( weapon, nextBand, nextPace ) * overTime;

	return Com_Clamp( 0.1f, 2.0f, low * ( 1.0f - overPace ) + high * overPace );
}

/*
=================
CL_AimAssistScatter

How far the shot is expected to land from the target, in units, after the lead
has been applied - what is left that no prediction can take away. Negative
until the box has seen enough shots to mean anything.
=================
*/
static float CL_AimAssistScatter( int weapon, float lead, float speed ) {
	const aimTune_t	*t;

	if ( !aimTuneLoaded ) {
		CL_AimAssistTuneLoad();
	}
	if ( weapon <= WP_NONE || weapon >= WP_NUM_WEAPONS || lead <= 0.0f ) {
		return -1.0f;
	}

	t = &aimTune[weapon][CL_AimAssistBand( lead )][CL_AimAssistSpeedBand( speed )];
	if ( t->samples < 6 || t->weight <= 0.0f ) {
		return -1.0f;
	}

	return sqrt( t->square / t->weight );
}

/*
=================
CL_AimAssistTuneUpdate

One learned shot into its box - the one it belongs to, not its neighbours. The
factor follows the ratio the shot really had, the scatter follows what the
corrected prediction still missed by, and both forget the distant past slowly
so the table can follow an opponent that changes without throwing away an
evening's worth of shots.
=================
*/
static void CL_AimAssistTuneUpdate( int weapon, float lead, float speed,
		float base, float aimed, float actual, float aside, float weight ) {
	aimTune_t	*t;
	float		ratio, along, miss;

	if ( !aimTuneLoaded ) {
		CL_AimAssistTuneLoad();
	}
	if ( weapon <= WP_NONE || weapon >= WP_NUM_WEAPONS || lead <= 0.0f
		|| base <= 1.0f || weight <= 0.0f ) {
		return;
	}

	t = &aimTune[weapon][CL_AimAssistBand( lead )][CL_AimAssistSpeedBand( speed )];

	// The factor is what the model has to be multiplied by, so it has to be
	// measured against the model - the expectation before this box touched it.
	// Measured against the already-corrected one, each sample would only ever
	// say how far the last correction was off, and the box would settle on the
	// square root of the correction it needs instead of the correction.
	ratio = Com_Clamp( 0.0f, 2.0f, actual / base );

	// The scatter is what the shot that was really aimed still missed by, so
	// it is measured against the point that was really aimed at - in both
	// directions. Only the along-track half used to be counted, and the half
	// that was thrown away is the larger one: over a session the target ran
	// twenty-two units off along its heading and twenty-eight units off to the
	// side of it, so the spread was reported at two thirds of its true size
	// and a measured box looked more dependable than an unmeasured one.
	along = actual - aimed;
	miss = sqrt( along * along + aside * aside );

	t->sum = t->sum * AIM_TUNE_DECAY + ratio * weight;
	t->weight = t->weight * AIM_TUNE_DECAY + weight;
	t->square = t->square * AIM_TUNE_DECAY + miss * miss * weight;
	t->samples++;
	aimTuneDirty = qtrue;
}


/*
=================
CL_AimAssistHitRadius

How far a shot may land from the target and still do something: the splash of
the weapons that have one (g_missile.c), and half the width of the box for the
ones that must land on it.
=================
*/
static float CL_AimAssistHitRadius( int weapon ) {
	switch ( weapon ) {
	case WP_ROCKET_LAUNCHER:	return 120.0f;
	case WP_GRENADE_LAUNCHER:	return 150.0f;
	case WP_BFG:				return 120.0f;
	case WP_PLASMAGUN:			return 20.0f;
#ifdef MISSIONPACK
	case WP_PROX_LAUNCHER:		return 150.0f;
#endif
	default:					return 15.0f;
	}
}

/*
=================
CL_AimAssistTuneDump

The whole table in one go, so the bench can show it whenever it likes and not
only while shots are arriving. Bound to the aimtune command and written once
when the connection goes.

These lines used to say "aim tune:", exactly like the one the learner writes
after a correction, and nothing on them said which kind they were. A reader
could only tell the two apart by the frame being zero - which is wrong for
every dump made while connected, 74 of 223 of them - or by the line happening
to follow a learn line, which is true only because the two Com_Printf calls
are neighbours in the same loop. Nobody wrote that down, and the first person
to put a line between them would have broken every reader silently. So the
dump says "aim table:" and the question does not arise.
=================
*/
void CL_AimAssistTuneDump( void ) {
	const aimTune_t	*t;
	int				weapon, band, pace, boxes = 0;

	if ( !aimTuneLoaded ) {
		CL_AimAssistTuneLoad();
	}

	for ( weapon = WP_NONE + 1; weapon < WP_NUM_WEAPONS; weapon++ ) {
		for ( band = 0; band < AIM_BANDS; band++ ) {
			for ( pace = 0; pace < AIM_SPEEDS; pace++ ) {
				t = &aimTune[weapon][band][pace];
				if ( !t->samples ) {
					continue;
				}
				boxes++;
				Com_Printf( "aim table: %s band %i from %.1f pace %i above %.0f factor %.2f"
					" scatter %.0f reach %.0f n %i frame %i\n",
					CL_AimAssistWeaponName( weapon ), band, CL_AimAssistBandStart( band ),
					pace, CL_AimAssistSpeedStart( pace ),
					CL_AimAssistTune( weapon, CL_AimAssistBandCentre( band ), CL_AimAssistSpeedCentre( pace ) ),
					CL_AimAssistScatter( weapon, CL_AimAssistBandCentre( band ), CL_AimAssistSpeedCentre( pace ) ),
					CL_AimAssistHitRadius( weapon ), t->samples, cl.snap.serverTime );
			}
		}
	}

	if ( !boxes ) {
		Com_Printf( "aim table: nothing measured yet\n" );
	}

	// and what the priorities were understood as, so a typo in the string is
	// visible instead of quietly leaving a default in place
	CL_AimAssistPriorityDump();
}


/*
=================
Die Trefferquote, je Waffe und Entfernung

Die Vorhalte-Tabelle darueber misst, WIE WEIT vorgehalten werden muss. Diese
hier misst etwas anderes: ob der Schuss ankommt. Beide Fragen haengen an
verschiedenen Groessen, und darum stehen hier andere Achsen.

Gemessen wurde das an rund viereinhalb Megabyte eigener Protokolle, 4182
unterstuetzten Schuessen aus sieben Sitzungen. Die Entfernung traegt am
meisten: die Rakete faellt ueber die fuenf Faecher von 73 auf 0 Prozent, das
Maschinengewehr von 90 auf 26, waehrend die Railgun ueberall bei etwa 83
steht. Dass die Railgun flach ist, ist der nuetzlichste einzelne Satz, den
diese Tabelle sagen kann - und der Grund, warum die Waffe eine eigene Achse
braucht: auf 1000 bis 1500 Einheiten steht die Rakete bei 9 und die Railgun
bei 82 Prozent.

Das Tempo des Ziels, die zweite Achse der Vorhalte-Tabelle, taugt hier nicht:
am dortigen Schnitt bei 200 u/s ist die schnelle Haelfte sogar 4,5 Punkte
BESSER, und erst oberhalb von 400 zeigt sich ein Unterschied, der mit 11,7
Punkten bei einem Fehler von 5,1 nichts entscheidet. Das eigene Tempo ist
null, geduckt kommt in 13 Schuessen vor.

Eines kommt noch hinzu, und das steht als zweites Zaehlerpaar im selben Fach
statt als dritte Achse: ob das Ziel waehrend des Fluges aufsetzt. Das sind
23 Punkte, nach Entfernung bereinigt, und es gibt sie fuer Hitscan nicht. Als
dritte Dimension wuerde es das duennste Fach von 184 Schuessen auf 31 kuerzen;
als Zaehlerpaar kostet es acht Byte und keine einzige Probe.

Die Faecher blenden nicht ineinander, anders als die Vorhalte-Tabelle. Die
blendet, weil ein Schuss dicht an der Grenze sonst eine andere Korrektur
bekaeme. Hier wird gezaehlt, und ein halb gezaehlter Schuss ist keine Zahl.

Die Grenzen sind fest verdrahtet und lernen nicht mit. Gesucht wurden sie mit
einer Rasterschaetzung ueber alle Schnitte in Hundertern, freie Quote je Fach:
fuer die Rakete allein kommen 500/1000/1500 heraus, fuer alle vier Waffen
zusammen dieselben, und jedes weitere Fach bringt danach immer gleich viel -
das Kennzeichen dafuer, dass nur noch Rauschen angepasst wird. Die runde
Fassung 500/1000/1500/2000 kostet gegen das freie Optimum vier Einheiten
Plausibilitaet und laesst die Rakete auf ihrem eigenen Bestwert. Eine
mitwandernde Grenze wuerde dem Rauschen nachlaufen und bei jedem Schritt alle
gespeicherten Faecher still umbenennen.
=================
*/
#define AIM_RANGES		5
#define AIM_RATE_FILE	"aimrate.cfg"
// Die Form der Datei. Steigt, sobald sich CL_AimAssistRange aendert: die
// Grenzen stecken darin, was ein Fach bedeutet, und eine verschobene Grenze
// macht aus jeder gespeicherten Zahl still eine Aussage ueber etwas anderes.
//
// Einmal ist sie bewusst NICHT gestiegen: die dritte Grenze wurde noch vor dem
// ersten ausgelieferten Bau von 1450 auf 1500 nachgezogen, weil die
// Rasterschaetzung auf 1500 fiel. Das sind fuenfzig Einheiten auf einer Kante,
// also ein knappes Zehntel eines Fachs; in der einzigen Datei, die es damals
// gab, betraf es eine Handvoll Proben von neunhundert, und das Altern loescht
// sie ohnehin in wenigen hundert Schuessen aus. Dafuer einen ganzen Abend
// Messung wegzuwerfen waere derselbe schlechte Tausch wie bei den Zwillingen
// weiter unten: Genauigkeit, die mehr Messung kostet als sie Fehler spart.
#define AIM_RATE_FORMAT	1
// Was ein Fach je gebuchtem Schuss von seiner Vergangenheit behaelt. Langsamer
// als die 0,98 der Vorhalte-Tabelle, und das mit Absicht: ein Zaehler braucht
// mehr Proben als ein Mittelwert, weil jede Probe nur ein Bit traegt. Bei 0,98
// zaehlt ein Fach effektiv 99 Schuesse und rauscht mit +-4,9 Punkten - die
// Raketenfaecher liegen weit draussen 11 Punkte auseinander, das waere schon
// halb verschluckt. Bei 0,99 sind es 199 Schuesse und +-3,5.
//
// Was das Altern NICHT ist: eine gemessene Notwendigkeit. Ueber sieben
// Sitzungen und vier verschiedene Baustaende steht das Fach 500-1000 der
// Rakete bei 40/38/41/41/41/43/41 Prozent - da ist keine Drift, der man
// nachlaufen muesste. Es ist eine Versicherung gegen einen anderen Gegner
// oder einen anderen Server, und darum darf es langsam sein. Je gebuchtem
// Schuss und nicht je Sekunde: so behalten die fuenf Schuesse, die die
// Railgun pro Abend in ein Fach legt, vier Abende Geschichte, waehrend das
// Maschinengewehr den letzten beiden folgt.
#define AIM_RATE_DECAY	0.99f
#define AIM_RATE_SPEAK	8		// darunter sagt ein Fach gar nichts
#define AIM_RATE_FIRM	25		// und darunter beansprucht es keinen Rang

typedef struct {
	float	shots;			// gebucht und gealtert
	float	hits;
	float	landShots;		// davon die, deren Ziel im Flug aufsetzen sollte
	float	landHits;
	int		samples;		// ungealtert: die Schranke und die ehrliche Spalte
} aimRate_t;

static aimRate_t	aimRate[WP_NUM_WEAPONS][AIM_RANGES];
static qboolean		aimRateLoaded;
static qboolean		aimRateDirty;
static int			aimRateWritten;		// wann die Tabelle zuletzt auf der Platte war

static int CL_AimAssistRange( float dist ) {
	if ( dist <  500.0f ) {
		return 0;
	}
	if ( dist < 1000.0f ) {
		return 1;
	}
	if ( dist < 1500.0f ) {
		return 2;
	}
	if ( dist < 2000.0f ) {
		return 3;
	}
	return 4;
}

static float CL_AimAssistRangeStart( int range ) {
	static const float	start[AIM_RANGES] = { 0.0f, 500.0f, 1000.0f, 1500.0f, 2000.0f };

	return start[range < 0 ? 0 : ( range >= AIM_RANGES ? AIM_RANGES - 1 : range )];
}

static void CL_AimAssistRateLoad( void ) {
	union { char *c; void *v; }	file;
	const char					*line;
	aimRate_t					*r;
	float						shots, hits, landShots, landHits;
	int							weapon, range, samples, format = 0;
	long						length;

	aimRateLoaded = qtrue;

	length = FS_ReadFile( AIM_RATE_FILE, &file.v );
	if ( length <= 0 || !file.c ) {
		return;
	}

	line = file.c;
	while ( *line ) {
		if ( sscanf( line, "format %i", &format ) == 1 && format != AIM_RATE_FORMAT ) {
			break;			// von einem aelteren Bau, seine Faecher meinen etwas anderes
		}
		// Ein Zaehler hat Bedingungen, die ein Mittelwert nicht hat: mehr
		// Treffer als Schuesse ist keine knappe Datei, sondern eine falsche.
		if ( *line != '/' && sscanf( line, "%i %i %f %f %f %f %i",
				&weapon, &range, &shots, &hits, &landShots, &landHits, &samples ) == 7
			&& weapon > WP_NONE && weapon < WP_NUM_WEAPONS
			&& range >= 0 && range < AIM_RANGES
			&& shots >= 0.0f && hits >= 0.0f && landShots >= 0.0f && landHits >= 0.0f
			&& hits <= shots && landShots <= shots && landHits <= landShots && samples >= 0 ) {
			r = &aimRate[weapon][range];
			r->shots = shots;
			r->hits = hits;
			r->landShots = landShots;
			r->landHits = landHits;
			r->samples = samples;
		}

		while ( *line && *line != '\n' ) {
			line++;
		}
		while ( *line == '\n' || *line == '\r' ) {
			line++;
		}
	}

	FS_FreeFile( file.v );
}

static void CL_AimAssistRateSave( void ) {
	char		text[8192];
	aimRate_t	*r;
	int			weapon, range;

	if ( !aimRateDirty ) {
		return;
	}

	Com_sprintf( text, sizeof( text ),
		"format %i\n// was die Zielhilfe ueber ihre eigene Trefferquote gemessen hat.\n"
		"// weapon range shots hits landShots landHits samples\n", AIM_RATE_FORMAT );

	for ( weapon = WP_NONE + 1; weapon < WP_NUM_WEAPONS; weapon++ ) {
		for ( range = 0; range < AIM_RANGES; range++ ) {
			r = &aimRate[weapon][range];
			if ( !r->samples ) {
				continue;
			}
			Q_strcat( text, sizeof( text ), va( "%i %i %.4f %.4f %.4f %.4f %i\t// %s ab %.0fu\n",
				weapon, range, r->shots, r->hits, r->landShots, r->landHits, r->samples,
				CL_AimAssistWeaponName( weapon ), CL_AimAssistRangeStart( range ) ) );
		}
	}

	FS_WriteFile( AIM_RATE_FILE, text, strlen( text ) );
	aimRateDirty = qfalse;
}

/*
=================
CL_AimAssistRateBook

Ein abgeschlossener Schuss. Ein Fehlschuss ist genauso eine Messung wie ein
Treffer und muss mitgebucht werden, sonst steht ueberall hundert Prozent.
=================
*/
static void CL_AimAssistRateBook( int weapon, int range, qboolean hit, qboolean landing ) {
	aimRate_t	*r;

	if ( weapon <= WP_NONE || weapon >= WP_NUM_WEAPONS || range < 0 || range >= AIM_RANGES ) {
		return;
	}
	if ( !aimRateLoaded ) {
		CL_AimAssistRateLoad();
	}

	r = &aimRate[weapon][range];
	r->shots = r->shots * AIM_RATE_DECAY + 1.0f;
	r->hits = r->hits * AIM_RATE_DECAY + ( hit ? 1.0f : 0.0f );
	if ( landing ) {
		r->landShots = r->landShots * AIM_RATE_DECAY + 1.0f;
		r->landHits = r->landHits * AIM_RATE_DECAY + ( hit ? 1.0f : 0.0f );
	} else {
		r->landShots *= AIM_RATE_DECAY;
		r->landHits *= AIM_RATE_DECAY;
	}
	r->samples++;
	aimRateDirty = qtrue;
}

/*
=================
Die offenen Schuesse

Ein zweiter Ring neben dem des Lerners, und mit Absicht ein eigener: der dort
weist Hitscan ab, weist ein springendes Ziel ab, weist ein kaum bewegtes ab
und laesst nur einen offenen Schuss je Ziel zu. Alle vier Regeln sind fuer den
Vorhalt richtig und fuer die Trefferquote falsch - drei der vier Waffen in der
Aufzeichnung sind Hitscan, und der Fall "Ziel setzt auf" ist hier gerade die
interessanteste Zeile.

Drei Grenzen der Zuordnung, alle nachgemessen, alle hier und nicht in einer
Fussnote:

Zwei Raketen auf dasselbe Ziel. Bei knapp fuenfzehn Prozent der Raketen kommt
eine zweite innerhalb von hundert Millisekunden an, und kein Merkmal beim
Klienten sagt, welcher der beiden der Schaden gehoert. Trotzdem wird die
aeltere gebucht und nicht etwa beide verworfen: 93 Prozent dieser Paare liegen
im selben Entfernungsfach, die Reihenfolge kostet also rund ein Prozent falsch
einsortierte Raketen - und beide wegzuwerfen kostete fuenfzehn.

Splash auf einen Umstehenden. PERS_ATTACKEE_REMAINING nennt nur die
Gesundheit des zuletzt Verletzten, keinen Namen. Eine Rakete, die neben dem
gemeinten Ziel noch jemanden erwischt, ist von einer, die nur den Umstehenden
erwischt, beim Klienten nicht zu unterscheiden: knapp fuenf Prozent der
Schadensereignisse in der Aufzeichnung fielen so. Die Raketenzeile steht damit
etwa fuenf Prozent zu hoch, und daran ist nichts zu machen.

Das Maschinengewehr. Es schiesst alle 100 ms, das Trefferfenster ist 50 ms
breit, und fast neun von zehn Kugeln liegen in einer Salve: die einzelne Kugel
ist nicht zuzuordnen, bei keinem Versatz. Was stimmt, ist das Fach, denn zwei
aufeinanderfolgende Kugeln liegen bei 320 u/s Annaeherung dreissig Einheiten
auseinander, tief in einem fuenfhundert Einheiten breiten Fach. Die Quote je
Fach stimmt also, die Zuordnung der einzelnen Kugel nicht.
=================
*/
#define AIM_RATE_PENDING	32

typedef struct {
	int			target;			// auf wen geschossen wurde
	int			weapon;
	int			range;
	int			open, shut;		// Serverzeit, in der der Schaden zaehlt
	int			arrive;			// wann er ankommen sollte, die Mitte des Fensters
	int			fired;			// wann er losging, fuer die Reihenfolge der Anspruecke
	qboolean	landing;		// das Ziel sollte im Flug aufsetzen
	qboolean	hit;
	qboolean	live;
} aimRatePending_t;

static aimRatePending_t	aimRatePending[AIM_RATE_PENDING];
static int				aimRateBooked;		// gebuchte Schuesse in dieser Sitzung
static int				aimRateLost;		// und was der Ring nicht fassen konnte

/*
=================
CL_AimAssistRateForget

Alles offene vergessen, ohne es zu buchen. Bei einem Kartenwechsel und beim
Verbindungsende: was da noch flog, ist nie beobachtet worden, und ein nicht
beobachteter Schuss ist kein Fehlschuss.
=================
*/
static void CL_AimAssistRateForget( void ) {
	int	i;

	for ( i = 0; i < AIM_RATE_PENDING; i++ ) {
		aimRatePending[i].live = qfalse;
	}
}

static void CL_AimAssistRateWatch( const entityState_t *entity, int weapon, float flight,
		const vec3_t viewOrigin, qboolean landing ) {
	aimRatePending_t	*p = NULL;
	vec3_t				offset;
	int					arrive, open, shut, i;

	if ( !cl_aimAssistLearn->integer ) {
		return;
	}

	// Eine Waffe feuert hoechstens einmal je Server-Bild - der schnellste
	// Zyklus im Spiel ist der des Blitzwerfers mit genau einem. Der Klient
	// baut aber mehrere Befehle je Bild, und die Vorhersage des Waffentimers
	// kann auf jedem davon "jetzt" sagen: ein Abzug erzeugte so drei
	// Schusszeilen auf den Befehlen 236798, 236799 und 236802, eine einzige
	// Rakete - und drei Buchungen, eine getroffen und zwei daneben. Der Lerner
	// wehrt das mit "already watching this target" ab, diese Tabelle hatte
	// nichts dergleichen.
	for ( i = 0; i < AIM_RATE_PENDING; i++ ) {
		if ( aimRatePending[i].live && aimRatePending[i].weapon == weapon
			&& aimRatePending[i].fired == cl.snap.serverTime ) {
			return;
		}
	}

	// Die Entfernung zum schlichten Koerper, nicht zum vorgehaltenen Punkt.
	// Das ist die Reichweite, die der Spieler sieht, sie ist fuer Hitscan und
	// Geschoss dieselbe Groesse, und sie ist die, aus der der Zielpunkt
	// abgeleitet wird statt umgekehrt: fast jede fuenfte Rakete faellt sonst
	// in ein anderes Fach, weil ihr Vorhalt sie dorthin geschoben hat.
	VectorSubtract( entity->pos.trBase, viewOrigin, offset );

	arrive = cl.snap.serverTime + (int)( flight * 1000.0f + 0.5f );
	if ( flight > 0.0f ) {
		// Gemessen an Schuessen, die allein standen - kein zweiter auf
		// dasselbe Ziel mit derselben Waffe binnen 700 ms. Der Versatz
		// zwischen erwarteter Ankunft und gemeldetem Schaden lag bei 274
		// Raketen auf 0, bei 114 auf +50, bei 45 auf -50 und bei 25 auf +100.
		open = arrive - 50;
		shut = arrive + 100;
	} else {
		// Hitscan trifft im selben Bild, und das Fenster ist in Wahrheit null
		// Millisekunden breit, nicht fuenfzig: der Schaden eines Bildes T
		// erreicht den Klienten mit dem Schnappschuss danach, und der ist
		// zugleich die einzige Gelegenheit, an der dieser Schuss ihn noch
		// beanspruchen kann. Schaden, der auf T+50 gestempelt ist, wird erst
		// bei T+100 gesehen und ist dann vorbei.
		//
		// Gemessen ueber drei Sitzungen: 582 von 631 Schuessen mit Schaden auf
		// Versatz null wurden gutgeschrieben, aber null von neun, deren
		// Schaden allein auf +50 lag. Das ist etwa ein Prozent, systematisch
		// nach unten, und es bleibt so. Das Fenster zu verbreitern hiesse beim
		// Maschinengewehr mit seinen hundert Millisekunden Takt, dass eine
		// Kugel nach dem Schaden der naechsten greift - ein gemessener Verlust
		// von einem Prozent gegen einen ungemessenen Diebstahl.
		open = cl.snap.serverTime;
		shut = cl.snap.serverTime + 50;
	}

	// Einen freien Platz, und nur wenn keiner frei ist den aeltesten. Ein
	// laufender Zaehler haette hier blind ueberschrieben: die Fenster gehen
	// erst mit dem naechsten Schnappschuss zu, waehrend geschossen wird,
	// sobald der Waffentimer es zulaesst - beim Blitzwerfer zwanzigmal die
	// Sekunde. Ein ueberschriebener Eintrag wird nie gebucht, und das faellt
	// nirgends auf; darum wird das wenigstens gezaehlt.
	for ( i = 0; i < AIM_RATE_PENDING; i++ ) {
		if ( !aimRatePending[i].live ) {
			p = &aimRatePending[i];
			break;
		}
		if ( !p || aimRatePending[i].fired < p->fired ) {
			p = &aimRatePending[i];
		}
	}
	if ( p->live ) {
		aimRateLost++;
		if ( cl_aimAssistDebug->integer ) {
			Com_Printf( "aim rate: ring full, dropped %s ab %.0fu n %i frame %i\n",
				CL_AimAssistWeaponName( p->weapon ), CL_AimAssistRangeStart( p->range ),
				aimRateLost, cl.snap.serverTime );
		}
	}

	p->target = entity->clientNum;
	p->weapon = weapon;
	p->range = CL_AimAssistRange( VectorLength( offset ) );
	p->open = open;
	p->shut = shut;
	p->arrive = arrive;
	p->fired = cl.snap.serverTime;
	p->landing = landing;
	p->hit = qfalse;
	p->live = qtrue;
}

/*
=================
CL_AimAssistRateCredit

Der Treffer. PERS_HITS ist der einzige Zaehler, den der Server dem Klienten
ueber eigene Treffer schickt, und er steht schon in CL_AimAssistWoundWatch.

Hoechstens ein Anspruch je Schnappschuss, egal wie weit der Zaehler gesprungen
ist: eine Rakete, die direkt trifft, steppt ihn zweimal im selben Bild - einmal
direkt, einmal Splash - und eine Schrotladung bis zu elfmal. Die Zahl der
Schadensereignisse ist nicht die Zahl der Treffer.

Wer getroffen wurde, sagt der Server nicht. PERS_ATTACKEE_REMAINING traegt die
Gesundheit des zuletzt Verletzten und keinen Namen dazu; aimShotTarget kennt
nur den letzten Schuss, und eine Rakete ist eine Sekunde unterwegs. Also wird
ueber die Zeit zugeordnet und nicht ueber den Namen: wessen Fenster dieses Bild
enthaelt, dem gehoert der Schaden, und bei mehreren der aelteste Schuss zuerst.

Einer bekommt ihn, und zwar der, dessen erwartete Ankunft diesem Bild am
naechsten liegt. Wo zwei Fenster einander ueberschneiden, ist die Zuordnung
geraten - aber zu 93 Prozent liegen solche Paare ohnehin im selben
Entfernungsfach, also kostet das Raten etwa ein Prozent falsch einsortierte
Schuesse, waehrend beide zu verwerfen fuenfzehn Prozent der Proben kosten
wuerde. Genauigkeit, die mehr Messung kostet als sie Fehler spart, ist keine.

Nach der Ankunft und nicht nach dem Alter, und das ist der Unterschied
zwischen zwei Waffen: eine Rakete auf tausendzweihundert Einheiten ist eine
Sekunde unterwegs, und wer waehrenddessen zum Maschinengewehr wechselt und
trifft, hat einen Schaden erzeugt, der genau jetzt ankommt - die Rakete
dagegen erst mit bis zu hundert Millisekunden Abstand. Der aeltere Schuss
waere die Rakete gewesen, der richtige ist die Kugel. Die Reihenfolge des
Abschusses entscheidet nur noch bei gleichem Abstand.
=================
*/
static void CL_AimAssistRateCredit( void ) {
	aimRatePending_t	*p, *best = NULL;
	int					i, gap, least = 0;

	for ( i = 0; i < AIM_RATE_PENDING; i++ ) {
		p = &aimRatePending[i];
		if ( !p->live || p->hit
			|| cl.snap.serverTime < p->open || cl.snap.serverTime > p->shut ) {
			continue;
		}
		gap = abs( cl.snap.serverTime - p->arrive );
		if ( !best || gap < least || ( gap == least && p->fired < best->fired ) ) {
			best = p;
			least = gap;
		}
	}
	if ( best ) {
		best->hit = qtrue;
	}
}

/*
=================
CL_AimAssistRateClose

Alles, dessen Fenster vorbei ist, wird gebucht. Einmal je Schnappschuss.
=================
*/
static void CL_AimAssistRateClose( void ) {
	aimRatePending_t	*p;
	int					i;

	for ( i = 0; i < AIM_RATE_PENDING; i++ ) {
		p = &aimRatePending[i];
		if ( !p->live ) {
			continue;
		}
		// Eine Uhr, die springt - ein neues Spiel, ein langer Hänger - hat die
		// Bilder dazwischen nie gezeigt. Was dort offen war, wurde nicht
		// beobachtet, und ein nicht beobachteter Schuss ist kein Fehlschuss:
		// er wird vergessen statt gebucht. Fuenf Sekunden sind dafuer weit
		// jenseits jedes echten Fensters, das laengste misst hundertfuenfzig
		// Millisekunden.
		if ( cl.snap.serverTime < p->open - 5000 || cl.snap.serverTime > p->shut + 5000 ) {
			p->live = qfalse;
			continue;
		}
		if ( cl.snap.serverTime <= p->shut ) {
			continue;
		}
		p->live = qfalse;
		CL_AimAssistRateBook( p->weapon, p->range, p->hit, p->landing );
		aimRateBooked++;
		if ( cl_aimAssistDebug->integer ) {
			Com_Printf( "aim rated: %s ab %.0fu %s%s n %i frame %i\n",
				CL_AimAssistWeaponName( p->weapon ), CL_AimAssistRangeStart( p->range ),
				p->hit ? "getroffen" : "daneben", p->landing ? " aufsetzend" : "",
				aimRateBooked, cl.snap.serverTime );
		}
	}

	// Dieselbe Sicherung, die die Vorhalte-Tabelle bekommen hat: was gemessen
	// ist, liegt auch dann auf der Platte, wenn das Spiel nicht sauber endet.
	if ( aimRateDirty && ( cl.serverTime - aimRateWritten > 15000
			|| aimRateWritten > cl.serverTime ) ) {
		aimRateWritten = cl.serverTime;
		CL_AimAssistRateSave();
	}
}

/*
=================
CL_AimAssistRateDump

Die ganze Tabelle auf einmal, an das Kommando aimrate gebunden, damit die
Bank sie jederzeit lesen kann und nicht nur waehrend Schuesse eintreffen.
=================
*/
void CL_AimAssistRateDump( void ) {
	const aimRate_t	*r;
	int				weapon, range, boxes = 0;

	if ( !aimRateLoaded ) {
		CL_AimAssistRateLoad();
	}

	for ( weapon = WP_NONE + 1; weapon < WP_NUM_WEAPONS; weapon++ ) {
		for ( range = 0; range < AIM_RANGES; range++ ) {
			r = &aimRate[weapon][range];
			if ( !r->samples ) {
				continue;
			}
			boxes++;
			// "says" ist die Schranke, nicht die Zahl: 0 misst noch, 1 nennt
			// eine Quote ohne Anspruch auf einen Rang, 2 darf verglichen
			// werden. Die Grenzen kommen aus dem Wilson-Intervall bei p = 0,5
			// - bei acht Proben faellt die halbe Breite zum ersten Mal unter
			// 30 Punkte, bei fuenfundzwanzig unter 18, und 18 Punkte ist der
			// Abstand, ab dem sich zwei Nachbarfaecher unterscheiden lassen.
			// Sie stehen hier auf der Zeile, damit die Bank sie nicht ein
			// zweites Mal fuehren muss.
			Com_Printf( "aim rate: %s range %i from %.0f shots %.1f hits %.1f rate %.3f"
				" land %.1f landhits %.1f n %i says %i frame %i\n",
				CL_AimAssistWeaponName( weapon ), range, CL_AimAssistRangeStart( range ),
				r->shots, r->hits, r->shots > 0.0f ? r->hits / r->shots : 0.0f,
				r->landShots, r->landHits, r->samples,
				r->samples < AIM_RATE_SPEAK ? 0 : ( r->samples < AIM_RATE_FIRM ? 1 : 2 ),
				cl.snap.serverTime );
		}
	}

	if ( !boxes ) {
		Com_Printf( "aim rate: nothing measured yet\n" );
	}
}


/*
=================
CL_AimAssistPredict

Where the target stands after the given time.  A player in the air is pulled
down by the same gravity the game uses, so carrying its upward speed on in a
straight line would aim high above a bot that merely jumped.  The box trace
keeps the guess out of the floor and out of walls the target cannot pass.
A negative time is a moment back along its line, wanted only for the smooth
picture between snapshots: nothing to clip and nothing to drop there.
=================
*/
static float CL_AimAssistLanding( const entityState_t *entity, const vec3_t motion,
		float gravity, float limit, vec3_t mins, vec3_t maxs, float *floorOut );

static void CL_AimAssistPredict( const entityState_t *entity, int weapon, float time, vec3_t predicted,
		qboolean *blocked, qboolean *pinned ) {
	vec3_t		mins, maxs, stepMins, start, end, remaining, motion, above, below;
	float		gravity, sideways, floor, pace, fall, rest, landing;
	trace_t		trace;
	qboolean	grounded, floats, stopped, snapped, walks;
	int			i;

	grounded = entity->groundEntityNum != ENTITYNUM_NONE;
	floats = CL_AimAssistFloats( entity );
	stopped = qfalse;
	snapped = qfalse;
	landing = 0.0f;

	CL_AimAssistVelocity( entity, motion );
	CL_AimAssistHull( entity, mins, maxs );
	pace = sqrt( motion[0] * motion[0] + motion[1] * motion[1] );
	gravity = cl.snap.ps.gravity > 0 ? cl.snap.ps.gravity : DEFAULT_GRAVITY;

	// A jump lasts about two thirds of a second and a rocket flies for about
	// one, so the shot commonly arrives after the target has come down again.
	// Carrying it forward at its jumping speed for the whole flight was worth
	// a hundred and seventy units of error where a target that stayed in the
	// air was guessed to within five: the record split the same shots forty-
	// nine per cent against seventeen on whether the feet stayed where they
	// were. So the moment the arc meets the floor is solved for, and what
	// comes after it is a run like any other runner's.
	fall = ( !grounded && !floats && time > 0.0f )
		? CL_AimAssistLanding( entity, motion, gravity, time, mins, maxs, &landing ) : -1.0f;
	walks = grounded || fall >= 0.0f;

	// Only the sideways guess of a target on the ground is damped, in two
	// ways: by how long bots hold a direction at all, and by how much this one
	// is turning right now; the climb of a ramp is part of the same run and is
	// damped with it. A target in the air cannot change where it is going -
	// the game gives it no friction and next to no steering - so its whole
	// course is kept, the rise and the fall included. Swimming and flying can
	// turn, and are damped like running.
	//
	// Two of those three do nothing for a landing target, and it is worth
	// saying so rather than letting the call list imply otherwise. The turn
	// damping is measured by comparing two snapshots' velocities, and a target
	// in the air has no turn to measure, so it answers one whatever is asked.
	// The learned factor is read at the band of the leftover time, but no
	// airborne shot ever teaches that table - those samples are dropped on
	// purpose - so it answers with the prior. What actually shortens the guess
	// is the hold time, and that is the one that should.
	if ( fall >= 0.0f ) {
		rest = time - fall;
		sideways = fall + CL_AimAssistSideways( rest ) * CL_AimAssistTrust( entity, rest )
			* CL_AimAssistTune( weapon, rest, pace );
	} else if ( !grounded && !floats ) {
		sideways = time;
	} else {
		sideways = CL_AimAssistSideways( time ) * CL_AimAssistTrust( entity, time )
			* CL_AimAssistTune( weapon, time, pace );
	}
	end[0] = entity->pos.trBase[0] + motion[0] * sideways;
	end[1] = entity->pos.trBase[1] + motion[1] * sideways;
	end[2] = fall >= 0.0f ? landing
		: entity->pos.trBase[2] + motion[2] * ( grounded ? sideways : time );

	if ( time <= 0.0f ) {
		VectorCopy( end, predicted );
		if ( blocked ) {
			*blocked = qfalse;
		}
		if ( pinned ) {
			*pinned = qfalse;
		}
		return;
	}

	// Gravity acts on anything off the floor, and trDelta[2] carries the rest:
	// a ramp, a jump pad, the first moment of a jump. Pinning the height threw
	// all of that away. In water and in flight there is no falling. One that
	// has already been set down on its floor above is past all of that.
	if ( !grounded && !floats && fall < 0.0f ) {
		end[2] -= 0.5f * gravity * time * time;
	}

	// The game lifts a walking player over anything up to STEPSIZE, so the box
	// that clips the guess starts above that height: a curb, a stair riser or a
	// ramp is no obstacle to the target and must not cut its lead short. Only
	// what would stop the target itself may stop the prediction.
	VectorCopy( mins, stepMins );
	stepMins[2] += STEPSIZE;
	VectorCopy( entity->pos.trBase, start );
	VectorSubtract( end, start, remaining );
	VectorCopy( end, predicted );

	for ( i = 0; i < 8; i++ ) {
		VectorAdd( start, remaining, end );
		CM_BoxTrace( &trace, start, end, stepMins, maxs, 0, MASK_PLAYERSOLID, qfalse );

		// A solid start says nothing about where the target can go. On the
		// first pass the whole lead is kept rather than thrown away; later on
		// the last good point stands.
		if ( trace.startsolid || trace.allsolid ) {
			if ( i == 0 ) {
				VectorCopy( end, predicted );
			} else {
				stopped = qtrue;
			}
			break;
		}

		VectorCopy( trace.endpos, predicted );
		if ( trace.fraction >= 1.0f ) {
			stopped = qfalse;		// the run got all the way, stairs or not
			break;
		}
		stopped = qtrue;
		if ( !walks ) {
			break;
		}

		// Stopped. The box already passes over a step's worth, so what stopped
		// it stands higher than that above the feet - unless the ground rose
		// underneath on the way, as stairs do, and the feet are now below it.
		// Settle onto that ground and go on from there, the way the game walks
		// a player up a flight of stairs one step at a time. Ground no higher
		// than before means a wall or a crate, and the target stops here.
		VectorCopy( trace.endpos, above );
		above[2] += STEPSIZE;
		VectorCopy( trace.endpos, below );
		below[2] -= STEPSIZE;
		CM_BoxTrace( &trace, above, below, mins, maxs, 0, MASK_PLAYERSOLID, qfalse );
		if ( trace.startsolid || trace.allsolid || trace.fraction >= 1.0f ) {
			break;
		}
		floor = trace.endpos[2];
		if ( floor <= start[2] + 0.5f ) {
			break;
		}

		VectorSubtract( end, predicted, remaining );
		VectorCopy( predicted, start );
		start[2] = floor;
		VectorCopy( start, predicted );
	}

	// Put the guess back on the ground it would be standing on: a target that
	// runs up stairs rises with them, one that lands does not sink into the
	// floor, and one high in the air finds nothing here and keeps its arc.
	VectorCopy( predicted, above );
	above[2] += STEPSIZE;
	VectorCopy( predicted, below );
	below[2] -= 8192.0f;
	CM_BoxTrace( &trace, above, below, mins, maxs, 0, MASK_PLAYERSOLID, qfalse );

	if ( !trace.startsolid && !trace.allsolid && trace.fraction < 1.0f ) {
		floor = trace.endpos[2];

		// only a move worth the name counts as being set down: a runner on
		// the flat sits on its floor to begin with, give or take a hair
		if ( predicted[2] < floor ) {
			snapped = floor - predicted[2] > 0.5f;
			predicted[2] = floor;					// landed, or walked up a step
		} else if ( walks && predicted[2] - floor <= STEPSIZE ) {
			snapped = predicted[2] - floor > 0.5f;
			predicted[2] = floor;					// walked down a step
		}
	}

	if ( blocked ) {
		*blocked = stopped;
	}
	// A guess that comes down on its floor within the flight is pinned there
	// just as surely as one the floor trace set down, and it matters that the
	// caller hears so: the smoothing correction asks this before deciding how
	// fast the point is moving upward, and a landing point is not moving at
	// all. Without it a jumping target made the aim bob by up to ninety units
	// between shots, because the correction went on treating a point that had
	// already settled as one still falling.
	if ( pinned ) {
		*pinned = snapped || fall >= 0.0f;
	}
}


/*
=================
CL_AimAssistLanding

How long a target in the air has before its feet meet the floor, in seconds,
or -1 if it is still falling when the shot gets there.

The arc itself needs no measuring: a player off the ground in Quake 3 has no
friction and almost no steering, so the parabola is exact, and the record bears
that out - a target still in the air on arrival was guessed to within five
units. What was missing was the end of it. A rocket flies for about a second
and a jump lasts about two thirds of one.

The landing point and the landing time each want the other, so the floor is
looked for under where the arc is heading, the time is solved from that floor,
and the pair is corrected once. A third pass moves nothing worth having.
=================
*/
static float CL_AimAssistLanding( const entityState_t *entity, const vec3_t motion,
		float gravity, float limit, vec3_t mins, vec3_t maxs, float *floorOut ) {
	vec3_t		from, to;
	trace_t		trace;
	float		fall, drop, root, floor, answer, best;
	int			i;

	if ( gravity <= 0.0f || limit <= 0.0f ) {
		return -1.0f;
	}

	// The first probe goes straight down from where the target is now, not out
	// at the far end of the flight. Probing at the far end asks about ground
	// the target may never reach: a bot hopping a gap was asked about the pit
	// it was jumping over, came back with a fall of three seconds, and the
	// whole model was abandoned for exactly the shot it was written for.
	answer = -1.0f;
	best = 0.0f;
	fall = 0.0f;

	for ( i = 0; i < 3; i++ ) {
		from[0] = entity->pos.trBase[0] + motion[0] * fall;
		from[1] = entity->pos.trBase[1] + motion[1] * fall;
		// looked for from the height it set out at, so the trace never starts
		// inside the very floor it is looking for
		from[2] = entity->pos.trBase[2];
		VectorCopy( from, to );
		to[2] -= 8192.0f;
		CM_BoxTrace( &trace, from, to, mins, maxs, 0, MASK_PLAYERSOLID, qfalse );
		if ( trace.startsolid || trace.allsolid || trace.fraction >= 1.0f ) {
			break;					// no ground under that point; keep what stands
		}
		floor = trace.endpos[2];

		// half g t squared less the climb, solved for the moment it arrives
		drop = entity->pos.trBase[2] - floor;
		root = motion[2] * motion[2] + 2.0f * gravity * drop;
		fall = ( motion[2] + sqrt( root ) ) / gravity;
		if ( fall <= 0.0f || fall >= limit ) {
			break;					// still in the air when the shot arrives
		}

		// A landing found is kept. A later pass may refine where it happens,
		// but if the refined probe finds nothing this answer still stands -
		// giving up on the second look would be worse than the first look.
		answer = fall;
		best = floor;
	}

	if ( answer >= 0.0f && floorOut ) {
		*floorOut = best;
	}
	return answer;
}

/*
=================
CL_AimAssistDueDown

Whether the target's feet are expected back on the floor before a shot fired
now would arrive. One place, because three of them ask: the record writes it
down, the shot grade steps on it, and the hit-rate table keeps a second pair
of counters for it. A target already standing is not "coming down"; neither is
one in water or on a ladder, which never lands at all.
=================
*/
static float CL_AimAssistDueDown( const entityState_t *entity, float flight ) {
	vec3_t	motion, mins, maxs;

	if ( flight <= 0.0f || entity->groundEntityNum != ENTITYNUM_NONE
		|| CL_AimAssistFloats( entity ) ) {
		return -1.0f;
	}

	CL_AimAssistVelocity( entity, motion );
	CL_AimAssistHull( entity, mins, maxs );
	return CL_AimAssistLanding( entity, motion,
		cl.snap.ps.gravity > 0 ? cl.snap.ps.gravity : DEFAULT_GRAVITY,
		flight, mins, maxs, NULL );
}


/*
=================
CL_AimAssistShotGrade

How good the record says this shot is, from nought to three.

The measured table first: how many splash radii wide the shot still scatters at
this flight time and this target speed. A box with too few samples to speak
falls back to the flight bands the record was cut on. Then the footing moves it
one step either way, because that splits a single band harder than the band
splits itself - at eight hundred to twelve hundred milliseconds of flight the
record ran twenty-five per cent against a target on its feet, eighty against one
that stays in the air, and eight against one that comes down on the way.
=================
*/
static int CL_AimAssistShotGrade( const entityState_t *entity, int weapon, float flight ) {
	vec3_t	motion, mins, maxs;
	float	pace, scatter, reach, touchdown, gravity;
	int		grade;

	CL_AimAssistVelocity( entity, motion );
	pace = sqrt( motion[0] * motion[0] + motion[1] * motion[1] );

	scatter = CL_AimAssistScatter( weapon, flight, pace );
	if ( scatter > 0.0f ) {
		reach = CL_AimAssistHitRadius( weapon ) / scatter;
		grade = reach >= 1.0f ? 3 : reach >= 0.5f ? 2 : reach >= 0.25f ? 1 : 0;
	} else {
		grade = flight >= 1.2f ? 0 : flight >= 0.8f ? 1 : flight >= 0.4f ? 2 : 3;
	}

	if ( entity->groundEntityNum == ENTITYNUM_NONE && !CL_AimAssistFloats( entity ) ) {
		CL_AimAssistHull( entity, mins, maxs );
		gravity = cl.snap.ps.gravity > 0 ? cl.snap.ps.gravity : DEFAULT_GRAVITY;
		touchdown = CL_AimAssistLanding( entity, motion, gravity, flight, mins, maxs, NULL );
		if ( touchdown >= 0.0f && flight > 0.4f ) {
			grade--;			// its feet are back down before the shot arrives
		} else if ( touchdown < 0.0f && flight > 0.8f ) {
			grade++;			// a solved parabola the whole way
		}
	}

	return grade < 0 ? 0 : grade > 3 ? 3 : grade;
}


/*
=================
CL_AimAssistImpact

The point on the target the shot is meant to reach. A splash weapon at a
target on the floor goes for the feet: a near miss still bursts on the ground
under it, where a miss past the body would fly on. Anything else, and anything
at a target in the air, goes for the body.
=================
*/
static void CL_AimAssistImpact( const entityState_t *entity, int weapon, const vec3_t predicted, vec3_t impact ) {
	VectorCopy( predicted, impact );
	if ( ( weapon == WP_ROCKET_LAUNCHER || weapon == WP_GRENADE_LAUNCHER || weapon == WP_BFG )
		&& entity->groundEntityNum != ENTITYNUM_NONE ) {
		impact[2] -= 20.0f;
	} else {
		impact[2] += CL_AimAssistBodyHeight( entity );
	}
}


/*
=================
CL_AimAssistArcWeapon

The weapons whose shot falls under gravity from the muzzle.
=================
*/
static qboolean CL_AimAssistArcWeapon( int weapon ) {
	return weapon == WP_GRENADE_LAUNCHER
#ifdef MISSIONPACK
		|| weapon == WP_PROX_LAUNCHER
#endif
		;
}


/*
=================
CL_AimAssistLaunch

How the game really throws a grenade at a point the view is on: the forward
vector tipped up by a fifth and normalised again, at 700 units a second, from
a muzzle fourteen units out along the untipped view (g_weapon.c, g_missile.c).
Returns the horizontal flight time to the point's distance, or zero straight up.
=================
*/
static float CL_AimAssistLaunch( const vec3_t eye, const vec3_t aim, vec3_t muzzle, vec3_t velocity ) {
	vec3_t	direction, tipped;
	float	distance, speed;

	VectorSubtract( aim, eye, direction );
	VectorNormalize( direction );
	VectorMA( eye, 14.0f, direction, muzzle );

	VectorCopy( direction, tipped );
	tipped[2] += 0.2f;
	VectorNormalize( tipped );
	VectorScale( tipped, 700.0f, velocity );

	distance = sqrt( ( aim[0] - muzzle[0] ) * ( aim[0] - muzzle[0] )
		+ ( aim[1] - muzzle[1] ) * ( aim[1] - muzzle[1] ) );
	speed = sqrt( velocity[0] * velocity[0] + velocity[1] * velocity[1] );
	if ( speed < 1.0f ) {
		return 0.0f;
	}
	return distance / speed;
}


/*
=================
CL_AimAssistArc

Where to point for a grenade to come down on the impact point, and how long
it takes to get there.

The aim has to be solved, not offset: point somewhere, see where the arc
crosses the target's distance, raise the point by the shortfall, and again -
each round brings the height at that distance closer, and a few rounds settle
it. Far below the eye a full step overshoots, so the step is halved whenever
the miss changes sign. Out of throwing range the miss stops shrinking and
would grow with every round from there; the best aim found is the one handed
out then - the throw that lands nearest - and the loop leaves with the time
that belongs to the aim it hands out.
=================
*/
static void CL_AimAssistArc( const vec3_t eye, const vec3_t impact, vec3_t aim, float *timeOut ) {
	vec3_t	muzzle, velocity;
	float	time, height, miss, lastMiss, bestMiss, bestHeight, bestTime, gain;
	int		i;

	VectorCopy( impact, aim );
	time = 0.0f;
	lastMiss = 0.0f;
	bestMiss = 0.0f;
	bestHeight = aim[2];
	bestTime = 0.0f;
	gain = 1.0f;

	for ( i = 0; i < 16; i++ ) {
		time = CL_AimAssistLaunch( eye, aim, muzzle, velocity );
		if ( time <= 0.0f ) {
			break;			// straight up: no arc reaches out from here
		}

		height = muzzle[2] + velocity[2] * time - 0.5f * DEFAULT_GRAVITY * time * time;
		miss = impact[2] - height;
		if ( i == 0 || fabs( miss ) < fabs( bestMiss ) ) {
			bestMiss = miss;
			bestHeight = aim[2];
			bestTime = time;
		}
		if ( fabs( miss ) < 0.25f || i == 15 ) {
			break;
		}
		if ( i > 0 ) {
			if ( ( miss > 0.0f ) != ( lastMiss > 0.0f ) ) {
				gain *= 0.5f;		// overshot: damp
			} else if ( fabs( miss ) >= fabs( lastMiss ) ) {
				break;				// no closer on the same side: out of range
			}
		}
		lastMiss = miss;
		if ( aim[2] - impact[2] + miss * gain > 4096.0f ) {
			break;			// out of range
		}
		aim[2] += miss * gain;
	}

	aim[2] = bestHeight;
	if ( timeOut ) {
		*timeOut = bestTime;
	}
}


/*
=================
CL_AimAssistArcClear

Whether a grenade thrown at the solved aim gets to its distance without
hitting anything on the way. The straight line to the aim point says nothing
here - that point is well above the arc, and under a ceiling the line hits
what the grenade clears. The arc itself is traced, a piece at a time.
=================
*/
static qboolean CL_AimAssistArcClear( const vec3_t eye, const vec3_t aim ) {
	vec3_t	muzzle, velocity, from, to;
	float	time, t;
	trace_t	trace;
	int		i;

	time = CL_AimAssistLaunch( eye, aim, muzzle, velocity );
	if ( time <= 0.0f ) {
		return qfalse;
	}

	VectorCopy( muzzle, from );
	for ( i = 1; i <= 8; i++ ) {
		t = time * i / 8.0f;
		to[0] = muzzle[0] + velocity[0] * t;
		to[1] = muzzle[1] + velocity[1] * t;
		to[2] = muzzle[2] + velocity[2] * t - 0.5f * DEFAULT_GRAVITY * t * t;
		CM_BoxTrace( &trace, from, to, vec3_origin, vec3_origin, 0, MASK_SHOT, qfalse );
		if ( trace.fraction < 1.0f ) {
			return qfalse;
		}
		VectorCopy( to, from );
	}

	return qtrue;
}


/*
=================
CL_AimAssistFlight

How long after the snapshot's world a missile fired now meets the target at
the given predicted position. The missile starts fourteen units out along the
view and meets the box at its near face, and it is spawned a prestep along,
which the game counts as already flown. A grenade flies its arc.
=================
*/
static float CL_AimAssistFlight( const entityState_t *entity, int weapon, const vec3_t eye, const vec3_t predicted ) {
	vec3_t	impact, aim, offset;
	float	speed, time;

	speed = CL_AimAssistProjectileSpeed( weapon );
	CL_AimAssistImpact( entity, weapon, predicted, impact );

	if ( CL_AimAssistArcWeapon( weapon ) ) {
		CL_AimAssistArc( eye, impact, aim, &time );
		return time - 15.0f / speed - MISSILE_PRESTEP;
	}

	VectorSubtract( impact, eye, offset );
	return ( VectorLength( offset ) - 14.0f - 15.0f ) / speed - MISSILE_PRESTEP;
}


/*
=================
CL_AimAssistTargetPoint

Predicts where the target will be when the shot reaches it, and how long that
takes. With exact set the point is the one the server will really test the
shot against; otherwise it is the smooth one for steering between shots.

A hitscan shot resolves against exactly the world the snapshot shows: on a
listen server the command runs before the next game frame, and nothing in the
world moves between game frames. Its lead is zero. A missile is different only
because the world does move while it flies, and it moves in frames: the
missile is traced one frame's flight at a time against the targets as they
stand at the end of that frame, so the frame whose segment reaches the target
is the one whose target position counts, and the lead is a whole number of
frames. Between shots the unrounded time is used instead, centred on the same
average, so the aim does not tick every time the distance crosses a frame.

onShot says this command is the one the shot goes out on, which is a different
question from exact: exact asks for the frame-quantised point, and only
single-shot weapons are given it, while the smoothing correction has to come
off for every weapon on the firing command. It is asked separately because the
two were conflated, and a machinegun - always exact 0 - was carrying the
correction into every shot it fired.
=================
*/
static void CL_AimAssistTargetPoint( const entityState_t *entity, const vec3_t viewOrigin,
		int weapon, qboolean exact, qboolean onShot, vec3_t targetOrigin, float *leadOut ) {
	vec3_t		motion, impact;
	float		projectileSpeed, lead, frame, flight, gravity;
	qboolean	blocked, pinned;
	int			i;

	projectileSpeed = CL_AimAssistProjectileSpeed( weapon );
	frame = CL_AimAssistFrameTime();
	lead = 0.0f;
	blocked = qfalse;
	pinned = qfalse;

	if ( projectileSpeed <= 0.0f ) {
		// hitscan: the world the snapshot shows is the one hit
		CL_AimAssistPredict( entity, weapon, 0.0f, targetOrigin, &blocked, &pinned );
	} else if ( exact ) {
		// The frame the missile meets the target in is the first whose
		// segment reaches the target where it stands at the end of that
		// frame. A target walking into the shot is met a frame sooner than
		// its distance now says, one walking away a frame later; asking frame
		// by frame settles that where a fixed-point search can go round in
		// circles between two answers.
		for ( i = 1; i < 60; i++ ) {
			lead = i * frame;
			CL_AimAssistPredict( entity, weapon, lead, targetOrigin, &blocked, &pinned );
			flight = CL_AimAssistFlight( entity, weapon, viewOrigin, targetOrigin );
			if ( flight <= lead ) {
				break;
			}
		}
	} else {
		// between shots: the unrounded time, centred on the frames above,
		// settled by repeating distance over speed
		for ( i = 0; i < 5; i++ ) {
			CL_AimAssistPredict( entity, weapon, lead, targetOrigin, NULL, NULL );
			flight = CL_AimAssistFlight( entity, weapon, viewOrigin, targetOrigin );
			lead = Com_Clamp( 0.0f, 3.0f, flight + frame * 0.5f );
		}
		CL_AimAssistPredict( entity, weapon, lead, targetOrigin, &blocked, &pinned );
	}

	// The smooth picture between snapshots: the zero-mean correction of
	// CL_AimAssistPhase, applied to the rate at which the predicted point
	// moves, so that it cancels the whole step the point takes when a
	// snapshot lands. For a runner that rate is its velocity. For a target in
	// the air the point is a lead ahead on a falling arc, and a snapshot
	// brings a velocity that has fallen a frame further, which moves the point
	// by gravity times the lead on top: the rate is the velocity less that.
	// A point that a wall stopped does not move with the target at all, and
	// one set down on the floor does not move in height; those get no
	// correction they would only wobble by.
	//
	// Not on the shot - and the shot is onShot, not exact. The correction is
	// zero-mean over all client frames, but firing commands are not drawn
	// evenly from those: a hundred-millisecond weapon cycle beats against a
	// fifty-millisecond snapshot cycle and lands on the same part of the
	// sawtooth again and again, so what averages to nothing over the picture
	// came to a standing lead of about seven milliseconds over the shots -
	// always ahead of the target, never behind.
	if ( !onShot && !blocked ) {
		CL_AimAssistVelocity( entity, motion );
		if ( pinned ) {
			motion[2] = 0.0f;
		} else if ( entity->groundEntityNum == ENTITYNUM_NONE && !CL_AimAssistFloats( entity ) ) {
			gravity = cl.snap.ps.gravity > 0 ? cl.snap.ps.gravity : DEFAULT_GRAVITY;
			motion[2] -= gravity * lead;
		}
		VectorMA( targetOrigin, CL_AimAssistPhase(), motion, targetOrigin );
	}

	// the body or the feet, and for a grenade the point to aim at so that
	// its arc comes down there
	CL_AimAssistImpact( entity, weapon, targetOrigin, impact );
	if ( CL_AimAssistArcWeapon( weapon ) ) {
		CL_AimAssistArc( viewOrigin, impact, targetOrigin, NULL );
	} else {
		VectorCopy( impact, targetOrigin );
	}

	if ( leadOut ) {
		*leadOut = lead;
	}
}


/*
=================
CL_AimAssistMoverAt

Where a mover stands at the given moment. The client does not link the game's
own trajectory code, so the few kinds a door or a platform uses are read here:
standing, running to a stop, swinging on a sine. Anything else is left where
the snapshot put it, which is the right answer for everything that holds still.
=================
*/
static void CL_AimAssistMoverAt( const trajectory_t *tr, int atTime, vec3_t result ) {
	float	elapsed, phase;

	switch ( tr->trType ) {
	case TR_LINEAR:
		elapsed = ( atTime - tr->trTime ) * 0.001f;
		VectorMA( tr->trBase, elapsed, tr->trDelta, result );
		break;
	case TR_LINEAR_STOP:
		if ( atTime > tr->trTime + tr->trDuration ) {
			atTime = tr->trTime + tr->trDuration;
		}
		elapsed = ( atTime - tr->trTime ) * 0.001f;
		if ( elapsed < 0.0f ) {
			elapsed = 0.0f;
		}
		VectorMA( tr->trBase, elapsed, tr->trDelta, result );
		break;
	case TR_SINE:
		if ( tr->trDuration <= 0 ) {
			VectorCopy( tr->trBase, result );
			break;
		}
		phase = sin( ( atTime - tr->trTime ) / (float)tr->trDuration * M_PI * 2.0f );
		VectorMA( tr->trBase, phase, tr->trDelta, result );
		break;
	default:
		VectorCopy( tr->trBase, result );
		break;
	}
}


/*
=================
CL_AimAssistShotClear

Whether a shot leaving now reaches the point it is aimed at.

It is traced from where the server builds the muzzle - fourteen units out
along the line, which is what carries it clear of the player's own box - to
that point, and no further. Not along the view for the target's distance:
the view is what the steering is about to change, and a led point does not
lie at the target's distance anyway. A grenade is judged by its arc, because
a straight line says nothing about where a thrown thing goes.

The world tree alone is not enough to answer this. A closed door is an inline
model that the world trace walks straight through, so the movers the snapshot
carries are clipped afterwards, the way the cgame does it for everything it
traces. The omission was one-sided: it could only ever wave a blocked shot
through, never hold a clear one.
=================
*/
static qboolean CL_AimAssistShotClear( const vec3_t eye, const vec3_t aim, int weapon ) {
	trace_t				trace;
	vec3_t				direction, muzzle, origin, angles, zero;
	const entityState_t	*entity;
	clipHandle_t		model;
	float				reach;
	int					i;

	if ( CL_AimAssistArcWeapon( weapon ) ) {
		return CL_AimAssistArcClear( eye, aim );
	}

	VectorSubtract( aim, eye, direction );
	reach = VectorNormalize( direction );
	if ( reach <= 0.0f ) {
		return qtrue;			// nowhere to reach, nothing in the way
	}
	// Never past the point itself: at arm's length the muzzle would end up
	// behind the target and the trace would run backwards through whatever
	// stands there, and report cover that is not in the way of anything.
	VectorMA( eye, reach < 28.0f ? reach * 0.5f : 14.0f, direction, muzzle );
	SnapVector( muzzle );
	VectorClear( zero );

	CM_BoxTrace( &trace, muzzle, aim, zero, zero, 0, MASK_SHOT, qfalse );
	if ( trace.fraction < 1.0f ) {
		return qfalse;
	}

	for ( i = 0; i < cl.snap.numEntities; i++ ) {
		entity = &cl.parseEntities[( cl.snap.parseEntitiesNum + i ) & ( MAX_PARSE_ENTITIES - 1 )];
		// The bound is not paranoia: CM_InlineModel drops the game on a number
		// it does not have, and this reads one straight out of a snapshot.
		if ( entity->eType != ET_MOVER || entity->solid != SOLID_BMODEL ||
			 entity->modelindex <= 0 || entity->modelindex >= CM_NumInlineModels() ) {
			continue;
		}
		model = CM_InlineModel( entity->modelindex );
		CL_AimAssistMoverAt( &entity->pos, cl.serverTime, origin );
		CL_AimAssistMoverAt( &entity->apos, cl.serverTime, angles );
		CM_TransformedBoxTrace( &trace, muzzle, aim, zero, zero,
			model, MASK_SHOT, origin, angles, qfalse );
		if ( trace.fraction < 1.0f ) {
			return qfalse;
		}
	}
	return qtrue;
}


/*
=================
CL_AimAssistHoldReason

Why the shot this command would fire is not worth firing, or -1 when it is.

Two things can make it pointless: the way to the point being aimed at is shut,
or the shot would go off in our own face. The first is asked of the led point
first and of the plain body second, exactly as the steering does, so a bot
behind a pillar whose lead stands in the open is still a shot worth taking.

It is asked before the shot is predicted, and that ordering is not cosmetic:
the game zeroes the weapon timer on any command that arrives without the
trigger, so a local prediction that had already counted the shot would sit one
shot ahead of the server until a snapshot caught up, and the exact aim would
land on the wrong command in the meantime.
=================
*/
#define AIM_HOLD_COVER		0
#define AIM_HOLD_SPLASH		1
#define AIM_HOLD_LOTTERY	2

// which weapons take the frame-quantised point, decided beside the weapon
// timer further down and wanted here to ask about the very same point
static qboolean CL_AimAssistSingleShot( int weapon );

static int CL_AimAssistHoldReason( const entityState_t *entity, int weapon, const vec3_t eye,
		float *distanceOut ) {
	vec3_t		aim, body, offset, motion;
	float		lead, splash, distance, spread;
	qboolean	exact;

	if ( distanceOut ) {
		*distanceOut = 0.0f;
	}
	// Nothing was picked, so this shot is the player's own. The gauntlet has
	// no line to speak of either - what it can reach is a matter of range, not
	// of what stands between.
	if ( !entity || weapon == WP_GAUNTLET ) {
		return -1;
	}

	// The point the shot will really be aimed at, smoothing correction left
	// out and frame-quantised where the steering would quantise it, exactly as
	// the firing command will have it. Asking about a point the shot is not
	// going to take would be a different question, and at a pillar edge a few
	// units of difference is the whole answer.
	exact = cl_aimAssistExact->integer == 2
		|| ( cl_aimAssistExact->integer == 1 && CL_AimAssistSingleShot( weapon ) );
	CL_AimAssistTargetPoint( entity, eye, weapon, exact, qtrue, aim, &lead );
	VectorSubtract( aim, eye, offset );
	distance = VectorLength( offset );
	if ( distanceOut ) {
		*distanceOut = distance;
	}

	// A shot the measurement already calls a lottery. The table keeps, per
	// weapon and per flight time, how wide the corrected shot still scattered;
	// where that is several times the radius the shot can do anything within,
	// nothing comes of it. Over four hundred and forty-five rockets the record
	// ran seventy-four per cent inside four hundred milliseconds of flight and
	// eleven per cent past twelve hundred, and a third of them were fired into
	// that last band - a hundred and forty-four rockets for sixteen hits.
	//
	// Off unless somebody asks for it: this is the one hold that refuses a
	// shot the player can see is possible, rather than one that is blocked.
	if ( cl_aimAssistHoldLottery->value > 0.0f ) {
		CL_AimAssistVelocity( entity, motion );
		spread = CL_AimAssistScatter( weapon, lead,
			sqrt( motion[0] * motion[0] + motion[1] * motion[1] ) );
		if ( spread > 0.0f
			&& spread > CL_AimAssistHitRadius( weapon ) * cl_aimAssistHoldLottery->value ) {
			if ( distanceOut ) {
				*distanceOut = spread;		// the spread is the news here, not the range
			}
			return AIM_HOLD_LOTTERY;
		}
	}

	// A splash weapon going off within its own reach of us did not get through
	// either. Here it is the bare radius, with none of the margin the steering
	// keeps: this one takes the shot away from the player, so it may only do
	// so where the blast would really arrive - a hundred and twenty units for
	// a rocket, twenty for plasma.
	if ( CL_AimAssistProjectileSpeed( weapon ) > 0.0f ) {
		splash = CL_AimAssistHitRadius( weapon );
		if ( distance < splash ) {
			return AIM_HOLD_SPLASH;
		}
	}

	if ( CL_AimAssistShotClear( eye, aim, weapon ) ) {
		return -1;
	}

	VectorCopy( entity->pos.trBase, body );
	body[2] += CL_AimAssistBodyHeight( entity );
	if ( CL_AimAssistShotClear( eye, body, weapon ) ) {
		return -1;
	}
	return AIM_HOLD_COVER;
}


/*
=================
What makes one bot a better target than another.

Nine things can speak for a target, and which of them counts how much is the
player's to decide: cl_aimAssistPriority carries a weight for each, and the
bench lets them be dragged into order. Every one of them answers with a value
between nothing and one, the weights are what turn those into a single number,
and the highest wins. A weight of zero takes the question out of the running
altogether.

They are deliberately smooth. A target does not stop counting because it is a
few degrees further from the crosshair or a hundred units further away, it
just counts a little less, so the pick does not flap between two bots standing
next to each other.
=================
*/
typedef enum {
	AIM_PRIO_SIGHT,			// can be shot at at all
	AIM_PRIO_CURSOR,		// near where the player is already pointing
	AIM_PRIO_ATTACKER,		// whoever is hurting us
	AIM_PRIO_SURE,			// the shot this weapon can actually make at that range
	AIM_PRIO_NEAR,			// near in the world
	AIM_PRIO_WOUNDED,		// already hurt, as far as we can tell
	AIM_PRIO_KEEP,			// the one we were already on
	AIM_PRIO_POWERUP,		// carrying quad, regeneration, and the like
	AIM_PRIO_AIR,			// off the ground and on a path it cannot change
	AIM_PRIO_COUNT
} aimPriority_t;

static const char	*aimPriorityName[AIM_PRIO_COUNT] = {
	"sight", "cursor", "attacker", "sure", "near", "wounded", "keep", "powerup", "air"
};

// what each is worth when nothing has been said about it
static const float	aimPriorityDefault[AIM_PRIO_COUNT] = {
	100.0f, 80.0f, 100.0f, 60.0f, 40.0f, 40.0f, 30.0f, 20.0f, 0.0f
};

// How long a reason stays a reason. Three of them are about something that
// happened rather than something that is: who hit us, who we hurt, who we were
// already on. Those fade to nothing over these seconds, because a bot that hit
// us a minute ago is not the one hurting us now. The rest are properties of
// the moment - near the crosshair, in the air - and have no clock.
//
// Sight is the exception among the properties, and it earned its clock the
// hard way. The line is traced from an eye that moves with every drawn frame
// against a body that only moves when a snapshot lands, so along an edge the
// answer flips several times inside one server frame - the record caught a
// pick alternating between two bots ten times in a quarter of a second, and
// shots fired within a tenth of a second of such a switch were three and a
// half times less accurate than the rest.
//
// A tenth of a second settles it. That is two server frames, which is all the
// flicker can ever span, and it is deliberately not longer: every millisecond
// of it is a millisecond the choice may sit on a target that really has gone
// behind something, and there is nothing to be gained there.
static const float	aimPriorityLife[AIM_PRIO_COUNT] = {
	0.1f, 0.0f, 6.0f, 0.0f, 0.0f, 12.0f, 4.0f, 0.0f, 0.0f
};

static float	aimPriorityWeight[AIM_PRIO_COUNT];
static float	aimPriorityTime[AIM_PRIO_COUNT];
static int		aimPriorityCount = -1;		// modification count the weights were read at

// The same nine per weapon, because what makes a good target depends on what
// is being fired at it. The record is blunt about it: plasma lands seventy-two
// per cent of its shots inside four hundred units and eighteen per cent beyond
// eight hundred, while the machinegun holds ninety-something across the same
// span and the railgun does not care at all. A weapon that loses its shot to
// distance wants the near enemy; one that does not, wants the one the
// crosshair is already on.
//
// These are the whole effective list per weapon, filled from the defaults and
// then from whatever the player named, so the pick never has to work out where
// a number came from.
// What each weapon holds differently, in the order of aimPriorityName:
// sight, cursor, attacker, sure, near, wounded, keep, powerup, air. A minus
// one means "whatever the general list says", so the hook and the mission-pack
// weapons need no numbers invented for them.
//
// Where these come from, weapon by weapon:
//
// sure is nought on every hitscan weapon because it is an exact no-op there -
// with no flight time there is no measured scatter, so it hands every
// candidate the same half-weight and orders nothing. Leaving it at sixty only
// made the totals in the record unreadable.
//
// near follows each weapon's own distance curve. Measured hit rate by band:
// the machinegun holds 87 to 91 per cent inside eight hundred units and falls
// to 61 at twelve hundred and 52 past sixteen hundred; plasma runs 94, 70, 56,
// 38, 20; the rocket 86, 83, 60, 35, 23 and 12 past twelve hundred - the same
// shape, because plasma's speed advantage is spent exactly on its smaller
// splash. The railgun is flat across the whole map, 86 to 100 at every range,
// and keeps the lowest near of all. The lightning cannot reach past 768 units
// at all, and the grenade's arc comes down by about 660, so both take the
// maximum. The gauntlet reaches forty-six units, which even a hundred cannot
// really express.
//
// cursor separates a weapon the view is snapped onto from one it is steered
// towards. The single-shot weapons log an aiming error of exactly nought, so
// distance from the crosshair costs them nothing; the machinegun and plasma
// are steered and pay for it - inside eight hundred units the machinegun hit
// 97 per cent within a degree of swing and 50 beyond eight.
//
// sure is high on the splash weapons because it is the strongest single
// predictor in the record and the only criterion that knows a shot is
// hopeless: rockets whose measured scatter sat inside the splash radius landed
// 81 per cent, those past four hundred units of scatter 14. Nearly a third of
// all rockets went into a band that lands under six.
//
// The three that name a target for a reason unrelated to whether the shot can
// land - who hit me, who is hurt, who carries a powerup - are nought on the
// weapons with a second of flight, and kept on the ones that arrive at once.
//
// Without shots to go by, the lightning, grenade, BFG and gauntlet are argued
// from their reach and their splash rather than measured. They are marked in
// the bench as guesses and should be settled by play.
static const float	aimWeaponDefault[WP_NUM_WEAPONS][AIM_PRIO_COUNT] = {
	{  -1, -1, -1, -1,  -1, -1, -1, -1, -1 },		// none
	{ 100, 70, 60,  0, 100, 20, 25,  0,  0 },		// gauntlet
	{ 100, 80,100,  0,  55, 35, 35, 15,  0 },		// machinegun
	{ 100, 85, 90,  0,  75, 25, 20, 15,  0 },		// shotgun
	{ 100, 60,  0, 85, 100,  0, 30,  0,  0 },		// grenade
	{ 100, 70,  0, 95,  80,  0, 30,  0, 35 },		// rocket
	{ 100, 55, 85,  0, 100, 30, 45, 10,  0 },		// lightning
	{ 100, 95, 80,  0,  10, 45, 15, 35,  0 },		// railgun
	{ 100, 85, 90, 45,  80, 30, 35, 10,  0 },		// plasma
	{ 100, 75,  0, 90,  45,  0, 30,  0, 25 },		// bfg
	{  -1, -1, -1, -1,  -1, -1, -1, -1, -1 },		// hook: it does no damage
#ifdef MISSIONPACK
	{  -1, -1, -1, -1,  -1, -1, -1, -1, -1 },		// nailgun
	{  -1, -1, -1, -1,  -1, -1, -1, -1, -1 },		// prox
	{  -1, -1, -1, -1,  -1, -1, -1, -1, -1 },		// chaingun
#endif
};

// Only the two slow single-shot weapons hold a target for less time than the
// rest: their cycle is a second and a half, so four seconds of extra loyalty
// would span several independent decisions.
static const float	aimWeaponLifeDefault[WP_NUM_WEAPONS][AIM_PRIO_COUNT] = {
	{ -1, -1, -1, -1, -1, -1, -1, -1, -1 },			// none
	{ -1, -1, -1, -1, -1, -1,  2, -1, -1 },			// gauntlet
	{ -1, -1, -1, -1, -1, -1, -1, -1, -1 },			// machinegun
	{ -1, -1, -1, -1, -1, -1,  2, -1, -1 },			// shotgun
	{ -1, -1, -1, -1, -1, -1, -1, -1, -1 },			// grenade
	{ -1, -1, -1, -1, -1, -1, -1, -1, -1 },			// rocket
	{ -1, -1, -1, -1, -1, -1, -1, -1, -1 },			// lightning
	{ -1, -1, -1, -1, -1, -1,  2, -1, -1 },			// railgun
	{ -1, -1, -1, -1, -1, -1, -1, -1, -1 },			// plasma
	{ -1, -1, -1, -1, -1, -1, -1, -1, -1 },			// bfg
	{ -1, -1, -1, -1, -1, -1, -1, -1, -1 },			// hook
#ifdef MISSIONPACK
	{ -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ -1, -1, -1, -1, -1, -1, -1, -1, -1 },
#endif
};

static float	aimWeaponWeight[WP_NUM_WEAPONS][AIM_PRIO_COUNT];
static float	aimWeaponTime[WP_NUM_WEAPONS][AIM_PRIO_COUNT];
static int		aimWeaponCount = -1;

static float CL_AimAssistWeight( int weapon, int prio ) {
	if ( weapon <= WP_NONE || weapon >= WP_NUM_WEAPONS ) {
		return aimPriorityWeight[prio];
	}
	return aimWeaponWeight[weapon][prio];
}

static float CL_AimAssistLife( int weapon, int prio ) {
	if ( weapon <= WP_NONE || weapon >= WP_NUM_WEAPONS ) {
		return aimPriorityTime[prio];
	}
	return aimWeaponTime[weapon][prio];
}

/*
=================
CL_AimAssistParsePriorities

Reads "name:weight" or "name:weight:seconds" out of a string, or, with a weapon
in front of a dot, "rocket.near:80". Anything not named keeps what it had, and
anything without a time keeps the time it was given. A name nobody knows is
passed over in silence, which is what makes the record's own line worth reading:
what is not in it was not understood.

Whitespace includes newlines, so the same reader serves a cvar and a file, and
anything from a double slash to the end of a line is a remark.
=================
*/
static void CL_AimAssistParsePriorities( const char *text, qboolean perWeapon ) {
	const char	*name;
	char		token[64], *dot;
	float		value, life;
	int			i, j, weapon;

	if ( !text ) {
		return;
	}

	while ( *text ) {
		while ( *text == ' ' || *text == '\t' || *text == '\n' || *text == '\r' ) {
			text++;
		}
		if ( text[0] == '/' && text[1] == '/' ) {
			while ( *text && *text != '\n' ) {
				text++;
			}
			continue;
		}
		for ( i = 0; *text && *text != ':' && *text != ' ' && *text != '\t'
			&& *text != '\n' && *text != '\r' && i < (int)sizeof( token ) - 1; i++ ) {
			token[i] = *text++;
		}
		token[i] = '\0';
		if ( *text != ':' ) {
			while ( *text && *text != ' ' && *text != '\n' && *text != '\r' ) {
				text++;
			}
			continue;
		}
		text++;
		value = atof( text );

		// a second colon, if it is there, is how long the reason lasts
		life = -1.0f;
		while ( *text && *text != ' ' && *text != '\t' && *text != '\n' && *text != '\r' ) {
			if ( *text == ':' ) {
				life = atof( text + 1 );
			}
			text++;
		}

		weapon = -1;
		name = token;
		if ( perWeapon ) {
			dot = strchr( token, '.' );
			if ( !dot ) {
				continue;
			}
			*dot = '\0';
			name = dot + 1;
			for ( j = WP_NONE + 1; j < WP_NUM_WEAPONS; j++ ) {
				if ( !Q_stricmp( token, CL_AimAssistWeaponName( j ) ) ) {
					weapon = j;
					break;
				}
			}
			if ( weapon < 0 ) {
				continue;
			}
		}

		for ( j = 0; j < AIM_PRIO_COUNT; j++ ) {
			if ( Q_stricmp( name, aimPriorityName[j] ) ) {
				continue;
			}
			// Sight at nought takes the visibility gate out altogether, and
			// then a bot behind a wall can win the pick - which strands the
			// steering on a target it can do nothing with while a reachable
			// one stands in the open. A cvar was a thin enough channel that
			// nobody did it by accident; a file invites hand editing.
			if ( j == AIM_PRIO_SIGHT && value < 1.0f ) {
				Com_Printf( "aim prio: sight cannot be nought, it is the only thing keeping"
					" a bot behind a wall out of the running - taken as one\n" );
				value = 1.0f;
			}
			if ( !perWeapon ) {
				aimPriorityWeight[j] = Com_Clamp( 0.0f, 100.0f, value );
				if ( life >= 0.0f ) {
					aimPriorityTime[j] = Com_Clamp( 0.0f, 120.0f, life );
				}
			} else {
				aimWeaponWeight[weapon][j] = Com_Clamp( 0.0f, 100.0f, value );
				if ( life >= 0.0f ) {
					aimWeaponTime[weapon][j] = Com_Clamp( 0.0f, 120.0f, life );
				}
			}
			break;
		}
	}
}


/*
=================
CL_AimAssistPriorityFile

The per-weapon lists, from a file rather than from a cvar.

A cvar holds two hundred and fifty-six characters. One weapon tuned by hand had
already taken two hundred and forty-eight of them, and nine weapons worth of
deliberate differences need something over five hundred, which no amount of
abbreviating fits. So they live beside the learned table instead, in a file with
no such ceiling, and the cvar stays as a last word for anyone at the console.
=================
*/
#define AIM_PRIO_FILE	"aimprio.cfg"

static char		aimPrioText[8192];
static qboolean	aimPrioLoaded;

#define AIM_PRIO_FORMAT	1

// What the weapon lists used to be shipped as, before they became a table.
// Anyone who never touched them has exactly this sitting in their q3config,
// and must not be frozen on it; anyone who changed it meant it.
#define AIM_PRIO_LEGACY	"rocket.near:70 grenade.near:85 plasma.near:60 shotgun.near:80" \
						" lightning.near:95 railgun.near:10 machinegun.near:25"

static qboolean CL_AimAssistSameWords( const char *a, const char *b ) {
	while ( *a && *b ) {
		while ( *a == ' ' || *a == '\t' || *a == '\n' || *a == '\r' ) {
			a++;
		}
		while ( *b == ' ' || *b == '\t' || *b == '\n' || *b == '\r' ) {
			b++;
		}
		if ( *a != *b ) {
			return qfalse;
		}
		if ( *a ) {
			a++;
			b++;
		}
	}
	return *a == *b;
}

static void CL_AimAssistPriorityLoad( void ) {
	union { char *c; void *v; } file;
	int		length, format;

	aimPrioLoaded = qtrue;
	aimPrioText[0] = '\0';

	length = FS_ReadFile( AIM_PRIO_FILE, &file.v );
	if ( length > 0 && file.c ) {
		// Said out loud, unlike the learned table, which breaks off without a
		// word: a priority file that quietly does nothing looks exactly like a
		// bench that is broken.
		if ( sscanf( file.c, "format %i", &format ) == 1 && format != AIM_PRIO_FORMAT ) {
			Com_Printf( "aim prio: %s is format %i and this build reads %i, so it was passed over\n",
				AIM_PRIO_FILE, format, AIM_PRIO_FORMAT );
			FS_FreeFile( file.v );
			return;
		}
		if ( length >= (int)sizeof( aimPrioText ) ) {
			Com_Printf( "aim prio: %s is %i bytes and only the first %i were read\n",
				AIM_PRIO_FILE, length, (int)sizeof( aimPrioText ) - 1 );
		}
		Q_strncpyz( aimPrioText, file.c, sizeof( aimPrioText ) );
		FS_FreeFile( file.v );
		return;
	}

	// No file, but a cvar that somebody set by hand: keep it, in the place it
	// now belongs. The old shipped string is passed over, or everyone who
	// never touched it would be pinned to yesterday's numbers for good.
	if ( !cl_aimAssistPriorityWeapon->string[0]
		|| CL_AimAssistSameWords( cl_aimAssistPriorityWeapon->string, AIM_PRIO_LEGACY ) ) {
		return;
	}
	Com_sprintf( aimPrioText, sizeof( aimPrioText ),
		"format %i\n// Taken over from cl_aimAssistPriorityWeapon, which used to hold this.\n%s\n",
		AIM_PRIO_FORMAT, cl_aimAssistPriorityWeapon->string );
	FS_WriteFile( AIM_PRIO_FILE, aimPrioText, strlen( aimPrioText ) );
	Cvar_Set( "cl_aimAssistPriorityWeapon", "" );
	Com_Printf( "aim prio: the weapon lists moved from the cvar into %s\n", AIM_PRIO_FILE );
}

void CL_AimAssistPriorityReload( void ) {
	aimPrioLoaded = qfalse;
	aimPriorityCount = -1;
	aimWeaponCount = -1;
}

static void CL_AimAssistPriorities( void ) {
	int	i, weapon;

	if ( aimPrioLoaded && cl_aimAssistPriority->modificationCount == aimPriorityCount
		&& cl_aimAssistPriorityWeapon->modificationCount == aimWeaponCount ) {
		return;
	}
	if ( !aimPrioLoaded ) {
		CL_AimAssistPriorityLoad();
	}
	aimPriorityCount = cl_aimAssistPriority->modificationCount;
	aimWeaponCount = cl_aimAssistPriorityWeapon->modificationCount;

	for ( i = 0; i < AIM_PRIO_COUNT; i++ ) {
		aimPriorityWeight[i] = aimPriorityDefault[i];
		aimPriorityTime[i] = aimPriorityLife[i];
	}
	CL_AimAssistParsePriorities( cl_aimAssistPriority->string, qfalse );

	// The per-weapon list starts from what that weapon is known to want, and
	// falls back to the general one wherever the table says nothing. The pick
	// then reads one number and never has to ask where it came from.
	for ( weapon = 0; weapon < WP_NUM_WEAPONS; weapon++ ) {
		for ( i = 0; i < AIM_PRIO_COUNT; i++ ) {
			aimWeaponWeight[weapon][i] = aimWeaponDefault[weapon][i] >= 0.0f
				? aimWeaponDefault[weapon][i] : aimPriorityWeight[i];
			aimWeaponTime[weapon][i] = aimWeaponLifeDefault[weapon][i] >= 0.0f
				? aimWeaponLifeDefault[weapon][i] : aimPriorityTime[i];
		}
	}

	// the file first, then the cvar on top of it: whoever types at the console
	// should be able to overrule what the bench wrote, and not the other way
	CL_AimAssistParsePriorities( aimPrioText, qtrue );
	CL_AimAssistParsePriorities( cl_aimAssistPriorityWeapon->string, qtrue );
}


static int	aimAssistTarget = -1;		// who the assist steered at last frame

// The flight of the shot the steer just worked out, for the countdown drawn
// over the bot. Seconds, as everything inside this file is; only the record
// multiplies by a thousand when it prints. Minus one means there is nothing to
// say, and the frame stamp keeps a stale number from being shown after the
// steer has stopped running.
static float	aimFlightTime = -1.0f;
static int		aimFlightGrade;			// nought worst, three best
static int		aimFlightFrame = -1;
static int	aimAttacker = -1;			// the bot that last hurt us, if any
static int	aimAttackerTime;			// server time it did
static int	aimShotTarget = -1;			// who the last shot was aimed at
static int	aimShotTime;				// and when, so a stale one is not believed
static int	aimKeepSince;				// server time the current target was taken
static int	aimPickLast = -1;			// who the record last named as the pick

/*
=================
CL_AimAssistFade

How much is left of a reason that happened at a certain moment. Full at the
moment itself, nothing once its seconds have run out, straight down in
between. A reason with no clock never fades.
=================
*/
static float CL_AimAssistFade( int since, float life ) {
	int	age;

	if ( life <= 0.0f ) {
		return 1.0f;
	}
	if ( !since ) {
		return 0.0f;
	}

	age = cl.snap.serverTime - since;
	if ( age < 0 || age > life * 1000.0f ) {
		return 0.0f;
	}

	return 1.0f - age / ( life * 1000.0f );
}

// what the weights were understood as, so a typo in the string shows up
static void CL_AimAssistPriorityDump( void ) {
	int			i, weapon;
	qboolean	said;

	CL_AimAssistPriorities();
	Com_Printf( "aim prio:" );
	for ( i = 0; i < AIM_PRIO_COUNT; i++ ) {
		if ( aimPriorityLife[i] > 0.0f ) {
			// two decimals, because the shortest of these is a third of a
			// second and whole seconds printed it as nothing at all
			Com_Printf( " %s %.0f for %.2fs", aimPriorityName[i], aimPriorityWeight[i],
				aimPriorityTime[i] );
		} else {
			Com_Printf( " %s %.0f", aimPriorityName[i], aimPriorityWeight[i] );
		}
	}
	Com_Printf( "\n" );

	// And where a weapon disagrees with that, one line each. Only the
	// differences: a weapon that follows the general list says nothing, so
	// what is on these lines is exactly what was meant to be different.
	for ( weapon = WP_NONE + 1; weapon < WP_NUM_WEAPONS; weapon++ ) {
		said = qfalse;
		for ( i = 0; i < AIM_PRIO_COUNT; i++ ) {
			if ( aimWeaponWeight[weapon][i] == aimPriorityWeight[i]
				&& aimWeaponTime[weapon][i] == aimPriorityTime[i] ) {
				continue;
			}
			if ( !said ) {
				Com_Printf( "aim prio: %s", CL_AimAssistWeaponName( weapon ) );
				said = qtrue;
			}
			// The duration is shown whenever this weapon holds a different one,
			// not only where the criterion has a clock by default: without
			// that, a line that changed only the seconds printed exactly what
			// the general list already said and looked like a typo.
			if ( aimPriorityLife[i] > 0.0f
				|| aimWeaponTime[weapon][i] != aimPriorityTime[i] ) {
				Com_Printf( " %s %.0f for %.2fs", aimPriorityName[i],
					aimWeaponWeight[weapon][i], aimWeaponTime[weapon][i] );
			} else {
				Com_Printf( " %s %.0f", aimPriorityName[i], aimWeaponWeight[weapon][i] );
			}
		}
		if ( said ) {
			Com_Printf( "\n" );
		}
	}
}


/*
=================
CL_AimAssistWounded

How badly a bot is hurt, as far as the client is allowed to know. The game
never sends another player's health, so the only thing to go on is the damage
we did ourselves: the server reports what the one we just hit has left, and
which one that was is the one we were steering at. That knowledge goes stale -
they pick health up - so it is forgotten after a while.
=================
*/
#define AIM_WOUND_MEMORY	12000		// how long a remembered health is worth anything

static int	aimWoundHealth[MAX_CLIENTS];
static int	aimWoundArmor[MAX_CLIENTS];
static int	aimWoundTime[MAX_CLIENTS];
static int	aimWoundHits = -1;			// PERS_HITS as last seen

// when the line to each bot was last open; a new map runs the clock back, and
// a stamp from the old one reads as ancient rather than as fresh
static int	aimSeenTime[MAX_CLIENTS];

static void CL_AimAssistWoundWatch( void ) {
	int	hits, remaining, health, target;

	hits = cl.snap.ps.persistant[PERS_HITS];
	if ( aimWoundHits < 0 || hits < aimWoundHits ) {
		aimWoundHits = hits;		// first snapshot, or a new life
		return;
	}
	if ( hits == aimWoundHits ) {
		return;
	}
	// The count moves on whether or not this one can be put to a name. Leaving
	// it behind would only hand this report to the next shot, which may well
	// have gone at somebody else.
	aimWoundHits = hits;

	// The hit-rate table wants this before anything below can refuse it: what
	// follows is about a bot's health and needs a shot from the last four
	// hundred milliseconds, while a rocket is in the air for twice that. The
	// counter moving is the hit; who it landed on, the table works out from
	// which shot is still open.
	CL_AimAssistRateCredit();

	// Whoever the shot was fired at - not whoever the aim happens to be on now.
	// The report arrives with the snapshot, a good fifty milliseconds after the
	// command that fired, and the pick is remade every frame in between: a
	// flick in that window would file one bot's health under another's name and
	// then draw an untouched bot as nearly dead. Only a recent shot counts.
	target = -1;
	if ( aimShotTime && cl.snap.serverTime - aimShotTime <= 400 ) {
		target = aimShotTarget;
	}
	remaining = cl.snap.ps.persistant[PERS_ATTACKEE_REMAINING];
	if ( target < 0 || target >= MAX_CLIENTS || !remaining ) {
		return;
	}

	health = ( ( remaining >> 8 ) & 0xff ) - 1;
	if ( health < 0 ) {
		health = 0;
	}
	aimWoundHealth[target] = health;
	aimWoundArmor[target] = remaining & 0xff;
	aimWoundTime[target] = cl.snap.serverTime;
}

/*
=================
CL_AimAssistKnownDamage

What a bot had left the last time we hit it, for anyone who wants to show it.
The game never sends another player's health, so this is the only thing there
is to go on - and it goes stale, because they pick health up. How old the news
is comes back with it, so a display can fade rather than lie.
=================
*/
qboolean CL_AimAssistKnownDamage( int clientNum, int *health, int *armor, float *freshness ) {
	float	left;

	if ( clientNum < 0 || clientNum >= MAX_CLIENTS || !aimWoundTime[clientNum] ) {
		return qfalse;
	}

	// the same clock the wounded priority runs on, so what is shown and what
	// is aimed at do not disagree about how old the news is
	CL_AimAssistPriorities();
	left = CL_AimAssistFade( aimWoundTime[clientNum], aimPriorityTime[AIM_PRIO_WOUNDED] );
	if ( left <= 0.0f ) {
		return qfalse;
	}

	*health = aimWoundHealth[clientNum];
	*armor = aimWoundArmor[clientNum];
	*freshness = left;
	return qtrue;
}


/*
=================
CL_AimAssistShotFlight

How long the shot in hand would take to reach the bot the assist is steering
at, and what the record thinks of that shot.

Nothing to say unless the assist is really steering, the weapon really throws
something, and the answer was worked out this very frame. The frame stamp is
safe because the command is built before the screen is drawn and the counter
only moves at the end of both.
=================
*/
qboolean CL_AimAssistShotFlight( int clientNum, float *seconds, int *grade ) {
	if ( clientNum < 0 || clientNum != aimAssistTarget
		|| aimFlightTime < 0.0f || aimFlightFrame != cls.framecount ) {
		return qfalse;
	}

	*seconds = aimFlightTime;
	*grade = aimFlightGrade;
	return qtrue;
}

static float CL_AimAssistWoundScore( int clientNum, int weapon ) {
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS || !aimWoundTime[clientNum] ) {
		return 0.0f;
	}

	// the less it had left the better, and the older the news the less it says
	return Com_Clamp( 0.0f, 1.0f, ( 100.0f - aimWoundHealth[clientNum] ) / 100.0f )
		* CL_AimAssistFade( aimWoundTime[clientNum], CL_AimAssistLife( weapon, AIM_PRIO_WOUNDED ) );
}


/*
=================
CL_AimAssistPickTarget

The bot to steer at: the one the priorities above like best. Only bots are
eligible, and only living enemies. With sticky off the pick is made on the
crosshair alone, which is what the record of unassisted shots is held against.
=================
*/
static entityState_t *CL_AimAssistPickTarget( const vec3_t viewOrigin, int localTeam, int weapon,
		qboolean sticky ) {
	entityState_t	*entity, *best = NULL;
	const char		*info;
	trace_t			trace;
	vec3_t			targetOrigin, direction, desired, motion, hullMins, hullMaxs;
	float			bestScore = -1.0f, score, angle, pitchDelta, yawDelta, distance;
	float			speed, scatter, fresh, flight, weight[AIM_PRIO_COUNT];
	float			part[AIM_PRIO_COUNT], bestPart[AIM_PRIO_COUNT];
	char			others[768];
	int				i, k, targetTeam;
	qboolean		visible, airborne;

	others[0] = '\0';
	Com_Memset( bestPart, 0, sizeof( bestPart ) );

	CL_AimAssistPriorities();
	for ( i = 0; i < AIM_PRIO_COUNT; i++ ) {
		weight[i] = sticky ? CL_AimAssistWeight( weapon, i ) : 0.0f;
	}
	if ( !sticky ) {
		// The plain crosshair pick, for the record of unassisted shots. It has
		// to answer the same question a player would: the bot being shot at.
		// One behind a wall, or behind the shooter, is not that - counting
		// those as missed shots would measure the rate on noise.
		weight[AIM_PRIO_CURSOR] = 1.0f;
		weight[AIM_PRIO_SIGHT] = 100.0f;
	}

	for ( i = 0; i < cl.snap.numEntities; i++ ) {
		entity = &cl.parseEntities[( cl.snap.parseEntitiesNum + i ) & ( MAX_PARSE_ENTITIES - 1 )];
		if ( entity->eType != ET_PLAYER || entity->clientNum == cl.snap.ps.clientNum ||
			 entity->clientNum < 0 || entity->clientNum >= MAX_CLIENTS ||
			 ( entity->eFlags & EF_DEAD ) || entity->number != entity->clientNum ) {
			continue;
		}

		info = cl.gameState.stringData + cl.gameState.stringOffsets[CS_PLAYERS + entity->clientNum];
		if ( !*Info_ValueForKey( info, "skill" ) && !cl_aimAssistHumanTargets->integer ) {
			continue;	// humans are an explicit local-lab opt-in
		}

		targetTeam = atoi( Info_ValueForKey( info, "t" ) );
		if ( localTeam != TEAM_FREE && targetTeam == localTeam ) {
			continue;
		}

		VectorCopy( entity->pos.trBase, targetOrigin );
		targetOrigin[2] += CL_AimAssistBodyHeight( entity );
		CM_BoxTrace( &trace, viewOrigin, targetOrigin, vec3_origin, vec3_origin,
			0, MASK_SOLID, qfalse );
		visible = trace.fraction >= 1.0f;
		if ( visible ) {
			aimSeenTime[entity->clientNum] = cl.snap.serverTime;
		}

		// How long ago it was last reachable, as a number between one and
		// nothing. Only just out of sight still counts for the seconds sight
		// was given, which is what keeps the choice from flickering along an
		// edge; the record's own pick asks the strict question, because a bot
		// behind a wall is not one a player was shooting at.
		//
		// And the memory is granted only to the target already being steered
		// at. That is the whole of what it is for - the line to one bot
		// flickering between client frames - and it is the difference between
		// riding out that flicker and picking up a bot that is behind a wall,
		// which the steering could then find no way through to while a
		// reachable one stood in the open.
		if ( visible || !sticky || entity->clientNum != aimAssistTarget
			|| CL_AimAssistLife( weapon, AIM_PRIO_SIGHT ) <= 0.0f ) {
			fresh = visible ? 1.0f : 0.0f;
		} else {
			fresh = CL_AimAssistFade( aimSeenTime[entity->clientNum],
				CL_AimAssistLife( weapon, AIM_PRIO_SIGHT ) );
		}

		// Out of sight is out of the running unless sight has been turned off
		// altogether. Anything in between would be a trap: a blocked bot that
		// won the pick cannot be steered at either - the steering finds no way
		// through and gives up - so it would strand the assist on a target it
		// can do nothing with while a reachable one stands in the open.
		if ( fresh <= 0.0f && weight[AIM_PRIO_SIGHT] > 0.0f ) {
			continue;
		}

		VectorSubtract( targetOrigin, viewOrigin, direction );
		distance = VectorLength( direction );

		vectoangles( direction, desired );
		desired[PITCH] -= SHORT2ANGLE( cl.snap.ps.delta_angles[PITCH] );
		desired[YAW] -= SHORT2ANGLE( cl.snap.ps.delta_angles[YAW] );
		pitchDelta = AngleNormalize180( desired[PITCH] - cl.viewangles[PITCH] );
		yawDelta = AngleNormalize180( desired[YAW] - cl.viewangles[YAW] );
		angle = sqrt( pitchDelta * pitchDelta + yawDelta * yawDelta );

		// nobody is shooting at something a quarter turn away from the
		// crosshair, and the record should not pretend they were
		if ( !sticky && angle > 30.0f ) {
			continue;
		}

		// Not a constant any more: with a memory behind it this separates a
		// bot standing in the open from one that slipped behind a corner a
		// moment ago, and the largest weight in the table finally decides
		// something instead of adding the same hundred to everybody.
		part[AIM_PRIO_SIGHT] = weight[AIM_PRIO_SIGHT] * fresh;
		part[AIM_PRIO_CURSOR] = weight[AIM_PRIO_CURSOR] / ( 1.0f + angle / 15.0f );
		part[AIM_PRIO_NEAR] = weight[AIM_PRIO_NEAR] / ( 1.0f + distance / 500.0f );
		part[AIM_PRIO_WOUNDED] = weight[AIM_PRIO_WOUNDED] * CL_AimAssistWoundScore( entity->clientNum, weapon );
		// Both of these are about something that happened, so both run out.
		part[AIM_PRIO_ATTACKER] = entity->clientNum == aimAttacker
			? weight[AIM_PRIO_ATTACKER] * CL_AimAssistFade( aimAttackerTime, CL_AimAssistLife( weapon, AIM_PRIO_ATTACKER ) )
			: 0.0f;
		// Half of it is unconditional and half runs out. Purely running out
		// was backwards where it mattered: a target just switched to had the
		// whole thirty points of protection and one tracked steadily for five
		// seconds had none, so the rule meant to stop the choice wandering
		// went quiet exactly when the choice had been settled longest. The
		// seconds now buy extra loyalty right after a switch, on top of a
		// floor that never expires.
		part[AIM_PRIO_KEEP] = entity->clientNum == aimAssistTarget
			? weight[AIM_PRIO_KEEP] * ( 0.5f + 0.5f
				* CL_AimAssistFade( aimKeepSince, CL_AimAssistLife( weapon, AIM_PRIO_KEEP ) ) )
			: 0.0f;
		part[AIM_PRIO_POWERUP] = ( entity->powerups & ( ( 1 << PW_QUAD ) | ( 1 << PW_REGEN )
			| ( 1 << PW_BATTLESUIT ) | ( 1 << PW_HASTE ) | ( 1 << PW_INVIS ) ) )
			? weight[AIM_PRIO_POWERUP] : 0.0f;

		// How well this weapon does at that range is measured, not guessed;
		// a shot that scatters wider than it reaches counts for little.
		speed = CL_AimAssistProjectileSpeed( weapon );
		CL_AimAssistVelocity( entity, motion );
		flight = speed > 0.0f ? distance / speed : 0.0f;
		scatter = speed > 0.0f ? CL_AimAssistScatter( weapon, flight,
			sqrt( motion[0] * motion[0] + motion[1] * motion[1] ) ) : -1.0f;

		// A target off the ground counts only while it is still off the ground
		// when the shot gets there. One that comes down on the way is the
		// worst target on the map and not the best: the record split those two
		// sixty-three per cent against sixteen, with a target on its feet at
		// fifty-two in between and the same average distance on either side,
		// so that is the footing talking and not the range. What was asked
		// before - whether sixty units under the target are clear - is about a
		// third of a second of falling, and these shots fly for over one.
		airborne = qfalse;
		if ( flight > 0.0f && entity->groundEntityNum == ENTITYNUM_NONE
			&& !CL_AimAssistFloats( entity ) ) {
			CL_AimAssistHull( entity, hullMins, hullMaxs );
			airborne = CL_AimAssistLanding( entity, motion,
				cl.snap.ps.gravity > 0 ? cl.snap.ps.gravity : DEFAULT_GRAVITY,
				flight, hullMins, hullMaxs, NULL ) < 0.0f;
		}
		part[AIM_PRIO_AIR] = airborne ? weight[AIM_PRIO_AIR] : 0.0f;
		part[AIM_PRIO_SURE] = scatter >= 0.0f
			? weight[AIM_PRIO_SURE] / ( 1.0f + scatter / CL_AimAssistHitRadius( weapon ) )
			: weight[AIM_PRIO_SURE] * 0.5f;		// nothing measured yet

		score = 0.0f;
		for ( k = 0; k < AIM_PRIO_COUNT; k++ ) {
			score += part[k];
		}

		// for the record: every candidate with what it came to
		if ( sticky && cl_aimAssistDebug->integer ) {
			info = cl.gameState.stringData + cl.gameState.stringOffsets[CS_PLAYERS + entity->clientNum];
			Q_strcat( others, sizeof( others ), va( " | %s %.0f",
				Info_ValueForKey( info, "n" ), score ) );
		}

		if ( score > bestScore ) {
			bestScore = score;
			best = entity;
			for ( k = 0; k < AIM_PRIO_COUNT; k++ ) {
				bestPart[k] = part[k];
			}
		}
	}

	// Why this one and not the others: printed when the choice changes, which
	// is when it is worth knowing. Every candidate's total is on the line, and
	// the winner's is broken down into what each priority contributed.
	if ( sticky && cl_aimAssistDebug->integer && best && best->clientNum != aimPickLast ) {
		info = cl.gameState.stringData + cl.gameState.stringOffsets[CS_PLAYERS + best->clientNum];
		Com_Printf( "aim pick: %s took %s %.0f (", CL_AimAssistWeaponName( weapon ),
			Info_ValueForKey( info, "n" ), bestScore );
		for ( k = 0; k < AIM_PRIO_COUNT; k++ ) {
			Com_Printf( " %s %.0f", aimPriorityName[k], bestPart[k] );
		}
		Com_Printf( " ) alle%s frame %i\n", others, cl.snap.serverTime );
	}
	// Only a real change moves this on. Clearing it whenever a frame found
	// nobody made every re-acquisition of the same bot read as a switch: two
	// out of every five lines in the record were one bot being found again,
	// and anyone counting the lines overstated the switching by half.
	if ( sticky && best ) {
		aimPickLast = best->clientNum;
	}

	return best;
}


/*
=================
CL_AimAssistSkip

Why the assist did nothing this frame. A miss the player blames on the aim is
often a frame in which there was no aim at all - nothing in sight, everything
behind cover, or the target so close that the shot would come back. Written
when the reason changes, not every frame, because the reason lasts.
=================
*/
static int	aimSkipLast = -1;

static void CL_AimAssistSkip( int reason, int weapon ) {
	static const char	*names[] = { "no target", "behind cover", "own splash" };

	if ( reason == aimSkipLast ) {
		return;
	}
	aimSkipLast = reason;

	if ( reason >= 0 && cl_aimAssistDebug->integer ) {
		Com_Printf( "aim skip: %s %s frame %i\n",
			CL_AimAssistWeaponName( weapon ), names[reason], cl.snap.serverTime );
	}
}


/*
=================
CL_AimAssistLogHold

Both ends of a held trigger, written down. Without this the option was
indistinguishable from a dead one: holding the trigger only delays a shot -
the game zeroes the weapon timer on a command without it, so the next command
that carries it fires at once - and a hold that lasted the two or three frames
of an aim swing cost about forty milliseconds inside a hundred-millisecond
weapon cycle. Nothing was felt and nothing was written, so there was no way to
tell it apart from an option that did nothing at all.
=================
*/
static int	aimHoldLast = -1;
static int	aimHoldFrom;
static int	aimHoldRun;					// commands in a row that found no way through

static void CL_AimAssistLogHold( int reason, int weapon, float distance ) {
	static const char	*names[] = { "behind cover", "own splash", "a lottery" };
	int					held;

	if ( reason == aimHoldLast ) {
		return;
	}

	// Measured on the command clock, not the snapshot's. A hold lasts the few
	// commands of an aim swing, and the snapshot only ticks five times in the
	// time a weapon cycles once: every hold worth measuring would have come
	// out as nought or as fifty milliseconds and nothing in between.
	held = cl.serverTime - aimHoldFrom;
	if ( held < 0 ) {
		held = 0;			// a new map ran the clock back
	}

	if ( cl_aimAssistDebug->integer ) {
		// One reason giving way to another closes the first, or the record
		// would show more holds beginning than ending.
		if ( aimHoldLast >= 0 ) {
			Com_Printf( "aim hold: %s released after %i ms frame %i\n",
				CL_AimAssistWeaponName( weapon ), held, cl.snap.serverTime );
		}
		if ( reason >= 0 ) {
			Com_Printf( "aim hold: %s %s at %.0f frame %i\n",
				CL_AimAssistWeaponName( weapon ), names[reason], distance,
				cl.snap.serverTime );
		}
	}
	if ( reason >= 0 ) {
		aimHoldFrom = cl.serverTime;
	}
	aimHoldLast = reason;
}


/*
=================
CL_AimAssistHoldForget

Everything the trigger was in the middle of. Dying, spectating, an
intermission or a new map all end a hold without releasing it, and what is
left behind would otherwise skip the debounce on the next press and print a
duration spanning the whole interruption.
=================
*/
static void CL_AimAssistHoldForget( void ) {
	aimHoldLast = -1;
	aimHoldRun = 0;
	aimHoldFrom = cl.serverTime;
}


/*
=================
CL_AimAssistFireDelay

The weapon's time between shots (PM_Weapon in bg_pmove.c), in milliseconds,
shortened by haste the way the game does it.
=================
*/
static int CL_AimAssistFireDelay( int weapon ) {
	int	delay;

	switch ( weapon ) {
	case WP_LIGHTNING:			delay = 50; break;
	case WP_SHOTGUN:			delay = 1000; break;
	case WP_MACHINEGUN:			delay = 100; break;
	case WP_GRENADE_LAUNCHER:	delay = 800; break;
	case WP_ROCKET_LAUNCHER:	delay = 800; break;
	case WP_PLASMAGUN:			delay = 100; break;
	case WP_RAILGUN:			delay = 1500; break;
	case WP_BFG:				delay = 200; break;
#ifdef MISSIONPACK
	case WP_NAILGUN:			delay = 1000; break;
	case WP_PROX_LAUNCHER:		delay = 800; break;
	case WP_CHAINGUN:			delay = 30; break;
#endif
	default:					delay = 400; break;
	}

	if ( cl.snap.ps.powerups[PW_HASTE] ) {
		delay /= 1.3;
	}

	return delay;
}


/*
=================
CL_AimAssistSingleShot

The weapons where one pull is one shot, and the shot is worth placing exactly.
=================
*/
static qboolean CL_AimAssistSingleShot( int weapon ) {
	return weapon == WP_SHOTGUN || weapon == WP_GRENADE_LAUNCHER
		|| weapon == WP_ROCKET_LAUNCHER || weapon == WP_RAILGUN || weapon == WP_BFG;
}


/*
=================
CL_AimAssistFiring

Whether the command being built is the one the server will fire on. The
weapon fires when its timer has run out while the trigger is held, and the
timer is in the snapshot: what is left of it now is what it was, less the
time of every command since. A shot predicted here restarts the timer locally
until a snapshot that has seen that command takes over.

Mirrors PM_Weapon: no shot while respawning, dead, changing weapon, or out of
ammo. Call it once per command, it keeps the timer.
=================
*/
/*
=================
CL_AimAssistInReach

Whether the gauntlet's swing touches the target: the game traces thirty-two
units out from the muzzle along the view, and the muzzle is fourteen units
out from the eye. It does that before it moves the player, with the view the
previous command was sent with. The target is its box.
=================
*/
static float	aimPrevAngles[2];		// the view the previous command was sent with

static qboolean CL_AimAssistInReach( const vec3_t eye, const entityState_t *target ) {
	vec3_t	angles, forward, start, end, mins, maxs;
	float	enter, leave, near, far, low, high, delta, swap;
	int		i;

	angles[PITCH] = aimPrevAngles[0] + SHORT2ANGLE( cl.snap.ps.delta_angles[PITCH] );
	angles[YAW] = aimPrevAngles[1] + SHORT2ANGLE( cl.snap.ps.delta_angles[YAW] );
	angles[ROLL] = 0.0f;
	AngleVectors( angles, forward, NULL, NULL );
	VectorMA( eye, 14.0f, forward, start );
	VectorMA( start, 32.0f, forward, end );

	CL_AimAssistHull( target, mins, maxs );
	enter = 0.0f;
	leave = 1.0f;

	for ( i = 0; i < 3; i++ ) {
		low = target->pos.trBase[i] + mins[i];
		high = target->pos.trBase[i] + maxs[i];
		delta = end[i] - start[i];
		if ( fabs( delta ) < 0.001f ) {
			if ( start[i] < low || start[i] > high ) {
				return qfalse;
			}
			continue;
		}
		near = ( low - start[i] ) / delta;
		far = ( high - start[i] ) / delta;
		if ( near > far ) {
			swap = near;
			near = far;
			far = swap;
		}
		if ( near > enter ) {
			enter = near;
		}
		if ( far < leave ) {
			leave = far;
		}
		if ( enter > leave ) {
			return qfalse;
		}
	}

	return qtrue;
}


/*
=================
CL_AimAssistFiring

Whether the command being built is the one the server will fire on. The
weapon fires when its timer has run out while the trigger is held, and the
timer is in the snapshot: what is left of it now is what it was, less the
time of every command since - counting down while it is above zero, then
stopped where it crossed, or zeroed by a command without the trigger, as
the game keeps it. A shot predicted here restarts the timer locally until a
snapshot that has seen that command takes over.

Mirrors PM_Weapon: no shot while respawning, dead, changing weapon, or out of
ammo. Call it once per command, it keeps the timer.
=================
*/
static int	aimFireTime = -1;		// serverTime of the command we last predicted a shot on
static int	aimFireDelay;			// what the weapon's timer was restarted with then
static int	aimRaiseDone = -1;		// serverTime of the command that finished raising the weapon
static int	aimCrossTime = -1;		// serverTime of the command on which the timer ran out
static int	aimCrossRemainder;		// what it stopped at then
static int	aimTimerBefore = 1;		// the timer as the previous command left it

static qboolean CL_AimAssistFiring( const usercmd_t *cmd, int weapon, const vec3_t eye,
		const entityState_t *target ) {
	const playerState_t	*ps = &cl.snap.ps;
	int					weaponTime, local, before;
	qboolean			attack, ready;

	attack = ( cmd->buttons & BUTTON_ATTACK ) != 0;

	// A new map starts the server's clock over. A timer left from the old one
	// would sit in the future and hold the trigger for as long as that map ran.
	if ( aimFireTime > cl.serverTime || aimCrossTime > cl.serverTime ) {
		aimFireTime = -1;
		aimRaiseDone = -1;
		aimCrossTime = -1;
		aimTimerBefore = 1;
	}

	// the snapshot has caught up with the crossing, or the timer was restarted
	if ( aimCrossTime >= 0 && ( ps->commandTime >= aimCrossTime || aimFireTime >= aimCrossTime ) ) {
		aimCrossTime = -1;
	}

	if ( ps->weaponTime > 0 ) {
		weaponTime = ps->weaponTime - ( cl.serverTime - ps->commandTime );
	} else {
		weaponTime = ps->weaponTime;
	}

	// a shot of ours the snapshot has not caught up with yet
	if ( aimFireTime > ps->commandTime ) {
		local = aimFireDelay - ( cl.serverTime - aimFireTime );
		if ( local > weaponTime ) {
			weaponTime = local;
		}
	}

	// The timer stops where it crossed zero, it does not run on below it:
	// the remainder of the crossing command is what the next shot restarts from.
	if ( aimCrossTime >= 0 ) {
		weaponTime = aimCrossRemainder;
	} else if ( weaponTime <= 0 && aimTimerBefore > 0 ) {
		aimCrossTime = cl.serverTime;
		aimCrossRemainder = weaponTime;
	}
	before = aimTimerBefore;

	// The raise is watched whether or not the trigger is held: the command
	// that ends it only readies the weapon, and the shot goes out on the one
	// after - which may well be the first one with the trigger down.
	if ( ps->weaponstate == WEAPON_RAISING ) {
		if ( weaponTime > 0 ) {
			aimRaiseDone = -1;
			ready = qfalse;
		} else if ( aimRaiseDone < 0 ) {
			aimRaiseDone = cl.serverTime;
			ready = qfalse;
		} else {
			ready = qtrue;
		}
	} else {
		aimRaiseDone = -1;
		ready = ps->weaponstate == WEAPON_READY || ps->weaponstate == WEAPON_FIRING;
	}

	// a command without the trigger that finds the timer out zeroes it
	if ( ready && weaponTime <= 0 && !attack ) {
		weaponTime = 0;
		aimCrossRemainder = 0;
		if ( aimCrossTime < 0 ) {
			aimCrossTime = cl.serverTime;
		}
	}
	aimTimerBefore = weaponTime;

	if ( !attack || !ready || weaponTime > 0 || ( ps->pm_flags & PMF_RESPAWNED )
		|| ps->stats[STAT_HEALTH] <= 0 || weapon != ps->weapon || ps->ammo[weapon] == 0 ) {
		return qfalse;
	}

	// The gauntlet swings on the command after its timer ran out: the game
	// checks its reach before it moves the player, with the timer and the
	// view as the previous command left them, and only a swing that touches
	// something counts as a shot.
	if ( weapon == WP_GAUNTLET && ( before > 0 || !target || !CL_AimAssistInReach( eye, target ) ) ) {
		return qfalse;
	}

	// The game adds the delay onto what is left of the timer, which is at or
	// below zero here: a shot that comes a little late leaves the next one a
	// little early.
	aimFireTime = cl.serverTime;
	aimFireDelay = CL_AimAssistFireDelay( weapon ) + weaponTime;
	aimTimerBefore = aimFireDelay;
	aimCrossTime = -1;
	return qtrue;
}


/*
=================
CL_AimAssistLogShot

One line per shot fired, so the test bench can tell which shots the
prediction got right. What is left of the two deltas after the blend is how
far the view still misses the predicted point. Unassisted shots are written
too, against the bot nearest the crosshair, which gives the bench a rate to
hold the assisted one against.

"touchdown" comes in rather than being worked out here: the landing search
costs a box trace per step, and the caller already has to ask it for the
hit-rate table. Negative means the target is not expected back on its feet
before the shot lands - the axis the record splits hardest on.
=================
*/
static void CL_AimAssistLogShot( const entityState_t *entity, int weapon, const vec3_t viewOrigin,
		const vec3_t targetOrigin, float lead, float error, float swing, qboolean assisted,
		qboolean exact, qboolean fallback, float touchdown ) {
	const char	*info;
	vec3_t		direction, motion;
	float		pace;

	info = cl.gameState.stringData + cl.gameState.stringOffsets[CS_PLAYERS + entity->clientNum];
	VectorSubtract( targetOrigin, viewOrigin, direction );
	CL_AimAssistVelocity( entity, motion );

	pace = sqrt( motion[0] * motion[0] + motion[1] * motion[1] );

	// "at" is where the aim was put, "plain" where the target really stood
	// and "vel" what it was doing - the applied lead is the difference.
	// "myspeed" and "eye" let the impact lines be matched to the shooter.
	//
	// Three names on this line were traps and are now what they say. "speed"
	// was the shooter's own, although the table's second axis is the target's,
	// so it is "pace" for the target and "myspeed" for us, as on the learn
	// line. "frame" was the command's own clock here and the snapshot's clock
	// on every other line, so joining two line types on it slipped by up to a
	// third of a server frame; it is "cmd" here and "world" stays the snapshot.
	// And "phase" was printed whenever the point was not the exact one, even
	// on the shots that fell back to the plain body and were given no phase at
	// all, which made the aim point impossible to reconstruct from the record.
	// "error" is what the shot really went out with; "swing" is how far the
	// view had to come to get there. On a snapped shot the error is nought by
	// construction - the view was put exactly on the point - so without the
	// swing beside it the field would say nothing at all about the railgun,
	// the rocket and the shotgun, which are the weapons that snap.
	//
	// There is no "phase" here any more. This line is only ever written on a
	// firing command, and a firing command is now given the point without the
	// smoothing correction, whatever the weapon - so the field could only
	// print a number that was not applied to anything.
	Com_Printf( "aim shot: %s target %s dist %.0f air %i lead %i trust %.2f error %.2f swing %.2f assist %i"
		" at %.0f %.0f %.0f plain %.0f %.0f %.0f vel %.0f %.0f %.0f eye %.0f %.0f %.0f"
		" pace %.0f myspeed %.0f me %i"
		" exact %i hold %.2f crouch %i tune %.2f scatter %.0f fall %i myair %i land %i"
		" world %i cmd %i\n",
		CL_AimAssistWeaponName( weapon ), Info_ValueForKey( info, "n" ),
		VectorLength( direction ),
		entity->groundEntityNum == ENTITYNUM_NONE ? 1 : 0,
		(int)( lead * 1000.0f ),
		CL_AimAssistTrust( entity, lead ),
		error, swing, assisted ? 1 : 0,
		targetOrigin[0], targetOrigin[1], targetOrigin[2],
		entity->pos.trBase[0], entity->pos.trBase[1], entity->pos.trBase[2],
		motion[0], motion[1], motion[2],
		viewOrigin[0], viewOrigin[1], viewOrigin[2],
		pace, VectorLength( cl.snap.ps.velocity ), cl.snap.ps.clientNum,
		exact ? 1 : 0,
		CL_AimAssistHold(), CL_AimAssistCrouched( entity ) ? 1 : 0,
		CL_AimAssistTune( weapon, lead, pace ),
		CL_AimAssistScatter( weapon, lead, pace ),
		fallback ? 1 : 0, cl.snap.ps.groundEntityNum == ENTITYNUM_NONE ? 1 : 0,
		touchdown >= 0.0f ? (int)( touchdown * 1000.0f ) : -1,
		cl.snap.serverTime, cl.serverTime );
}


/*
=================
CL_AimAssistRemember / CL_AimAssistLearn

Learns how long bots hold a direction, from what they actually do. Every
assisted projectile shot is written down with where the target stood, which
way it ran and how far the prediction expected it to get by the time the shot
arrives; when the snapshot of that arrival frame comes in, the target's real
distance along that line is compared with the expected one. Ran further: the
hold time grows; fell short or came back: it shrinks. Each step is bounded,
and a shot whose target mostly went sideways - a jump pad, a dodge - says
little about holding a line and counts for less. The result lives in
cl_aimAssistLead, in seconds, which is what the prediction reads.
=================
*/
#define AIM_PENDING		16

typedef struct {
	int		target;			// client number of the bot aimed at
	int		arrival;		// server time whose snapshot decides the shot, 0 when done
	int		weapon;
	vec3_t	origin;			// where the target stood at the shot
	vec3_t	along;			// unit vector of its sideways motion then
	float	straight;		// sideways distance a straight line gives it by arrival
	float	base;			// what the model alone expected, before the box's factor
	float	expected;		// what the prediction really aimed for, tuning included
	float	rate;			// its sideways speed after the trust of the moment
	float	speed;			// and before it, which is the axis the table is kept on
	float	lead;			// flight time, a whole number of frames
} aimPending_t;

static aimPending_t	aimPending[AIM_PENDING];
static int			aimPendingNum;
static int			aimLearned;		// shots learned from so far

// Warum ein Schuss dem Lerner nichts beibringt. Fuellt sich die Tabelle nicht,
// steht hier, woran es liegt.
static void CL_AimAssistDrop( int weapon, const char *why ) {
	if ( cl_aimAssistDebug->integer ) {
		Com_Printf( "aim drop: %s %s frame %i\n",
			CL_AimAssistWeaponName( weapon ), why, cl.snap.serverTime );
	}
}

static void CL_AimAssistRemember( const entityState_t *entity, int weapon, float lead,
		const vec3_t aimed ) {
	aimPending_t	*p;
	vec3_t			motion, offset, along;
	float			speed, frame, trust, used, unclipped;
	int				i;

	if ( !cl_aimAssistLearn->integer || CL_AimAssistProjectileSpeed( weapon ) <= 0.0f || lead <= 0.0f ) {
		return;
	}

	// A jump says nothing about how long a bot holds a line on the ground,
	// and its course is not damped by the hold time in the first place.
	if ( entity->groundEntityNum == ENTITYNUM_NONE ) {
		CL_AimAssistDrop( weapon, "target in the air" );
		return;
	}

	CL_AimAssistVelocity( entity, motion );
	motion[2] = 0.0f;
	speed = VectorLength( motion );
	if ( speed * lead < 40.0f ) {
		CL_AimAssistDrop( weapon, "target barely moving" );
		return;
	}
	VectorScale( motion, 1.0f / speed, along );

	// A prediction that a wall or a ledge cut short never aimed at the run it
	// expected, so where the target got to says nothing about the hold time.
	trust = CL_AimAssistTrust( entity, lead );
	VectorSubtract( aimed, entity->pos.trBase, offset );
	offset[2] = 0.0f;
	used = DotProduct( offset, along );
	// No phase term here. This only ever runs on the command the shot goes out
	// on, and that command is now given the point without the smoothing
	// correction, for every weapon and not only the single-shot ones. Adding
	// it back would compare the aim point against one up to twenty units
	// further out, against a tolerance of two, and throw good samples away as
	// clipped geometry - out of a learner that already loses four in five.
	unclipped = speed * CL_AimAssistSideways( lead ) * trust * CL_AimAssistTune( weapon, lead, speed );
	if ( used < unclipped - 2.0f ) {
		CL_AimAssistDrop( weapon, "prediction cut short by geometry" );
		return;
	}

	// One open sample per target at a time. A stream of plasma at one bot
	// watches the same run over and over, and would step the hold time once
	// per bolt for what is one observation.
	for ( i = 0; i < AIM_PENDING; i++ ) {
		if ( aimPending[i].arrival && aimPending[i].target == entity->clientNum
			&& aimPending[i].arrival - cl.snap.serverTime <= 5000 ) {
			CL_AimAssistDrop( weapon, "already watching this target" );
			return;
		}
	}

	// the arrival is a game frame, whichever lead was steered with
	frame = CL_AimAssistFrameTime();
	lead = (int)( lead / frame + 0.5f ) * frame;
	if ( lead <= 0.0f ) {
		return;
	}

	p = &aimPending[aimPendingNum++ & ( AIM_PENDING - 1 )];
	p->target = entity->clientNum;
	p->arrival = cl.snap.serverTime + (int)( lead * 1000.0f + 0.5f );
	p->weapon = weapon;
	VectorCopy( entity->pos.trBase, p->origin );
	VectorCopy( along, p->along );
	p->straight = speed * lead;
	p->speed = speed;
	p->base = speed * CL_AimAssistSideways( lead ) * trust;
	p->expected = p->base * CL_AimAssistTune( weapon, lead, speed );
	p->rate = speed * trust;
	p->lead = lead;
}

static void CL_AimAssistLearn( void ) {
	aimPending_t		*p;
	const entityState_t	*entity, *found;
	const char			*info;
	vec3_t				moved;
	float				actual, lateral, expected, error, weight, hold, before;
	int					i, j, band, pace;

	for ( i = 0; i < AIM_PENDING; i++ ) {
		p = &aimPending[i];
		if ( !p->arrival ) {
			continue;
		}
		if ( p->arrival - cl.snap.serverTime > 5000 ) {
			p->arrival = 0;		// a new map turned the clock back; forget it
			continue;
		}
		if ( cl.snap.serverTime < p->arrival ) {
			continue;
		}

		// Only the arrival frame's own snapshot can say where the target was
		// then. If it never came - a hitch, or fewer snapshots than frames -
		// the next one shows a longer run than the shot was led by, and would
		// only ever say the target ran further.
		if ( cl.snap.serverTime > p->arrival ) {
			p->arrival = 0;
			CL_AimAssistDrop( p->weapon, "no snapshot at the arrival frame" );
			continue;
		}
		p->arrival = 0;

		found = NULL;
		for ( j = 0; j < cl.snap.numEntities; j++ ) {
			entity = &cl.parseEntities[( cl.snap.parseEntitiesNum + j ) & ( MAX_PARSE_ENTITIES - 1 )];
			if ( entity->eType == ET_PLAYER && entity->clientNum == p->target
				&& !( entity->eFlags & EF_DEAD ) ) {
				found = entity;
				break;
			}
		}
		if ( !found || p->straight < 1.0f ) {
			CL_AimAssistDrop( p->weapon, "target gone before the shot arrived" );
			continue;
		}

		VectorSubtract( found->pos.trBase, p->origin, moved );
		moved[2] = 0.0f;
		if ( VectorLength( moved ) > 1500.0f ) {
			CL_AimAssistDrop( p->weapon, "target teleported" );
			continue;
		}

		actual = DotProduct( moved, p->along );
		lateral = sqrt( fabs( DotProduct( moved, moved ) - actual * actual ) );

		// How far short of or beyond the expectation the target got, as a share
		// of the straight run, moves the hold time by a bounded step; a target
		// that mostly went sideways counts for less. The step is small, so the
		// value settles on how the bots behave rather than on the last one's
		// last turn.
		expected = p->expected;
		error = Com_Clamp( -1.0f, 1.0f, ( actual - expected ) / p->straight );
		weight = p->straight / ( p->straight + lateral );
		// Into the box for this weapon at this flight time, and nowhere else.
		// There used to be a single hold time that every shot moved, and that
		// was wrong: a run of long rockets pulled it down and shortened the
		// lead for close plasma with it, where nothing had been measured at
		// all. What is learned at one range belongs to that range.
		// Welches Fach getroffen wird und was darin stand, bevor der Schuss es
		// bewegt hat. Das ist die eine Zahl, die sich aus dem Protokoll nicht
		// zurueckrechnen laesst: "factor" weiter unten ist der ueber bis zu
		// vier Faecher verblendete Wert an genau diesem Vorhalt, nicht der des
		// Faches - und darum addieren sich die einzelnen Korrekturen auch
		// nicht zur Bewegung des Faches auf.
		band = CL_AimAssistBand( p->lead );
		pace = CL_AimAssistSpeedBand( p->speed );
		before = CL_AimAssistBoxFactor( p->weapon, band, pace );

		CL_AimAssistTuneUpdate( p->weapon, p->lead, p->speed, p->base, expected, actual,
			lateral, weight );

		// Put it on disk while the game is still running. Until now the table
		// was only written when the map ended, so a game that stopped any
		// other way took the whole evening's measuring with it - and a box
		// fills a handful of samples at a time, so an evening is what it is.
		// The file is a few hundred bytes and a sample arrives a few times a
		// minute; a quarter of a minute between writes is more than enough
		// caution for that.
		if ( cl.serverTime - aimTuneWritten > 15000 || aimTuneWritten > cl.serverTime ) {
			aimTuneWritten = cl.serverTime;
			CL_AimAssistTuneSave();
		}

		hold = CL_AimAssistHold();
		aimLearned++;

		info = cl.gameState.stringData + cl.gameState.stringOffsets[CS_PLAYERS + p->target];
		// Beide Geschwindigkeiten stehen dabei: die des Ziels ist die Achse der
		// Tabelle, die eigene ist es bewusst nicht - ob sie es sein sollte,
		// laesst sich nur an diesen Zeilen entscheiden.
		Com_Printf( "aim learn: %s target %s ran %.0f of %.0f expected %.0f aside %.0f"
			" error %.2f weight %.2f pace %.0f myspeed %.0f hold %.2f n %i frame %i\n",
			CL_AimAssistWeaponName( p->weapon ), Info_ValueForKey( info, "n" ),
			actual, p->straight, expected, lateral, error, weight,
			p->speed, VectorLength( cl.snap.ps.velocity ), hold,
			aimLearned, cl.snap.serverTime );
		// "was" und "now" sind das Fach selbst, vorher und nachher - der
		// Eintrag im Hauptbuch. "factor" daneben ist der verblendete Wert an
		// diesem einen Vorhalt: was die Zielhilfe fuer diesen Schuss gegeben
		// haette, nicht was in der Tabelle steht. Zwei verschiedene Zahlen,
		// und ohne die ersten beiden war die Bewegung des Faches aus dem
		// Protokoll nicht zu lesen.
		Com_Printf( "aim tune: %s band %i from %.1f pace %i above %.0f was %.2f now %.2f"
			" factor %.2f scatter %.0f reach %.0f n %i frame %i\n",
			CL_AimAssistWeaponName( p->weapon ), band, CL_AimAssistBandStart( band ),
			pace, CL_AimAssistSpeedStart( pace ),
			before, CL_AimAssistBoxFactor( p->weapon, band, pace ),
			CL_AimAssistTune( p->weapon, p->lead, p->speed ),
			CL_AimAssistScatter( p->weapon, p->lead, p->speed ),
			CL_AimAssistHitRadius( p->weapon ), aimTune[p->weapon][band][pace].samples,
			cl.snap.serverTime );
	}
}


/*
=================
CL_AimAssistWatch

Bookkeeping done once for every snapshot: who hurt us last, and whether a
target the learner is waiting on has died on the way.
=================
*/
static int	aimDamageEvent = -1;	// the player state's pain counter as last seen
static int	aimDamageCount = -1;	// its damage count as last seen
static int	aimAttackerSeen = -1;	// PERS_ATTACKER as last seen
static int	aimSpawnCount = -1;		// PERS_SPAWN_COUNT as last seen
static int	aimWatchedTime = -1;	// server time of the previous snapshot

static void CL_AimAssistWatch( void ) {
	const playerState_t	*ps = &cl.snap.ps;
	const entityState_t	*entity;
	int					i, j, attacker;

	// a new map: nothing from the old one still holds
	if ( cl.snap.serverTime < aimWatchedTime ) {
		aimAttacker = -1;
		aimSpawnCount = -1;
		for ( i = 0; i < AIM_PENDING; i++ ) {
			aimPending[i].arrival = 0;
		}
		CL_AimAssistRateForget();
	}
	aimWatchedTime = cl.snap.serverTime;

	CL_AimAssistWoundWatch();
	// after the credit, never before it: a window that closes on the same
	// frame its damage arrives must still be able to take it
	CL_AimAssistRateClose();

	// Whoever hurt us last. The player state names the attacker, but it names
	// client zero before anyone has, and it carries the last life's killer
	// into the next; so a new life - a respawn or a map restart, both of which
	// the spawn count shows - starts without a grudge and only syncs the
	// counters. Within a life a hit is believed when something moved: the pain
	// counter, which the game steps at most once in 700 ms; the damage count,
	// which it rewrites on every damaged frame; or the attacker itself, which
	// it rewrites on every hit.
	// A restart or a team change goes through the same reset as a connect,
	// which leaves the spawn count where a life with no death had it; the
	// damage count going back to nothing tells those apart from a hit, which
	// always leaves a count behind.
	if ( ps->persistant[PERS_SPAWN_COUNT] != aimSpawnCount
		|| ( ps->damageCount == 0 && aimDamageCount != 0 ) ) {
		aimSpawnCount = ps->persistant[PERS_SPAWN_COUNT];
		aimAttacker = -1;
		aimDamageEvent = ps->damageEvent;
		aimDamageCount = ps->damageCount;
		aimAttackerSeen = ps->persistant[PERS_ATTACKER];
	} else if ( ps->damageEvent != aimDamageEvent || ps->damageCount != aimDamageCount
		|| ps->persistant[PERS_ATTACKER] != aimAttackerSeen ) {
		aimDamageEvent = ps->damageEvent;
		aimDamageCount = ps->damageCount;
		aimAttackerSeen = ps->persistant[PERS_ATTACKER];
		attacker = ps->persistant[PERS_ATTACKER];
		if ( attacker >= 0 && attacker < MAX_CLIENTS && attacker != ps->clientNum ) {
			aimAttacker = attacker;
			aimAttackerTime = cl.snap.serverTime;
		}
	}
	if ( ps->stats[STAT_HEALTH] <= 0 ) {
		aimAttacker = -1;
	}

	// A target that died on the way says nothing about where it was going.
	// Only the player's own entity counts: the corpse the game leaves behind
	// carries the same client number for a while after the player is back.
	for ( i = 0; i < cl.snap.numEntities; i++ ) {
		entity = &cl.parseEntities[( cl.snap.parseEntitiesNum + i ) & ( MAX_PARSE_ENTITIES - 1 )];
		if ( entity->eType != ET_PLAYER || !( entity->eFlags & EF_DEAD )
			|| entity->number != entity->clientNum ) {
			continue;
		}
		for ( j = 0; j < AIM_PENDING; j++ ) {
			if ( aimPending[j].arrival && aimPending[j].target == entity->clientNum ) {
				aimPending[j].arrival = 0;
			}
		}
	}
}


/*
=================
CL_AimAssistSnapshot

Called once for every snapshot that arrives. Feeds the lead learner, and with
cl_aimAssistDebug writes down where shots really ended up, which is the one
thing the shot line cannot know: every impact event with its position and
whoever it belongs to, every missile the moment it first appears (its entity
number ties the launch to the explosion later), and with each impact where
every bot stood at that moment - so a miss can be measured in units and in
direction, not only counted.
=================
*/
void CL_AimAssistSnapshot( void ) {
	static const char	*names[] = {
		"bullet-flesh", "bullet-wall", "missile-hit", "missile-miss",
		"missile-metal", "rail", "shotgun",
	};
	static int			missileSeen[MAX_GENTITIES];	// message number a missile was last in
	static int			lastEvent[MAX_GENTITIES];	// event an entity carried in the previous snapshot
	static int			lastMessage;
	const entityState_t	*entity;
	const char			*info;
	char				bots[1024];
	vec3_t				far;
	int					i, j, event, kind, present;

	// Localhost and private LAN test servers are allowed. Do not remove this
	// boundary: without it the aim assist could run on public Internet servers.
	if ( !cl.snap.valid || clc.demoplaying
		|| cl.snap.messageNum == lastMessage ) {
		return;
	}

	// Womit dieses Protokoll geschrieben wurde. Die Zahl steigt, sobald sich
	// eine der Zeilen aendert, damit eine Auswertung nicht stillschweigend
	// Felder liest, die es damals noch nicht gab.
	if ( !aimLogStamped ) {
		aimLogStamped = qtrue;
		Com_Printf( "aim log: version %i built %s %s frame %i\n",
			AIM_LOG_VERSION, __DATE__, __TIME__, cl.snap.serverTime );
	}

	CL_AimAssistWatch();
	if ( cl_aimAssistLearn->integer ) {
		CL_AimAssistLearn();
	}

	if ( !cl_aimAssistDebug->integer ) {
		lastMessage = cl.snap.messageNum;
		return;
	}

	bots[0] = '\0';

	for ( i = 0; i < cl.snap.numEntities; i++ ) {
		entity = &cl.parseEntities[( cl.snap.parseEntitiesNum + i ) & ( MAX_PARSE_ENTITIES - 1 )];
		if ( entity->number < 0 || entity->number >= MAX_GENTITIES ) {
			continue;
		}

		// the bots, for the line that needs them
		if ( entity->eType == ET_PLAYER && !( entity->eFlags & EF_DEAD )
			&& entity->clientNum >= 0 && entity->clientNum < MAX_CLIENTS ) {
			info = cl.gameState.stringData + cl.gameState.stringOffsets[CS_PLAYERS + entity->clientNum];
			if ( *Info_ValueForKey( info, "skill" ) || cl_aimAssistHumanTargets->integer ) {
				Q_strcat( bots, sizeof( bots ), va( " | %s %.0f %.0f %.0f air %i",
					Info_ValueForKey( info, "n" ),
					entity->pos.trBase[0], entity->pos.trBase[1], entity->pos.trBase[2],
					entity->groundEntityNum == ENTITYNUM_NONE ? 1 : 0 ) );
			}
		}

		// a missile that was not there a snapshot ago has just been fired
		if ( entity->eType == ET_MISSILE ) {
			if ( missileSeen[entity->number] != lastMessage ) {
				Com_Printf( "aim missile: num %i at %.0f %.0f %.0f frame %i\n",
					entity->number, entity->pos.trBase[0], entity->pos.trBase[1],
					entity->pos.trBase[2], cl.snap.serverTime );
			}
			missileSeen[entity->number] = cl.snap.messageNum;
		}
	}

	for ( i = 0; i < cl.snap.numEntities; i++ ) {
		entity = &cl.parseEntities[( cl.snap.parseEntitiesNum + i ) & ( MAX_PARSE_ENTITIES - 1 )];
		if ( entity->number < 0 || entity->number >= MAX_GENTITIES ) {
			continue;
		}

		if ( entity->eType >= ET_EVENTS ) {
			event = entity->eType - ET_EVENTS;
		} else {
			event = entity->event;
		}
		event &= ~EV_EVENT_BITS;

		switch ( event ) {
		case EV_BULLET_HIT_FLESH:	kind = 0; break;
		case EV_BULLET_HIT_WALL:	kind = 1; break;
		case EV_MISSILE_HIT:		kind = 2; break;
		case EV_MISSILE_MISS:		kind = 3; break;
		case EV_MISSILE_MISS_METAL:	kind = 4; break;
		case EV_RAILTRAIL:			kind = 5; break;
		case EV_SHOTGUN:			kind = 6; break;
		default:					kind = -1; break;
		}

		// the same event on the same entity as last time is the same event
		if ( kind < 0 || lastEvent[entity->number] == event + 1 ) {
			lastEvent[entity->number] = kind < 0 ? 0 : event + 1;
			continue;
		}
		lastEvent[entity->number] = event + 1;

		// The shotgun event sits at the muzzle and carries the direction of its
		// centre ray in origin2, scaled to 4096 units; the pellets themselves
		// are traced on the client and never reported. The far end of that ray
		// is the one point that puts the shot on a line. Every other impact is
		// where it happened.
		if ( event == EV_SHOTGUN ) {
			VectorAdd( entity->pos.trBase, entity->origin2, far );
			Com_Printf( "aim impact: %s num %i other %i client %i at %.0f %.0f %.0f frame %i%s\n",
				names[kind], entity->number, entity->otherEntityNum, entity->clientNum,
				far[0], far[1], far[2], cl.snap.serverTime, bots );
		} else {
			Com_Printf( "aim impact: %s num %i other %i client %i at %.0f %.0f %.0f frame %i%s\n",
				names[kind], entity->number, entity->otherEntityNum, entity->clientNum,
				entity->pos.trBase[0], entity->pos.trBase[1], entity->pos.trBase[2],
				cl.snap.serverTime, bots );
		}
	}

	// forget the events of entities that are gone, so a number reused later
	// is not mistaken for a repeat
	for ( i = 0; i < MAX_GENTITIES; i++ ) {
		if ( !lastEvent[i] ) {
			continue;
		}
		present = 0;
		for ( j = 0; j < cl.snap.numEntities; j++ ) {
			if ( cl.parseEntities[( cl.snap.parseEntitiesNum + j ) & ( MAX_PARSE_ENTITIES - 1 )].number == i ) {
				present = 1;
				break;
			}
		}
		if ( !present ) {
			lastEvent[i] = 0;
		}
	}

	lastMessage = cl.snap.messageNum;
}


static float	aimSmooth[2];			// the filtered desired pitch and yaw
static int		aimSmoothTarget = -1;	// who the filter was following

static void CL_AimAssistSteer( usercmd_t *cmd, const vec3_t oldAngles ) {
	entityState_t	*entity;
	trace_t			trace;
	vec3_t			viewOrigin, targetOrigin, direction, desired;
	float			pitchDelta, yawDelta, pitchStep, low, high, blend, lead, flight, touchdown;
	float			frameTime, k;
	float			holdRange = 0.0f;
	int				i, key, localTeam, weapon, hold;
	qboolean		aimKeyHasAttack, otherAttackKey, firing, steering, exact, plain, clear;

	// Cleared here rather than at each way out. There are six of them, and two
	// leave after the target has already been recorded - so anything keyed on
	// the target alone would go on showing last frame's number while you stand
	// on top of a bot.
	aimFlightTime = -1.0f;

	// Localhost and private LAN test servers are allowed. Do not remove this
	// boundary: without it the aim assist could run on public Internet servers.
	if ( clc.state != CA_ACTIVE || clc.demoplaying || !cl.snap.valid ||
		 cl.snap.ps.pm_type == PM_INTERMISSION || cl.snap.ps.pm_type == PM_DEAD ||
		 ( cl.snap.ps.pm_flags & PMF_FOLLOW ) ) {
		CL_AimAssistHoldForget();
		aimAssistTarget = -1;
		aimSmoothTarget = -1;
		return;
	}

	localTeam = cl.snap.ps.persistant[PERS_TEAM];
	if ( localTeam == TEAM_SPECTATOR ) {
		CL_AimAssistHoldForget();
		aimAssistTarget = -1;
		aimSmoothTarget = -1;
		return;
	}

	CL_AimAssistPhaseUpdate();

	key = Key_StringToKeynum( cl_aimAssistKey->string );
	steering = cl_aimAssist->integer && key >= 0 && Key_IsDown( key );

	CL_AimAssistEye( viewOrigin );
	// The weapon about to be held, not the one still in hand: during a change
	// the command already carries the new one, and leading for the weapon that
	// is going to fire is the point. But only if it is really owned - a
	// hand-written switch order naming a weapon nobody has would otherwise
	// leave the assist computing that weapon's lead for good, and silently,
	// because the shot record refuses to write a line while the weapon in the
	// command and the weapon in hand disagree.
	weapon = cl.cgameUserCmdValue;
	if ( weapon <= WP_NONE || weapon >= WP_NUM_WEAPONS
		|| !( cl.snap.ps.stats[STAT_WEAPONS] & ( 1 << weapon ) ) ) {
		weapon = cl.snap.ps.weapon;
	}

	if ( !steering ) {
		// no help this frame, but a shot is still worth a line for the record
		entity = cl_aimAssistDebug->integer
			? CL_AimAssistPickTarget( viewOrigin, localTeam, weapon, qfalse ) : NULL;
		firing = CL_AimAssistFiring( cmd, weapon, viewOrigin, entity );
		if ( firing && entity ) {
			// The same aim point the steered shots are measured against, or
			// the two columns are not a comparison at all: this used to force
			// the frame-quantised point while the steered shots took the
			// smooth one, so the control group was judged by another model.
			exact = cl_aimAssistExact->integer == 2
				|| ( cl_aimAssistExact->integer == 1 && CL_AimAssistSingleShot( weapon ) );
			CL_AimAssistTargetPoint( entity, viewOrigin, weapon, exact, qtrue, targetOrigin, &lead );
			VectorSubtract( targetOrigin, viewOrigin, direction );
			vectoangles( direction, desired );
			desired[PITCH] -= SHORT2ANGLE( cl.snap.ps.delta_angles[PITCH] );
			desired[YAW] -= SHORT2ANGLE( cl.snap.ps.delta_angles[YAW] );
			pitchDelta = AngleNormalize180( desired[PITCH] - cl.viewangles[PITCH] );
			yawDelta = AngleNormalize180( desired[YAW] - cl.viewangles[YAW] );
			CL_AimAssistLogShot( entity, weapon, viewOrigin, targetOrigin, lead,
				sqrt( pitchDelta * pitchDelta + yawDelta * yawDelta ),
				sqrt( pitchDelta * pitchDelta + yawDelta * yawDelta ),
				qfalse, exact, qfalse, CL_AimAssistDueDown( entity, lead ) );
		}
		// The trigger is nobody's business while the key is up, so a hold left
		// standing from the last press is closed here rather than reported as
		// having lasted however long the key was released.
		CL_AimAssistLogHold( -1, weapon, 0.0f );
		aimHoldRun = 0;
		aimAssistTarget = -1;
		aimSmoothTarget = -1;
		return;
	}

	// The hold key may already be bound to +attack in q3config.cfg.  Do not
	// let that binding turn aim activation into automatic fire; preserve an
	// attack only when a separate fire key is held as well.
	aimKeyHasAttack = qfalse;
	otherAttackKey = qfalse;
	for ( i = 0; i < 2; i++ ) {
		if ( in_buttons[0].down[i] == key ) {
			aimKeyHasAttack = qtrue;
		} else if ( in_buttons[0].down[i] ) {
			otherAttackKey = qtrue;
		}
	}
	if ( aimKeyHasAttack && !otherAttackKey ) {
		cmd->buttons &= ~BUTTON_ATTACK;
	}

	entity = CL_AimAssistPickTarget( viewOrigin, localTeam, weapon, qtrue );

	// A shot into cover is a wasted one: hold the trigger until there is a way
	// through, for anyone who asked for that. What has to be clear is the line
	// the shot will really take - the led point a rocket is aimed at, not the
	// target itself. A bot behind a pillar whose lead stands in the open is a
	// shot worth taking, and one standing in the open whose lead is behind the
	// pillar is not. Only when neither can be reached is there nothing to do.
	//
	// It is decided here, before the shot is predicted, so that a command that
	// leaves without the trigger never leaves a fired shot behind in the timer.
	// Only worth asking while the trigger is actually down. There is nothing
	// to take away otherwise, and the question costs a prediction and a
	// handful of traces on every command that is built.
	hold = ( cl_aimAssistHoldFire->integer && ( cmd->buttons & BUTTON_ATTACK ) )
		? CL_AimAssistHoldReason( entity, weapon, viewOrigin, &holdRange ) : -1;

	// A target crossing the edge of a pillar answers blocked, clear, blocked
	// at the rate commands are built, and a trigger that follows that stutters.
	// So the block has to stand for a few commands before it takes the shot
	// away, while a single clear one gives it straight back: being slow to
	// refuse and quick to allow is the right way round for a gun.
	if ( hold >= 0 ) {
		aimHoldRun++;
		if ( aimHoldRun >= 3 ) {
			cmd->buttons &= ~BUTTON_ATTACK;
		} else {
			hold = -1;
		}
	} else {
		aimHoldRun = 0;
	}
	CL_AimAssistLogHold( hold, weapon, holdRange );

	// the weapon timer runs whether or not there is anything to steer at
	firing = CL_AimAssistFiring( cmd, weapon, viewOrigin, entity );

	if ( !entity ) {
		CL_AimAssistSkip( 0, weapon );
		aimAssistTarget = -1;
		aimSmoothTarget = -1;
		return;
	}
	if ( entity->clientNum != aimAssistTarget ) {
		aimKeepSince = cl.snap.serverTime;		// the clock on staying with it
	}
	aimAssistTarget = entity->clientNum;

	exact = firing && ( cl_aimAssistExact->integer == 2
		|| ( cl_aimAssistExact->integer == 1 && CL_AimAssistSingleShot( weapon ) ) );

	CL_AimAssistTargetPoint( entity, viewOrigin, weapon, exact, firing, targetOrigin, &lead );
	flight = lead;
	plain = qfalse;

	// The shot has to be able to get there. A led point can end up behind a
	// corner or below an edge, and steering a rocket into the floor in front
	// of you is worse than not helping at all: fall back to the plain position
	// the target was picked by, and leave the aim alone when even that is
	// blocked or the impact would land in your own splash. A grenade is judged
	// by its arc, not by the line to the point the view is put on.
	if ( CL_AimAssistArcWeapon( weapon ) ) {
		clear = CL_AimAssistArcClear( viewOrigin, targetOrigin );
	} else {
		CM_BoxTrace( &trace, viewOrigin, targetOrigin, vec3_origin, vec3_origin,
			0, MASK_SHOT, qfalse );
		clear = trace.fraction >= 1.0f;
	}
	if ( !clear ) {
		VectorCopy( entity->pos.trBase, targetOrigin );
		targetOrigin[2] += CL_AimAssistBodyHeight( entity );
		plain = qtrue;
		CM_BoxTrace( &trace, viewOrigin, targetOrigin, vec3_origin, vec3_origin,
			0, MASK_SHOT, qfalse );
		if ( trace.fraction < 1.0f ) {
			CL_AimAssistSkip( 1, weapon );
			aimSmoothTarget = -1;
			return;
		}

		// The shot goes at the body now, not at the led point, so it does not
		// fly as far. Everything downstream measures against this number: the
		// countdown drawn over the bot, the grade the hold-fire offer reads,
		// the "lead" field of the record - and the arrival window the hit-rate
		// table waits in, which is only a hundred and fifty milliseconds wide
		// against a lead that averages a third of a second. Without this a
		// fallback shot's damage lands outside its own window and books as a
		// miss, which made the worst-hitting group of shots look worse still.
		if ( CL_AimAssistProjectileSpeed( weapon ) > 0.0f ) {
			flight = CL_AimAssistFlight( entity, weapon, viewOrigin, targetOrigin );
			if ( flight < 0.0f ) {
				flight = 0.0f;		// point blank: it is there the moment it leaves
			}

			// On the frame grid, like every other lead in the system. The led
			// point is searched frame by frame, so its flight is a whole
			// number of frames by construction; this one is a plain distance
			// over a speed and is not. Six shots in a session went out with
			// leads of 718 and 1121 milliseconds, and they were exactly the
			// six that fell back - the only leads off the grid in the whole
			// record. That costs twice: the arrival window covers three
			// snapshot boundaries instead of four, and every reading of the
			// log that matches damage frames against world+lead can never
			// land on one, so those shots read as misses by construction.
			frameTime = CL_AimAssistFrameTime();
			flight = (int)( flight / frameTime + 0.5f ) * frameTime;
		}
	}

	// Inside our own splash the player aims this one alone. How close that is
	// belongs to the weapon: the flat hundred and sixty units this used to be
	// were measured for a rocket, whose splash reaches a hundred and twenty,
	// and plasma reaches twenty. It cost twenty plasma shots their help in one
	// session for a blast that could not have touched us.
	VectorSubtract( targetOrigin, viewOrigin, direction );
	if ( CL_AimAssistProjectileSpeed( weapon ) > 0.0f
		&& VectorLength( direction ) < CL_AimAssistHitRadius( weapon ) + 40.0f ) {
		CL_AimAssistSkip( 2, weapon );
		aimSmoothTarget = -1;
		return;
	}
	CL_AimAssistSkip( -1, weapon );			// steering again

	// Past both ways out, so this only runs when the view is really being
	// moved. Only a weapon that throws something has a flight worth counting
	// down, and that is decided here rather than in the drawing.
	if ( CL_AimAssistProjectileSpeed( weapon ) > 0.0f ) {
		aimFlightTime = flight;
		aimFlightGrade = CL_AimAssistShotGrade( entity, weapon, flight );
		aimFlightFrame = cls.framecount;
	}
	vectoangles( direction, desired );
	desired[PITCH] -= SHORT2ANGLE( cl.snap.ps.delta_angles[PITCH] );
	desired[YAW] -= SHORT2ANGLE( cl.snap.ps.delta_angles[YAW] );

	// The optional low pass on the point followed. It starts afresh on a new
	// target, so the view does not swing over from where the last one was,
	// and it is skipped on the firing command, which takes the exact point.
	if ( cl_aimAssistSmooth->value > 0.0f && !exact && aimSmoothTarget == entity->clientNum ) {
		k = frame_msec / ( cl_aimAssistSmooth->value + frame_msec );
		aimSmooth[0] += AngleNormalize180( desired[PITCH] - aimSmooth[0] ) * k;
		aimSmooth[1] += AngleNormalize180( desired[YAW] - aimSmooth[1] ) * k;
		desired[PITCH] = aimSmooth[0];
		desired[YAW] = aimSmooth[1];
	} else {
		aimSmooth[0] = desired[PITCH];
		aimSmooth[1] = desired[YAW];
	}
	aimSmoothTarget = entity->clientNum;

	// Level 10 snaps directly onto the target.  Lower levels retain the same
	// target choice but ease towards it. The one hurting us is snapped onto
	// at once, that is the point of it; and so is the exact point on the
	// command the shot goes out on.
	blend = ( exact || cl_aimAssist->integer == 10
		|| ( cl_aimAssistAttacker->integer && entity->clientNum == aimAttacker ) )
		? 1.0f : cl_aimAssist->value * frame_msec / 200.0f;
	blend = Com_Clamp( 0.0f, 1.0f, blend );
	pitchDelta = AngleNormalize180( desired[PITCH] - cl.viewangles[PITCH] );
	yawDelta = AngleNormalize180( desired[YAW] - cl.viewangles[YAW] );

	// The command builder caps the change of pitch since the command began
	// at ninety degrees, the mouse's share this frame included. Stay under
	// it, so that the shot goes where the record says it went.
	low = oldAngles[PITCH] - 89.0f - cl.viewangles[PITCH];
	high = oldAngles[PITCH] + 89.0f - cl.viewangles[PITCH];
	if ( low > high ) {
		low = high;
	}
	pitchStep = Com_Clamp( low, high, pitchDelta * blend );
	cl.viewangles[PITCH] += pitchStep;
	cl.viewangles[YAW] += yawDelta * blend;

	if ( firing ) {
		// who this shot went at, for the damage report that arrives later
		aimShotTarget = entity->clientNum;
		aimShotTime = cl.snap.serverTime;

		// A shot at the plain body has no led point to learn from, but it
		// flies the same time, which the record needs to match its impact.
		CL_AimAssistRemember( entity, weapon, plain ? 0.0f : lead, targetOrigin );

		// Once, not twice: the landing search walks outward from under the
		// target with a box trace per step, and both the table and the record
		// want the same answer for the same flight.
		touchdown = CL_AimAssistDueDown( entity, flight );

		// And every shot, whatever it is, goes on the hit-rate table's own
		// list. That one has none of the learner's four refusals: it wants
		// the hitscan weapons, which are three of the four in the record, and
		// it wants the jumping target most of all.
		CL_AimAssistRateWatch( entity, weapon, flight, viewOrigin, touchdown >= 0.0f );
		if ( cl_aimAssistDebug->integer ) {
			CL_AimAssistLogShot( entity, weapon, viewOrigin, targetOrigin, flight,
				sqrt( ( pitchDelta - pitchStep ) * ( pitchDelta - pitchStep )
					+ yawDelta * yawDelta * ( 1.0f - blend ) * ( 1.0f - blend ) ),
				sqrt( pitchDelta * pitchDelta + yawDelta * yawDelta ),
				qtrue, exact && !plain, plain, touchdown );
		}
	}
}


/*
=================
CL_AimAssist

Helps the hit-sound lab produce repeatable hits.  This deliberately does not
use sv_cheats: the hard safety boundary is the local network check itself.
Bots are eligible by default; cl_aimAssistHumanTargets can add human test
clients, but cannot make the assist run against a public Internet server.

Two points are computed for the target. The smooth one moves the way the
picture moves and is what the view is steered towards between shots. The
exact one is where the server will really test the shot, and on the command
the shot fires on - which is predicted from the weapon timer - the view is put
on it outright, when cl_aimAssistExact says so. That is how the follow can be
smooth without the shot paying for it. cl_aimAssistSmooth adds a low pass on
top for anyone who wants the follow softer still.
=================
*/
/*
=================
CL_AutoSwitchEmpty

Reach for another weapon on the round that empties the one in hand.

The game only notices a dry weapon when the trigger is pulled on it again, and
that notice costs five hundred milliseconds of weapon timer with the state left
at firing, so the change it asks for right afterwards is refused for the whole
of it. What it then reaches for is decided by counting down from the highest
weapon number, which finds the grappling hook first - that carries endless
ammunition on every spawn - and the BFG after it.

This sits in the engine and not in the cgame on purpose. A cgame that comes
with a mod, a HUD pack for instance, replaces the one built here, and a change
made there would simply not be running. Going through the console command the
cgame already answers works whichever cgame is loaded.

The order is the player's own, best first. Two passes: the first wants at least
two rounds, so one railgun slug does not beat eighty machinegun rounds; the
second takes whatever is left.
=================
*/
static int CL_AutoSwitchPick( int avoid ) {
	const playerState_t	*ps = &cl.snap.ps;
	const char			*text;
	char				token[32];
	int					order[WP_NUM_WEAPONS], ordered = 0;
	int					i, j, least;
	qboolean			known;

	text = cl_autoSwitchEmptyOrder->string;
	while ( *text && ordered < WP_NUM_WEAPONS ) {
		while ( *text == ' ' || *text == '\t' ) {
			text++;
		}
		for ( i = 0; *text && *text != ' ' && *text != '\t' && i < (int)sizeof( token ) - 1; i++ ) {
			token[i] = *text++;
		}
		token[i] = '\0';
		if ( !token[0] ) {
			continue;
		}
		for ( j = WP_NONE + 1; j < WP_NUM_WEAPONS; j++ ) {
			if ( Q_stricmp( token, CL_AimAssistWeaponName( j ) ) ) {
				continue;
			}
			known = qfalse;
			for ( i = 0; i < ordered; i++ ) {
				if ( order[i] == j ) {
					known = qtrue;
				}
			}
			if ( !known ) {
				order[ordered++] = j;
			}
			break;
		}
	}

	for ( least = 2; least >= 1; least-- ) {
		for ( i = 0; i < ordered; i++ ) {
			j = order[i];
			if ( j == avoid || !( ps->stats[STAT_WEAPONS] & ( 1 << j ) ) ) {
				continue;
			}
			if ( ps->ammo[j] < 0 || ps->ammo[j] >= least ) {
				return j;
			}
		}
	}

	// Nobody named the rest, so count down - but never onto the grappling
	// hook, which is always loaded and would win every time.
	for ( j = WP_NUM_WEAPONS - 1; j > WP_NONE; j-- ) {
		if ( j == avoid || j == WP_GRAPPLING_HOOK
			|| !( ps->stats[STAT_WEAPONS] & ( 1 << j ) ) ) {
			continue;
		}
		if ( ps->ammo[j] < 0 || ps->ammo[j] >= 1 ) {
			return j;
		}
	}
	return WP_NONE;
}

static void CL_AutoSwitchEmpty( void ) {
	const playerState_t	*ps = &cl.snap.ps;
	static int			watched = WP_NONE;
	static int			before = -1;
	static int			asked;
	int					held, next;

	if ( !cl_autoSwitchEmpty->integer || clc.state != CA_ACTIVE || clc.demoplaying
		|| !cl.snap.valid || ps->pm_type == PM_DEAD || ps->pm_type == PM_INTERMISSION
		|| ( ps->pm_flags & ( PMF_FOLLOW | PMF_RESPAWNED ) )
		|| ps->persistant[PERS_TEAM] == TEAM_SPECTATOR ) {
		watched = WP_NONE;
		before = -1;
		return;
	}

	held = ps->weapon;
	if ( held <= WP_NONE || held >= WP_NUM_WEAPONS ) {
		return;
	}
	if ( held != watched ) {
		watched = held;					// a change is under way or just landed
		before = ps->ammo[held];
		asked = 0;
		return;
	}

	// The command already carries a different weapon: somebody is changing by
	// hand, and two of us pulling at it would be worse than neither.
	if ( cl.cgameUserCmdValue != held ) {
		before = ps->ammo[held];
		return;
	}

	next = ps->ammo[held];
	if ( next != 0 || before < 0 ) {
		before = next;
		return;						// loaded, or endless
	}
	// At one, only the round that emptied it counts. At two, a weapon that is
	// empty for any other reason counts as well - one picked up empty, say.
	if ( cl_autoSwitchEmpty->integer < 2 && before <= 0 ) {
		return;
	}
	before = next;

	// The change goes through the command buffer and takes a frame; without
	// this the same request would be sent again on every frame until the
	// snapshot caught up, and the weapon would never settle.
	if ( asked && cl.serverTime - asked < 600 ) {
		return;
	}

	next = CL_AutoSwitchPick( held );
	if ( next <= WP_NONE ) {
		return;						// nothing loaded to reach for
	}
	asked = cl.serverTime;
	if ( cl_aimAssistDebug->integer ) {
		Com_Printf( "aim swap: %s empty to %s frame %i\n",
			CL_AimAssistWeaponName( held ), CL_AimAssistWeaponName( next ),
			cl.snap.serverTime );
	}
	Cbuf_AddText( va( "weapon %i\n", next ) );
}


static void CL_AimAssist( usercmd_t *cmd, const vec3_t oldAngles ) {
	CL_AimAssistSteer( cmd, oldAngles );

	// the view this command goes out with, for the next command's reach test
	aimPrevAngles[0] = cl.viewangles[PITCH];
	aimPrevAngles[1] = cl.viewangles[YAW];
}


/*
==============
CL_CmdButtons
==============
*/
void CL_CmdButtons( usercmd_t *cmd ) {
	int		i;

	//
	// figure button bits
	// send a button bit even if the key was pressed and released in
	// less than a frame
	//	
	for (i = 0 ; i < 15 ; i++) {
		if ( in_buttons[i].active || in_buttons[i].wasPressed ) {
			cmd->buttons |= 1 << i;
		}
		in_buttons[i].wasPressed = qfalse;
	}

	if ( Key_GetCatcher( ) ) {
		cmd->buttons |= BUTTON_TALK;
	}

	// allow the game to know if any key at all is
	// currently pressed, even if it isn't bound to anything
	if ( anykeydown && Key_GetCatcher( ) == 0 ) {
		cmd->buttons |= BUTTON_ANY;
	}
}


/*
==============
CL_FinishMove
==============
*/
void CL_FinishMove( usercmd_t *cmd ) {
	int		i;

	// copy the state that the cgame is currently sending
	cmd->weapon = cl.cgameUserCmdValue;

	// send the current server time so the amount of movement
	// can be determined without allowing cheating
	cmd->serverTime = cl.serverTime;

	for (i=0 ; i<3 ; i++) {
		cmd->angles[i] = ANGLE2SHORT(cl.viewangles[i]);
	}
}


/*
=================
CL_CreateCmd
=================
*/
usercmd_t CL_CreateCmd( void ) {
	usercmd_t	cmd;
	vec3_t		oldAngles;

	VectorCopy( cl.viewangles, oldAngles );

	// keyboard angle adjustment
	CL_AdjustAngles ();
	
	Com_Memset( &cmd, 0, sizeof( cmd ) );

	CL_CmdButtons( &cmd );

	// get basic movement from keyboard
	CL_KeyMove( &cmd );

	// get basic movement from mouse
	CL_MouseMove( &cmd );

	// get basic movement from joystick
	CL_JoystickMove( &cmd );

	// local bot-only helper used by the hit-sound test bench
	CL_AutoSwitchEmpty();
	CL_AimAssist( &cmd, oldAngles );

	// check to make sure the angles haven't wrapped
	if ( cl.viewangles[PITCH] - oldAngles[PITCH] > 90 ) {
		cl.viewangles[PITCH] = oldAngles[PITCH] + 90;
	} else if ( oldAngles[PITCH] - cl.viewangles[PITCH] > 90 ) {
		cl.viewangles[PITCH] = oldAngles[PITCH] - 90;
	} 

	// store out the final values
	CL_FinishMove( &cmd );

	// draw debug graphs of turning for mouse testing
	if ( cl_debugMove->integer ) {
		if ( cl_debugMove->integer == 1 ) {
			SCR_DebugGraph( fabs(cl.viewangles[YAW] - oldAngles[YAW]) );
		}
		if ( cl_debugMove->integer == 2 ) {
			SCR_DebugGraph( fabs(cl.viewangles[PITCH] - oldAngles[PITCH]) );
		}
	}

	return cmd;
}


/*
=================
CL_CreateNewCommands

Create a new usercmd_t structure for this frame
=================
*/
void CL_CreateNewCommands( void ) {
	int			cmdNum;

	// no need to create usercmds until we have a gamestate
	if ( clc.state < CA_PRIMED ) {
		return;
	}

	frame_msec = com_frameTime - old_com_frameTime;

	// if running over 1000fps, act as if each frame is 1ms
	// prevents divisions by zero
	if ( frame_msec < 1 ) {
		frame_msec = 1;
	}

	// if running less than 5fps, truncate the extra time to prevent
	// unexpected moves after a hitch
	if ( frame_msec > 200 ) {
		frame_msec = 200;
	}
	old_com_frameTime = com_frameTime;


	// generate a command for this frame
	cl.cmdNumber++;
	cmdNum = cl.cmdNumber & CMD_MASK;
	cl.cmds[cmdNum] = CL_CreateCmd ();
}

/*
=================
CL_ReadyToSendPacket

Returns qfalse if we are over the maxpackets limit
and should choke back the bandwidth a bit by not sending
a packet this frame.  All the commands will still get
delivered in the next packet, but saving a header and
getting more delta compression will reduce total bandwidth.
=================
*/
qboolean CL_ReadyToSendPacket( void ) {
	int		oldPacketNum;
	int		delta;

	// don't send anything if playing back a demo
	if ( clc.demoplaying || clc.state == CA_CINEMATIC ) {
		return qfalse;
	}

	// If we are downloading, we send no less than 50ms between packets
	if ( *clc.downloadTempName &&
		cls.realtime - clc.lastPacketSentTime < 50 ) {
		return qfalse;
	}

	// if we don't have a valid gamestate yet, only send
	// one packet a second
	if ( clc.state != CA_ACTIVE && 
		clc.state != CA_PRIMED && 
		!*clc.downloadTempName &&
		cls.realtime - clc.lastPacketSentTime < 1000 ) {
		return qfalse;
	}


	// send every frame for LAN
	if ( cl_lanForcePackets->integer ) {
		return qtrue;
	}

	// check for exceeding cl_maxpackets
	if ( cl_maxpackets->integer < 15 ) {
		Cvar_Set( "cl_maxpackets", "15" );
	} else if ( cl_maxpackets->integer > 125 ) {
		Cvar_Set( "cl_maxpackets", "125" );
	}
	oldPacketNum = (clc.netchan.outgoingSequence - 1) & PACKET_MASK;
	delta = cls.realtime -  cl.outPackets[ oldPacketNum ].p_realtime;
	if ( delta < 1000 / cl_maxpackets->integer ) {
		// the accumulated commands will go out in the next packet
		return qfalse;
	}

	return qtrue;
}

/*
===================
CL_WritePacket

Create and send the command packet to the server
Including both the reliable commands and the usercmds

During normal gameplay, a client packet will contain something like:

4	sequence number
2	qport
4	serverid
4	acknowledged sequence number
4	clc.serverCommandSequence
<optional reliable commands>
1	clc_move or clc_moveNoDelta
1	command count
<count * usercmds>

===================
*/
void CL_WritePacket( void ) {
	msg_t		buf;
	byte		data[MAX_MSGLEN];
	int			i, j;
	usercmd_t	*cmd, *oldcmd;
	usercmd_t	nullcmd;
	int			packetNum;
	int			oldPacketNum;
	int			count, key;

	// don't send anything if playing back a demo
	if ( clc.demoplaying || clc.state == CA_CINEMATIC ) {
		return;
	}

	Com_Memset( &nullcmd, 0, sizeof(nullcmd) );
	oldcmd = &nullcmd;

	MSG_Init( &buf, data, sizeof(data) );

	MSG_Bitstream( &buf );
	// write the current serverId so the server
	// can tell if this is from the current gameState
	MSG_WriteLong( &buf, cl.serverId );

	// write the last message we received, which can
	// be used for delta compression, and is also used
	// to tell if we dropped a gamestate
	MSG_WriteLong( &buf, clc.serverMessageSequence );

	// write the last reliable message we received
	MSG_WriteLong( &buf, clc.serverCommandSequence );

	// write any unacknowledged clientCommands
	for ( i = clc.reliableAcknowledge + 1 ; i <= clc.reliableSequence ; i++ ) {
		MSG_WriteByte( &buf, clc_clientCommand );
		MSG_WriteLong( &buf, i );
		MSG_WriteString( &buf, clc.reliableCommands[ i & (MAX_RELIABLE_COMMANDS-1) ] );
	}

	// we want to send all the usercmds that were generated in the last
	// few packet, so even if a couple packets are dropped in a row,
	// all the cmds will make it to the server
	if ( cl_packetdup->integer < 0 ) {
		Cvar_Set( "cl_packetdup", "0" );
	} else if ( cl_packetdup->integer > 5 ) {
		Cvar_Set( "cl_packetdup", "5" );
	}
	oldPacketNum = (clc.netchan.outgoingSequence - 1 - cl_packetdup->integer) & PACKET_MASK;
	count = cl.cmdNumber - cl.outPackets[ oldPacketNum ].p_cmdNumber;
	if ( count > MAX_PACKET_USERCMDS ) {
		count = MAX_PACKET_USERCMDS;
		Com_Printf("MAX_PACKET_USERCMDS\n");
	}

#ifdef USE_VOIP
	if (clc.voipOutgoingDataSize > 0)
	{
		if((clc.voipFlags & VOIP_SPATIAL) || Com_IsVoipTarget(clc.voipTargets, sizeof(clc.voipTargets), -1))
		{
			MSG_WriteByte (&buf, clc_voipOpus);
			MSG_WriteByte (&buf, clc.voipOutgoingGeneration);
			MSG_WriteLong (&buf, clc.voipOutgoingSequence);
			MSG_WriteByte (&buf, clc.voipOutgoingDataFrames);
			MSG_WriteData (&buf, clc.voipTargets, sizeof(clc.voipTargets));
			MSG_WriteByte(&buf, clc.voipFlags);
			MSG_WriteShort (&buf, clc.voipOutgoingDataSize);
			MSG_WriteData (&buf, clc.voipOutgoingData, clc.voipOutgoingDataSize);

			// If we're recording a demo, we have to fake a server packet with
			//  this VoIP data so it gets to disk; the server doesn't send it
			//  back to us, and we might as well eliminate concerns about dropped
			//  and misordered packets here.
			if(clc.demorecording && !clc.demowaiting)
			{
				const int voipSize = clc.voipOutgoingDataSize;
				msg_t fakemsg;
				byte fakedata[MAX_MSGLEN];
				MSG_Init (&fakemsg, fakedata, sizeof (fakedata));
				MSG_Bitstream (&fakemsg);
				MSG_WriteLong (&fakemsg, clc.reliableAcknowledge);
				MSG_WriteByte (&fakemsg, svc_voipOpus);
				MSG_WriteShort (&fakemsg, clc.clientNum);
				MSG_WriteByte (&fakemsg, clc.voipOutgoingGeneration);
				MSG_WriteLong (&fakemsg, clc.voipOutgoingSequence);
				MSG_WriteByte (&fakemsg, clc.voipOutgoingDataFrames);
				MSG_WriteShort (&fakemsg, clc.voipOutgoingDataSize );
				MSG_WriteBits (&fakemsg, clc.voipFlags, VOIP_FLAGCNT);
				MSG_WriteData (&fakemsg, clc.voipOutgoingData, voipSize);
				MSG_WriteByte (&fakemsg, svc_EOF);
				CL_WriteDemoMessage (&fakemsg, 0);
			}

			clc.voipOutgoingSequence += clc.voipOutgoingDataFrames;
			clc.voipOutgoingDataSize = 0;
			clc.voipOutgoingDataFrames = 0;
		}
		else
		{
			// We have data, but no targets. Silently discard all data
			clc.voipOutgoingDataSize = 0;
			clc.voipOutgoingDataFrames = 0;
		}
	}
#endif

	if ( count >= 1 ) {
		if ( cl_showSend->integer ) {
			Com_Printf( "(%i)", count );
		}

		// begin a client move command
		if ( cl_nodelta->integer || !cl.snap.valid || clc.demowaiting
			|| clc.serverMessageSequence != cl.snap.messageNum ) {
			MSG_WriteByte (&buf, clc_moveNoDelta);
		} else {
			MSG_WriteByte (&buf, clc_move);
		}

		// write the command count
		MSG_WriteByte( &buf, count );

		// use the checksum feed in the key
		key = clc.checksumFeed;
		// also use the message acknowledge
		key ^= clc.serverMessageSequence;
		// also use the last acknowledged server command in the key
		key ^= MSG_HashKey(clc.serverCommands[ clc.serverCommandSequence & (MAX_RELIABLE_COMMANDS-1) ], 32);

		// write all the commands, including the predicted command
		for ( i = 0 ; i < count ; i++ ) {
			j = (cl.cmdNumber - count + i + 1) & CMD_MASK;
			cmd = &cl.cmds[j];
			MSG_WriteDeltaUsercmdKey (&buf, key, oldcmd, cmd);
			oldcmd = cmd;
		}
	}

	//
	// deliver the message
	//
	packetNum = clc.netchan.outgoingSequence & PACKET_MASK;
	cl.outPackets[ packetNum ].p_realtime = cls.realtime;
	cl.outPackets[ packetNum ].p_serverTime = oldcmd->serverTime;
	cl.outPackets[ packetNum ].p_cmdNumber = cl.cmdNumber;
	clc.lastPacketSentTime = cls.realtime;

	if ( cl_showSend->integer ) {
		Com_Printf( "%i ", buf.cursize );
	}

	CL_Netchan_Transmit (&clc.netchan, &buf);	
}

/*
=================
CL_SendCmd

Called every frame to builds and sends a command packet to the server.
=================
*/
void CL_SendCmd( void ) {
	// don't send any message if not connected
	if ( clc.state < CA_CONNECTED ) {
		return;
	}

	// don't send commands if paused
	if ( com_sv_running->integer && sv_paused->integer && cl_paused->integer ) {
		return;
	}

	// we create commands even if a demo is playing,
	CL_CreateNewCommands();

	// don't send a packet if the last packet was sent too recently
	if ( !CL_ReadyToSendPacket() ) {
		if ( cl_showSend->integer ) {
			Com_Printf( ". " );
		}
		return;
	}

	CL_WritePacket();
}

/*
============
CL_InitInput
============
*/
void CL_InitInput( void ) {
	Cmd_AddCommand ("centerview",IN_CenterView);

	Cmd_AddCommand ("+moveup",IN_UpDown);
	Cmd_AddCommand ("-moveup",IN_UpUp);
	Cmd_AddCommand ("+movedown",IN_DownDown);
	Cmd_AddCommand ("-movedown",IN_DownUp);
	Cmd_AddCommand ("+left",IN_LeftDown);
	Cmd_AddCommand ("-left",IN_LeftUp);
	Cmd_AddCommand ("+right",IN_RightDown);
	Cmd_AddCommand ("-right",IN_RightUp);
	Cmd_AddCommand ("+forward",IN_ForwardDown);
	Cmd_AddCommand ("-forward",IN_ForwardUp);
	Cmd_AddCommand ("+back",IN_BackDown);
	Cmd_AddCommand ("-back",IN_BackUp);
	Cmd_AddCommand ("+lookup", IN_LookupDown);
	Cmd_AddCommand ("-lookup", IN_LookupUp);
	Cmd_AddCommand ("+lookdown", IN_LookdownDown);
	Cmd_AddCommand ("-lookdown", IN_LookdownUp);
	Cmd_AddCommand ("+strafe", IN_StrafeDown);
	Cmd_AddCommand ("-strafe", IN_StrafeUp);
	Cmd_AddCommand ("+moveleft", IN_MoveleftDown);
	Cmd_AddCommand ("-moveleft", IN_MoveleftUp);
	Cmd_AddCommand ("+moveright", IN_MoverightDown);
	Cmd_AddCommand ("-moveright", IN_MoverightUp);
	Cmd_AddCommand ("+speed", IN_SpeedDown);
	Cmd_AddCommand ("-speed", IN_SpeedUp);
	Cmd_AddCommand ("+attack", IN_Button0Down);
	Cmd_AddCommand ("-attack", IN_Button0Up);
	Cmd_AddCommand ("+button0", IN_Button0Down);
	Cmd_AddCommand ("-button0", IN_Button0Up);
	Cmd_AddCommand ("+button1", IN_Button1Down);
	Cmd_AddCommand ("-button1", IN_Button1Up);
	Cmd_AddCommand ("+button2", IN_Button2Down);
	Cmd_AddCommand ("-button2", IN_Button2Up);
	Cmd_AddCommand ("+button3", IN_Button3Down);
	Cmd_AddCommand ("-button3", IN_Button3Up);
	Cmd_AddCommand ("+button4", IN_Button4Down);
	Cmd_AddCommand ("-button4", IN_Button4Up);
	Cmd_AddCommand ("+button5", IN_Button5Down);
	Cmd_AddCommand ("-button5", IN_Button5Up);
	Cmd_AddCommand ("+button6", IN_Button6Down);
	Cmd_AddCommand ("-button6", IN_Button6Up);
	Cmd_AddCommand ("+button7", IN_Button7Down);
	Cmd_AddCommand ("-button7", IN_Button7Up);
	Cmd_AddCommand ("+button8", IN_Button8Down);
	Cmd_AddCommand ("-button8", IN_Button8Up);
	Cmd_AddCommand ("+button9", IN_Button9Down);
	Cmd_AddCommand ("-button9", IN_Button9Up);
	Cmd_AddCommand ("+button10", IN_Button10Down);
	Cmd_AddCommand ("-button10", IN_Button10Up);
	Cmd_AddCommand ("+button11", IN_Button11Down);
	Cmd_AddCommand ("-button11", IN_Button11Up);
	Cmd_AddCommand ("+button12", IN_Button12Down);
	Cmd_AddCommand ("-button12", IN_Button12Up);
	Cmd_AddCommand ("+button13", IN_Button13Down);
	Cmd_AddCommand ("-button13", IN_Button13Up);
	Cmd_AddCommand ("+button14", IN_Button14Down);
	Cmd_AddCommand ("-button14", IN_Button14Up);
	Cmd_AddCommand ("+mlook", IN_MLookDown);
	Cmd_AddCommand ("-mlook", IN_MLookUp);

#ifdef USE_VOIP
	Cmd_AddCommand ("+voiprecord", IN_VoipRecordDown);
	Cmd_AddCommand ("-voiprecord", IN_VoipRecordUp);
#endif

	cl_nodelta = Cvar_Get ("cl_nodelta", "0", 0);
	cl_debugMove = Cvar_Get ("cl_debugMove", "0", 0);
}

/*
============
CL_ShutdownInput
============
*/
void CL_ShutdownInput(void)
{
	Cmd_RemoveCommand("centerview");

	Cmd_RemoveCommand("+moveup");
	Cmd_RemoveCommand("-moveup");
	Cmd_RemoveCommand("+movedown");
	Cmd_RemoveCommand("-movedown");
	Cmd_RemoveCommand("+left");
	Cmd_RemoveCommand("-left");
	Cmd_RemoveCommand("+right");
	Cmd_RemoveCommand("-right");
	Cmd_RemoveCommand("+forward");
	Cmd_RemoveCommand("-forward");
	Cmd_RemoveCommand("+back");
	Cmd_RemoveCommand("-back");
	Cmd_RemoveCommand("+lookup");
	Cmd_RemoveCommand("-lookup");
	Cmd_RemoveCommand("+lookdown");
	Cmd_RemoveCommand("-lookdown");
	Cmd_RemoveCommand("+strafe");
	Cmd_RemoveCommand("-strafe");
	Cmd_RemoveCommand("+moveleft");
	Cmd_RemoveCommand("-moveleft");
	Cmd_RemoveCommand("+moveright");
	Cmd_RemoveCommand("-moveright");
	Cmd_RemoveCommand("+speed");
	Cmd_RemoveCommand("-speed");
	Cmd_RemoveCommand("+attack");
	Cmd_RemoveCommand("-attack");
	Cmd_RemoveCommand("+button0");
	Cmd_RemoveCommand("-button0");
	Cmd_RemoveCommand("+button1");
	Cmd_RemoveCommand("-button1");
	Cmd_RemoveCommand("+button2");
	Cmd_RemoveCommand("-button2");
	Cmd_RemoveCommand("+button3");
	Cmd_RemoveCommand("-button3");
	Cmd_RemoveCommand("+button4");
	Cmd_RemoveCommand("-button4");
	Cmd_RemoveCommand("+button5");
	Cmd_RemoveCommand("-button5");
	Cmd_RemoveCommand("+button6");
	Cmd_RemoveCommand("-button6");
	Cmd_RemoveCommand("+button7");
	Cmd_RemoveCommand("-button7");
	Cmd_RemoveCommand("+button8");
	Cmd_RemoveCommand("-button8");
	Cmd_RemoveCommand("+button9");
	Cmd_RemoveCommand("-button9");
	Cmd_RemoveCommand("+button10");
	Cmd_RemoveCommand("-button10");
	Cmd_RemoveCommand("+button11");
	Cmd_RemoveCommand("-button11");
	Cmd_RemoveCommand("+button12");
	Cmd_RemoveCommand("-button12");
	Cmd_RemoveCommand("+button13");
	Cmd_RemoveCommand("-button13");
	Cmd_RemoveCommand("+button14");
	Cmd_RemoveCommand("-button14");
	Cmd_RemoveCommand("+mlook");
	Cmd_RemoveCommand("-mlook");

#ifdef USE_VOIP
	Cmd_RemoveCommand("+voiprecord");
	Cmd_RemoveCommand("-voiprecord");
#endif
}
