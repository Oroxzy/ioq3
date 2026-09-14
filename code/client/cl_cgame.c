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
// cl_cgame.c  -- client system interaction with client game

#include "client.h"

#include "../botlib/botlib.h"

#ifdef USE_MUMBLE
#include "libmumblelink.h"
#endif

extern	botlib_export_t	*botlib_export;

extern qboolean loadCamera(const char *name);
extern void startCamera(int time);
extern qboolean getCameraInfo(int time, vec3_t *origin, vec3_t *angles);

// The hit sound is pitched by the health and armor the target has left,
// see cl_hitPitch. This follows the baseq3 playerState_t conventions.
#define HIT_SOUND			"sound/feedback/hit.wav"
#define HIT_SOUND_QC		"sound/feedback/hit_qc.wav"	// from zz-hitsound-qc.pk3

static sfxHandle_t	hitSound = -1;		// -1 until the cgame registers HIT_SOUND
static sfxHandle_t	customHitSound = -1;	// -1 until the chosen file is registered
static char			customHitSoundFile[MAX_QPATH];	// what customHitSound was registered from
static int			cgameSnapshotNum;	// newest snapshot the cgame has read
static int			hitsSounded = -1;	// hit counter the last played hit sound belongs to

// The wire box is drawn on a shell close around the eye, which keeps it in
// front of the level without touching the depth buffer. The width is in the
// same near space, so it stays the same thickness on screen at any range.
#define BOT_OUTLINE_SHELL	40.0f
#define BOT_OUTLINE_WIDTH	0.07f

static refdef_t		botOutlineView;		// the view the cgame rendered last
static qboolean		botOutlineViewValid;
static qhandle_t	botOutlineShader;

static void CL_AddBotOutlines( void );
static void CL_AddItemOutlines( void );

// Items are made invisible when taken and come back after a fixed wait, so the
// moment one goes away is enough to count it down.  The waits are the ones in
// g_items.c; a weapon uses g_weaponrespawn, which is five seconds by default.
#define ITEM_RESPAWN_WEAPON		5
#define ITEM_RESPAWN_POWERUP	120
#define ITEM_RESPAWN_ARMOR		25
#define ITEM_RESPAWN_HEALTH		35
#define ITEM_RESPAWN_AMMO		40
#define ITEM_RESPAWN_HOLDABLE	60

typedef struct {
	int		taken;			// server time the item went away, 0 while it is there
	int		present;		// the last frame the snapshot still had it
	vec3_t	origin;			// where it lies, remembered for while it is gone
	int		respawn;		// seconds it stays away
	int		item;			// which entry of the item list it is, for its colour
	float	x, y;			// where its label goes on the screen
	int		labelFrame;		// the frame that label was worked out for
	char	label[8];
} itemTimer_t;

static itemTimer_t	itemTimers[MAX_GENTITIES];
static int			itemLabels;
static int			itemFrame;		// counts the drawn frames, to spot a stale label

/*
====================
CL_GetGameState
====================
*/
void CL_GetGameState( gameState_t *gs ) {
	*gs = cl.gameState;
}

/*
====================
CL_GetGlconfig
====================
*/
void CL_GetGlconfig( glconfig_t *glconfig ) {
	*glconfig = cls.glconfig;
}


/*
====================
CL_GetUserCmd
====================
*/
qboolean CL_GetUserCmd( int cmdNumber, usercmd_t *ucmd ) {
	// cmds[cmdNumber] is the last properly generated command

	// can't return anything that we haven't created yet
	if ( cmdNumber > cl.cmdNumber ) {
		Com_Error( ERR_DROP, "CL_GetUserCmd: %i >= %i", cmdNumber, cl.cmdNumber );
	}

	// the usercmd has been overwritten in the wrapping
	// buffer because it is too far out of date
	if ( cmdNumber <= cl.cmdNumber - CMD_BACKUP ) {
		return qfalse;
	}

	*ucmd = cl.cmds[ cmdNumber & CMD_MASK ];

	return qtrue;
}

int CL_GetCurrentCmdNumber( void ) {
	return cl.cmdNumber;
}


/*
====================
CL_GetParseEntityState
====================
*/
qboolean	CL_GetParseEntityState( int parseEntityNumber, entityState_t *state ) {
	// can't return anything that hasn't been parsed yet
	if ( parseEntityNumber >= cl.parseEntitiesNum ) {
		Com_Error( ERR_DROP, "CL_GetParseEntityState: %i >= %i",
			parseEntityNumber, cl.parseEntitiesNum );
	}

	// can't return anything that has been overwritten in the circular buffer
	if ( parseEntityNumber <= cl.parseEntitiesNum - MAX_PARSE_ENTITIES ) {
		return qfalse;
	}

	*state = cl.parseEntities[ parseEntityNumber & ( MAX_PARSE_ENTITIES - 1 ) ];
	return qtrue;
}

/*
====================
CL_GetCurrentSnapshotNumber
====================
*/
void	CL_GetCurrentSnapshotNumber( int *snapshotNumber, int *serverTime ) {
	*snapshotNumber = cl.snap.messageNum;
	*serverTime = cl.snap.serverTime;
}

/*
====================
CL_GetSnapshot
====================
*/
qboolean	CL_GetSnapshot( int snapshotNumber, snapshot_t *snapshot ) {
	clSnapshot_t	*clSnap;
	int				i, count;

	if ( snapshotNumber > cl.snap.messageNum ) {
		Com_Error( ERR_DROP, "CL_GetSnapshot: snapshotNumber > cl.snapshot.messageNum" );
	}

	// if the frame has fallen out of the circular buffer, we can't return it
	if ( cl.snap.messageNum - snapshotNumber >= PACKET_BACKUP ) {
		return qfalse;
	}

	// if the frame is not valid, we can't return it
	clSnap = &cl.snapshots[snapshotNumber & PACKET_MASK];
	if ( !clSnap->valid ) {
		return qfalse;
	}

	// if the entities in the frame have fallen out of their
	// circular buffer, we can't return it
	if ( cl.parseEntitiesNum - clSnap->parseEntitiesNum >= MAX_PARSE_ENTITIES ) {
		return qfalse;
	}

	if ( snapshotNumber > cgameSnapshotNum ) {
		cgameSnapshotNum = snapshotNumber;
	}

	// write the snapshot
	snapshot->snapFlags = clSnap->snapFlags;
	snapshot->serverCommandSequence = clSnap->serverCommandNum;
	snapshot->ping = clSnap->ping;
	snapshot->serverTime = clSnap->serverTime;
	Com_Memcpy( snapshot->areamask, clSnap->areamask, sizeof( snapshot->areamask ) );
	snapshot->ps = clSnap->ps;
	count = clSnap->numEntities;
	if ( count > MAX_ENTITIES_IN_SNAPSHOT ) {
		Com_DPrintf( "CL_GetSnapshot: truncated %i entities to %i\n", count, MAX_ENTITIES_IN_SNAPSHOT );
		count = MAX_ENTITIES_IN_SNAPSHOT;
	}
	snapshot->numEntities = count;
	for ( i = 0 ; i < count ; i++ ) {
		snapshot->entities[i] = 
			cl.parseEntities[ ( clSnap->parseEntitiesNum + i ) & (MAX_PARSE_ENTITIES-1) ];
	}

	// FIXME: configstring changes and server commands!!!

	return qtrue;
}

/*
=====================
CL_SetUserCmdValue
=====================
*/
void CL_SetUserCmdValue( int userCmdValue, float sensitivityScale ) {
	cl.cgameUserCmdValue = userCmdValue;
	cl.cgameSensitivity = sensitivityScale;
}

/*
=====================
CL_AddCgameCommand
=====================
*/
void CL_AddCgameCommand( const char *cmdName ) {
	Cmd_AddCommand( cmdName, NULL );
}


/*
=====================
CL_ConfigstringModified
=====================
*/
void CL_ConfigstringModified( void ) {
	char		*old, *s;
	int			i, index;
	char		*dup;
	gameState_t	oldGs;
	int			len;

	index = atoi( Cmd_Argv(1) );
	if ( index < 0 || index >= MAX_CONFIGSTRINGS ) {
		Com_Error( ERR_DROP, "CL_ConfigstringModified: bad index %i", index );
	}
	// get everything after "cs <num>"
	s = Cmd_ArgsFrom(2);

	old = cl.gameState.stringData + cl.gameState.stringOffsets[ index ];
	if ( !strcmp( old, s ) ) {
		return;		// unchanged
	}

	// build the new gameState_t
	oldGs = cl.gameState;

	Com_Memset( &cl.gameState, 0, sizeof( cl.gameState ) );

	// leave the first 0 for uninitialized strings
	cl.gameState.dataCount = 1;
		
	for ( i = 0 ; i < MAX_CONFIGSTRINGS ; i++ ) {
		if ( i == index ) {
			dup = s;
		} else {
			dup = oldGs.stringData + oldGs.stringOffsets[ i ];
		}
		if ( !dup[0] ) {
			continue;		// leave with the default empty string
		}

		len = strlen( dup );

		if ( len + 1 + cl.gameState.dataCount > MAX_GAMESTATE_CHARS ) {
			Com_Error( ERR_DROP, "MAX_GAMESTATE_CHARS exceeded" );
		}

		// append it to the gameState string buffer
		cl.gameState.stringOffsets[ i ] = cl.gameState.dataCount;
		Com_Memcpy( cl.gameState.stringData + cl.gameState.dataCount, dup, len + 1 );
		cl.gameState.dataCount += len + 1;
	}

	if ( index == CS_SYSTEMINFO ) {
		// parse serverId and other cvars
		CL_SystemInfoChanged();
	}

}


