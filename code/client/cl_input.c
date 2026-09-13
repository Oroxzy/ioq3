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
CL_AimAssistLag

How far behind the world the last snapshot is: its own age plus the way the
command being built still has to travel to the server.
=================
*/
static float CL_AimAssistLag( void ) {
	float	lag;

	// cl.serverTime runs behind the newest snapshot, not ahead of it: the client
	// renders between the last two snapshots. The shot, however, is fired on the
	// server after that snapshot was sent, so the target has moved on by the
	// render lag plus the way the command still has to travel.
	lag = ( cl.snap.serverTime - cl.serverTime + cl.snap.ping / 2 ) * 0.001f;

	return Com_Clamp( 0.0f, 0.3f, lag );
}


/*
=================
CL_AimAssistPredict

Where the target stands after the given time.  A player in the air is pulled
down by the same gravity the game uses, so carrying its upward speed on in a
straight line would aim high above a bot that merely jumped.  The box trace
keeps the guess out of the floor and out of walls the target cannot pass.
=================
*/
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
	vec3_t				change;
	float				speed, spread, interval;
	int					i;

	// A target in the air cannot change where it is going, and the falling part
	// is modelled, so its course is the surest one there is.
	if ( entity->groundEntityNum == ENTITYNUM_NONE ) {
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

		// only sideways counts: up and down is the gravity above
		VectorSubtract( entity->pos.trDelta, old->pos.trDelta, change );
		change[2] = 0.0f;
		speed = sqrt( entity->pos.trDelta[0] * entity->pos.trDelta[0]
			+ entity->pos.trDelta[1] * entity->pos.trDelta[1] );
		if ( speed < 1.0f ) {
			return 1.0f;
		}

		// how far the course would wander over the whole lead at the rate it
		// is wandering right now
		spread = VectorLength( change ) / speed * ( time / interval );

		return 1.0f / ( 1.0f + spread );
	}

	return 1.0f;
}


static void CL_AimAssistPredict( const entityState_t *entity, float time, vec3_t predicted ) {
	// The game lifts a walking player over anything up to STEPSIZE, so the box
	// that clips the guess starts above that height: a curb, a stair riser or a
	// ramp is no obstacle to the target and must not cut its lead short. Only
	// what would stop the target itself may stop the prediction.
	static vec3_t	stepMins = { -15, -15, -24 + STEPSIZE };
	static vec3_t	groundMins = { -15, -15, -24 };
	static vec3_t	maxs = { 15, 15, 32 };
	vec3_t			end, velocity, motion, above, below;
	float			gravity, trust, floor;
	trace_t			trace;

	// Only the sideways guess is damped. Falling is physics and stays whole.
	CL_AimAssistVelocity( entity, motion );
	trust = CL_AimAssistTrust( entity, time );
	VectorScale( motion, trust, velocity );
	velocity[2] = motion[2];
	VectorMA( entity->pos.trBase, time, velocity, end );

	// Gravity acts on anything off the floor, and trDelta[2] carries the rest:
	// a ramp, a jump pad, the first moment of a jump. Pinning the height threw
	// all of that away.
	if ( entity->groundEntityNum == ENTITYNUM_NONE ) {
		gravity = cl.snap.ps.gravity > 0 ? cl.snap.ps.gravity : DEFAULT_GRAVITY;
		end[2] -= 0.5f * gravity * time * time;
	}

	CM_BoxTrace( &trace, entity->pos.trBase, end, stepMins, maxs, 0, MASK_PLAYERSOLID, qfalse );

	// A solid start says nothing about where the target can go, and taking the
	// trace at its word there would throw the whole lead away.
	if ( trace.startsolid || trace.allsolid ) {
		VectorCopy( end, predicted );
	} else {
		VectorCopy( trace.endpos, predicted );
	}

	// Put the guess back on the ground it would be standing on: a target that
	// runs up stairs rises with them, one that lands does not sink into the
	// floor, and one high in the air finds nothing here and keeps its arc.
	VectorCopy( predicted, above );
	above[2] += STEPSIZE;
	VectorCopy( predicted, below );
	below[2] -= 8192.0f;
	CM_BoxTrace( &trace, above, below, groundMins, maxs, 0, MASK_PLAYERSOLID, qfalse );

	if ( !trace.startsolid && !trace.allsolid && trace.fraction < 1.0f ) {
		floor = trace.endpos[2];

		if ( predicted[2] < floor ) {
			predicted[2] = floor;					// landed, or walked up a step
		} else if ( entity->groundEntityNum != ENTITYNUM_NONE
			&& predicted[2] - floor <= STEPSIZE ) {
			predicted[2] = floor;					// walked down a step
		}
	}
}


