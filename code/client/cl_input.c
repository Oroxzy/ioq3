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
#define AIM_LOG_VERSION	1

static qboolean	aimLogStamped;			// ob diese Verbindung schon gestempelt ist
static int		aimHoldMarked = -1;		// server time the learned value was last marked for q3config.cfg
static qboolean	aimHoldPending;			// a learned step the config has not been marked for yet

void Com_WriteConfiguration( void );

/*
=================
CL_AimAssistHold

The hold time in use is the cvar itself. The learner writes every step
straight into it, so whatever sets the cvar - a config, the console, the test
bench - simply becomes the value the learner goes on from, and there is no
second copy that could quietly disagree with what the cvar shows.
=================
*/
static float CL_AimAssistHold( void ) {
	return cl_aimAssistLead->value;
}

/*
=================
CL_AimAssistSetHold

The learner's write. The cvar is archived, and an ordinary set of an archived
cvar has the engine write q3config.cfg on the next frame - a file written on
every learned shot would be a hitch on every shot. So the step is written with
the archive flag masked, which changes the value at once and the file not at
all; the file is marked for writing every couple of seconds instead, and the
rest is flushed when the connection goes (CL_AimAssistFlush).
=================
*/
static void CL_AimAssistSetHold( float hold ) {
	int	flags;

	flags = cl_aimAssistLead->flags;
	cl_aimAssistLead->flags &= ~CVAR_ARCHIVE;
	Cvar_SetValue( "cl_aimAssistLead", hold );
	cl_aimAssistLead->flags = flags;
	aimHoldPending = qtrue;

	if ( aimHoldMarked < 0 || cl.snap.serverTime - aimHoldMarked > 2000 || aimHoldMarked > cl.snap.serverTime ) {
		cvar_modifiedFlags |= flags & CVAR_ARCHIVE;
		aimHoldMarked = cl.snap.serverTime;
		aimHoldPending = qfalse;
	}
}

/*
=================
CL_AimAssistFlush

Called when the connection goes. The mark alone is not enough on the way out
of the program: the config is written at the top of a frame, and no frame
follows a quit. So the file is written here as well.
=================
*/
static void CL_AimAssistTuneSave( void );
void CL_AimAssistTuneDump( void );
static void CL_AimAssistPriorityDump( void );