/*
===================
CL_GetServerCommand

Set up argc/argv for the given command
===================
*/
qboolean CL_GetServerCommand( int serverCommandNumber ) {
	char	*s;
	char	*cmd;
	static char bigConfigString[BIG_INFO_STRING];
	int argc;

	// if we have irretrievably lost a reliable command, drop the connection
	if ( serverCommandNumber <= clc.serverCommandSequence - MAX_RELIABLE_COMMANDS ) {
		// when a demo record was started after the client got a whole bunch of
		// reliable commands then the client never got those first reliable commands
		if ( clc.demoplaying )
			return qfalse;
		Com_Error( ERR_DROP, "CL_GetServerCommand: a reliable command was cycled out" );
		return qfalse;
	}

	if ( serverCommandNumber > clc.serverCommandSequence ) {
		Com_Error( ERR_DROP, "CL_GetServerCommand: requested a command not received" );
		return qfalse;
	}

	s = clc.serverCommands[ serverCommandNumber & ( MAX_RELIABLE_COMMANDS - 1 ) ];
	clc.lastExecutedServerCommand = serverCommandNumber;

	Com_DPrintf( "serverCommand: %i : %s\n", serverCommandNumber, s );

rescan:
	Cmd_TokenizeString( s );
	cmd = Cmd_Argv(0);
	argc = Cmd_Argc();

	if ( !strcmp( cmd, "disconnect" ) ) {
		// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=552
		// allow server to indicate why they were disconnected
		if ( argc >= 2 )
			Com_Error( ERR_SERVERDISCONNECT, "Server disconnected - %s", Cmd_Argv( 1 ) );
		else
			Com_Error( ERR_SERVERDISCONNECT, "Server disconnected" );
	}

	if ( !strcmp( cmd, "bcs0" ) ) {
		Com_sprintf( bigConfigString, BIG_INFO_STRING, "cs %s \"%s", Cmd_Argv(1), Cmd_Argv(2) );
		return qfalse;
	}

	if ( !strcmp( cmd, "bcs1" ) ) {
		s = Cmd_Argv(2);
		if( strlen(bigConfigString) + strlen(s) >= BIG_INFO_STRING ) {
			Com_Error( ERR_DROP, "bcs exceeded BIG_INFO_STRING" );
		}
		strcat( bigConfigString, s );
		return qfalse;
	}

	if ( !strcmp( cmd, "bcs2" ) ) {
		s = Cmd_Argv(2);
		if( strlen(bigConfigString) + strlen(s) + 1 >= BIG_INFO_STRING ) {
			Com_Error( ERR_DROP, "bcs exceeded BIG_INFO_STRING" );
		}
		strcat( bigConfigString, s );
		strcat( bigConfigString, "\"" );
		s = bigConfigString;
		goto rescan;
	}

	if ( !strcmp( cmd, "cs" ) ) {
		CL_ConfigstringModified();
		// reparse the string, because CL_ConfigstringModified may have done another Cmd_TokenizeString()
		Cmd_TokenizeString( s );
		return qtrue;
	}

	if ( !strcmp( cmd, "map_restart" ) ) {
		// clear notify lines and outgoing commands before passing
		// the restart to the cgame
		Con_ClearNotify();
		// reparse the string, because Con_ClearNotify() may have done another Cmd_TokenizeString()
		Cmd_TokenizeString( s );
		Com_Memset( cl.cmds, 0, sizeof( cl.cmds ) );
		return qtrue;
	}

	// the clientLevelShot command is used during development
	// to generate 128*128 screenshots from the intermission
	// point of levels for the menu system to use
	// we pass it along to the cgame to make appropriate adjustments,
	// but we also clear the console and notify lines here
	if ( !strcmp( cmd, "clientLevelShot" ) ) {
		// don't do it if we aren't running the server locally,
		// otherwise malicious remote servers could overwrite
		// the existing thumbnails
		if ( !com_sv_running->integer ) {
			return qfalse;
		}
		// close the console
		Con_Close();
		// take a special screenshot next frame
		Cbuf_AddText( "wait ; wait ; wait ; wait ; screenshot levelshot\n" );
		return qtrue;
	}

	// we may want to put a "connect to other server" command here

	// cgame can now act on the command
	return qtrue;
}


/*
====================
CL_CM_LoadMap

Just adds default parameters that cgame doesn't need to know about
====================
*/
void CL_CM_LoadMap( const char *mapname ) {
	int		checksum;

	CM_LoadMap( mapname, qtrue, &checksum );
}

/*
====================
CL_ShutdownCGame

====================
*/
void CL_ShutdownCGame( void ) {
	Key_SetCatcher( Key_GetCatcher( ) & ~KEYCATCH_CGAME );
	cls.cgameStarted = qfalse;
	if ( !cgvm ) {
		return;
	}
	VM_Call( cgvm, CG_SHUTDOWN );
	VM_Free( cgvm );
	cgvm = NULL;
}

static int	FloatAsInt( float f ) {
	floatint_t fi;
	fi.f = f;
	return fi.i;
}

/*
====================
CL_RegisterCGameSound
====================
*/
static sfxHandle_t CL_RegisterCGameSound( const char *name, qboolean compressed ) {
	sfxHandle_t	sfx;

	sfx = S_RegisterSound( name, compressed );
	if ( !Q_stricmp( name, HIT_SOUND ) ) {
		hitSound = sfx;
	}

	return sfx;
}

/*
====================
CL_SnapshotHasNewKill

Looks for the obituary of a kill by killer that is in snap but was not in
prev yet, temporary entities stay around for a few snapshots
====================
*/
static qboolean CL_SnapshotHasNewKill( const clSnapshot_t *snap, const clSnapshot_t *prev, int killer ) {
	const entityState_t	*es, *old;
	int					i, j;

	// prev is older, so if its entities are still there, so are snap's
	if ( cl.parseEntitiesNum - prev->parseEntitiesNum >= MAX_PARSE_ENTITIES ) {
		return qfalse;
	}

	for ( i = 0; i < snap->numEntities; i++ ) {
		es = &cl.parseEntities[ ( snap->parseEntitiesNum + i ) & ( MAX_PARSE_ENTITIES - 1 ) ];
		if ( es->eType != ET_EVENTS + EV_OBITUARY
			|| es->otherEntityNum2 != killer || es->otherEntityNum == killer ) {
			continue;
		}

		for ( j = 0; j < prev->numEntities; j++ ) {
			old = &cl.parseEntities[ ( prev->parseEntitiesNum + j ) & ( MAX_PARSE_ENTITIES - 1 ) ];
			if ( old->number == es->number && old->eType == es->eType
				&& old->otherEntityNum == es->otherEntityNum ) {
				break;
			}
		}
		if ( j == prev->numEntities ) {
			return qtrue;
		}
	}

	return qfalse;
}

/*
====================
CL_EstimateHitRemaining

Servers without PERS_ATTACKEE_REMAINING only report what the target had
before the hit, so take off what the weapon does in baseq3 (game/g_weapon.c,
g_missile.c). Splash weapons are counted as direct hits.
====================
*/
static void CL_EstimateHitRemaining( const clSnapshot_t *hit, int *health, int *armor ) {
	const char	*info;
	int			attackee, damage, save;

	// persistant[] only has 16 bits on the network, so mask off the sign
	attackee = hit->ps.persistant[PERS_ATTACKEE_ARMOR];
	*health = ( attackee >> 8 ) & 0xff;
	*armor = attackee & 0xff;

	switch ( hit->ps.weapon ) {
	case WP_GAUNTLET:
		damage = 50;
		break;
	case WP_MACHINEGUN:
		info = cl.gameState.stringData + cl.gameState.stringOffsets[ CS_SERVERINFO ];
		damage = ( atoi( Info_ValueForKey( info, "g_gametype" ) ) == GT_TEAM ) ? 5 : 7;
		break;
	case WP_SHOTGUN:
		damage = 10;	// the report is from before the last pellet
		break;
	case WP_LIGHTNING:
		damage = 8;
		break;
	case WP_PLASMAGUN:
		damage = 20;
		break;
	case WP_GRENADE_LAUNCHER:
	case WP_ROCKET_LAUNCHER:
	case WP_RAILGUN:
	case WP_BFG:
		damage = 100;
		break;
	default:
		damage = 0;
		break;
	}

	if ( hit->ps.powerups[PW_QUAD] ) {
		damage *= 3;	// default g_quadfactor
	}
	damage = damage * hit->ps.stats[STAT_MAX_HEALTH] / 100;	// handicap

	// armor takes its share first, as in CheckArmor
	save = ceil( damage * ARMOR_PROTECTION );
	if ( save > *armor ) {
		save = *armor;
	}
	*health -= damage - save;
	*armor -= save;

	// this was not the killing hit, so the target has something left
	if ( *health < 1 ) {
		*health = 1;
	}
}