/*
=================
CL_AimAssistTargetPoint

Predicts where the target will be when the shot reaches it.  Even a hitscan
weapon has to lead a little: the snapshot the position comes from is already
a frame old, and the command being built still has to travel to the server.
A projectile adds its own flight time on top, which is solved by repeating
the distance over speed until it settles.
=================
*/
static void CL_AimAssistTargetPoint( const entityState_t *entity,
		const vec3_t viewOrigin, vec3_t targetOrigin, float *leadOut ) {
	vec3_t	offset;
	float	projectileSpeed, travelTime, lag;
	int		i, weapon;

	lag = CL_AimAssistLag();

	weapon = cl.cgameUserCmdValue;
	if ( weapon <= WP_NONE || weapon >= WP_NUM_WEAPONS ) {
		weapon = cl.snap.ps.weapon;
	}
	projectileSpeed = CL_AimAssistProjectileSpeed( weapon );

	travelTime = 0.0f;
	for ( i = 0; i < 5; i++ ) {
		CL_AimAssistPredict( entity, lag + travelTime, targetOrigin );

		// A splash weapon at a target on the floor goes for the feet: a near
		// miss still bursts on the ground under it, where a miss past the body
		// would fly on. In the air only the body itself can be hit.
		if ( ( weapon == WP_ROCKET_LAUNCHER || weapon == WP_GRENADE_LAUNCHER || weapon == WP_BFG )
			&& entity->groundEntityNum != ENTITYNUM_NONE ) {
			targetOrigin[2] -= 20.0f;
		} else {
			targetOrigin[2] += 8.0f;	// inside the box standing and crouched alike
		}

		// Grenades and proximity mines follow TR_GRAVITY.  Raise the aim point
		// by their drop during the calculated flight time - less the lift the
		// game gives them for free, a fifth of the forward vector tipped up
		// (g_weapon.c), which lifts the arc by that share of the range.
		if ( weapon == WP_GRENADE_LAUNCHER
#ifdef MISSIONPACK
			 || weapon == WP_PROX_LAUNCHER
#endif
		) {
			VectorSubtract( targetOrigin, viewOrigin, offset );
			targetOrigin[2] += 0.5f * DEFAULT_GRAVITY * travelTime * travelTime
				- 0.2f * sqrt( offset[0] * offset[0] + offset[1] * offset[1] );
		}

		if ( leadOut ) {
			*leadOut = lag + travelTime;
		}

		if ( projectileSpeed <= 0.0f ) {
			return;			// hitscan, the lag above is the whole lead
		}

		VectorSubtract( targetOrigin, viewOrigin, offset );
		// The shot does not start at the eye and does not start at rest: the
		// muzzle sits 14 units ahead, and every missile is spawned a frame of
		// flight early. Both make it arrive sooner than the plain distance says.
		travelTime = ( VectorLength( offset ) - 14.0f ) / projectileSpeed - MISSILE_PRESTEP;
		travelTime = Com_Clamp( 0.0f, 1.5f, travelTime );
	}
}


static int	aimAssistTarget = -1;		// who the assist steered at last frame