void CL_AimAssistFlush( void ) {
	aimLogStamped = qfalse;			// die naechste Sitzung stempelt neu
	CL_AimAssistTuneSave();
	CL_AimAssistTuneDump();

	if ( !cl_aimAssistLead || !aimHoldPending ) {
		return;
	}

	cvar_modifiedFlags |= cl_aimAssistLead->flags & CVAR_ARCHIVE;
	aimHoldPending = qfalse;
	aimHoldMarked = -1;
	Com_WriteConfiguration();
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
The record of what the prediction is really worth, per weapon and per flight
time.

The lead model says how far a target gets while a shot is on its way. How well
that holds depends on how long the shot is on its way: a plasma bolt arrives
before anyone can change their mind, a rocket at the far end of the map gives
them all the time in the world. It also depends on the weapon, because each
one asks the question at its own distances. So every learned shot is written
into the box for its weapon and its band of flight time, and two things are
kept there: the factor the expectation has to be multiplied by to match what
really happened, and how far the result still scatters after that.

The factor tells the prediction how much of the lead to believe. The scatter
tells the target choice which shots are worth steering at all - a shot whose
scatter is wider than the splash it would do is a lottery, whoever is being
shot at, and that is measured, not assumed.

A box fills slowly, a handful of shots an evening, so the table is written
next to the config and read back at the start: it goes on learning from where
it left off instead of starting over every session.
=================
*/
#define AIM_BANDS		4
#define AIM_TUNE_FILE	"aimtune.cfg"
#define AIM_TUNE_PRIOR	4.0f		// weight the untouched factor 1.0 carries
#define AIM_TUNE_DECAY	0.98f		// what a box keeps of its past per sample

typedef struct {
	float	sum;			// weighted sum of actual/expected
	float	weight;			// weight behind it
	float	square;			// weighted mean square of what is left over, in units
	int		samples;
} aimTune_t;

static aimTune_t	aimTune[WP_NUM_WEAPONS][AIM_BANDS];
static qboolean		aimTuneLoaded;
static qboolean		aimTuneDirty;

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

static void CL_AimAssistTuneLoad( void ) {
	union { char *c; void *v; }	file;
	const char					*line;
	aimTune_t					*t;
	float						sum, weight, square;
	int							weapon, band, samples;
	long						length;

	aimTuneLoaded = qtrue;

	length = FS_ReadFile( AIM_TUNE_FILE, &file.v );
	if ( length <= 0 || !file.c ) {
		return;
	}

	line = file.c;
	while ( *line ) {
		if ( *line != '#' && sscanf( line, "%i %i %f %f %f %i",
				&weapon, &band, &sum, &weight, &square, &samples ) == 6
			&& weapon > WP_NONE && weapon < WP_NUM_WEAPONS
			&& band >= 0 && band < AIM_BANDS
			&& weight >= 0.0f && square >= 0.0f && samples >= 0 ) {
			t = &aimTune[weapon][band];
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
	char		text[4096];
	const char	*name;
	aimTune_t	*t;
	int			weapon, band;

	if ( !aimTuneDirty ) {
		return;
	}

	Q_strncpyz( text, "// what the aim assist has measured about its own lead.\n"
		"// weapon band sum weight square samples\n", sizeof( text ) );

	for ( weapon = WP_NONE + 1; weapon < WP_NUM_WEAPONS; weapon++ ) {
		for ( band = 0; band < AIM_BANDS; band++ ) {
			t = &aimTune[weapon][band];
			if ( !t->samples ) {
				continue;
			}
			name = CL_AimAssistWeaponName( weapon );
			Q_strcat( text, sizeof( text ), va( "%i %i %.4f %.4f %.1f %i\t// %s %.1fs\n",
				weapon, band, t->sum, t->weight, t->square, t->samples,
				name, CL_AimAssistBandStart( band ) ) );
		}
	}

	FS_WriteFile( AIM_TUNE_FILE, text, strlen( text ) );
	aimTuneDirty = qfalse;
}

/*
=================
CL_AimAssistTune

How much of the modelled lead to believe for this weapon at this flight time.
One until something has been learned, and it never strays far: the box starts
with a prior weight of its own on the untouched value, so a first sample
nudges rather than decides.
=================
*/
static float CL_AimAssistTune( int weapon, float lead ) {
	const aimTune_t	*t;

	if ( !aimTuneLoaded ) {
		CL_AimAssistTuneLoad();
	}
	if ( weapon <= WP_NONE || weapon >= WP_NUM_WEAPONS || lead <= 0.0f ) {
		return 1.0f;
	}

	t = &aimTune[weapon][CL_AimAssistBand( lead )];
	return Com_Clamp( 0.1f, 2.0f,
		( t->sum + AIM_TUNE_PRIOR ) / ( t->weight + AIM_TUNE_PRIOR ) );
}

/*
=================
CL_AimAssistScatter

How far the shot is expected to land from the target, in units, after the lead
has been applied - what is left that no prediction can take away. Negative
until the box has seen enough shots to mean anything.
=================
*/
static float CL_AimAssistScatter( int weapon, float lead ) {
	const aimTune_t	*t;

	if ( !aimTuneLoaded ) {
		CL_AimAssistTuneLoad();
	}
	if ( weapon <= WP_NONE || weapon >= WP_NUM_WEAPONS || lead <= 0.0f ) {
		return -1.0f;
	}

	t = &aimTune[weapon][CL_AimAssistBand( lead )];
	if ( t->samples < 6 || t->weight <= 0.0f ) {
		return -1.0f;
	}

	return sqrt( t->square / t->weight );
}

/*
=================
CL_AimAssistTuneUpdate

One learned shot into its box. The factor follows the ratio the shot really
had, the scatter follows what the corrected prediction still missed by, and
both forget the distant past slowly so the table can follow an opponent that
changes without throwing away an evening's worth of shots.
=================
*/
static void CL_AimAssistTuneUpdate( int weapon, float lead, float expected, float actual, float weight ) {
	aimTune_t	*t;
	float		ratio, miss;

	if ( !aimTuneLoaded ) {
		CL_AimAssistTuneLoad();
	}
	if ( weapon <= WP_NONE || weapon >= WP_NUM_WEAPONS || lead <= 0.0f
		|| expected <= 1.0f || weight <= 0.0f ) {
		return;
	}

	t = &aimTune[weapon][CL_AimAssistBand( lead )];
	ratio = Com_Clamp( 0.0f, 2.0f, actual / expected );
	miss = actual - expected * CL_AimAssistTune( weapon, lead );

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

The whole table in one go, in the same lines the learner writes, so the bench
can show it whenever it likes and not only while shots are arriving. Bound to
the aimtune command and written once when the connection goes.
=================
*/
void CL_AimAssistTuneDump( void ) {
	const aimTune_t	*t;
	int				weapon, band, boxes = 0;

	if ( !aimTuneLoaded ) {
		CL_AimAssistTuneLoad();
	}

	for ( weapon = WP_NONE + 1; weapon < WP_NUM_WEAPONS; weapon++ ) {
		for ( band = 0; band < AIM_BANDS; band++ ) {
			t = &aimTune[weapon][band];
			if ( !t->samples ) {
				continue;
			}
			boxes++;
			Com_Printf( "aim tune: %s band %i from %.1f factor %.2f scatter %.0f reach %.0f n %i frame %i\n",
				CL_AimAssistWeaponName( weapon ), band, CL_AimAssistBandStart( band ),
				CL_AimAssistTune( weapon, CL_AimAssistBandStart( band ) + 0.05f ),
				CL_AimAssistScatter( weapon, CL_AimAssistBandStart( band ) + 0.05f ),
				CL_AimAssistHitRadius( weapon ), t->samples, cl.snap.serverTime );
		}
	}

	if ( !boxes ) {
		Com_Printf( "aim tune: nothing measured yet\n" );
	}

	// and what the priorities were understood as, so a typo in the string is
	// visible instead of quietly leaving a default in place
	CL_AimAssistPriorityDump();
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
static void CL_AimAssistPredict( const entityState_t *entity, int weapon, float time, vec3_t predicted,
		qboolean *blocked, qboolean *pinned ) {
	vec3_t		mins, maxs, stepMins, start, end, remaining, motion, above, below;
	float		gravity, sideways, floor;
	trace_t		trace;
	qboolean	grounded, floats, stopped, snapped;
	int			i;

	grounded = entity->groundEntityNum != ENTITYNUM_NONE;
	floats = CL_AimAssistFloats( entity );
	stopped = qfalse;
	snapped = qfalse;

	// Only the sideways guess of a target on the ground is damped, in two
	// ways: by how long bots hold a direction at all, and by how much this one
	// is turning right now; the climb of a ramp is part of the same run and is
	// damped with it. A target in the air cannot change where it is going -
	// the game gives it no friction and next to no steering - so its whole
	// course is kept, the rise and the fall included. Swimming and flying can
	// turn, and are damped like running.
	CL_AimAssistVelocity( entity, motion );
	if ( !grounded && !floats ) {
		sideways = time;
	} else {
		sideways = CL_AimAssistSideways( time ) * CL_AimAssistTrust( entity, time )
			* CL_AimAssistTune( weapon, time );
	}
	end[0] = entity->pos.trBase[0] + motion[0] * sideways;
	end[1] = entity->pos.trBase[1] + motion[1] * sideways;
	end[2] = entity->pos.trBase[2] + motion[2] * ( grounded ? sideways : time );

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
	// all of that away. In water and in flight there is no falling.
	if ( !grounded && !floats ) {
		gravity = cl.snap.ps.gravity > 0 ? cl.snap.ps.gravity : DEFAULT_GRAVITY;
		end[2] -= 0.5f * gravity * time * time;
	}

	// The game lifts a walking player over anything up to STEPSIZE, so the box
	// that clips the guess starts above that height: a curb, a stair riser or a
	// ramp is no obstacle to the target and must not cut its lead short. Only
	// what would stop the target itself may stop the prediction.
	CL_AimAssistHull( entity, mins, maxs );
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
		if ( !grounded ) {
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
		} else if ( grounded && predicted[2] - floor <= STEPSIZE ) {
			snapped = predicted[2] - floor > 0.5f;
			predicted[2] = floor;					// walked down a step
		}
	}

	if ( blocked ) {
		*blocked = stopped;
	}
	if ( pinned ) {
		*pinned = snapped;
	}
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
=================
*/
static void CL_AimAssistTargetPoint( const entityState_t *entity, const vec3_t viewOrigin,
		int weapon, qboolean exact, vec3_t targetOrigin, float *leadOut ) {
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
	// correction they would only wobble by. Not on the shot.
	if ( !exact && !blocked ) {
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

static float	aimPriorityWeight[AIM_PRIO_COUNT];
static int		aimPriorityCount = -1;		// modification count the weights were read at

static void CL_AimAssistPriorities( void ) {
	const char	*text;
	char		token[64];
	float		value;
	int			i, j;

	if ( cl_aimAssistPriority->modificationCount == aimPriorityCount ) {
		return;
	}
	aimPriorityCount = cl_aimAssistPriority->modificationCount;

	for ( i = 0; i < AIM_PRIO_COUNT; i++ ) {
		aimPriorityWeight[i] = aimPriorityDefault[i];
	}

	// "name:weight name:weight ...", anything not named keeps its default
	text = cl_aimAssistPriority->string;
	while ( *text ) {
		while ( *text == ' ' || *text == '\t' ) {
			text++;
		}
		for ( i = 0; *text && *text != ':' && *text != ' ' && i < (int)sizeof( token ) - 1; i++ ) {
			token[i] = *text++;
		}
		token[i] = '\0';
		if ( *text != ':' ) {
			while ( *text && *text != ' ' ) {
				text++;
			}
			continue;
		}
		text++;
		value = atof( text );
		while ( *text && *text != ' ' ) {
			text++;
		}

		for ( j = 0; j < AIM_PRIO_COUNT; j++ ) {
			if ( !Q_stricmp( token, aimPriorityName[j] ) ) {
				aimPriorityWeight[j] = Com_Clamp( 0.0f, 100.0f, value );
				break;
			}
		}
	}
}


static int	aimAssistTarget = -1;		// who the assist steered at last frame
static int	aimAttacker = -1;			// the bot that last hurt us, if any
static int	aimPickLast = -1;			// who the record last named as the pick

// what the weights were understood as, so a typo in the string shows up
static void CL_AimAssistPriorityDump( void ) {
	int	i;

	CL_AimAssistPriorities();
	Com_Printf( "aim prio:" );
	for ( i = 0; i < AIM_PRIO_COUNT; i++ ) {
		Com_Printf( " %s %.0f", aimPriorityName[i], aimPriorityWeight[i] );
	}
	Com_Printf( "\n" );
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
static int	aimWoundTime[MAX_CLIENTS];
static int	aimWoundHits = -1;			// PERS_HITS as last seen

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
	aimWoundHits = hits;

	// we hit someone, and the one we were aiming at is the one we hit
	target = aimAssistTarget;
	remaining = cl.snap.ps.persistant[PERS_ATTACKEE_REMAINING];
	if ( target < 0 || target >= MAX_CLIENTS || !remaining ) {
		return;
	}

	health = ( ( remaining >> 8 ) & 0xff ) - 1;
	if ( health < 0 ) {
		health = 0;
	}
	aimWoundHealth[target] = health;
	aimWoundTime[target] = cl.snap.serverTime;
}

static float CL_AimAssistWoundScore( int clientNum ) {
	int	age;

	if ( clientNum < 0 || clientNum >= MAX_CLIENTS || !aimWoundTime[clientNum] ) {
		return 0.0f;
	}

	age = cl.snap.serverTime - aimWoundTime[clientNum];
	if ( age < 0 || age > AIM_WOUND_MEMORY ) {
		return 0.0f;
	}

	// the less it had left the better, and the older the news the less it says
	return Com_Clamp( 0.0f, 1.0f, ( 100.0f - aimWoundHealth[clientNum] ) / 100.0f )
		* ( 1.0f - (float)age / AIM_WOUND_MEMORY );
}


/*
=================
CL_AimAssistAirborne

Whether a target is really in the air rather than in the middle of an ordinary
jump. A plain jump rises about forty-five units and is over in half a second;
anything much higher off the ground fell from somewhere or was thrown, and
holds its path all the way down - which is what makes it easy to hit.
=================
*/
static qboolean CL_AimAssistAirborne( const entityState_t *entity ) {
	vec3_t	mins, maxs, below;
	trace_t	trace;

	if ( entity->groundEntityNum != ENTITYNUM_NONE || CL_AimAssistFloats( entity ) ) {
		return qfalse;
	}

	CL_AimAssistHull( entity, mins, maxs );
	VectorCopy( entity->pos.trBase, below );
	below[2] -= 60.0f;
	CM_BoxTrace( &trace, entity->pos.trBase, below, mins, maxs, 0, MASK_PLAYERSOLID, qfalse );

	return trace.fraction >= 1.0f;
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
	vec3_t			targetOrigin, direction, desired;
	float			bestScore = -1.0f, score, angle, pitchDelta, yawDelta, distance;
	float			speed, scatter, weight[AIM_PRIO_COUNT];
	float			part[AIM_PRIO_COUNT], bestPart[AIM_PRIO_COUNT];
	char			others[768];
	int				i, k, targetTeam;
	qboolean		visible;

	others[0] = '\0';
	Com_Memset( bestPart, 0, sizeof( bestPart ) );

	CL_AimAssistPriorities();
	for ( i = 0; i < AIM_PRIO_COUNT; i++ ) {
		weight[i] = sticky ? aimPriorityWeight[i] : 0.0f;
	}
	if ( !sticky ) {
		weight[AIM_PRIO_CURSOR] = 1.0f;		// the plain crosshair pick for the record
		weight[AIM_PRIO_SIGHT] = 1.0f;
	}

	for ( i = 0; i < cl.snap.numEntities; i++ ) {
		entity = &cl.parseEntities[( cl.snap.parseEntitiesNum + i ) & ( MAX_PARSE_ENTITIES - 1 )];
		if ( entity->eType != ET_PLAYER || entity->clientNum == cl.snap.ps.clientNum ||
			 entity->clientNum < 0 || entity->clientNum >= MAX_CLIENTS ||
			 ( entity->eFlags & EF_DEAD ) || entity->number != entity->clientNum ) {
			continue;
		}

		info = cl.gameState.stringData + cl.gameState.stringOffsets[CS_PLAYERS + entity->clientNum];
		if ( !*Info_ValueForKey( info, "skill" ) ) {
			continue;	// human player
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

		// Out of sight is only out of the running while sight counts for
		// something; whoever turns it down asks for the one behind the wall.
		if ( !visible && weight[AIM_PRIO_SIGHT] >= 99.0f ) {
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

		part[AIM_PRIO_SIGHT] = weight[AIM_PRIO_SIGHT] * ( visible ? 1.0f : 0.0f );
		part[AIM_PRIO_CURSOR] = weight[AIM_PRIO_CURSOR] / ( 1.0f + angle / 15.0f );
		part[AIM_PRIO_NEAR] = weight[AIM_PRIO_NEAR] / ( 1.0f + distance / 500.0f );
		part[AIM_PRIO_WOUNDED] = weight[AIM_PRIO_WOUNDED] * CL_AimAssistWoundScore( entity->clientNum );
		part[AIM_PRIO_ATTACKER] = entity->clientNum == aimAttacker ? weight[AIM_PRIO_ATTACKER] : 0.0f;
		part[AIM_PRIO_KEEP] = entity->clientNum == aimAssistTarget ? weight[AIM_PRIO_KEEP] : 0.0f;
		part[AIM_PRIO_AIR] = CL_AimAssistAirborne( entity ) ? weight[AIM_PRIO_AIR] : 0.0f;
		part[AIM_PRIO_POWERUP] = ( entity->powerups & ( ( 1 << PW_QUAD ) | ( 1 << PW_REGEN )
			| ( 1 << PW_BATTLESUIT ) | ( 1 << PW_HASTE ) | ( 1 << PW_INVIS ) ) )
			? weight[AIM_PRIO_POWERUP] : 0.0f;

		// How well this weapon does at that range is measured, not guessed;
		// a shot that scatters wider than it reaches counts for little.
		speed = CL_AimAssistProjectileSpeed( weapon );
		scatter = speed > 0.0f ? CL_AimAssistScatter( weapon, distance / speed ) : -1.0f;
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
	if ( sticky ) {
		aimPickLast = best ? best->clientNum : -1;
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
CL_AimAssistVisible

Whether there is a clear shot at this target from here. Used to hold the
trigger when cl_aimAssistHoldFire says a shot into cover is not worth taking.
=================
*/
static qboolean CL_AimAssistVisible( const vec3_t viewOrigin, const entityState_t *entity ) {
	vec3_t	targetOrigin;
	trace_t	trace;

	VectorCopy( entity->pos.trBase, targetOrigin );
	targetOrigin[2] += CL_AimAssistBodyHeight( entity );
	CM_BoxTrace( &trace, viewOrigin, targetOrigin, vec3_origin, vec3_origin,
		0, MASK_SHOT, qfalse );

	return trace.fraction >= 1.0f;
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
=================
*/
static void CL_AimAssistLogShot( const entityState_t *entity, int weapon, const vec3_t viewOrigin,
		const vec3_t targetOrigin, float lead, float error, qboolean assisted, qboolean exact,
		qboolean fallback ) {
	const char	*info;
	vec3_t		direction, motion;

	info = cl.gameState.stringData + cl.gameState.stringOffsets[CS_PLAYERS + entity->clientNum];
	VectorSubtract( targetOrigin, viewOrigin, direction );
	CL_AimAssistVelocity( entity, motion );

	// "at" is where the aim was put, "plain" where the target really stood
	// and "vel" what it was doing - the applied lead is the difference. "me"
	// and "eye" let the impact lines be matched to the shooter.
	Com_Printf( "aim shot: %s target %s dist %.0f air %i lead %i trust %.2f error %.2f assist %i"
		" at %.0f %.0f %.0f plain %.0f %.0f %.0f vel %.0f %.0f %.0f eye %.0f %.0f %.0f speed %.0f me %i"
		" exact %i phase %i hold %.2f crouch %i tune %.2f scatter %.0f fall %i myair %i"
		" world %i frame %i\n",
		CL_AimAssistWeaponName( weapon ), Info_ValueForKey( info, "n" ),
		VectorLength( direction ),
		entity->groundEntityNum == ENTITYNUM_NONE ? 1 : 0,
		(int)( lead * 1000.0f ),
		CL_AimAssistTrust( entity, lead ),
		error, assisted ? 1 : 0,
		targetOrigin[0], targetOrigin[1], targetOrigin[2],
		entity->pos.trBase[0], entity->pos.trBase[1], entity->pos.trBase[2],
		motion[0], motion[1], motion[2],
		viewOrigin[0], viewOrigin[1], viewOrigin[2],
		VectorLength( cl.snap.ps.velocity ), cl.snap.ps.clientNum,
		exact ? 1 : 0, exact ? 0 : (int)( CL_AimAssistPhase() * 1000.0f ),
		CL_AimAssistHold(), CL_AimAssistCrouched( entity ) ? 1 : 0,
		CL_AimAssistTune( weapon, lead ), CL_AimAssistScatter( weapon, lead ),
		fallback ? 1 : 0, cl.snap.ps.groundEntityNum == ENTITYNUM_NONE ? 1 : 0,
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
	float	expected;		// what the prediction really aimed for, tuning included
	float	rate;			// its sideways speed after the trust of the moment
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
		const vec3_t aimed, qboolean exact ) {
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
	unclipped = speed * CL_AimAssistSideways( lead ) * trust * CL_AimAssistTune( weapon, lead );
	if ( !exact ) {
		unclipped += speed * CL_AimAssistPhase();
	}
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
	p->expected = speed * CL_AimAssistSideways( lead ) * trust * CL_AimAssistTune( weapon, lead );
	p->rate = speed * trust;
	p->lead = lead;
}

static void CL_AimAssistLearn( void ) {
	aimPending_t		*p;
	const entityState_t	*entity, *found;
	const char			*info;
	vec3_t				moved;
	float				actual, lateral, expected, error, weight, hold;
	int					i, j, band;

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
		hold = CL_AimAssistHold() * exp( 0.1f * error * weight );
		hold = Com_Clamp( 0.1f, 5.0f, hold );
		CL_AimAssistSetHold( hold );

		// and into the box for this weapon at this flight time, which is what
		// the prediction and the target choice really read
		CL_AimAssistTuneUpdate( p->weapon, p->lead, expected, actual, weight );
		band = CL_AimAssistBand( p->lead );
		aimLearned++;

		info = cl.gameState.stringData + cl.gameState.stringOffsets[CS_PLAYERS + p->target];
		Com_Printf( "aim learn: %s target %s ran %.0f of %.0f expected %.0f aside %.0f hold %.2f n %i frame %i\n",
			CL_AimAssistWeaponName( p->weapon ), Info_ValueForKey( info, "n" ),
			actual, p->straight, expected, lateral, hold, aimLearned, cl.snap.serverTime );
		Com_Printf( "aim tune: %s band %i from %.1f factor %.2f scatter %.0f reach %.0f n %i frame %i\n",
			CL_AimAssistWeaponName( p->weapon ), band, CL_AimAssistBandStart( band ),
			CL_AimAssistTune( p->weapon, p->lead ), CL_AimAssistScatter( p->weapon, p->lead ),
			CL_AimAssistHitRadius( p->weapon ), aimTune[p->weapon][band].samples,
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
	}
	aimWatchedTime = cl.snap.serverTime;

	CL_AimAssistWoundWatch();

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

	if ( !cl.snap.valid || clc.demoplaying || clc.netchan.remoteAddress.type != NA_LOOPBACK
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
			if ( *Info_ValueForKey( info, "skill" ) ) {
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
	float			pitchDelta, yawDelta, pitchStep, low, high, blend, lead, flight, k;
	int				i, key, localTeam, weapon;
	qboolean		aimKeyHasAttack, otherAttackKey, firing, steering, exact, plain, clear;

	if ( clc.state != CA_ACTIVE || clc.demoplaying || !cl.snap.valid ||
		 clc.netchan.remoteAddress.type != NA_LOOPBACK ||
		 cl.snap.ps.pm_type == PM_INTERMISSION || cl.snap.ps.pm_type == PM_DEAD ||
		 ( cl.snap.ps.pm_flags & PMF_FOLLOW ) ) {
		aimAssistTarget = -1;
		aimSmoothTarget = -1;
		return;
	}

	localTeam = cl.snap.ps.persistant[PERS_TEAM];
	if ( localTeam == TEAM_SPECTATOR ) {
		aimAssistTarget = -1;
		aimSmoothTarget = -1;
		return;
	}

	CL_AimAssistPhaseUpdate();

	key = Key_StringToKeynum( cl_aimAssistKey->string );
	steering = cl_aimAssist->integer && key >= 0 && Key_IsDown( key );

	CL_AimAssistEye( viewOrigin );
	weapon = cl.cgameUserCmdValue;
	if ( weapon <= WP_NONE || weapon >= WP_NUM_WEAPONS ) {
		weapon = cl.snap.ps.weapon;
	}

	if ( !steering ) {
		// no help this frame, but a shot is still worth a line for the record
		entity = cl_aimAssistDebug->integer
			? CL_AimAssistPickTarget( viewOrigin, localTeam, weapon, qfalse ) : NULL;
		firing = CL_AimAssistFiring( cmd, weapon, viewOrigin, entity );
		if ( firing && entity ) {
			CL_AimAssistTargetPoint( entity, viewOrigin, weapon, qtrue, targetOrigin, &lead );
			VectorSubtract( targetOrigin, viewOrigin, direction );
			vectoangles( direction, desired );
			desired[PITCH] -= SHORT2ANGLE( cl.snap.ps.delta_angles[PITCH] );
			desired[YAW] -= SHORT2ANGLE( cl.snap.ps.delta_angles[YAW] );
			pitchDelta = AngleNormalize180( desired[PITCH] - cl.viewangles[PITCH] );
			yawDelta = AngleNormalize180( desired[YAW] - cl.viewangles[YAW] );
			CL_AimAssistLogShot( entity, weapon, viewOrigin, targetOrigin, lead,
				sqrt( pitchDelta * pitchDelta + yawDelta * yawDelta ), qfalse, qtrue, qfalse );
		}
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
	// through, for anyone who asked for that.
	if ( cl_aimAssistHoldFire->integer && entity && !CL_AimAssistVisible( viewOrigin, entity ) ) {
		cmd->buttons &= ~BUTTON_ATTACK;
	}

	// the weapon timer runs whether or not there is anything to steer at
	firing = CL_AimAssistFiring( cmd, weapon, viewOrigin, entity );

	if ( !entity ) {
		CL_AimAssistSkip( 0, weapon );
		aimAssistTarget = -1;
		aimSmoothTarget = -1;
		return;
	}
	aimAssistTarget = entity->clientNum;

	exact = firing && ( cl_aimAssistExact->integer == 2
		|| ( cl_aimAssistExact->integer == 1 && CL_AimAssistSingleShot( weapon ) ) );

	CL_AimAssistTargetPoint( entity, viewOrigin, weapon, exact, targetOrigin, &lead );
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
	}

	VectorSubtract( targetOrigin, viewOrigin, direction );
	if ( CL_AimAssistProjectileSpeed( weapon ) > 0.0f && VectorLength( direction ) < 160.0f ) {
		CL_AimAssistSkip( 2, weapon );
		aimSmoothTarget = -1;
		return;			// inside our own splash, the player aims this one alone
	}
	CL_AimAssistSkip( -1, weapon );			// steering again
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
		// A shot at the plain body has no led point to learn from, but it
		// flies the same time, which the record needs to match its impact.
		CL_AimAssistRemember( entity, weapon, plain ? 0.0f : lead, targetOrigin, exact );
		if ( cl_aimAssistDebug->integer ) {
			CL_AimAssistLogShot( entity, weapon, viewOrigin, targetOrigin, flight,
				sqrt( ( pitchDelta - pitchStep ) * ( pitchDelta - pitchStep )
					+ yawDelta * yawDelta * ( 1.0f - blend ) * ( 1.0f - blend ) ),
				qtrue, exact && !plain, plain );
		}
	}
}


/*
=================
CL_AimAssist

Helps the hit-sound lab produce repeatable hits.  This deliberately does not
use sv_cheats: the safety boundary is the loopback connection itself, and the
only eligible targets are bots identified by the server's player configstring.

Two points are computed for the target. The smooth one moves the way the
picture moves and is what the view is steered towards between shots. The
exact one is where the server will really test the shot, and on the command
the shot fires on - which is predicted from the weapon timer - the view is put
on it outright, when cl_aimAssistExact says so. That is how the follow can be
smooth without the shot paying for it. cl_aimAssistSmooth adds a low pass on
top for anyone who wants the follow softer still.
=================
*/
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

	// send every frame for loopbacks
	if ( clc.netchan.remoteAddress.type == NA_LOOPBACK ) {
		return qtrue;
	}

	// send every frame for LAN
	if ( cl_lanForcePackets->integer && Sys_IsLANAddress( clc.netchan.remoteAddress ) ) {
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