/*
====================
CL_HitSoundPitch

The cgame plays the hit sound when it reads a snapshot in which the hit
counter went up. The pitch follows how much health and armor the target
has left after the hit, see cl_hitPitchFull and cl_hitPitchEmpty.

Returns qfalse if the sound does not belong to a new hit.
====================
*/
static qboolean CL_FindHitSnapshot( const clSnapshot_t **hitOut, const clSnapshot_t **prevOut ) {
	const clSnapshot_t	*list[PACKET_BACKUP];
	const clSnapshot_t	*snap, *hit, *prev;
	int					num, count, i;

	// collect the snapshots the cgame has seen, newest first
	count = 0;
	for ( num = cgameSnapshotNum; cl.snap.messageNum - num < PACKET_BACKUP; num-- ) {
		snap = &cl.snapshots[ num & PACKET_MASK ];
		if ( snap->valid && snap->messageNum == num ) {
			list[count++] = snap;
		}
	}

	if ( count < 2 ) {
		return qfalse;
	}

	// a counter that went backwards means a new player, a respawn or a
	// match restart, so take it as the new starting point
	if ( hitsSounded < 0 || list[0]->ps.persistant[PERS_HITS] < hitsSounded ) {
		if ( cl_hitSoundDebug->integer ) {
			Com_Printf( "hit sound: counter resync %i -> %i\n", hitsSounded, list[0]->ps.persistant[PERS_HITS] );
		}
		hitsSounded = list[0]->ps.persistant[PERS_HITS];
		return qfalse;
	}

	// report the oldest hit that has not been played yet, the cgame can be
	// a snapshot behind what it has already read (teleports and respawns)
	for ( i = count - 1; i > 0; i-- ) {
		prev = list[i];
		hit = list[i - 1];

		if ( prev->ps.clientNum != hit->ps.clientNum ) {
			continue;
		}
		if ( hit->ps.persistant[PERS_HITS] > prev->ps.persistant[PERS_HITS]
			&& hit->ps.persistant[PERS_HITS] > hitsSounded ) {
			*hitOut = hit;
			*prevOut = prev;
			return qtrue;
		}
	}

	return qfalse;
}

// the file cl_hitSound asks for, empty for the game's own hit sound
static const char *CL_HitSoundFile( void ) {
	switch ( cl_hitSound->integer ) {
	case 1:
		return HIT_SOUND_QC;
	case 2:
		return cl_hitSoundFile->string;
	default:
		return "";
	}
}

static sfxHandle_t CL_HitSoundHandle( void ) {
	const char *file = CL_HitSoundFile();

	if ( !file[0] ) {
		return hitSound;
	}

	// look the file up again only when the choice changed
	if ( Q_stricmp( file, customHitSoundFile ) ) {
		Q_strncpyz( customHitSoundFile, file, sizeof( customHitSoundFile ) );

		if ( FS_FOpenFileRead( file, NULL, qfalse ) >= 0 ) {
			customHitSound = S_RegisterSound( file, qfalse );

			if ( cl_hitSoundDebug->integer ) {
				Com_Printf( "hit sound: playing %s\n", file );
			}
		} else {
			customHitSound = -1;
			Com_Printf( S_COLOR_YELLOW "cl_hitSound: %s not found, using the game's hit sound\n",
				file );
		}
	}

	return ( customHitSound >= 0 ) ? customHitSound : hitSound;
}

static qboolean CL_HitSoundPitch( float *pitch, const char *source ) {
	const clSnapshot_t	*hit, *prev;
	int					remaining, health, armor;
	float				frac;

	if ( !CL_FindHitSnapshot( &hit, &prev ) ) {
		return qfalse;
	}
	hitsSounded = hit->ps.persistant[PERS_HITS];

	remaining = hit->ps.persistant[PERS_ATTACKEE_REMAINING];
	if ( remaining ) {
		// the server reports health plus one, so 0 health is a dead target
		health = ( ( remaining >> 8 ) & 0xff ) - 1;
		armor = remaining & 0xff;
	} else {
		CL_EstimateHitRemaining( hit, &health, &armor );
	}

	// the hit killed the target
	if ( health <= 0 || CL_SnapshotHasNewKill( hit, prev, hit->ps.clientNum )
		|| hit->ps.persistant[PERS_SCORE] > prev->ps.persistant[PERS_SCORE] ) {
		*pitch = cl_hitPitchKill->value;
		if ( cl_hitSoundDebug->integer ) {
			Com_Printf( "hit sound: kill, pitch %.2f (%s)\n", *pitch, source );
		}
		return qtrue;
	}

	// cl_hitPitchStack health and armor combined counts as a full target
	frac = ( health + armor ) / cl_hitPitchStack->value;
	if ( frac > 1.0f ) {
		frac = 1.0f;
	}

	*pitch = cl_hitPitchEmpty->value + ( cl_hitPitchFull->value - cl_hitPitchEmpty->value ) * frac;
	if ( cl_hitSoundDebug->integer ) {
		Com_Printf( "hit sound: %i health %i armor left%s, pitch %.2f (%s)\n",
			health, armor, remaining ? "" : " (estimated)", *pitch, source );
	}
	return qtrue;
}

/*
====================
CL_CheckMissedHitSound

In frames where no user command runs, the cgame returns from
CG_PredictPlayerState before it looks at the hit counter (the "if ( !moved )"
path), and the hit sound for that snapshot is lost for good. Play it here
when that happens, so that every hit is heard.
====================
*/
static void CL_CheckMissedHitSound( void ) {
	const clSnapshot_t	*hit, *prev;
	float				pitch;

	if ( !cl_hitPitch->integer || hitSound < 0 || clc.state != CA_ACTIVE ) {
		return;
	}

	if ( !CL_FindHitSnapshot( &hit, &prev ) ) {
		return;
	}

	// the cgame stays quiet in these cases as well, see CG_TransitionPlayerState
	if ( cl.snap.ps.pm_type == PM_INTERMISSION
		|| hit->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR
		|| hit->ps.persistant[PERS_TEAM] != prev->ps.persistant[PERS_TEAM] ) {
		return;
	}

	// without prediction the cgame plays a snapshot's sounds when it reaches
	// its time, so only step in once it has moved on to a newer snapshot
	if ( ( clc.demoplaying || ( cl.snap.ps.pm_flags & PMF_FOLLOW )
			|| Cvar_VariableIntegerValue( "cg_nopredict" )
			|| Cvar_VariableIntegerValue( "cg_synchronousClients" ) )
		&& cgameSnapshotNum <= hit->messageNum ) {
		return;
	}

	if ( CL_HitSoundPitch( &pitch, "engine" ) ) {
		S_StartLocalSoundWithPitch( CL_HitSoundHandle(), CHAN_LOCAL_SOUND, pitch );
	}
}