/*
=================
CL_AimAssistPickTarget

The bot to steer at: visible, an enemy, within the weapon's reach, and by
preference the one nearest the crosshair - or, for a short weapon with
cl_aimAssistPrefer, the nearest one outright. The target we already had
keeps a head start so the aim does not hop between two bots running side by
side, and whoever is hurting us comes first when cl_aimAssistAttacker says so.
With sticky off it is a plain pick, used for the record of unassisted shots.
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
		targetOrigin[2] += 8.0f;	// score the bot's current crosshair position
		CM_BoxTrace( &trace, viewOrigin, targetOrigin, vec3_origin, vec3_origin,
			0, MASK_SOLID, qfalse );
		if ( trace.fraction < 1.0f ) {
			continue;
		}

		VectorSubtract( targetOrigin, viewOrigin, direction );
		distance = VectorLength( direction );

		// A weapon that cannot reach the target has no business steering
		// towards it: a rocket at fourteen hundred units hits nothing, and
		// pulling the aim there only costs the shot at whoever is close.
		if ( reach > 0.0f && distance > reach ) {
			continue;
		}

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
				&& entity->clientNum == cl.snap.ps.persistant[PERS_ATTACKER] ) {
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
CL_AimAssistLogShot

One line per trigger pull, so the test bench can tell which shots the
prediction got right. What is left of the two deltas after the blend is how
far the view still misses the predicted point. Unassisted pulls are written
too, against the bot nearest the crosshair, which gives the bench a rate to
hold the assisted one against.
=================
*/
static void CL_AimAssistLogShot( const entityState_t *entity, int weapon, const vec3_t viewOrigin,
		const vec3_t targetOrigin, float lead, float error, qboolean assisted ) {
	const char	*info;
	vec3_t		direction, motion;

	info = cl.gameState.stringData + cl.gameState.stringOffsets[CS_PLAYERS + entity->clientNum];
	VectorSubtract( targetOrigin, viewOrigin, direction );
	CL_AimAssistVelocity( entity, motion );

	// "at" is where the aim was put, "plain" where the target really stood
	// and "vel" what it was doing - the applied lead is the difference. "me"
	// and "eye" let the impact lines be matched to the shooter.
	Com_Printf( "aim shot: %s target %s dist %.0f air %i lead %i trust %.2f error %.2f assist %i"
		" at %.0f %.0f %.0f plain %.0f %.0f %.0f vel %.0f %.0f %.0f eye %.0f %.0f %.0f speed %.0f me %i frame %i\n",
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
		VectorLength( cl.snap.ps.velocity ), cl.snap.ps.clientNum, cl.serverTime );
}


