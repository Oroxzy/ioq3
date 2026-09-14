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


/*
=================
CL_AimAssistReach

How far a weapon is worth steering for. Measured in play: beyond these
distances the rocket and the shotgun stop hitting anything, the lightning gun
simply ends and the gauntlet has to touch. Zero means no limit.
=================
*/
static float CL_AimAssistReach( int weapon ) {
	switch ( weapon ) {
	case WP_GAUNTLET:
		return 64.0f;
	case WP_SHOTGUN:
		return 800.0f;
	case WP_GRENADE_LAUNCHER:
		return 700.0f;
	case WP_ROCKET_LAUNCHER:
		return 900.0f;
	case WP_LIGHTNING:
		return 768.0f;
	case WP_PLASMAGUN:
		return 1400.0f;
	case WP_BFG:
		return 2000.0f;
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
	return CL_AimAssistCrouched( entity ) ? 0.0f : 8.0f;
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
void CL_AimAssistFlush( void ) {
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
CL_AimAssistPredict

Where the target stands after the given time.  A player in the air is pulled
down by the same gravity the game uses, so carrying its upward speed on in a
straight line would aim high above a bot that merely jumped.  The box trace
keeps the guess out of the floor and out of walls the target cannot pass.
A negative time is a moment back along its line, wanted only for the smooth
picture between snapshots: nothing to clip and nothing to drop there.
=================
*/
static void CL_AimAssistPredict( const entityState_t *entity, float time, vec3_t predicted,
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
		sideways = CL_AimAssistSideways( time ) * CL_AimAssistTrust( entity, time );
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

		if ( predicted[2] < floor ) {
			predicted[2] = floor;					// landed, or walked up a step
			snapped = qtrue;
		} else if ( grounded && predicted[2] - floor <= STEPSIZE ) {
			predicted[2] = floor;					// walked down a step
			snapped = qtrue;
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
the miss changes sign. The loop leaves with the time that belongs to the aim
it hands out. A target out of throwing range keeps the aim rising; the cap
stops that.
=================
*/
static void CL_AimAssistArc( const vec3_t eye, const vec3_t impact, vec3_t aim, float *timeOut ) {
	vec3_t	muzzle, velocity;
	float	time, height, miss, lastMiss, gain;
	int		i;

	VectorCopy( impact, aim );
	time = 0.0f;
	lastMiss = 0.0f;
	gain = 1.0f;

	for ( i = 0; i < 16; i++ ) {
		time = CL_AimAssistLaunch( eye, aim, muzzle, velocity );
		if ( time <= 0.0f ) {
			break;			// straight up: no arc reaches out from here
		}

		height = muzzle[2] + velocity[2] * time - 0.5f * DEFAULT_GRAVITY * time * time;
		miss = impact[2] - height;
		if ( fabs( miss ) < 0.25f || i == 15 ) {
			break;
		}
		if ( i > 0 && ( miss > 0.0f ) != ( lastMiss > 0.0f ) ) {
			gain *= 0.5f;
		}
		lastMiss = miss;
		if ( aim[2] - impact[2] + miss * gain > 4096.0f ) {
			break;			// out of range
		}
		aim[2] += miss * gain;
	}

	if ( timeOut ) {
		*timeOut = time;
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
		CL_AimAssistPredict( entity, 0.0f, targetOrigin, &blocked, &pinned );
	} else if ( exact ) {
		// The frame the missile meets the target in is the first whose
		// segment reaches the target where it stands at the end of that
		// frame. A target walking into the shot is met a frame sooner than
		// its distance now says, one walking away a frame later; asking frame
		// by frame settles that where a fixed-point search can go round in
		// circles between two answers.
		for ( i = 1; i < 60; i++ ) {
			lead = i * frame;
			CL_AimAssistPredict( entity, lead, targetOrigin, &blocked, &pinned );
			flight = CL_AimAssistFlight( entity, weapon, viewOrigin, targetOrigin );
			if ( flight <= lead ) {
				break;
			}
		}
	} else {
		// between shots: the unrounded time, centred on the frames above,
		// settled by repeating distance over speed
		for ( i = 0; i < 5; i++ ) {
			CL_AimAssistPredict( entity, lead, targetOrigin, NULL, NULL );
			flight = CL_AimAssistFlight( entity, weapon, viewOrigin, targetOrigin );
			lead = Com_Clamp( 0.0f, 3.0f, flight + frame * 0.5f );
		}
		CL_AimAssistPredict( entity, lead, targetOrigin, &blocked, &pinned );
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


static int	aimAssistTarget = -1;		// who the assist steered at last frame
static int	aimAttacker = -1;		// the bot that last hurt us, if any

/*
=================
CL_AimAssistPickTarget

The bot to steer at: visible, an enemy, and by preference the one nearest the
crosshair - or, for a short weapon with cl_aimAssistPrefer, the nearest one
outright. The target we already had keeps a head start so the aim does not
hop between two bots running side by side, and whoever is hurting us comes
first when cl_aimAssistAttacker says so. With sticky off it is a plain pick,
used for the record of unassisted shots.
=================
*/
static entityState_t *CL_AimAssistPickTarget( const vec3_t viewOrigin, int localTeam,
		float reach, qboolean prefer, qboolean sticky ) {
	entityState_t	*entity, *best = NULL;
	const char		*info;
	trace_t			trace;
	vec3_t			targetOrigin, direction, desired;
	float			bestScore = 999999.0f, score, pitchDelta, yawDelta, distance;
	int				i, targetTeam;

	for ( i = 0; i < cl.snap.numEntities; i++ ) {
		entity = &cl.parseEntities[( cl.snap.parseEntitiesNum + i ) & ( MAX_PARSE_ENTITIES - 1 )];
		if ( entity->eType != ET_PLAYER || entity->clientNum == cl.snap.ps.clientNum ||
			 entity->clientNum < 0 || entity->clientNum >= MAX_CLIENTS ||
			 ( entity->eFlags & EF_DEAD ) ) {
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
		targetOrigin[2] += CL_AimAssistBodyHeight( entity );	// score the bot's current crosshair position
		CM_BoxTrace( &trace, viewOrigin, targetOrigin, vec3_origin, vec3_origin,
			0, MASK_SOLID, qfalse );
		if ( trace.fraction < 1.0f ) {
			continue;
		}

		VectorSubtract( targetOrigin, viewOrigin, direction );
		distance = VectorLength( direction );

		vectoangles( direction, desired );
		desired[PITCH] -= SHORT2ANGLE( cl.snap.ps.delta_angles[PITCH] );
		desired[YAW] -= SHORT2ANGLE( cl.snap.ps.delta_angles[YAW] );
		pitchDelta = AngleNormalize180( desired[PITCH] - cl.viewangles[PITCH] );
		yawDelta = AngleNormalize180( desired[YAW] - cl.viewangles[YAW] );

		if ( reach > 0.0f && prefer ) {
			// short weapon: the closest target first, the crosshair only
			// decides between two at the same range
			score = distance + sqrt( pitchDelta * pitchDelta + yawDelta * yawDelta );
		} else {
			score = pitchDelta * pitchDelta + yawDelta * yawDelta;
		}

		if ( sticky ) {
			if ( entity->clientNum == aimAssistTarget ) {
				score *= 0.6f;
			}

			// The server names whoever hurt us last in the player state, so
			// this needs nothing the client would not already know.
			if ( cl_aimAssistAttacker->integer
				&& entity->clientNum == aimAttacker ) {
				score = -1.0f;
			}
		}

		if ( score < bestScore ) {
			bestScore = score;
			best = entity;
		}
	}

	return best;
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
		const vec3_t targetOrigin, float lead, float error, qboolean assisted, qboolean exact ) {
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
		" exact %i phase %i hold %.2f crouch %i world %i frame %i\n",
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
	float	rate;			// its sideways speed after the trust of the moment
	float	lead;			// flight time, a whole number of frames
} aimPending_t;

static aimPending_t	aimPending[AIM_PENDING];
static int			aimPendingNum;
static int			aimLearned;		// shots learned from so far

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
		return;
	}

	CL_AimAssistVelocity( entity, motion );
	motion[2] = 0.0f;
	speed = VectorLength( motion );
	if ( speed * lead < 40.0f ) {
		return;		// too little motion to learn anything from
	}
	VectorScale( motion, 1.0f / speed, along );

	// A prediction that a wall or a ledge cut short never aimed at the run it
	// expected, so where the target got to says nothing about the hold time.
	trust = CL_AimAssistTrust( entity, lead );
	VectorSubtract( aimed, entity->pos.trBase, offset );
	offset[2] = 0.0f;
	used = DotProduct( offset, along );
	unclipped = speed * CL_AimAssistSideways( lead ) * trust;
	if ( !exact ) {
		unclipped += speed * CL_AimAssistPhase();
	}
	if ( used < unclipped - 2.0f ) {
		return;
	}

	// One open sample per target at a time. A stream of plasma at one bot
	// watches the same run over and over, and would step the hold time once
	// per bolt for what is one observation.
	for ( i = 0; i < AIM_PENDING; i++ ) {
		if ( aimPending[i].arrival && aimPending[i].target == entity->clientNum
			&& aimPending[i].arrival - cl.snap.serverTime <= 5000 ) {
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
	p->rate = speed * trust;
	p->lead = lead;
}

static void CL_AimAssistLearn( void ) {
	aimPending_t		*p;
	const entityState_t	*entity, *found;
	const char			*info;
	vec3_t				moved;
	float				actual, lateral, expected, error, weight, hold;
	int					i, j;

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
			continue;
		}

		VectorSubtract( found->pos.trBase, p->origin, moved );
		moved[2] = 0.0f;
		if ( VectorLength( moved ) > 1500.0f ) {
			continue;		// teleported, nothing to learn from
		}

		actual = DotProduct( moved, p->along );
		lateral = sqrt( fabs( DotProduct( moved, moved ) - actual * actual ) );

		// How far short of or beyond the expectation the target got, as a share
		// of the straight run, moves the hold time by a bounded step; a target
		// that mostly went sideways counts for less. The step is small, so the
		// value settles on how the bots behave rather than on the last one's
		// last turn.
		expected = p->rate * CL_AimAssistSideways( p->lead );
		error = Com_Clamp( -1.0f, 1.0f, ( actual - expected ) / p->straight );
		weight = p->straight / ( p->straight + lateral );
		hold = CL_AimAssistHold() * exp( 0.1f * error * weight );
		hold = Com_Clamp( 0.1f, 5.0f, hold );
		CL_AimAssistSetHold( hold );
		aimLearned++;

		info = cl.gameState.stringData + cl.gameState.stringOffsets[CS_PLAYERS + p->target];
		Com_Printf( "aim learn: %s target %s ran %.0f of %.0f expected %.0f aside %.0f hold %.2f n %i frame %i\n",
			CL_AimAssistWeaponName( p->weapon ), Info_ValueForKey( info, "n" ),
			actual, p->straight, expected, lateral, hold, aimLearned, cl.snap.serverTime );
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

	// Whoever hurt us last. The player state names the attacker, but it names
	// client zero before anyone has, and it carries the last life's killer
	// into the next; so a new life - a respawn or a map restart, both of which
	// the spawn count shows - starts without a grudge and only syncs the
	// counters. Within a life a hit is believed when something moved: the pain
	// counter, which the game steps at most once in 700 ms; the damage count,
	// which it rewrites on every damaged frame; or the attacker itself, which
	// it rewrites on every hit.
	if ( ps->persistant[PERS_SPAWN_COUNT] != aimSpawnCount ) {
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
	float			pitchDelta, yawDelta, pitchStep, low, high, blend, lead, flight, reach, k;
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
	reach = CL_AimAssistReach( weapon );

	if ( !steering ) {
		// no help this frame, but a shot is still worth a line for the record
		entity = cl_aimAssistDebug->integer
			? CL_AimAssistPickTarget( viewOrigin, localTeam, 0.0f, qfalse, qfalse ) : NULL;
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
				sqrt( pitchDelta * pitchDelta + yawDelta * yawDelta ), qfalse, qtrue );
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

	entity = CL_AimAssistPickTarget( viewOrigin, localTeam, reach,
		cl_aimAssistPrefer->integer != 0, qtrue );

	// the weapon timer runs whether or not there is anything to steer at
	firing = CL_AimAssistFiring( cmd, weapon, viewOrigin, entity );

	if ( !entity ) {
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
			aimSmoothTarget = -1;
			return;
		}
	}

	VectorSubtract( targetOrigin, viewOrigin, direction );
	if ( CL_AimAssistProjectileSpeed( weapon ) > 0.0f && VectorLength( direction ) < 160.0f ) {
		aimSmoothTarget = -1;
		return;			// inside our own splash, the player aims this one alone
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
		// A shot at the plain body has no led point to learn from, but it
		// flies the same time, which the record needs to match its impact.
		CL_AimAssistRemember( entity, weapon, plain ? 0.0f : lead, targetOrigin, exact );
		if ( cl_aimAssistDebug->integer ) {
			CL_AimAssistLogShot( entity, weapon, viewOrigin, targetOrigin, flight,
				sqrt( ( pitchDelta - pitchStep ) * ( pitchDelta - pitchStep )
					+ yawDelta * yawDelta * ( 1.0f - blend ) * ( 1.0f - blend ) ),
				qtrue, exact && !plain );
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