/*
====================
CL_CgameSystemCalls

The cgame module is making a system call
====================
*/
intptr_t CL_CgameSystemCalls( intptr_t *args ) {
	switch( args[0] ) {
	case CG_PRINT:
		Com_Printf( "%s", (const char*)VMA(1) );
		return 0;
	case CG_ERROR:
		Com_Error( ERR_DROP, "%s", (const char*)VMA(1) );
		return 0;
	case CG_MILLISECONDS:
		return Sys_Milliseconds();
	case CG_CVAR_REGISTER:
		Cvar_Register( VMA(1), VMA(2), VMA(3), args[4] ); 
		return 0;
	case CG_CVAR_UPDATE:
		Cvar_Update( VMA(1) );
		return 0;
	case CG_CVAR_SET:
		Cvar_SetSafe( VMA(1), VMA(2) );
		return 0;
	case CG_CVAR_VARIABLESTRINGBUFFER:
		Cvar_VariableStringBuffer( VMA(1), VMA(2), args[3] );
		return 0;
	case CG_ARGC:
		return Cmd_Argc();
	case CG_ARGV:
		Cmd_ArgvBuffer( args[1], VMA(2), args[3] );
		return 0;
	case CG_ARGS:
		Cmd_ArgsBuffer( VMA(1), args[2] );
		return 0;
	case CG_FS_FOPENFILE:
		return FS_FOpenFileByMode( VMA(1), VMA(2), args[3] );
	case CG_FS_READ:
		FS_Read( VMA(1), args[2], args[3] );
		return 0;
	case CG_FS_WRITE:
		FS_Write( VMA(1), args[2], args[3] );
		return 0;
	case CG_FS_FCLOSEFILE:
		FS_FCloseFile( args[1] );
		return 0;
	case CG_FS_SEEK:
		return FS_Seek( args[1], args[2], args[3] );
	case CG_SENDCONSOLECOMMAND:
		Cbuf_AddText( VMA(1) );
		return 0;
	case CG_ADDCOMMAND:
		CL_AddCgameCommand( VMA(1) );
		return 0;
	case CG_REMOVECOMMAND:
		Cmd_RemoveCommandSafe( VMA(1) );
		return 0;
	case CG_SENDCLIENTCOMMAND:
		CL_AddReliableCommand(VMA(1), qfalse);
		return 0;
	case CG_UPDATESCREEN:
		// this is used during lengthy level loading, so pump message loop
//		Com_EventLoop();	// FIXME: if a server restarts here, BAD THINGS HAPPEN!
// We can't call Com_EventLoop here, a restart will crash and this _does_ happen
// if there is a map change while we are downloading at pk3.
// ZOID
		SCR_UpdateScreen();
		return 0;
	case CG_CM_LOADMAP:
		CL_CM_LoadMap( VMA(1) );
		return 0;
	case CG_CM_NUMINLINEMODELS:
		return CM_NumInlineModels();
	case CG_CM_INLINEMODEL:
		return CM_InlineModel( args[1] );
	case CG_CM_TEMPBOXMODEL:
		return CM_TempBoxModel( VMA(1), VMA(2), /*int capsule*/ qfalse );
	case CG_CM_TEMPCAPSULEMODEL:
		return CM_TempBoxModel( VMA(1), VMA(2), /*int capsule*/ qtrue );
	case CG_CM_POINTCONTENTS:
		return CM_PointContents( VMA(1), args[2] );
	case CG_CM_TRANSFORMEDPOINTCONTENTS:
		return CM_TransformedPointContents( VMA(1), args[2], VMA(3), VMA(4) );
	case CG_CM_BOXTRACE:
		CM_BoxTrace( VMA(1), VMA(2), VMA(3), VMA(4), VMA(5), args[6], args[7], /*int capsule*/ qfalse );
		return 0;
	case CG_CM_CAPSULETRACE:
		CM_BoxTrace( VMA(1), VMA(2), VMA(3), VMA(4), VMA(5), args[6], args[7], /*int capsule*/ qtrue );
		return 0;
	case CG_CM_TRANSFORMEDBOXTRACE:
		CM_TransformedBoxTrace( VMA(1), VMA(2), VMA(3), VMA(4), VMA(5), args[6], args[7], VMA(8), VMA(9), /*int capsule*/ qfalse );
		return 0;
	case CG_CM_TRANSFORMEDCAPSULETRACE:
		CM_TransformedBoxTrace( VMA(1), VMA(2), VMA(3), VMA(4), VMA(5), args[6], args[7], VMA(8), VMA(9), /*int capsule*/ qtrue );
		return 0;
	case CG_CM_MARKFRAGMENTS:
		return re.MarkFragments( args[1], VMA(2), VMA(3), args[4], VMA(5), args[6], VMA(7) );
	case CG_S_STARTSOUND:
		S_StartSound( VMA(1), args[2], args[3], args[4] );
		return 0;
	case CG_S_STARTLOCALSOUND:
		{
			float		pitch;
			sfxHandle_t	sfx = args[1];

			if ( args[1] == hitSound ) {
				sfx = CL_HitSoundHandle();

				// still say that a hit was answered when the pitch is off,
				// otherwise a test run reads as if every sound went missing
				if ( !cl_hitPitch->integer && cl_hitSoundDebug->integer ) {
					Com_Printf( "hit sound: unpitched (cgame)\n" );
				}

				if ( cl_hitPitch->integer ) {
					// the hit sound is ours now: either it belongs to a hit
					// that has not been played yet, or it is a repeat and
					// stays silent instead of leaking an unpitched copy
					if ( CL_HitSoundPitch( &pitch, "cgame" ) ) {
						S_StartLocalSoundWithPitch( sfx, args[2], pitch );
					}
					return 0;
				}
			}
			S_StartLocalSound( sfx, args[2] );
		}
		return 0;
	case CG_S_CLEARLOOPINGSOUNDS:
		S_ClearLoopingSounds(args[1]);
		return 0;
	case CG_S_ADDLOOPINGSOUND:
		S_AddLoopingSound( args[1], VMA(2), VMA(3), args[4] );
		return 0;
	case CG_S_ADDREALLOOPINGSOUND:
		S_AddRealLoopingSound( args[1], VMA(2), VMA(3), args[4] );
		return 0;
	case CG_S_STOPLOOPINGSOUND:
		S_StopLoopingSound( args[1] );
		return 0;
	case CG_S_UPDATEENTITYPOSITION:
		S_UpdateEntityPosition( args[1], VMA(2) );
		return 0;
	case CG_S_RESPATIALIZE:
		S_Respatialize( args[1], VMA(2), VMA(3), args[4] );
		return 0;
	case CG_S_REGISTERSOUND:
		return CL_RegisterCGameSound( VMA(1), args[2] );
	case CG_S_STARTBACKGROUNDTRACK:
		S_StartBackgroundTrack( VMA(1), VMA(2) );
		return 0;
	case CG_R_LOADWORLDMAP:
		re.LoadWorld( VMA(1) );
		return 0; 
	case CG_R_REGISTERMODEL:
		return re.RegisterModel( VMA(1) );
	case CG_R_REGISTERSKIN:
		return re.RegisterSkin( VMA(1) );
	case CG_R_REGISTERSHADER:
		return re.RegisterShader( VMA(1) );
	case CG_R_REGISTERSHADERNOMIP:
		return re.RegisterShaderNoMip( VMA(1) );
	case CG_R_REGISTERFONT:
		re.RegisterFont( VMA(1), args[2], VMA(3));
		return 0;
	case CG_R_CLEARSCENE:
		re.ClearScene();
		return 0;
	case CG_R_ADDREFENTITYTOSCENE:
		re.AddRefEntityToScene( VMA(1) );
		return 0;
	case CG_R_ADDPOLYTOSCENE:
		re.AddPolyToScene( args[1], args[2], VMA(3), 1 );
		return 0;
	case CG_R_ADDPOLYSTOSCENE:
		re.AddPolyToScene( args[1], args[2], VMA(3), args[4] );
		return 0;
	case CG_R_LIGHTFORPOINT:
		return re.LightForPoint( VMA(1), VMA(2), VMA(3), VMA(4) );
	case CG_R_ADDLIGHTTOSCENE:
		re.AddLightToScene( VMA(1), VMF(2), VMF(3), VMF(4), VMF(5) );
		return 0;
	case CG_R_ADDADDITIVELIGHTTOSCENE:
		re.AddAdditiveLightToScene( VMA(1), VMF(2), VMF(3), VMF(4), VMF(5) );
		return 0;
	case CG_R_RENDERSCENE:
		// The cgame renders several scenes a frame: the world first, then the
		// little ones inside the status bar. Only the first is the view the
		// outlines belong on.
		if ( !botOutlineViewValid ) {
			botOutlineView = *(const refdef_t *)VMA(1);
			botOutlineViewValid = qtrue;
			CL_AddBotOutlines();
			CL_AddItemOutlines();
		}

		re.RenderScene( VMA(1) );
		return 0;
	case CG_R_SETCOLOR:
		re.SetColor( VMA(1) );
		return 0;
	case CG_R_DRAWSTRETCHPIC:
		re.DrawStretchPic( VMF(1), VMF(2), VMF(3), VMF(4), VMF(5), VMF(6), VMF(7), VMF(8), args[9] );
		return 0;
	case CG_R_MODELBOUNDS:
		re.ModelBounds( args[1], VMA(2), VMA(3) );
		return 0;
	case CG_R_LERPTAG:
		return re.LerpTag( VMA(1), args[2], args[3], args[4], VMF(5), VMA(6) );
	case CG_GETGLCONFIG:
		CL_GetGlconfig( VMA(1) );
		return 0;
	case CG_GETGAMESTATE:
		CL_GetGameState( VMA(1) );
		return 0;
	case CG_GETCURRENTSNAPSHOTNUMBER:
		CL_GetCurrentSnapshotNumber( VMA(1), VMA(2) );
		return 0;
	case CG_GETSNAPSHOT:
		return CL_GetSnapshot( args[1], VMA(2) );
	case CG_GETSERVERCOMMAND:
		return CL_GetServerCommand( args[1] );
	case CG_GETCURRENTCMDNUMBER:
		return CL_GetCurrentCmdNumber();
	case CG_GETUSERCMD:
		return CL_GetUserCmd( args[1], VMA(2) );
	case CG_SETUSERCMDVALUE:
		CL_SetUserCmdValue( args[1], VMF(2) );
		return 0;
	case CG_MEMORY_REMAINING:
		return Hunk_MemoryRemaining();
  case CG_KEY_ISDOWN:
		return Key_IsDown( args[1] );
  case CG_KEY_GETCATCHER:
		return Key_GetCatcher();
  case CG_KEY_SETCATCHER:
		// Don't allow the cgame module to close the console
		Key_SetCatcher( args[1] | ( Key_GetCatcher( ) & KEYCATCH_CONSOLE ) );
    return 0;
  case CG_KEY_GETKEY:
		return Key_GetKey( VMA(1) );



	case CG_MEMSET:
		Com_Memset( VMA(1), args[2], args[3] );
		return 0;
	case CG_MEMCPY:
		Com_Memcpy( VMA(1), VMA(2), args[3] );
		return 0;
	case CG_STRNCPY:
		strncpy( VMA(1), VMA(2), args[3] );
		return args[1];
	case CG_SIN:
		return FloatAsInt( sin( VMF(1) ) );
	case CG_COS:
		return FloatAsInt( cos( VMF(1) ) );
	case CG_ATAN2:
		return FloatAsInt( atan2( VMF(1), VMF(2) ) );
	case CG_SQRT:
		return FloatAsInt( sqrt( VMF(1) ) );
	case CG_FLOOR:
		return FloatAsInt( floor( VMF(1) ) );
	case CG_CEIL:
		return FloatAsInt( ceil( VMF(1) ) );
	case CG_ACOS:
		return FloatAsInt( Q_acos( VMF(1) ) );

	case CG_PC_ADD_GLOBAL_DEFINE:
		return botlib_export->PC_AddGlobalDefine( VMA(1) );
	case CG_PC_LOAD_SOURCE:
		return botlib_export->PC_LoadSourceHandle( VMA(1) );
	case CG_PC_FREE_SOURCE:
		return botlib_export->PC_FreeSourceHandle( args[1] );
	case CG_PC_READ_TOKEN:
		return botlib_export->PC_ReadTokenHandle( args[1], VMA(2) );
	case CG_PC_SOURCE_FILE_AND_LINE:
		return botlib_export->PC_SourceFileAndLine( args[1], VMA(2), VMA(3) );

	case CG_S_STOPBACKGROUNDTRACK:
		S_StopBackgroundTrack();
		return 0;

	case CG_REAL_TIME:
		return Com_RealTime( VMA(1) );
	case CG_SNAPVECTOR:
		Q_SnapVector(VMA(1));
		return 0;

	case CG_CIN_PLAYCINEMATIC:
	  return CIN_PlayCinematic(VMA(1), args[2], args[3], args[4], args[5], args[6]);

	case CG_CIN_STOPCINEMATIC:
	  return CIN_StopCinematic(args[1]);

	case CG_CIN_RUNCINEMATIC:
	  return CIN_RunCinematic(args[1]);

	case CG_CIN_DRAWCINEMATIC:
	  CIN_DrawCinematic(args[1]);
	  return 0;

	case CG_CIN_SETEXTENTS:
	  CIN_SetExtents(args[1], args[2], args[3], args[4], args[5]);
	  return 0;

	case CG_R_REMAP_SHADER:
		re.RemapShader( VMA(1), VMA(2), VMA(3) );
		return 0;

/*
	case CG_LOADCAMERA:
		return loadCamera(VMA(1));

	case CG_STARTCAMERA:
		startCamera(args[1]);
		return 0;

	case CG_GETCAMERAINFO:
		return getCameraInfo(args[1], VMA(2), VMA(3));
*/
	case CG_GET_ENTITY_TOKEN:
		return re.GetEntityToken( VMA(1), args[2] );
	case CG_R_INPVS:
		return re.inPVS( VMA(1), VMA(2) );

	default:
	        assert(0);
		Com_Error( ERR_DROP, "Bad cgame system trap: %ld", (long int) args[0] );
	}
	return 0;
}