/*
=================
CL_AimAssistSnapshot

Called once for every snapshot that arrives. Writes down where shots really
ended up, which is the one thing the shot line cannot know: every impact
event with its position and whoever it belongs to, every missile the moment
it first appears (its entity number ties the launch to the explosion later),
and with each impact where every bot stood at that moment - so a miss can be
measured in units and in direction, not only counted.
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
	int					i, j, event, kind, present;

	if ( !cl_aimAssistDebug->integer || !cl.snap.valid || clc.demoplaying
		|| clc.netchan.remoteAddress.type != NA_LOOPBACK || cl.snap.messageNum == lastMessage ) {
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

		Com_Printf( "aim impact: %s num %i other %i client %i at %.0f %.0f %.0f frame %i%s\n",
			names[kind], entity->number, entity->otherEntityNum, entity->clientNum,
			entity->pos.trBase[0], entity->pos.trBase[1], entity->pos.trBase[2],
			cl.snap.serverTime, bots );
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


/*
=================
CL_AimAssist

Helps the hit-sound lab produce repeatable hits.  This deliberately does not
use sv_cheats: the safety boundary is the loopback connection itself, and the
only eligible targets are bots identified by the server's player configstring.
=================
*/
static void CL_AimAssist( usercmd_t *cmd ) {
	static int		aimAssistButtons;	// buttons of the previous command, to spot a trigger pull
	int				previousButtons;
	entityState_t	*entity;
	trace_t			trace;
	vec3_t			viewOrigin, targetOrigin, direction, desired;
	float			pitchDelta, yawDelta, blend, lead, reach;
	int				i, key, localTeam, weapon;
	qboolean		aimKeyHasAttack, otherAttackKey, pulled, steering;

	// Remember what the trigger did on every frame, not only on the frames that
	// get as far as steering: otherwise the next shot after a missing target
	// looks like the button was already down and goes unlogged.
	previousButtons = aimAssistButtons;
	aimAssistButtons = cmd->buttons;
	pulled = ( cmd->buttons & BUTTON_ATTACK ) && !( previousButtons & BUTTON_ATTACK );

	if ( clc.state != CA_ACTIVE || clc.demoplaying ||
		 clc.netchan.remoteAddress.type != NA_LOOPBACK ||
		 cl.snap.ps.pm_type == PM_INTERMISSION ||
		 ( cl.snap.ps.pm_flags & PMF_FOLLOW ) ) {
		return;
	}

	localTeam = cl.snap.ps.persistant[PERS_TEAM];
	if ( localTeam == TEAM_SPECTATOR ) {
		return;
	}

	key = Key_StringToKeynum( cl_aimAssistKey->string );
	steering = cl_aimAssist->integer && key >= 0 && Key_IsDown( key );

	// The shooter has moved on since this snapshot too, so carry the eye
	// forward as well; a strafing player would otherwise aim from beside
	// the muzzle the server ends up firing from.
	VectorMA( cl.snap.ps.origin, CL_AimAssistLag(), cl.snap.ps.velocity, viewOrigin );
	viewOrigin[2] += cl.snap.ps.viewheight;
	weapon = cl.cgameUserCmdValue;
	if ( weapon <= WP_NONE || weapon >= WP_NUM_WEAPONS ) {
		weapon = cl.snap.ps.weapon;
	}
	reach = CL_AimAssistReach( weapon );

	if ( !steering ) {
		// no help this frame, but a pull is still worth a line for the record
		if ( pulled && cl_aimAssistDebug->integer ) {
			entity = CL_AimAssistPickTarget( viewOrigin, localTeam, 0.0f, qfalse, qfalse );
			if ( entity ) {
				lead = 0.0f;
				CL_AimAssistTargetPoint( entity, viewOrigin, targetOrigin, &lead );
				VectorSubtract( targetOrigin, viewOrigin, direction );
				vectoangles( direction, desired );
				desired[PITCH] -= SHORT2ANGLE( cl.snap.ps.delta_angles[PITCH] );
				desired[YAW] -= SHORT2ANGLE( cl.snap.ps.delta_angles[YAW] );
				pitchDelta = AngleNormalize180( desired[PITCH] - cl.viewangles[PITCH] );
				yawDelta = AngleNormalize180( desired[YAW] - cl.viewangles[YAW] );
				CL_AimAssistLogShot( entity, weapon, viewOrigin, targetOrigin, lead,
					sqrt( pitchDelta * pitchDelta + yawDelta * yawDelta ), qfalse );
			}
		}
		aimAssistTarget = -1;
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
		pulled = qfalse;
	}

	entity = CL_AimAssistPickTarget( viewOrigin, localTeam, reach,
		cl_aimAssistPrefer->integer != 0, qtrue );
	if ( !entity ) {
		aimAssistTarget = -1;
		return;
	}
	aimAssistTarget = entity->clientNum;

	lead = 0.0f;
	CL_AimAssistTargetPoint( entity, viewOrigin, targetOrigin, &lead );

	// The shot has to be able to get there. A led point can end up behind a
	// corner or below an edge, and steering a rocket into the floor in front
	// of you is worse than not helping at all: fall back to the plain position
	// the target was picked by, and leave the aim alone when even that is
	// blocked or the impact would land in your own splash.
	CM_BoxTrace( &trace, viewOrigin, targetOrigin, vec3_origin, vec3_origin,
		0, MASK_SHOT, qfalse );
	if ( trace.fraction < 1.0f ) {
		VectorCopy( entity->pos.trBase, targetOrigin );
		targetOrigin[2] += 8.0f;
		lead = 0.0f;
		CM_BoxTrace( &trace, viewOrigin, targetOrigin, vec3_origin, vec3_origin,
			0, MASK_SHOT, qfalse );
		if ( trace.fraction < 1.0f ) {
			return;
		}
	}

	VectorSubtract( targetOrigin, viewOrigin, direction );
	if ( CL_AimAssistProjectileSpeed( weapon ) > 0.0f && VectorLength( direction ) < 160.0f ) {
		return;			// inside our own splash, the player aims this one alone
	}
	vectoangles( direction, desired );
	desired[PITCH] -= SHORT2ANGLE( cl.snap.ps.delta_angles[PITCH] );
	desired[YAW] -= SHORT2ANGLE( cl.snap.ps.delta_angles[YAW] );

	// Level 10 snaps directly onto the closest crosshair target.  Lower
	// levels retain the same target choice but ease towards it.
	// The one hurting us is snapped onto at once, that is the point of it
	blend = ( cl_aimAssist->integer == 10
		|| ( cl_aimAssistAttacker->integer
			&& entity->clientNum == cl.snap.ps.persistant[PERS_ATTACKER] ) )
		? 1.0f : cl_aimAssist->value * frame_msec / 200.0f;
	blend = Com_Clamp( 0.0f, 1.0f, blend );
	pitchDelta = AngleNormalize180( desired[PITCH] - cl.viewangles[PITCH] );
	yawDelta = AngleNormalize180( desired[YAW] - cl.viewangles[YAW] );
	cl.viewangles[PITCH] += pitchDelta * blend;
	cl.viewangles[YAW] += yawDelta * blend;

	if ( pulled && cl_aimAssistDebug->integer ) {
		CL_AimAssistLogShot( entity, weapon, viewOrigin, targetOrigin, lead,
			sqrt( pitchDelta * pitchDelta + yawDelta * yawDelta ) * ( 1.0f - blend ), qtrue );
	}
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
	CL_AimAssist( &cmd );

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