/*
====================
CL_InitCGame

Should only be called by CL_StartHunkUsers
====================
*/
void CL_InitCGame( void ) {
	const char			*info;
	const char			*mapname;
	int					t1, t2;
	vmInterpret_t		interpret;

	t1 = Sys_Milliseconds();

	// put away the console
	Con_Close();

	// find the current mapname
	info = cl.gameState.stringData + cl.gameState.stringOffsets[ CS_SERVERINFO ];
	mapname = Info_ValueForKey( info, "mapname" );
	Com_sprintf( cl.mapname, sizeof( cl.mapname ), "maps/%s.bsp", mapname );

	// load the dll or bytecode
	interpret = Cvar_VariableValue("vm_cgame");
	if(cl_connectedToPureServer)
	{
		// if sv_pure is set we only allow qvms to be loaded
		if(interpret != VMI_COMPILED && interpret != VMI_BYTECODE)
			interpret = VMI_COMPILED;
	}

	botOutlineShader = re.RegisterShader( "white" );
	Com_Memset( itemTimers, 0, sizeof( itemTimers ) );
	hitSound = -1;
	customHitSound = -1;
	customHitSoundFile[0] = '\0';
	cgameSnapshotNum = 0;
	hitsSounded = -1;

	cgvm = VM_Create( "cgame", CL_CgameSystemCalls, interpret );
	if ( !cgvm ) {
		Com_Error( ERR_DROP, "VM_Create on cgame failed" );
	}
	clc.state = CA_LOADING;

	// init for this gamestate
	// use the lastExecutedServerCommand instead of the serverCommandSequence
	// otherwise server commands sent just before a gamestate are dropped
	VM_Call( cgvm, CG_INIT, clc.serverMessageSequence, clc.lastExecutedServerCommand, clc.clientNum );

	// reset any CVAR_CHEAT cvars registered by cgame
	if ( !clc.demoplaying && !cl_connectedToCheatServer )
		Cvar_SetCheatState();

	// we will send a usercmd this frame, which
	// will cause the server to send us the first snapshot
	clc.state = CA_PRIMED;

	t2 = Sys_Milliseconds();

	Com_Printf( "CL_InitCGame: %5.2f seconds\n", (t2-t1)/1000.0 );

	// have the renderer touch all its images, so they are present
	// on the card even if the driver does deferred loading
	re.EndRegistration();

	// make sure everything is paged in
	if (!Sys_LowPhysicalMemory()) {
		Com_TouchMemory();
	}

	// clear anything that got printed
	Con_ClearNotify ();
}


/*
====================
CL_GameCommand

See if the current console command is claimed by the cgame
====================
*/
qboolean CL_GameCommand( void ) {
	if ( !cgvm ) {
		return qfalse;
	}

	return VM_Call( cgvm, CG_CONSOLE_COMMAND );
}



/*
====================
CL_BotOutlineOrigin

Where the target is at the moment being drawn, not where the last snapshot
left it.  Snapshots arrive twenty times a second and the picture is drawn far
more often, so the box has to move between them the same way the model does.
====================
*/
static qboolean CL_BotOutlineOrigin( int entityNum, vec3_t origin ) {
	const clSnapshot_t	*previous;
	const entityState_t	*now = NULL, *before = NULL;
	float				span, fraction;
	int					i;

	for ( i = 0; i < cl.snap.numEntities; i++ ) {
		const entityState_t *state =
			&cl.parseEntities[( cl.snap.parseEntitiesNum + i ) & ( MAX_PARSE_ENTITIES - 1 )];
		if ( state->number == entityNum ) {
			now = state;
			break;
		}
	}

	if ( !now ) {
		return qfalse;
	}

	VectorCopy( now->pos.trBase, origin );

	previous = &cl.snapshots[( cl.snap.messageNum - 1 ) & PACKET_MASK];
	span = (float)( cl.snap.serverTime - previous->serverTime );
	if ( !previous->valid || span <= 0.0f ) {
		return qtrue;
	}

	for ( i = 0; i < previous->numEntities; i++ ) {
		const entityState_t *state =
			&cl.parseEntities[( previous->parseEntitiesNum + i ) & ( MAX_PARSE_ENTITIES - 1 )];
		if ( state->number == entityNum ) {
			before = state;
			break;
		}
	}

	if ( !before ) {
		return qtrue;		// it was not there a moment ago, nothing to come from
	}

	fraction = Com_Clamp( 0.0f, 1.0f, ( cl.serverTime - previous->serverTime ) / span );
	VectorSubtract( now->pos.trBase, before->pos.trBase, origin );
	VectorMA( before->pos.trBase, fraction, origin, origin );

	return qtrue;
}


/*
====================
CL_BotOutlineNearPoint

Puts a world point on a shell close around the eye, keeping the direction it
is seen from.  Drawn there it lands on exactly the same place on the screen as
the real point but in front of every wall, which is what lets the outline show
through the level without touching the depth buffer.
====================
*/
static qboolean CL_BotOutlineNearPoint( const vec3_t point, vec3_t near ) {
	vec3_t	direction;
	float	forward;

	VectorSubtract( point, botOutlineView.vieworg, direction );
	forward = DotProduct( direction, botOutlineView.viewaxis[0] );
	if ( forward < 1.0f ) {
		return qfalse;		// behind the eye
	}

	VectorMA( botOutlineView.vieworg, BOT_OUTLINE_SHELL / forward, direction, near );

	return qtrue;
}


/*
====================
CL_ProjectToScreen

Where a point in the world lands on the view being drawn. Anything behind the
eye has no place on the screen and is refused.
====================
*/
static qboolean CL_ProjectToScreen( const vec3_t point, float *screenX, float *screenY ) {
	vec3_t	local;
	float	forward, left, up, halfWidth, halfHeight;

	VectorSubtract( point, botOutlineView.vieworg, local );
	forward = DotProduct( local, botOutlineView.viewaxis[0] );
	if ( forward < 1.0f ) {
		return qfalse;
	}

	// viewaxis[1] points left, the screen counts to the right
	left = DotProduct( local, botOutlineView.viewaxis[1] );
	up = DotProduct( local, botOutlineView.viewaxis[2] );

	halfWidth = forward * tan( DEG2RAD( botOutlineView.fov_x * 0.5f ) );
	halfHeight = forward * tan( DEG2RAD( botOutlineView.fov_y * 0.5f ) );
	if ( halfWidth <= 0.0f || halfHeight <= 0.0f ) {
		return qfalse;
	}

	*screenX = botOutlineView.x + botOutlineView.width * 0.5f * ( 1.0f - left / halfWidth );
	*screenY = botOutlineView.y + botOutlineView.height * 0.5f * ( 1.0f - up / halfHeight );

	return qtrue;
}


/*
====================
CL_BotOutlineEdge

One edge of the wire box, as a thin strip turned towards the eye.
====================
*/
static void CL_BotOutlineEdge( const vec3_t from, const vec3_t to, const byte *colour ) {
	polyVert_t	quad[4];
	vec3_t		along, out, side;
	int			i;

	VectorSubtract( to, from, along );
	VectorSubtract( from, botOutlineView.vieworg, out );
	CrossProduct( along, out, side );
	if ( VectorNormalize( side ) == 0.0f ) {
		return;
	}
	VectorScale( side, BOT_OUTLINE_WIDTH, side );

	VectorSubtract( from, side, quad[0].xyz );
	VectorAdd( from, side, quad[1].xyz );
	VectorAdd( to, side, quad[2].xyz );
	VectorSubtract( to, side, quad[3].xyz );

	for ( i = 0; i < 4; i++ ) {
		quad[i].st[0] = ( i == 1 || i == 2 ) ? 1.0f : 0.0f;
		quad[i].st[1] = ( i >= 2 ) ? 1.0f : 0.0f;
		Com_Memcpy( quad[i].modulate, colour, 4 );
	}

	re.AddPolyToScene( botOutlineShader, 4, quad, 1 );
}


/*
====================
CL_AddBotOutlines

Draws a wire box around every enemy bot, brighter when the shot would reach
it.  It carries the same two locks as the aim assist: a loopback connection,
so it cannot run on anyone else's server, and only the players the server
itself reports as bots, so a human is never outlined.
====================
*/
/*
====================
CL_BotOutlineWireBox

The twelve edges of a box whose corners are already on the near shell.
====================
*/
static void CL_BotOutlineWireBox( const vec3_t near[8], const byte *colour ) {
	static const int	edges[12][2] = {
		{ 0, 1 }, { 1, 3 }, { 3, 2 }, { 2, 0 },		// the floor of the box
		{ 4, 5 }, { 5, 7 }, { 7, 6 }, { 6, 4 },		// and its lid
		{ 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 },		// the uprights
	};
	int	i;

	for ( i = 0; i < 12; i++ ) {
		CL_BotOutlineEdge( near[edges[i][0]], near[edges[i][1]], colour );
	}
}


/*
====================
CL_BotDamageColour

Green while the bot is untouched, through yellow and orange to red when the
next hit finishes it. Dimmed when it is behind something, and faded towards
the plain outline colour as the knowledge ages, so old news looks like old
news instead of a promise.
====================
*/
static void CL_BotDamageColour( int health, float fresh, qboolean seen, byte *out ) {
	static const float	full[3] = { 80.0f, 220.0f, 90.0f };		// untouched
	static const float	half[3] = { 245.0f, 210.0f, 60.0f };
	static const float	gone[3] = { 235.0f, 60.0f, 50.0f };		// one hit left
	const float			*low, *high;
	float				part, share, dim;
	int					i;

	share = Com_Clamp( 0.0f, 1.0f, health / 100.0f );
	if ( share > 0.5f ) {
		low = half;
		high = full;
		part = ( share - 0.5f ) * 2.0f;
	} else {
		low = gone;
		high = half;
		part = share * 2.0f;
	}

	dim = ( seen ? 1.0f : 0.5f ) * ( 0.45f + 0.55f * Com_Clamp( 0.0f, 1.0f, fresh ) );
	for ( i = 0; i < 3; i++ ) {
		out[i] = (byte)Com_Clamp( 0.0f, 255.0f, ( low[i] + ( high[i] - low[i] ) * part ) * dim );
	}
	out[3] = 255;
}


// What a bot had left, ready to be written once the world is on the screen
typedef struct {
	float	x, y;
	byte	colour[3];
	char	text[12];
} botLabel_t;

static botLabel_t	botLabel[MAX_CLIENTS];
static int			botLabels;

static void CL_DrawBotLabels( void ) {
	vec4_t	tint;
	int		i;

	for ( i = 0; i < botLabels; i++ ) {
		tint[0] = botLabel[i].colour[0] / 255.0f;
		tint[1] = botLabel[i].colour[1] / 255.0f;
		tint[2] = botLabel[i].colour[2] / 255.0f;
		tint[3] = 1.0f;
		SCR_DrawStringExt(
			(int)( botLabel[i].x * 640.0f / cls.glconfig.vidWidth - strlen( botLabel[i].text ) * 5.0f ),
			(int)( botLabel[i].y * 480.0f / cls.glconfig.vidHeight - 10.0f ),
			10.0f, botLabel[i].text, tint, qtrue, qfalse );
	}
	botLabels = 0;
}

static void CL_AddBotOutlines( void ) {
	static const byte	visible[4] = { 255, 115, 25, 255 };
	static const byte	hidden[4] = { 120, 40, 10, 255 };
	static vec3_t		mins = { -15, -15, -24 };
	static vec3_t		maxs = { 15, 15, 32 };
	const entityState_t	*entity;
	const char			*info;
	const byte			*colour;
	byte				shade[4];
	trace_t				trace;
	vec3_t				origin, corner[8], near[8], eye, label;
	qboolean			ahead, seen;
	float				top, fresh, x, y;
	int					i, j, health = -1, armor = 0;

	if ( !cl_botOutline->integer || clc.state != CA_ACTIVE || clc.demoplaying
		|| clc.netchan.remoteAddress.type != NA_LOOPBACK || !cl.snap.valid ) {
		return;
	}

	VectorCopy( cl.snap.ps.origin, eye );
	eye[2] += cl.snap.ps.viewheight;

	for ( i = 0; i < cl.snap.numEntities; i++ ) {
		entity = &cl.parseEntities[( cl.snap.parseEntitiesNum + i ) & ( MAX_PARSE_ENTITIES - 1 )];
		if ( entity->eType != ET_PLAYER || entity->clientNum == cl.snap.ps.clientNum
			|| entity->clientNum < 0 || entity->clientNum >= MAX_CLIENTS
			|| ( entity->eFlags & EF_DEAD ) ) {
			continue;
		}

		info = cl.gameState.stringData + cl.gameState.stringOffsets[CS_PLAYERS + entity->clientNum];
		if ( !*Info_ValueForKey( info, "skill" ) ) {
			continue;	// a human player is never outlined
		}

		if ( !CL_BotOutlineOrigin( entity->number, origin ) ) {
			continue;
		}

		// a ducked bot is half as tall, and so is its box
		top = CL_AimAssistCrouched( entity ) ? CROUCH_HEIGHT : DEFAULT_HEIGHT;

		for ( j = 0; j < 8; j++ ) {
			corner[j][0] = origin[0] + ( ( j & 1 ) ? maxs[0] : mins[0] );
			corner[j][1] = origin[1] + ( ( j & 2 ) ? maxs[1] : mins[1] );
			corner[j][2] = origin[2] + ( ( j & 4 ) ? top : mins[2] );
		}

		ahead = qtrue;
		for ( j = 0; j < 8 && ahead; j++ ) {
			ahead = CL_BotOutlineNearPoint( corner[j], near[j] );
		}
		if ( !ahead ) {
			continue;
		}

		CM_BoxTrace( &trace, eye, origin, vec3_origin, vec3_origin, 0, MASK_SOLID, qfalse );
		seen = trace.fraction >= 1.0f;

		// What a bot had left the last time we hit it. The game never sends
		// another player's health, but the hit sound is told what the one we
		// just hit has left, and that is worth showing: green while it is
		// healthy, red when the next shot does it. Faded as the news ages,
		// because they pick health up and we would not know.
		if ( CL_AimAssistKnownDamage( entity->clientNum, &health, &armor, &fresh ) ) {
			CL_BotDamageColour( health, fresh, seen, shade );
			colour = shade;
		} else {
			colour = seen ? visible : hidden;
		}

		CL_BotOutlineWireBox( near, colour );

		// The numbers are flat on the screen, and everything flat has to wait
		// until the world has been painted or the world paints over it. So
		// they are only worked out here and drawn later, with the item clocks.
		if ( health >= 0 && cl_botOutline->integer > 1 && botLabels < MAX_CLIENTS ) {
			VectorCopy( origin, label );
			label[2] += top + 14.0f;
			if ( CL_ProjectToScreen( label, &x, &y ) ) {
				Com_sprintf( botLabel[botLabels].text, sizeof( botLabel[0].text ),
					armor > 0 ? "%i+%i" : "%i", health, armor );
				botLabel[botLabels].x = x;
				botLabel[botLabels].y = y;
				botLabel[botLabels].colour[0] = colour[0];
				botLabel[botLabels].colour[1] = colour[1];
				botLabel[botLabels].colour[2] = colour[2];
				botLabels++;
			}
		}
		health = -1;
	}
}


/*
====================
CL_ItemRespawnTime

How long the kind of item stays away once it has been taken, or zero for the
ones this is not asked about.
====================
*/
/*
====================
CL_ItemOutlineColour

The colour the item wears in the game, so a box can be told apart at a glance:
the rocket launcher red, the railgun green, the quad blue, and so on. A taken
one keeps its colour but is dimmed right down.
====================
*/
static void CL_ItemOutlineColour( const gitem_t *item, qboolean taken, byte *colour ) {
	static const byte	weapon[WP_NUM_WEAPONS][3] = {
		{ 160, 160, 170 },		// none
		{ 170, 170, 180 },		// gauntlet
		{ 200, 200, 150 },		// machinegun
		{ 230, 150, 60 },		// shotgun
		{ 150, 180, 70 },		// grenade launcher
		{ 235, 70, 50 },		// rocket launcher
		{ 120, 200, 255 },		// lightning
		{ 60, 225, 120 },		// railgun
		{ 185, 110, 255 },		// plasma
		{ 140, 255, 80 },		// bfg
		{ 170, 170, 180 },		// grappling hook
	};
	static const byte	powerup[PW_NUM_POWERUPS][3] = {
		{ 200, 200, 200 },		// none
		{ 80, 120, 255 },		// quad
		{ 210, 200, 90 },		// battle suit
		{ 245, 230, 70 },		// haste
		{ 220, 220, 235 },		// invisibility
		{ 235, 90, 60 },		// regeneration
		{ 90, 230, 230 },		// flight
	};
	const byte	*base;
	int			i;

	switch ( item->giType ) {
	case IT_WEAPON:
	case IT_AMMO:
		base = weapon[( item->giTag > WP_NONE && item->giTag < WP_NUM_WEAPONS ) ? item->giTag : 0];
		break;
	case IT_POWERUP:
	case IT_PERSISTANT_POWERUP:
		base = powerup[( item->giTag > PW_NONE && item->giTag < PW_NUM_POWERUPS ) ? item->giTag : 0];
		break;
	case IT_ARMOR:
		// shard, yellow and red, as they look on the floor
		base = item->quantity <= 5 ? (const byte[]){ 110, 230, 110 }
			: item->quantity >= 100 ? (const byte[]){ 235, 80, 60 }
			: (const byte[]){ 245, 210, 60 };
		break;
	case IT_HEALTH:
		base = item->quantity >= 100 ? (const byte[]){ 90, 160, 255 }
			: (const byte[]){ 235, 190, 80 };
		break;
	case IT_HOLDABLE:
		base = item->giTag == HI_MEDKIT ? (const byte[]){ 235, 120, 120 }
			: (const byte[]){ 200, 200, 235 };
		break;
	default:
		base = (const byte[]){ 200, 200, 200 };
		break;
	}

	for ( i = 0; i < 3; i++ ) {
		colour[i] = taken ? (byte)( base[i] * 0.35f ) : base[i];
	}
	colour[3] = 255;
}


static int CL_ItemRespawnTime( const entityState_t *entity ) {
	const gitem_t	*item;
	qboolean		everything;

	if ( entity->modelindex <= 0 || entity->modelindex >= bg_numItems ) {
		return 0;
	}

	item = &bg_itemlist[entity->modelindex];

	// 1 marks what a match is usually timed by, 2 everything that can be picked
	// up at all, down to the ammo boxes
	everything = cl_itemOutline->integer > 1;

	switch ( item->giType ) {
	case IT_WEAPON:
		return ITEM_RESPAWN_WEAPON;
	case IT_POWERUP:
	case IT_PERSISTANT_POWERUP:
		return ITEM_RESPAWN_POWERUP;
	case IT_ARMOR:
		// the shards are small change, the rest is worth a clock
		return ( everything || item->quantity > 5 ) ? ITEM_RESPAWN_ARMOR : 0;
	case IT_HEALTH:
		return ( everything || item->quantity == 100 ) ? ITEM_RESPAWN_HEALTH : 0;
	case IT_HOLDABLE:
		return ITEM_RESPAWN_HOLDABLE;
	case IT_AMMO:
		return everything ? ITEM_RESPAWN_AMMO : 0;
	case IT_TEAM:
		return everything ? ITEM_RESPAWN_ARMOR : 0;
	default:
		return 0;
	}
}


/*
====================
CL_AddItemOutlines

A wire box around every weapon and powerup, and once one has been taken the
seconds until it comes back.  The labels are collected here and drawn over the
finished picture, since text is flat and the boxes are not.
====================
*/
static void CL_AddItemOutlines( void ) {
	static vec3_t		mins = { -14, -14, -6 };
	static vec3_t		maxs = { 14, 14, 26 };
	const entityState_t	*entity;
	itemTimer_t			*timer;
	byte				colour[4];
	vec3_t				corner[8], near[8], top;
	qboolean			ahead;
	int					i, j, respawn, left, vanished = 0;

	itemFrame++;

	if ( !cl_itemOutline->integer || clc.state != CA_ACTIVE || clc.demoplaying
		|| clc.netchan.remoteAddress.type != NA_LOOPBACK || !cl.snap.valid ) {
		return;
	}

	// what the snapshot still holds is lying there to be had
	for ( i = 0; i < cl.snap.numEntities; i++ ) {
		entity = &cl.parseEntities[( cl.snap.parseEntitiesNum + i ) & ( MAX_PARSE_ENTITIES - 1 )];
		if ( entity->eType != ET_ITEM || entity->number < 0 || entity->number >= MAX_GENTITIES ) {
			continue;
		}

		respawn = CL_ItemRespawnTime( entity );
		if ( !respawn ) {
			continue;
		}

		timer = &itemTimers[entity->number];
		VectorCopy( entity->pos.trBase, timer->origin );
		timer->respawn = respawn;
		timer->item = entity->modelindex;
		timer->taken = 0;
		timer->present = itemFrame;
	}

	// A taken item does not stay in the snapshot as an invisible one, the
	// server unlinks it and it is simply gone. Whatever was there a moment ago
	// and is missing now has been picked up - unless a whole roomful goes at
	// once, which means we walked out of it rather than someone emptying it.
	for ( i = 0; i < MAX_GENTITIES; i++ ) {
		timer = &itemTimers[i];
		if ( timer->respawn && !timer->taken && timer->present == itemFrame - 1 ) {
			vanished++;
		}
	}

	for ( i = 0; i < MAX_GENTITIES; i++ ) {
		timer = &itemTimers[i];
		if ( !timer->respawn ) {
			continue;
		}

		if ( !timer->taken && timer->present == itemFrame - 1 ) {
			if ( vanished > 2 ) {
				timer->respawn = 0;			// out of sight, not taken
				continue;
			}
			timer->taken = cl.serverTime;
		}

		if ( !timer->taken && timer->present != itemFrame ) {
			continue;						// long gone and none of our business
		}

		left = timer->taken ? timer->respawn - ( cl.serverTime - timer->taken ) / 1000 : 0;
		if ( timer->taken && left < -3 ) {
			timer->respawn = 0;				// it should be back by now, we just cannot see it
			continue;
		}
		if ( left < 0 ) {
			left = 0;
		}

		for ( j = 0; j < 8; j++ ) {
			corner[j][0] = timer->origin[0] + ( ( j & 1 ) ? maxs[0] : mins[0] );
			corner[j][1] = timer->origin[1] + ( ( j & 2 ) ? maxs[1] : mins[1] );
			corner[j][2] = timer->origin[2] + ( ( j & 4 ) ? maxs[2] : mins[2] );
		}

		ahead = qtrue;
		for ( j = 0; j < 8 && ahead; j++ ) {
			ahead = CL_BotOutlineNearPoint( corner[j], near[j] );
		}
		if ( !ahead ) {
			continue;
		}

		CL_ItemOutlineColour( &bg_itemlist[timer->item], timer->taken != 0, colour );
		CL_BotOutlineWireBox( near, colour );

		if ( !timer->taken ) {
			continue;						// no clock on something that is there
		}

		VectorCopy( timer->origin, top );
		top[2] += maxs[2] + 12.0f;
		if ( CL_ProjectToScreen( top, &timer->x, &timer->y ) ) {
			Com_sprintf( timer->label, sizeof( timer->label ), "%i", left );
			timer->labelFrame = itemFrame;
			itemLabels++;
		}
	}
}


/*
====================
CL_DrawItemTimers

The countdowns over the finished picture.
====================
*/
static void CL_ItemTimerColour( int seconds, vec4_t out ) {
	// Red while it is a long way off, through orange and yellow into green as
	// it is about to come back, so a glance at the number is enough.
	static const float	stop[4] = { 0.0f, 3.0f, 8.0f, 20.0f };
	static const vec3_t	shade[4] = {
		{ 0.35f, 1.00f, 0.40f },		// back any moment
		{ 1.00f, 0.90f, 0.25f },
		{ 1.00f, 0.55f, 0.15f },
		{ 1.00f, 0.25f, 0.25f },		// a long way off
	};
	float	part;
	int		i;

	out[3] = 1.0f;
	for ( i = 1; i < 4; i++ ) {
		if ( seconds < stop[i] ) {
			part = ( seconds - stop[i - 1] ) / ( stop[i] - stop[i - 1] );
			part = Com_Clamp( 0.0f, 1.0f, part );
			out[0] = shade[i - 1][0] + ( shade[i][0] - shade[i - 1][0] ) * part;
			out[1] = shade[i - 1][1] + ( shade[i][1] - shade[i - 1][1] ) * part;
			out[2] = shade[i - 1][2] + ( shade[i][2] - shade[i - 1][2] ) * part;
			return;
		}
	}

	VectorCopy( shade[3], out );
}

static void CL_DrawItemTimers( void ) {
	vec4_t				colour;
	itemTimer_t			*timer;
	float				size, x, y;
	int					i;

	if ( !itemLabels ) {
		return;
	}

	for ( i = 0; i < MAX_GENTITIES; i++ ) {
		timer = &itemTimers[i];
		// only what was worked out for this very frame: an item that went out
		// of sight must not leave its clock hanging in the air
		if ( !timer->taken || !timer->label[0] || timer->labelFrame != itemFrame ) {
			continue;
		}

		// The bigger letters are laid out on the 640x480 grid the menus use,
		// unlike the small ones, which take real pixels. The projection hands
		// out real pixels, so it has to be put on that grid first.
		size = 14.0f;
		x = timer->x * 640.0f / cls.glconfig.vidWidth;
		y = timer->y * 480.0f / cls.glconfig.vidHeight;

		CL_ItemTimerColour( atoi( timer->label ), colour );
		SCR_DrawStringExt( (int)( x - strlen( timer->label ) * size * 0.5f ),
			(int)( y - size ), size, timer->label, colour, qtrue, qfalse );
	}
}


/*
=====================
CL_CGameRendering
=====================
*/
void CL_CGameRendering( stereoFrame_t stereo ) {
	botOutlineViewValid = qfalse;		// the world view of this frame is yet to come

	VM_Call( cgvm, CG_DRAW_ACTIVE_FRAME, cl.serverTime, stereo, clc.demoplaying );
	VM_Debug( 0 );

	CL_DrawItemTimers();
	CL_DrawBotLabels();

	CL_CheckMissedHitSound();
}


/*
=================
CL_AdjustTimeDelta

Adjust the clients view of server time.

We attempt to have cl.serverTime exactly equal the server's view
of time plus the timeNudge, but with variable latencies over
the internet it will often need to drift a bit to match conditions.

Our ideal time would be to have the adjusted time approach, but not pass,
the very latest snapshot.

Adjustments are only made when a new snapshot arrives with a rational
latency, which keeps the adjustment process framerate independent and
prevents massive overadjustment during times of significant packet loss
or bursted delayed packets.
=================
*/

#define	RESET_TIME	500

void CL_AdjustTimeDelta( void ) {
	int		newDelta;
	int		deltaDelta;

	cl.newSnapshots = qfalse;

	// the delta never drifts when replaying a demo
	if ( clc.demoplaying ) {
		return;
	}

	newDelta = cl.snap.serverTime - cls.realtime;
	deltaDelta = abs( newDelta - cl.serverTimeDelta );

	if ( deltaDelta > RESET_TIME ) {
		cl.serverTimeDelta = newDelta;
		cl.oldServerTime = cl.snap.serverTime;	// FIXME: is this a problem for cgame?
		cl.serverTime = cl.snap.serverTime;
		if ( cl_showTimeDelta->integer ) {
			Com_Printf( "<RESET> " );
		}
	} else if ( deltaDelta > 100 ) {
		// fast adjust, cut the difference in half
		if ( cl_showTimeDelta->integer ) {
			Com_Printf( "<FAST> " );
		}
		cl.serverTimeDelta = ( cl.serverTimeDelta + newDelta ) >> 1;
	} else {
		// slow drift adjust, only move 1 or 2 msec

		// if any of the frames between this and the previous snapshot
		// had to be extrapolated, nudge our sense of time back a little
		// the granularity of +1 / -2 is too high for timescale modified frametimes
		if ( com_timescale->value == 0 || com_timescale->value == 1 ) {
			if ( cl.extrapolatedSnapshot ) {
				cl.extrapolatedSnapshot = qfalse;
				cl.serverTimeDelta -= 2;
			} else {
				// otherwise, move our sense of time forward to minimize total latency
				cl.serverTimeDelta++;
			}
		}
	}

	if ( cl_showTimeDelta->integer ) {
		Com_Printf( "%i ", cl.serverTimeDelta );
	}
}


/*
==================
CL_FirstSnapshot
==================
*/
void CL_FirstSnapshot( void ) {
	// ignore snapshots that don't have entities
	if ( cl.snap.snapFlags & SNAPFLAG_NOT_ACTIVE ) {
		return;
	}
	clc.state = CA_ACTIVE;

	// set the timedelta so we are exactly on this first frame
	cl.serverTimeDelta = cl.snap.serverTime - cls.realtime;
	cl.oldServerTime = cl.snap.serverTime;

	clc.timeDemoBaseTime = cl.snap.serverTime;

	// if this is the first frame of active play,
	// execute the contents of activeAction now
	// this is to allow scripting a timedemo to start right
	// after loading
	if ( cl_activeAction->string[0] ) {
		Cbuf_AddText( cl_activeAction->string );
		Cvar_Set( "activeAction", "" );
	}

#ifdef USE_MUMBLE
	if ((cl_useMumble->integer) && !mumble_islinked()) {
		int ret = mumble_link(CLIENT_WINDOW_TITLE);
		Com_Printf("Mumble: Linking to Mumble application %s\n", ret==0?"ok":"failed");
	}
#endif

#ifdef USE_VOIP
	if (!clc.voipCodecInitialized) {
		int i;
		int error;

		clc.opusEncoder = opus_encoder_create(48000, 1, OPUS_APPLICATION_VOIP, &error);

		if ( error ) {
			Com_DPrintf("VoIP: Error opus_encoder_create %d\n", error);
			return;
		}

		for (i = 0; i < MAX_CLIENTS; i++) {
			clc.opusDecoder[i] = opus_decoder_create(48000, 1, &error);
			if ( error ) {
				Com_DPrintf("VoIP: Error opus_decoder_create(%d) %d\n", i, error);
				return;
			}
			clc.voipIgnore[i] = qfalse;
			clc.voipGain[i] = 1.0f;
		}
		clc.voipCodecInitialized = qtrue;
		clc.voipMuteAll = qfalse;
		Cmd_AddCommand ("voip", CL_Voip_f);
		Cvar_Set("cl_voipSendTarget", "spatial");
		Com_Memset(clc.voipTargets, ~0, sizeof(clc.voipTargets));
	}
#endif
}

/*
==================
CL_SetCGameTime
==================
*/
void CL_SetCGameTime( void ) {
	// getting a valid frame message ends the connection process
	if ( clc.state != CA_ACTIVE ) {
		if ( clc.state != CA_PRIMED ) {
			return;
		}
		if ( clc.demoplaying ) {
			// we shouldn't get the first snapshot on the same frame
			// as the gamestate, because it causes a bad time skip
			if ( !clc.firstDemoFrameSkipped ) {
				clc.firstDemoFrameSkipped = qtrue;
				return;
			}
			CL_ReadDemoMessage();
		}
		if ( cl.newSnapshots ) {
			cl.newSnapshots = qfalse;
			CL_FirstSnapshot();
		}
		if ( clc.state != CA_ACTIVE ) {
			return;
		}
	}	

	// if we have gotten to this point, cl.snap is guaranteed to be valid
	if ( !cl.snap.valid ) {
		Com_Error( ERR_DROP, "CL_SetCGameTime: !cl.snap.valid" );
	}

	// allow pause in single player
	if ( sv_paused->integer && CL_CheckPaused() && com_sv_running->integer ) {
		// paused
		return;
	}

	if ( cl.snap.serverTime < cl.oldFrameServerTime ) {
		Com_Error( ERR_DROP, "cl.snap.serverTime < cl.oldFrameServerTime" );
	}
	cl.oldFrameServerTime = cl.snap.serverTime;


	// get our current view of time

	if ( clc.demoplaying && cl_freezeDemo->integer ) {
		// cl_freezeDemo is used to lock a demo in place for single frame advances

	} else {
		// cl_timeNudge is a user adjustable cvar that allows more
		// or less latency to be added in the interest of better 
		// smoothness or better responsiveness.
		int tn;
		
		tn = cl_timeNudge->integer;
		if (tn<-30) {
			tn = -30;
		} else if (tn>30) {
			tn = 30;
		}

		cl.serverTime = cls.realtime + cl.serverTimeDelta - tn;

		// guarantee that time will never flow backwards, even if
		// serverTimeDelta made an adjustment or cl_timeNudge was changed
		if ( cl.serverTime < cl.oldServerTime ) {
			cl.serverTime = cl.oldServerTime;
		}
		cl.oldServerTime = cl.serverTime;

		// note if we are almost past the latest frame (without timeNudge),
		// so we will try and adjust back a bit when the next snapshot arrives
		if ( cls.realtime + cl.serverTimeDelta >= cl.snap.serverTime - 5 ) {
			cl.extrapolatedSnapshot = qtrue;
		}
	}

	// if we have gotten new snapshots, drift serverTimeDelta
	// don't do this every frame, or a period of packet loss would
	// make a huge adjustment
	if ( cl.newSnapshots ) {
		CL_AdjustTimeDelta();
	}

	if ( !clc.demoplaying ) {
		return;
	}

	// if we are playing a demo back, we can just keep reading
	// messages from the demo file until the cgame definitely
	// has valid snapshots to interpolate between

	// a timedemo will always use a deterministic set of time samples
	// no matter what speed machine it is run on,
	// while a normal demo may have different time samples
	// each time it is played back
	if ( cl_timedemo->integer ) {
		int now = Sys_Milliseconds( );
		int frameDuration;

		if (!clc.timeDemoStart) {
			clc.timeDemoStart = clc.timeDemoLastFrame = now;
			clc.timeDemoMinDuration = INT_MAX;
			clc.timeDemoMaxDuration = 0;
		}

		frameDuration = now - clc.timeDemoLastFrame;
		clc.timeDemoLastFrame = now;

		// Ignore the first measurement as it'll always be 0
		if( clc.timeDemoFrames > 0 )
		{
			if( frameDuration > clc.timeDemoMaxDuration )
				clc.timeDemoMaxDuration = frameDuration;

			if( frameDuration < clc.timeDemoMinDuration )
				clc.timeDemoMinDuration = frameDuration;

			// 255 ms = about 4fps
			if( frameDuration > UCHAR_MAX )
				frameDuration = UCHAR_MAX;

			clc.timeDemoDurations[ ( clc.timeDemoFrames - 1 ) %
				MAX_TIMEDEMO_DURATIONS ] = frameDuration;
		}

		clc.timeDemoFrames++;
		cl.serverTime = clc.timeDemoBaseTime + clc.timeDemoFrames * 50;
	}

	while ( cl.serverTime >= cl.snap.serverTime ) {
		// feed another messag, which should change
		// the contents of cl.snap
		CL_ReadDemoMessage();
		if ( clc.state != CA_ACTIVE ) {
			return;		// end of demo
		}
	}

}



