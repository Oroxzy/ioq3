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
//

/*****************************************************************************
 * name:		ai_main.c
 *
 * desc:		Quake3 bot AI
 *
 * $Archive: /MissionPack/code/game/ai_main.c $
 *
 *****************************************************************************/


#include "g_local.h"
#include "../qcommon/q_shared.h"
#include "../botlib/botlib.h"		//bot lib interface
#include "../botlib/be_aas.h"
#include "../botlib/be_ea.h"
#include "../botlib/be_ai_char.h"
#include "../botlib/be_ai_chat.h"
#include "../botlib/be_ai_gen.h"
#include "../botlib/be_ai_goal.h"
#include "../botlib/be_ai_move.h"
#include "../botlib/be_ai_weap.h"
//
#include "ai_main.h"
#include "ai_dmq3.h"
#include "ai_chat.h"
#include "ai_cmd.h"
#include "ai_dmnet.h"
#include "ai_vcmd.h"

//
#include "chars.h"
#include "inv.h"
#include "syn.h"


//bot states
bot_state_t	*botstates[MAX_CLIENTS];
//number of bots
int numbots;
//floating point time
float floattime;
//time to do a regular update
float regularupdate_time;
//
int bot_interbreed;
int bot_interbreedmatchcount;
//
vmCvar_t bot_thinktime;
vmCvar_t bot_memorydump;
vmCvar_t bot_saveroutingcache;
vmCvar_t bot_pause;
vmCvar_t bot_report;
vmCvar_t bot_testsolid;
vmCvar_t bot_testclusters;
vmCvar_t bot_developer;
vmCvar_t bot_interbreedchar;
vmCvar_t bot_interbreedbots;
vmCvar_t bot_interbreedcycle;
vmCvar_t bot_interbreedwrite;


void ExitLevel( void );


/*
==================
BotAI_Print
==================
*/
void QDECL BotAI_Print(int type, char *fmt, ...) {
	char str[2048];
	va_list ap;

	va_start(ap, fmt);
	Q_vsnprintf(str, sizeof(str), fmt, ap);
	va_end(ap);

	switch(type) {
		case PRT_MESSAGE: {
			G_Printf("%s", str);
			break;
		}
		case PRT_WARNING: {
			G_Printf( S_COLOR_YELLOW "Warning: %s", str );
			break;
		}
		case PRT_ERROR: {
			G_Printf( S_COLOR_RED "Error: %s", str );
			break;
		}
		case PRT_FATAL: {
			G_Printf( S_COLOR_RED "Fatal: %s", str );
			break;
		}
		case PRT_EXIT: {
			G_Error( S_COLOR_RED "Exit: %s", str );
			break;
		}
		default: {
			G_Printf( "unknown print type\n" );
			break;
		}
	}
}


/*
==================
BotLogPrintf

Das Bot-Protokoll. Eine eigene Datei und nicht die Konsole: zehn Bots denken
zusammen hundertmal je Sekunde, das will niemand im Spiel mitlesen. Jede Zeile
wird sofort herausgeschrieben, weil der Messlauf den Server am Ende einfach
beendet - ein Puffer waere dann weg.

Die Zeilenarten, jeweils mit Spielzeit in Millisekunden und Clientnummer:

  T  ein Denkschritt: Knoten, Ort, Tempo, Boden, Leben, Ruestung, Waffe,
     Gegner, Ziel, Reiseart, Flaggen (BOTLOG_*), Knotenwechsel
  S  ein Knotenwechsel, mit dem Grund aus dem Quelltext
  G  eine Zielwahl: L Fernziel, N Nahziel, - keines gefunden
  I  ein Gegenstand wurde genommen: Entitaet, von wem, Zeit bis zur
     Wiederkehr, Klasse, und ob ihn jemand fallen liess (dann kommt er nicht)
==================
*/
static fileHandle_t	botLogFile;
static qboolean		botLogTried;

void QDECL BotLogPrintf( const char *fmt, ... ) {
	char	str[512];
	va_list	ap;

	if ( !g_botLog.integer ) {
		return;
	}
	if ( !botLogFile ) {
		if ( botLogTried ) {
			return;
		}
		botLogTried = qtrue;
		trap_FS_FOpenFile( "botlog.log", &botLogFile, FS_APPEND_SYNC );
		if ( !botLogFile ) {
			G_Printf( "bot log: botlog.log laesst sich nicht oeffnen\n" );
			return;
		}
	}
	va_start( ap, fmt );
	Q_vsnprintf( str, sizeof( str ), fmt, ap );
	va_end( ap );
	trap_FS_Write( str, strlen( str ), botLogFile );
}

static void BotLogClose( void ) {
	if ( botLogFile ) {
		trap_FS_FCloseFile( botLogFile );
		botLogFile = 0;
	}
	botLogTried = qfalse;
}

/*
==================
BotAI_Trace
==================
*/
void BotAI_Trace(bsp_trace_t *bsptrace, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int passent, int contentmask) {
	trace_t trace;

	trap_Trace(&trace, start, mins, maxs, end, passent, contentmask);
	//copy the trace information
	bsptrace->allsolid = trace.allsolid;
	bsptrace->startsolid = trace.startsolid;
	bsptrace->fraction = trace.fraction;
	VectorCopy(trace.endpos, bsptrace->endpos);
	bsptrace->plane.dist = trace.plane.dist;
	VectorCopy(trace.plane.normal, bsptrace->plane.normal);
	bsptrace->plane.signbits = trace.plane.signbits;
	bsptrace->plane.type = trace.plane.type;
	bsptrace->surface.value = 0;
	bsptrace->surface.flags = trace.surfaceFlags;
	bsptrace->ent = trace.entityNum;
	bsptrace->exp_dist = 0;
	bsptrace->sidenum = 0;
	bsptrace->contents = 0;
}

/*
==================
BotAI_GetClientState
==================
*/
int BotAI_GetClientState( int clientNum, playerState_t *state ) {
	gentity_t	*ent;

	ent = &g_entities[clientNum];
	if ( !ent->inuse ) {
		return qfalse;
	}
	if ( !ent->client ) {
		return qfalse;
	}

	memcpy( state, &ent->client->ps, sizeof(playerState_t) );
	return qtrue;
}

/*
==================
BotAI_GetEntityState
==================
*/
int BotAI_GetEntityState( int entityNum, entityState_t *state ) {
	gentity_t	*ent;

	ent = &g_entities[entityNum];
	memset( state, 0, sizeof(entityState_t) );
	if (!ent->inuse) return qfalse;
	if (!ent->r.linked) return qfalse;
	if (ent->r.svFlags & SVF_NOCLIENT) return qfalse;
	memcpy( state, &ent->s, sizeof(entityState_t) );
	return qtrue;
}

/*
==================
BotAI_GetSnapshotEntity
==================
*/
int BotAI_GetSnapshotEntity( int clientNum, int sequence, entityState_t *state ) {
	int		entNum;

	entNum = trap_BotGetSnapshotEntity( clientNum, sequence );
	if ( entNum == -1 ) {
		memset(state, 0, sizeof(entityState_t));
		return -1;
	}

	BotAI_GetEntityState( entNum, state );

	return sequence + 1;
}

/*
==================
BotAI_BotInitialChat
==================
*/
void QDECL BotAI_BotInitialChat( bot_state_t *bs, char *type, ... ) {
	int		i, mcontext;
	va_list	ap;
	char	*p;
	char	*vars[MAX_MATCHVARIABLES];

	memset(vars, 0, sizeof(vars));
	va_start(ap, type);
	p = va_arg(ap, char *);
	for (i = 0; i < MAX_MATCHVARIABLES; i++) {
		if( !p ) {
			break;
		}
		vars[i] = p;
		p = va_arg(ap, char *);
	}
	va_end(ap);

	mcontext = BotSynonymContext(bs);

	trap_BotInitialChat( bs->cs, type, mcontext, vars[0], vars[1], vars[2], vars[3], vars[4], vars[5], vars[6], vars[7] );
}


/*
==================
BotTestAAS
==================
*/
void BotTestAAS(vec3_t origin) {
	int areanum;
	aas_areainfo_t info;

	trap_Cvar_Update(&bot_testsolid);
	trap_Cvar_Update(&bot_testclusters);
	if (bot_testsolid.integer) {
		if (!trap_AAS_Initialized()) return;
		areanum = BotPointAreaNum(origin);
		if (areanum) BotAI_Print(PRT_MESSAGE, "\rempty area");
		else BotAI_Print(PRT_MESSAGE, "\r^1SOLID area");
	}
	else if (bot_testclusters.integer) {
		if (!trap_AAS_Initialized()) return;
		areanum = BotPointAreaNum(origin);
		if (!areanum)
			BotAI_Print(PRT_MESSAGE, "\r^1Solid!                              ");
		else {
			trap_AAS_AreaInfo(areanum, &info);
			BotAI_Print(PRT_MESSAGE, "\rarea %d, cluster %d       ", areanum, info.cluster);
		}
	}
}

/*
==================
BotReportStatus
==================
*/
void BotReportStatus(bot_state_t *bs) {
	char goalname[MAX_MESSAGE_SIZE];
	char netname[MAX_MESSAGE_SIZE];
	char *leader, flagstatus[32];
	//
	ClientName(bs->client, netname, sizeof(netname));
	if (Q_stricmp(netname, bs->teamleader) == 0) leader = "L";
	else leader = " ";

	strcpy(flagstatus, "  ");
	if (gametype == GT_CTF) {
		if (BotCTFCarryingFlag(bs)) {
			if (BotTeam(bs) == TEAM_RED) strcpy(flagstatus, S_COLOR_RED"F ");
			else strcpy(flagstatus, S_COLOR_BLUE"F ");
		}
	}
#ifdef MISSIONPACK
	else if (gametype == GT_1FCTF) {
		if (Bot1FCTFCarryingFlag(bs)) {
			if (BotTeam(bs) == TEAM_RED) strcpy(flagstatus, S_COLOR_RED"F ");
			else strcpy(flagstatus, S_COLOR_BLUE"F ");
		}
	}
	else if (gametype == GT_HARVESTER) {
		if (BotHarvesterCarryingCubes(bs)) {
			if (BotTeam(bs) == TEAM_RED) Com_sprintf(flagstatus, sizeof(flagstatus), S_COLOR_RED"%2d", bs->inventory[INVENTORY_REDCUBE]);
			else Com_sprintf(flagstatus, sizeof(flagstatus), S_COLOR_BLUE"%2d", bs->inventory[INVENTORY_BLUECUBE]);
		}
	}
#endif

	switch(bs->ltgtype) {
		case LTG_TEAMHELP:
		{
			EasyClientName(bs->teammate, goalname, sizeof(goalname));
			BotAI_Print(PRT_MESSAGE, "%-20s%s%s: helping %s\n", netname, leader, flagstatus, goalname);
			break;
		}
		case LTG_TEAMACCOMPANY:
		{
			EasyClientName(bs->teammate, goalname, sizeof(goalname));
			BotAI_Print(PRT_MESSAGE, "%-20s%s%s: accompanying %s\n", netname, leader, flagstatus, goalname);
			break;
		}
		case LTG_DEFENDKEYAREA:
		{
			trap_BotGoalName(bs->teamgoal.number, goalname, sizeof(goalname));
			BotAI_Print(PRT_MESSAGE, "%-20s%s%s: defending %s\n", netname, leader, flagstatus, goalname);
			break;
		}
		case LTG_GETITEM:
		{
			trap_BotGoalName(bs->teamgoal.number, goalname, sizeof(goalname));
			BotAI_Print(PRT_MESSAGE, "%-20s%s%s: getting item %s\n", netname, leader, flagstatus, goalname);
			break;
		}
		case LTG_KILL:
		{
			ClientName(bs->teamgoal.entitynum, goalname, sizeof(goalname));
			BotAI_Print(PRT_MESSAGE, "%-20s%s%s: killing %s\n", netname, leader, flagstatus, goalname);
			break;
		}
		case LTG_CAMP:
		case LTG_CAMPORDER:
		{
			BotAI_Print(PRT_MESSAGE, "%-20s%s%s: camping\n", netname, leader, flagstatus);
			break;
		}
		case LTG_PATROL:
		{
			BotAI_Print(PRT_MESSAGE, "%-20s%s%s: patrolling\n", netname, leader, flagstatus);
			break;
		}
		case LTG_GETFLAG:
		{
			BotAI_Print(PRT_MESSAGE, "%-20s%s%s: capturing flag\n", netname, leader, flagstatus);
			break;
		}
		case LTG_RUSHBASE:
		{
			BotAI_Print(PRT_MESSAGE, "%-20s%s%s: rushing base\n", netname, leader, flagstatus);
			break;
		}
		case LTG_RETURNFLAG:
		{
			BotAI_Print(PRT_MESSAGE, "%-20s%s%s: returning flag\n", netname, leader, flagstatus);
			break;
		}
		case LTG_ATTACKENEMYBASE:
		{
			BotAI_Print(PRT_MESSAGE, "%-20s%s%s: attacking the enemy base\n", netname, leader, flagstatus);
			break;
		}
		case LTG_HARVEST:
		{
			BotAI_Print(PRT_MESSAGE, "%-20s%s%s: harvesting\n", netname, leader, flagstatus);
			break;
		}
		default:
		{
			BotAI_Print(PRT_MESSAGE, "%-20s%s%s: roaming\n", netname, leader, flagstatus);
			break;
		}
	}
}

/*
==================
BotTeamplayReport
==================
*/
void BotTeamplayReport(void) {
	int i;
	char buf[MAX_INFO_STRING];

	BotAI_Print(PRT_MESSAGE, S_COLOR_RED"RED\n");
	for (i = 0; i < level.maxclients; i++) {
		//
		if ( !botstates[i] || !botstates[i]->inuse ) continue;
		//
		trap_GetConfigstring(CS_PLAYERS+i, buf, sizeof(buf));
		//if no config string or no name
		if (!strlen(buf) || !strlen(Info_ValueForKey(buf, "n"))) continue;
		//skip spectators
		if (atoi(Info_ValueForKey(buf, "t")) == TEAM_RED) {
			BotReportStatus(botstates[i]);
		}
	}
	BotAI_Print(PRT_MESSAGE, S_COLOR_BLUE"BLUE\n");
	for (i = 0; i < level.maxclients; i++) {
		//
		if ( !botstates[i] || !botstates[i]->inuse ) continue;
		//
		trap_GetConfigstring(CS_PLAYERS+i, buf, sizeof(buf));
		//if no config string or no name
		if (!strlen(buf) || !strlen(Info_ValueForKey(buf, "n"))) continue;
		//skip spectators
		if (atoi(Info_ValueForKey(buf, "t")) == TEAM_BLUE) {
			BotReportStatus(botstates[i]);
		}
	}
}

/*
==================
BotSetInfoConfigString
==================
*/
void BotSetInfoConfigString(bot_state_t *bs) {
	char goalname[MAX_MESSAGE_SIZE];
	char netname[MAX_MESSAGE_SIZE];
	char action[MAX_MESSAGE_SIZE];
	char *leader, carrying[32], *cs;
	bot_goal_t goal;
	//
	ClientName(bs->client, netname, sizeof(netname));
	if (Q_stricmp(netname, bs->teamleader) == 0) leader = "L";
	else leader = " ";

	strcpy(carrying, "  ");
	if (gametype == GT_CTF) {
		if (BotCTFCarryingFlag(bs)) {
			strcpy(carrying, "F ");
		}
	}
#ifdef MISSIONPACK
	else if (gametype == GT_1FCTF) {
		if (Bot1FCTFCarryingFlag(bs)) {
			strcpy(carrying, "F ");
		}
	}
	else if (gametype == GT_HARVESTER) {
		if (BotHarvesterCarryingCubes(bs)) {
			if (BotTeam(bs) == TEAM_RED) Com_sprintf(carrying, sizeof(carrying), "%2d", bs->inventory[INVENTORY_REDCUBE]);
			else Com_sprintf(carrying, sizeof(carrying), "%2d", bs->inventory[INVENTORY_BLUECUBE]);
		}
	}
#endif

	switch(bs->ltgtype) {
		case LTG_TEAMHELP:
		{
			EasyClientName(bs->teammate, goalname, sizeof(goalname));
			Com_sprintf(action, sizeof(action), "helping %s", goalname);
			break;
		}
		case LTG_TEAMACCOMPANY:
		{
			EasyClientName(bs->teammate, goalname, sizeof(goalname));
			Com_sprintf(action, sizeof(action), "accompanying %s", goalname);
			break;
		}
		case LTG_DEFENDKEYAREA:
		{
			trap_BotGoalName(bs->teamgoal.number, goalname, sizeof(goalname));
			Com_sprintf(action, sizeof(action), "defending %s", goalname);
			break;
		}
		case LTG_GETITEM:
		{
			trap_BotGoalName(bs->teamgoal.number, goalname, sizeof(goalname));
			Com_sprintf(action, sizeof(action), "getting item %s", goalname);
			break;
		}
		case LTG_KILL:
		{
			ClientName(bs->teamgoal.entitynum, goalname, sizeof(goalname));
			Com_sprintf(action, sizeof(action), "killing %s", goalname);
			break;
		}
		case LTG_CAMP:
		case LTG_CAMPORDER:
		{
			Com_sprintf(action, sizeof(action), "camping");
			break;
		}
		case LTG_PATROL:
		{
			Com_sprintf(action, sizeof(action), "patrolling");
			break;
		}
		case LTG_GETFLAG:
		{
			Com_sprintf(action, sizeof(action), "capturing flag");
			break;
		}
		case LTG_RUSHBASE:
		{
			Com_sprintf(action, sizeof(action), "rushing base");
			break;
		}
		case LTG_RETURNFLAG:
		{
			Com_sprintf(action, sizeof(action), "returning flag");
			break;
		}
		case LTG_ATTACKENEMYBASE:
		{
			Com_sprintf(action, sizeof(action), "attacking the enemy base");
			break;
		}
		case LTG_HARVEST:
		{
			Com_sprintf(action, sizeof(action), "harvesting");
			break;
		}
		default:
		{
			trap_BotGetTopGoal(bs->gs, &goal);
			trap_BotGoalName(goal.number, goalname, sizeof(goalname));
			Com_sprintf(action, sizeof(action), "roaming %s", goalname);
			break;
		}
	}
  	cs = va("l\\%s\\c\\%s\\a\\%s",
				leader,
				carrying,
				action);
  	trap_SetConfigstring (CS_BOTINFO + bs->client, cs);
}

/*
==============
BotUpdateInfoConfigStrings
==============
*/
void BotUpdateInfoConfigStrings(void) {
	int i;
	char buf[MAX_INFO_STRING];

	for (i = 0; i < level.maxclients; i++) {
		//
		if ( !botstates[i] || !botstates[i]->inuse )
			continue;
		//
		trap_GetConfigstring(CS_PLAYERS+i, buf, sizeof(buf));
		//if no config string or no name
		if (!strlen(buf) || !strlen(Info_ValueForKey(buf, "n")))
			continue;
		BotSetInfoConfigString(botstates[i]);
	}
}

/*
==============
BotInterbreedBots
==============
*/
void BotInterbreedBots(void) {
	float ranks[MAX_CLIENTS];
	int parent1, parent2, child;
	int i;

	// get rankings for all the bots
	for (i = 0; i < MAX_CLIENTS; i++) {
		if ( botstates[i] && botstates[i]->inuse ) {
			ranks[i] = botstates[i]->num_kills * 2 - botstates[i]->num_deaths;
		}
		else {
			ranks[i] = -1;
		}
	}

	if (trap_GeneticParentsAndChildSelection(MAX_CLIENTS, ranks, &parent1, &parent2, &child)) {
		trap_BotInterbreedGoalFuzzyLogic(botstates[parent1]->gs, botstates[parent2]->gs, botstates[child]->gs);
		trap_BotMutateGoalFuzzyLogic(botstates[child]->gs, 1);
	}
	// reset the kills and deaths
	for (i = 0; i < MAX_CLIENTS; i++) {
		if (botstates[i] && botstates[i]->inuse) {
			botstates[i]->num_kills = 0;
			botstates[i]->num_deaths = 0;
		}
	}
}

/*
==============
BotWriteInterbreeded
==============
*/
void BotWriteInterbreeded(char *filename) {
	float rank, bestrank;
	int i, bestbot;

	bestrank = 0;
	bestbot = -1;
	// get the best bot
	for (i = 0; i < MAX_CLIENTS; i++) {
		if ( botstates[i] && botstates[i]->inuse ) {
			rank = botstates[i]->num_kills * 2 - botstates[i]->num_deaths;
		}
		else {
			rank = -1;
		}
		if (rank > bestrank) {
			bestrank = rank;
			bestbot = i;
		}
	}
	if (bestbot >= 0) {
		//write out the new goal fuzzy logic
		trap_BotSaveGoalFuzzyLogic(botstates[bestbot]->gs, filename);
	}
}

/*
==============
BotInterbreedEndMatch

add link back into ExitLevel?
==============
*/
void BotInterbreedEndMatch(void) {

	if (!bot_interbreed) return;
	bot_interbreedmatchcount++;
	if (bot_interbreedmatchcount >= bot_interbreedcycle.integer) {
		bot_interbreedmatchcount = 0;
		//
		trap_Cvar_Update(&bot_interbreedwrite);
		if (strlen(bot_interbreedwrite.string)) {
			BotWriteInterbreeded(bot_interbreedwrite.string);
			trap_Cvar_Set("bot_interbreedwrite", "");
		}
		BotInterbreedBots();
	}
}

/*
==============
BotInterbreeding
==============
*/
void BotInterbreeding(void) {
	int i;

	trap_Cvar_Update(&bot_interbreedchar);
	if (!strlen(bot_interbreedchar.string)) return;
	//make sure we are in tournament mode
	if (gametype != GT_TOURNAMENT) {
		trap_Cvar_Set("g_gametype", va("%d", GT_TOURNAMENT));
		ExitLevel();
		return;
	}
	//shutdown all the bots
	for (i = 0; i < MAX_CLIENTS; i++) {
		if (botstates[i] && botstates[i]->inuse) {
			BotAIShutdownClient(botstates[i]->client, qfalse);
		}
	}
	//make sure all item weight configs are reloaded and Not shared
	trap_BotLibVarSet("bot_reloadcharacters", "1");
	//add a number of bots using the desired bot character
	for (i = 0; i < bot_interbreedbots.integer; i++) {
		trap_SendConsoleCommand( EXEC_INSERT, va("addbot %s 4 free %i %s%d\n",
						bot_interbreedchar.string, i * 50, bot_interbreedchar.string, i) );
	}
	//
	trap_Cvar_Set("bot_interbreedchar", "");
	bot_interbreed = qtrue;
}

/*
==============
BotEntityInfo
==============
*/
void BotEntityInfo(int entnum, aas_entityinfo_t *info) {
	trap_AAS_EntityInfo(entnum, info);
}

/*
==============
NumBots
==============
*/
int NumBots(void) {
	return numbots;
}

/*
==============
BotTeamLeader
==============
*/
int BotTeamLeader(bot_state_t *bs) {
	int leader;

	leader = ClientFromName(bs->teamleader);
	if (leader < 0) return qfalse;
	if (!botstates[leader] || !botstates[leader]->inuse) return qfalse;
	return qtrue;
}

/*
==============
AngleDifference
==============
*/
float AngleDifference(float ang1, float ang2) {
	float diff;

	diff = ang1 - ang2;
	if (ang1 > ang2) {
		if (diff > 180.0) diff -= 360.0;
	}
	else {
		if (diff < -180.0) diff += 360.0;
	}
	return diff;
}

/*
==============
BotChangeViewAngle
==============
*/
float BotChangeViewAngle(float angle, float ideal_angle, float speed) {
	float move;

	angle = AngleMod(angle);
	ideal_angle = AngleMod(ideal_angle);
	if (angle == ideal_angle) return angle;
	move = ideal_angle - angle;
	if (ideal_angle > angle) {
		if (move > 180.0) move -= 360.0;
	}
	else {
		if (move < -180.0) move += 360.0;
	}
	if (move > 0) {
		if (move > speed) move = speed;
	}
	else {
		if (move < -speed) move = -speed;
	}
	return AngleMod(angle + move);
}

/*
==============
BotChangeViewAngles
==============
*/
void BotChangeViewAngles(bot_state_t *bs, float thinktime) {
	float diff, factor, maxchange, anglespeed, disired_speed;
	int i;

	if (bs->ideal_viewangles[PITCH] > 180) bs->ideal_viewangles[PITCH] -= 360;
	//
	if (bs->enemy >= 0) {
		factor = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_VIEW_FACTOR, 0.01f, 1);
		maxchange = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_VIEW_MAXCHANGE, 1, 1800);
	}
	else {
		factor = 0.05f;
		maxchange = 360;
	}
	if (maxchange < 240) maxchange = 240;
	maxchange *= thinktime;
	for (i = 0; i < 2; i++) {
		//
		if (bot_challenge.integer) {
			//smooth slowdown view model
			diff = fabs(AngleDifference(bs->viewangles[i], bs->ideal_viewangles[i]));
			anglespeed = diff * factor;
			if (anglespeed > maxchange) anglespeed = maxchange;
			bs->viewangles[i] = BotChangeViewAngle(bs->viewangles[i],
											bs->ideal_viewangles[i], anglespeed);
		}
		else {
			//over reaction view model
			bs->viewangles[i] = AngleMod(bs->viewangles[i]);
			bs->ideal_viewangles[i] = AngleMod(bs->ideal_viewangles[i]);
			diff = AngleDifference(bs->viewangles[i], bs->ideal_viewangles[i]);
			disired_speed = diff * factor;
			bs->viewanglespeed[i] += (bs->viewanglespeed[i] - disired_speed);
			if (bs->viewanglespeed[i] > 180) bs->viewanglespeed[i] = maxchange;
			if (bs->viewanglespeed[i] < -180) bs->viewanglespeed[i] = -maxchange;
			anglespeed = bs->viewanglespeed[i];
			if (anglespeed > maxchange) anglespeed = maxchange;
			if (anglespeed < -maxchange) anglespeed = -maxchange;
			bs->viewangles[i] += anglespeed;
			bs->viewangles[i] = AngleMod(bs->viewangles[i]);
			//demping
			bs->viewanglespeed[i] *= 0.45 * (1 - factor);
		}
		//BotAI_Print(PRT_MESSAGE, "ideal_angles %f %f\n", bs->ideal_viewangles[0], bs->ideal_viewangles[1], bs->ideal_viewangles[2]);`
		//bs->viewangles[i] = bs->ideal_viewangles[i];
	}
	//bs->viewangles[PITCH] = 0;
	if (bs->viewangles[PITCH] > 180) bs->viewangles[PITCH] -= 360;
	//elementary action: view
	trap_EA_View(bs->client, bs->viewangles);
}

/*
==============
BotInputToUserCommand
==============
*/
void BotInputToUserCommand(bot_input_t *bi, usercmd_t *ucmd, int delta_angles[3], int time) {
	vec3_t angles, forward, right;
	short temp;
	int j;
	float f, r, u, m;

	//clear the whole structure
	memset(ucmd, 0, sizeof(usercmd_t));
	//the duration for the user command in milli seconds
	ucmd->serverTime = time;
	//
	if (bi->actionflags & ACTION_DELAYEDJUMP) {
		bi->actionflags |= ACTION_JUMP;
		bi->actionflags &= ~ACTION_DELAYEDJUMP;
	}
	//set the buttons
	if (bi->actionflags & ACTION_RESPAWN) ucmd->buttons = BUTTON_ATTACK;
	if (bi->actionflags & ACTION_ATTACK) ucmd->buttons |= BUTTON_ATTACK;
	if (bi->actionflags & ACTION_TALK) ucmd->buttons |= BUTTON_TALK;
	if (bi->actionflags & ACTION_GESTURE) ucmd->buttons |= BUTTON_GESTURE;
	if (bi->actionflags & ACTION_USE) ucmd->buttons |= BUTTON_USE_HOLDABLE;
	if (bi->actionflags & ACTION_WALK) ucmd->buttons |= BUTTON_WALKING;
	if (bi->actionflags & ACTION_AFFIRMATIVE) ucmd->buttons |= BUTTON_AFFIRMATIVE;
	if (bi->actionflags & ACTION_NEGATIVE) ucmd->buttons |= BUTTON_NEGATIVE;
	if (bi->actionflags & ACTION_GETFLAG) ucmd->buttons |= BUTTON_GETFLAG;
	if (bi->actionflags & ACTION_GUARDBASE) ucmd->buttons |= BUTTON_GUARDBASE;
	if (bi->actionflags & ACTION_PATROL) ucmd->buttons |= BUTTON_PATROL;
	if (bi->actionflags & ACTION_FOLLOWME) ucmd->buttons |= BUTTON_FOLLOWME;
	//
	ucmd->weapon = bi->weapon;
	//set the view angles
	//NOTE: the ucmd->angles are the angles WITHOUT the delta angles
	ucmd->angles[PITCH] = ANGLE2SHORT(bi->viewangles[PITCH]);
	ucmd->angles[YAW] = ANGLE2SHORT(bi->viewangles[YAW]);
	ucmd->angles[ROLL] = ANGLE2SHORT(bi->viewangles[ROLL]);
	//subtract the delta angles
	for (j = 0; j < 3; j++) {
		temp = ucmd->angles[j] - delta_angles[j];
		/*NOTE: disabled because temp should be mod first
		if ( j == PITCH ) {
			// don't let the player look up or down more than 90 degrees
			if ( temp > 16000 ) temp = 16000;
			else if ( temp < -16000 ) temp = -16000;
		}
		*/
		ucmd->angles[j] = temp;
	}
	//NOTE: movement is relative to the REAL view angles
	//get the horizontal forward and right vector
	//get the pitch in the range [-180, 180]
	if (bi->dir[2]) angles[PITCH] = bi->viewangles[PITCH];
	else angles[PITCH] = 0;
	angles[YAW] = bi->viewangles[YAW];
	angles[ROLL] = 0;
	AngleVectors(angles, forward, right, NULL);
	//bot input speed is in the range [0, 400]
	bi->speed = bi->speed * 127 / 400;
	//set the view independent movement
	f = DotProduct(forward, bi->dir);
	r = DotProduct(right, bi->dir);
	u = fabs(forward[2]) * bi->dir[2];
	m = fabs(f);

	if (fabs(r) > m) {
		m = fabs(r);
	}

	if (fabs(u) > m) {
		m = fabs(u);
	}

	if (m > 0) {
		f *= bi->speed / m;
		r *= bi->speed / m;
		u *= bi->speed / m;
	}

	ucmd->forwardmove = f;
	ucmd->rightmove = r;
	ucmd->upmove = u;

	if (bi->actionflags & ACTION_MOVEFORWARD) ucmd->forwardmove = 127;
	if (bi->actionflags & ACTION_MOVEBACK) ucmd->forwardmove = -127;
	if (bi->actionflags & ACTION_MOVELEFT) ucmd->rightmove = -127;
	if (bi->actionflags & ACTION_MOVERIGHT) ucmd->rightmove = 127;
	//jump/moveup
	if (bi->actionflags & ACTION_JUMP) ucmd->upmove = 127;
	//crouch/movedown
	if (bi->actionflags & ACTION_CROUCH) ucmd->upmove = -127;
}

/*
==============
BotUpdateInput
==============
*/
/*
==================
BotItemWorthWaiting

Worauf auch ein Mensch wartet: Powerups, Ruestung, die grossen Medipacks, und
eine Waffe, die er noch nicht hat. Ohne Bot gefragt (NULL) zaehlt jede Waffe.
Fuer eine Munitionskiste bleibt niemand stehen.
==================
*/
qboolean BotItemWorthWaiting( bot_state_t *bs, gitem_t *item ) {
	switch ( item->giType ) {
		case IT_WEAPON:
			if ( !bs ) {
				return qtrue;
			}
			// Das lebende Inventar, nicht die Kopie vom letzten Denkschritt:
			// wer die Waffe eben genommen hat, traegt sie schon.
			if ( g_entities[bs->client].client ) {
				return !( g_entities[bs->client].client->ps.stats[STAT_WEAPONS] & ( 1 << item->giTag ) );
			}
			return !( bs->cur_ps.stats[STAT_WEAPONS] & ( 1 << item->giTag ) );
		case IT_POWERUP:
		case IT_HOLDABLE:
			return qtrue;
		case IT_ARMOR:
			return ( item->quantity >= 25 );
		case IT_HEALTH:
			return ( item->quantity >= 50 );
		default:
			return qfalse;
	}
}

/*
==================
BotItemTaken

Jemand hat einen Gegenstand genommen - allen Bots sagen, wann er wiederkommt.

Bisher wusste das niemand ausser dem, der ihn genommen hat: die einzige
Respawn-Kenntnis eines Bots ist seine private Vermeidungsliste, und die wird
nur scharf, wenn ER den Gegenstand angefasst oder ausgewaehlt hat. Nimmt der
Mensch das Quad, bewertet jeder Bot es weiterhin, als laege es da - die
Anwesenheitspruefung in BotChooseLTGItem fragt li->entitynum ab, und das wird
nach dem Verknuepfen nie wieder geloescht. Die Bots laufen also zu einer
leeren Stelle, und wenn der Gegenstand wiederkommt, steht keiner dort.

Dabei kennt das Spielmodul den Zeitpunkt auf die Millisekunde genau - es gibt
ihn nur nicht weiter. Genau das passiert hier.

Zwei Sekunden Vorlauf: die Sperre endet etwas frueher als der Gegenstand
kommt, damit ein Bot sich rechtzeitig auf den Weg macht statt erst loszugehen,
wenn das Ding schon wieder liegt. Das ist die billige Naeherung an das, was
ein Mensch tut, wenn er mitzaehlt.
==================
*/
/*
==================
BotDamagedBy / BotRecentDamage

Wer einen Bot in den letzten anderthalb Sekunden wie stark getroffen hat. Ein
Fenster, kein Gedaechtnis: ein Treffer, der laenger zurueckliegt, zaehlt nicht
mehr, wie bei einem Menschen, der sich nach dem letzten Einschlag richtet.
==================
*/
#define RETALIATE_WINDOW	1.5f

void BotDamagedBy( int target, int attacker, int amount ) {
	bot_state_t	*bs;

	if ( target < 0 || target >= MAX_CLIENTS || attacker < 0 || attacker >= MAX_CLIENTS ) {
		return;
	}
	bs = botstates[target];
	if ( !bs || !bs->inuse ) {
		return;
	}
	if ( FloatTime() - bs->hurt_time[attacker] > RETALIATE_WINDOW ) {
		bs->hurt_amount[attacker] = 0;
	}
	bs->hurt_amount[attacker] += amount;
	bs->hurt_time[attacker] = FloatTime();
}

int BotRecentDamage( bot_state_t *bs, int attacker ) {
	if ( attacker < 0 || attacker >= MAX_CLIENTS ) {
		return 0;
	}
	if ( FloatTime() - bs->hurt_time[attacker] > RETALIATE_WINDOW ) {
		return 0;
	}
	return bs->hurt_amount[attacker];
}

void BotItemTaken( gentity_t *ent, gentity_t *other, float respawn ) {
	bot_goal_t	goal;
	int			i, index;
	float		avoid;

	if ( !ent->item || !ent->item->pickup_name ) {
		return;
	}
	BotLogPrintf( "I %i %i %i %.0f %s %i\n", level.time, ent->s.number,
		other ? other->s.number : -1, respawn, ent->item->classname,
		( ent->flags & FL_DROPPED_ITEM ) ? 1 : 0 );
	if ( !g_botTiming.integer ) {
		return;
	}
	// Die Level-Item-Nummer zu dieser Entitaet suchen. Der Name ist der
	// Aufsammelname ("Quad Damage"), nicht der Klassenname.
	index = trap_BotGetLevelItemGoal( -1, ent->item->pickup_name, &goal );
	while ( index >= 0 && goal.entitynum != ent->s.number ) {
		index = trap_BotGetLevelItemGoal( index, ent->item->pickup_name, &goal );
	}
	if ( index < 0 ) {
		return;				// steht nicht in der Gegenstandsliste der Karte
	}

	// Der Vorlauf von zwei Sekunden gilt nur fuer das, worauf ein Bot auch
	// wartet (BotItemWorthWaiting). Fuer alles andere waere er verkehrt herum:
	// der Bot kaeme zwei Sekunden zu frueh, saehe die leere Stelle, hielte das
	// Ziel fuer erledigt und ginge wieder - gemessen auf q3dm6 acht solcher
	// Wechsel je Bot und Minute.
	// Je Bot gefragt: wer die Waffe schon traegt, wartet nicht auf sie.
	// Was ein Toter fallen liess, kommt nicht wieder. botlib fuehrt es trotzdem
	// dreissig Sekunden lang weiter in seiner Liste, und jeder Bot laeuft
	// einmal hin, um nachzusehen - gemessen war "das Ziel ist weg" der Grund
	// fuer zwei von drei Zielwahlen.
	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( !botstates[i] || !botstates[i]->inuse || BotStock( botstates[i] ) ) {
			continue;
		}
		if ( ent->flags & FL_DROPPED_ITEM ) {
			avoid = 30.0f;
		} else if ( BotItemWorthWaiting( botstates[i], ent->item ) ) {
			avoid = respawn - 2.0f;
		} else {
			avoid = respawn + 0.3f;
		}
		if ( avoid < 1.0f ) {
			avoid = 1.0f;
		}
		trap_BotSetAvoidGoalTime( botstates[i]->gs, goal.number, avoid );
	}
}

// Ist in dieser Richtung, so weit voraus, ueberhaupt Boden? Tausend Einheiten
// tief gesucht: ein gewollter Absatz ist auf q3dm17 im Mittel 174 tief, alles
// darunter ist die Leere. Steckt die Probe in einer Wand, zaehlt das als Boden
// - dann laeuft der Bot ohnehin nicht weiter.
//
// Gesucht wird mit der Standflaeche des Koerpers, nicht mit einem Punkt. Die
// erste Fassung nahm den Punkt, und der faellt durch jede Luecke eines
// Gitterbodens: gemessen fand der Koerper bei 190 von 316 Bremsungen Boden,
// wo der Punkt keinen sah - sechzig Prozent der Bremsungen waren falsch.
//
// Und Boden ist nur, worauf man stehen bleiben kann. Auf q3dm17 ist die Grube
// bodenlos, da genuegte "ist ueberhaupt etwas darunter". Der Lauf ueber alle
// fuenfundzwanzig Karten hat gezeigt, dass das die Ausnahme ist: anderswo hat
// die Grube einen Boden, vierhundert Einheiten tiefer und mit einem
// trigger_hurt darueber, oder der Boden ist der Grund eines Lavasees. Beides
// galt als Boden - der Ausweichschritt ging hinein, und die Bremse liess den
// Bot hineinlaufen. Gemessen auf q3dm18: 37 Tode in der Grube gegen 22 beim
// Original; auf q3dm13 sieben in der Lava gegen keinen.
static qboolean BotHurtTriggerInBox( vec3_t mins, vec3_t maxs ) {
	int			touch[32], n, i;
	gentity_t	*hit;

	n = trap_EntitiesInBox( mins, maxs, touch, 32 );
	for ( i = 0; i < n; i++ ) {
		hit = &g_entities[touch[i]];
		if ( !hit->classname || strcmp( hit->classname, "trigger_hurt" ) ) {
			continue;
		}
		// der Kasten der Entitaet ist groesser als sie selbst
		if ( trap_EntityContact( mins, maxs, hit ) ) {
			return qtrue;
		}
	}
	return qfalse;
}

// Die Rechnung sieht durch andere Spieler hindurch. Sie stehen dort, wo sie
// jetzt sind, und in einer halben Sekunde woanders - fuer die Rechnung waren
// sie aber Waende. Gemessen auf q3tourney6: 22 der 76 als Absturz
// vorhergesagten Fluege vom Sprungfeld prallten in der Rechnung an einem Bot
// ab, der im echten Flug laengst weg war.
#define FOOT_MASK		( MASK_PLAYERSOLID & ~CONTENTS_BODY )

static qboolean BotSafeGroundBelow( bot_state_t *bs, vec3_t ahead ) {
	vec3_t		below, spot, cmins, cmaxs;
	vec3_t		mins = { -15, -15, 0 }, maxs = { 15, 15, 8 };
	trace_t		tr;

	VectorCopy( ahead, below );
	below[2] -= 1024.0f;
	trap_Trace( &tr, ahead, mins, maxs, below, bs->entitynum, FOOT_MASK );
	if ( tr.startsolid ) {
		return qtrue;
	}
	if ( tr.fraction >= 1.0f ) {
		return qfalse;
	}
	// der Grund eines Lava- oder Schleimsees
	VectorCopy( tr.endpos, spot );
	spot[2] += 4.0f;
	if ( trap_PointContents( spot, bs->entitynum ) & ( CONTENTS_LAVA | CONTENTS_SLIME ) ) {
		return qfalse;
	}
	// eine Todeszone irgendwo auf dem Weg hinunter
	VectorSet( cmins, ahead[0] - 15.0f, ahead[1] - 15.0f, tr.endpos[2] );
	VectorSet( cmaxs, ahead[0] + 15.0f, ahead[1] + 15.0f, ahead[2] );
	if ( BotHurtTriggerInBox( cmins, cmaxs ) ) {
		return qfalse;
	}
	return qtrue;
}

qboolean BotGroundAhead( bot_state_t *bs, vec3_t dir, float dist ) {
	vec3_t		ahead;

	VectorMA( bs->origin, dist, dir, ahead );
	ahead[2] = bs->origin[2] + 24.0f;
	return BotSafeGroundBelow( bs, ahead );
}

/*
==================
BotEnemySpotSafe

Ob man dem Gegner dorthin nachlaufen kann, wo er gerade ist.

Der Knoten merkt sich in jedem Denkschritt, wo der Gegner zuletzt war, als
Feld des Wegnetzes - und laeuft dorthin, wenn er ihn verfolgt oder die
Kampfbewegung nicht weiterweiss. Fliegt der Gegner in dem Moment ueber eine
Grube, ist sein Feld die Luft ueber der Grube, und der Weg dorthin endet, wo
die Luft aufhoert: auf q3dm9 ein Schritt von der Kante, 576 Einheiten tief,
waehrend der Verfolgte auf dem Sprungfeld laengst drueben war.

Ein Gegner in der Luft, unter dem kein sicherer Boden ist, wird deshalb nicht
gemerkt; es bleibt die letzte Stelle, an der er stand.
==================
*/
qboolean BotEnemySpotSafe( bot_state_t *bs, vec3_t origin ) {
	if ( !BotSw(bs, g_botEdgeCare) ) {
		return qtrue;
	}
	if ( bs->enemy >= 0 && bs->enemy < MAX_CLIENTS && g_entities[bs->enemy].client
		&& g_entities[bs->enemy].client->ps.groundEntityNum != ENTITYNUM_NONE ) {
		return qtrue;
	}
	return BotSafeGroundBelow( bs, origin );
}

/*
==================
BotPredictFooting

Wo der Bot landet, wenn er tut, was er vorhat - und ob man dort stehen kann.

Der Vorgaenger dieser Funktion war eine Bremse: geht es in Richtung des
Schwungs ins Nichts, laeuft der Bot rueckwaerts. Sie hat auf q3dm17 die Stuerze
fast halbiert (15,9 auf 9,1 Prozent), und der Lauf ueber alle Karten hat
gezeigt, wo sie danebenliegt. Mit der Todesursache im Bot-Protokoll (die
K-Zeile) liess sich fuer jeden Tod durch die Karte nachsehen, was der Bot in
den Sekunden davor getan hatte. Sechs Karten mit Grube oder Lava, zwoelf Bots,
vier Minuten, je Lauf:

                                        Original   Menschlich
  auf dem Weg gelaufen, nicht getroffen     16        25,5
  davon mit der Bremse kurz davor            -         6,5
  vom Treffer hinausgeworfen                14        18,5
  im Kampf gesprungen                       13        14,5
  im Kampf gelaufen                          5         7,5

Dreierlei steckt darin.

Die Bremse sah nur den Schwung. Auf q3dm9 biegt der Weg an einer Plattformecke
ab, auf einen Steg, von dem es planmaessig eine Etage tiefer geht. Der Bot hat
beim Abbiegen noch Schwung geradeaus, dort ist Leere, die Bremse schiebt ihn
zurueck - und er tritt einen halben Schritt neben dem Steg von der Kante. Von
acht Bots, die so ankamen, starben fuenf; die beiden, bei denen die Bremse
nicht griff, kamen heil hinunter. Dabei haette der Schwung ihn nie
hinausgetragen: auf dem Boden folgt die Geschwindigkeit dem Befehl binnen zwei
Bildern.

Der eigene Weg galt als sicher. Auf q3dm18 laufen die Bots - auch die des
Originals - mit vollem Tempo auf eine Treppe aus schwebenden Absaetzen und
fliegen ueber den ersten hinweg in die Tiefe; mit halbem Tempo landen sie
darauf.

Und die Kampfbewegung prueft ihre Spruenge mit dem Wegnetz, das keine
Todeszonen kennt: der Boden einer Grube ist dort ein Landeplatz wie jeder
andere.

Deshalb wird hier nicht mehr geraten, sondern gerechnet: die naechsten vier
Zehntelsekunden auf dem Boden mit Reibung und Beschleunigung wie im Spiel
(PM_Friction, PM_Accelerate), und wenn der Bot dabei den Boden verliert, der
Flug bis zur Landung, mit Schwerkraft und der Steuerung in der Luft. Sicher ist
eine Landung auf festem Grund, im Wasser, auf einem Sprungfeld oder in einem
Teleporter. Nicht sicher sind Lava, Schleim, eine Todeszone, drei Sekunden
freier Fall - und ein Aufprall, den der Bot mit seinem Leben nicht mehr
uebersteht.
==================
*/
#define FOOT_GROUNDFRAMES	8			// vier Zehntelsekunden auf dem Boden voraus
#define FOOT_SLOWFRAMES		14			// im Schritttempo dauert es laenger bis zur Kante
#define FOOT_AIRTIME		3.0f		// laenger faellt niemand, der noch landet
#define FOOT_FRAME			0.05f		// ein Bild des Servers
#define FOOT_JUMP			270.0f		// JUMP_VELOCITY aus bg_local.h
#define FOOT_SLOW			130.0f		// Schritttempo: reicht, um von einer Kante auf den Absatz darunter zu fallen

static vec3_t	footMins = { -15, -15, -24 };
static vec3_t	footMaxs = { 15, 15, 32 };
// wo die letzte Rechnung geendet hat und warum - nur fuer das Protokoll
static vec3_t	footEnd;
static int		footWhy;
// Sprungfelder: welches den Bot gerade abschiesst (die Rechnung beginnt darin
// und darf es nicht gleich wieder fuer einen sicheren Halt nehmen), welches
// sie zuletzt beruehrt hat, und ob die letzte Rechnung ueber einen Flug von
// einem Sprungfeld lief
static int		footSkipPad = -1;
static gentity_t	*footPadHit;
static int		footPadFlight;

// 0 nichts, 1 Todeszone, 2 Sprungfeld oder Teleporter
static int BotFootTrigger( vec3_t origin ) {
	int			touch[32], n, i, carried = 0;
	gentity_t	*hit;
	vec3_t		mins, maxs;

	footPadHit = NULL;
	VectorAdd( origin, footMins, mins );
	VectorAdd( origin, footMaxs, maxs );
	n = trap_EntitiesInBox( mins, maxs, touch, 32 );
	for ( i = 0; i < n; i++ ) {
		hit = &g_entities[touch[i]];
		if ( !( hit->r.contents & CONTENTS_TRIGGER ) ) {
			continue;
		}
		if ( hit->s.eType == ET_PUSH_TRIGGER || hit->s.eType == ET_TELEPORT_TRIGGER ) {
			if ( touch[i] != footSkipPad && trap_EntityContact( mins, maxs, hit ) ) {
				carried = 2;
				if ( hit->s.eType == ET_PUSH_TRIGGER ) {
					footPadHit = hit;
				}
			}
			continue;
		}
		// der Kasten der Entitaet ist groesser als sie selbst
		if ( hit->classname && !strcmp( hit->classname, "trigger_hurt" )
			&& trap_EntityContact( mins, maxs, hit ) ) {
			return 1;
		}
	}
	return carried;
}

// 0 nichts, 1 toedlich, 2 traegt (Wasser, Sprungfeld, Teleporter)
static int BotFootHazard( bot_state_t *bs, vec3_t origin ) {
	vec3_t		feet;
	int			pc;

	VectorCopy( origin, feet );
	feet[2] -= 23.0f;
	pc = trap_PointContents( feet, bs->entitynum );
	if ( pc & ( CONTENTS_LAVA | CONTENTS_SLIME ) ) {
		return 1;
	}
	if ( pc & CONTENTS_WATER ) {
		return 2;
	}
	return BotFootTrigger( origin );
}

/*
==================
BotFootLanded

Ob eine Landung eine ist - oder nur die Kante.

Die Rechnung landet, sobald irgendeine Ecke des Koerpers aufsetzt. Das Spiel
ist da weniger grosszuegig, und der Unterschied ist eine Einheit: gemessen auf
q3dm18 sah die Rechnung einen Kampfsprung bei (-770, 881) auf der Plattform
aufsetzen, der Bot kam an derselben Stelle auf Hoehe 23 statt 24 vorbei,
streifte die Kante von der Seite und fiel. Auf so etwas verlaesst sich auch
ein Mensch nicht. Gelandet ist deshalb nur, wer mit der Koerpermitte ueber
Boden aufsetzt und einen Schritt weiter in Flugrichtung auch noch welchen hat.
==================
*/
static qboolean BotFootLanded( bot_state_t *bs, vec3_t origin, vec3_t velocity ) {
	vec3_t		start, end, dir;
	trace_t		tr;

	VectorCopy( origin, end );
	end[2] -= 32.0f;
	trap_Trace( &tr, origin, NULL, NULL, end, bs->entitynum, FOOT_MASK );
	if ( !tr.startsolid && tr.fraction >= 1.0f ) {
		return qfalse;
	}
	VectorSet( dir, velocity[0], velocity[1], 0 );
	if ( VectorNormalize( dir ) < 1.0f ) {
		return qtrue;
	}
	VectorMA( origin, 20.0f, dir, start );
	VectorCopy( start, end );
	// eine Stufe tiefer darf es dort sein
	end[2] -= 48.0f;
	trap_Trace( &tr, start, NULL, NULL, end, bs->entitynum, FOOT_MASK );
	return ( tr.startsolid || tr.fraction < 1.0f );
}

// weiter mit dem Flug, den das naechste Sprungfeld gibt
#define BOTFOOT_CHAIN	do { VectorCopy( footPadHit->s.origin2, vel ); \
	footSkipPad = footPadHit->s.number; footPadFlight = 1; padflight = 1; \
	wishspeed = 0.0f; chain++; limit = t + FOOT_AIRTIME; } while ( 0 )

static qboolean BotPredictFootingRun( bot_state_t *bs, playerState_t *ps, vec3_t wishdir, float wishspeed, qboolean jump, int frames, int airwish ) {
	vec3_t		org, vel, wish, end, up;
	trace_t		tr, tr2;
	float		speed, control, cur, add, acc, t, oldz, delta, drop, step;
	int			frame, hazard, padflight = 0, chain = 0;
	float		limit = FOOT_AIRTIME;
	qboolean	onground;

	VectorCopy( ps->origin, org );
	VectorCopy( ps->velocity, vel );
	VectorCopy( wishdir, wish );
	wish[2] = 0.0f;
	if ( VectorNormalize( wish ) <= 0.0f || wishspeed < 0.0f ) {
		wishspeed = 0.0f;
	}
	onground = ( ps->groundEntityNum != ENTITYNUM_NONE );

	if ( onground ) {
		for ( frame = 0; frame < frames; frame++ ) {
			if ( jump ) {
				vel[2] = FOOT_JUMP;
				onground = qfalse;
				break;
			}
			// PM_Friction
			vel[2] = 0.0f;
			speed = sqrt( vel[0] * vel[0] + vel[1] * vel[1] );
			if ( speed < 1.0f ) {
				vel[0] = vel[1] = 0.0f;
			} else {
				control = ( speed < 100.0f ) ? 100.0f : speed;
				add = speed - control * 6.0f * FOOT_FRAME;
				if ( add < 0.0f ) {
					add = 0.0f;
				}
				vel[0] *= add / speed;
				vel[1] *= add / speed;
			}
			// PM_Accelerate, am Boden mit zehn
			if ( wishspeed > 0.0f ) {
				cur = vel[0] * wish[0] + vel[1] * wish[1];
				add = wishspeed - cur;
				if ( add > 0.0f ) {
					acc = 10.0f * FOOT_FRAME * wishspeed;
					if ( acc > add ) {
						acc = add;
					}
					vel[0] += acc * wish[0];
					vel[1] += acc * wish[1];
				}
			}
			VectorMA( org, FOOT_FRAME, vel, end );
			trap_Trace( &tr, org, footMins, footMaxs, end, bs->entitynum, FOOT_MASK );
			if ( tr.startsolid ) {
				VectorCopy( org, footEnd ); footWhy = 2;
				return qtrue;			// steckt fest: er bleibt, wo er ist
			}
			// eine Stufe tiefer ist noch derselbe Boden
			drop = 20.0f;
			if ( tr.fraction < 1.0f ) {
				// Etwas im Weg. Die erste Fassung hoerte hier auf - "er bleibt
				// auf dem Boden" - und war damit auf jeder Treppe und jeder Rampe
				// blind: dort ist in jedem Bild etwas im Weg, naemlich die
				// naechste Stufe. Gemessen auf q3dm10: die Rampe zum Sims ueber
				// der Lava, oben angekommen fliegt der Bot mit vollem Tempo
				// hinaus, und die Rechnung hatte bis zum letzten Bild nichts
				// gesehen. Also wie PM_StepSlideMove: hoch, vor, und die
				// Bodenprobe holt ihn wieder herunter.
				step = ( pmove_StepHeight.value > 0.0f ) ? pmove_StepHeight.value : 18.0f;
				VectorCopy( org, up );
				up[2] += step;
				trap_Trace( &tr2, org, footMins, footMaxs, up, bs->entitynum, FOOT_MASK );
				VectorCopy( tr2.endpos, up );
				VectorMA( up, FOOT_FRAME, vel, end );
				trap_Trace( &tr2, up, footMins, footMaxs, end, bs->entitynum, FOOT_MASK );
				if ( !tr2.startsolid && tr2.fraction > tr.fraction ) {
					VectorCopy( tr2.endpos, org );
					drop += step;
				} else {
					// eine Wand: daran entlang
					VectorCopy( tr.endpos, org );
					cur = vel[0] * tr.plane.normal[0] + vel[1] * tr.plane.normal[1];
					vel[0] -= cur * tr.plane.normal[0];
					vel[1] -= cur * tr.plane.normal[1];
				}
			} else {
				VectorCopy( end, org );
			}
			VectorCopy( org, end );
			end[2] -= drop;
			trap_Trace( &tr, org, footMins, footMaxs, end, bs->entitynum, FOOT_MASK );
			if ( tr.fraction >= 1.0f || tr.plane.normal[2] < 0.7f ) {
				onground = qfalse;
				break;
			}
			VectorCopy( tr.endpos, org );
			hazard = BotFootHazard( bs, org );
			// knoecheltiefes Wasser traegt nicht, darin laeuft man weiter -
			// auch ueber eine Kante
			if ( hazard == 2 && BotFootTrigger( org ) != 2 ) {
				hazard = 0;
			}
			// Ein Sprungfeld galt als sicher - es traegt ja. Auf q3tourney6
			// fuehrt der Flug aber durch die Quetschfalle, und wenn deren Rohre
			// unten sind, prallt der Bot daran ab und faellt: 22 von 22
			// Abstuerzen mit Einbruch im Flug, kein einziger an einem anderen
			// Bot. Also den Flug mitrechnen, den das Feld gibt (BG_TouchJumpPad
			// setzt genau s.origin2), durch alles, was jetzt im Weg steht.
			if ( hazard == 2 && footPadHit ) {
				VectorCopy( footPadHit->s.origin2, vel );
				footSkipPad = footPadHit->s.number;
				footPadFlight = 1;
				padflight = 1;
				onground = qfalse;
				break;
			}
			if ( hazard ) {
				VectorCopy( org, footEnd ); footWhy = 4 + hazard;
				return ( hazard == 2 );
			}
		}
		if ( onground ) {
			VectorCopy( org, footEnd ); footWhy = 1;
			return qtrue;
		}
	}

	// in der Luft: 0 ohne Befehl, 1 mit demselben, 2 mit vollem Tempo. Ein
	// Sprungfeld zielt so, dass man ohne Steuern ankommt (AimAtTarget).
	if ( !airwish || padflight ) {
		wishspeed = 0.0f;
	} else if ( airwish == 2 && ( wish[0] || wish[1] ) ) {
		wishspeed = ps->speed;
	}
	for ( t = 0.0f; t < limit; t += FOOT_FRAME ) {
		// PM_Accelerate, in der Luft mit eins
		if ( wishspeed > 0.0f ) {
			cur = vel[0] * wish[0] + vel[1] * wish[1];
			add = wishspeed - cur;
			if ( add > 0.0f ) {
				acc = ( pmove_AirAccel.value > 0.0f ? pmove_AirAccel.value : 1.0f ) * FOOT_FRAME * wishspeed;
				if ( acc > add ) {
					acc = add;
				}
				vel[0] += acc * wish[0];
				vel[1] += acc * wish[1];
			}
		}
		oldz = vel[2];
		vel[2] -= g_gravity.value * FOOT_FRAME;
		end[0] = org[0] + vel[0] * FOOT_FRAME;
		end[1] = org[1] + vel[1] * FOOT_FRAME;
		end[2] = org[2] + ( oldz + vel[2] ) * 0.5f * FOOT_FRAME;
		trap_Trace( &tr, org, footMins, footMaxs, end, bs->entitynum, FOOT_MASK );
		if ( tr.allsolid ) {
			VectorCopy( org, footEnd ); footWhy = 7;
			return qtrue;
		}
		VectorCopy( tr.endpos, org );
		if ( tr.fraction < 1.0f ) {
			if ( tr.plane.normal[2] >= 0.7f ) {
				// gelandet. PM_CrashLand: ab 40 fuenf Schaden, ab 60 zehn
				delta = vel[2] * vel[2] * 0.0001f;
				if ( ( delta > 60.0f && ps->stats[STAT_HEALTH] <= 10 )
					|| ( delta > 40.0f && ps->stats[STAT_HEALTH] <= 5 ) ) {
					VectorCopy( org, footEnd ); footWhy = 8;
					return qfalse;
				}
				// erst, was dort ist - ein Sprungfeld am Ziel eines Sprungfelds
				// traegt weiter, auch wenn der Bot auf dessen Kante aufsetzt
				hazard = BotFootHazard( bs, org );
				if ( hazard == 2 && footPadHit && chain < 3 ) {
					BOTFOOT_CHAIN;
					continue;
				}
				if ( hazard ) {
					VectorCopy( org, footEnd ); footWhy = 4 + hazard;
					return ( hazard == 2 );
				}
				if ( !BotFootLanded( bs, org, vel ) ) {
					VectorCopy( org, footEnd ); footWhy = 10;
					return qfalse;
				}
				VectorCopy( org, footEnd ); footWhy = 3;
				return qtrue;
			}
			// Wand oder Schraege: daran entlang
			cur = DotProduct( vel, tr.plane.normal );
			VectorMA( vel, -cur, tr.plane.normal, vel );
		}
		hazard = BotFootHazard( bs, org );
		// Ein zweites Sprungfeld im Flug wirft weiter. Auf q3tourney6 haengen
		// zwei hintereinander, und erst das zweite wirft in den Raum unter der
		// Quetschfalle - die Rechnung hoerte beim zweiten auf ("es traegt") und
		// sah die heruntergefahrenen Rohre nie: 25 Abstuerze in drei Laeufen,
		// alle an denselben zwei Stellen.
		if ( hazard == 2 && footPadHit && chain < 3 ) {
			BOTFOOT_CHAIN;
			continue;
		}
		if ( hazard ) {
			VectorCopy( org, footEnd ); footWhy = 4 + hazard;
			return ( hazard == 2 );
		}
	}
	VectorCopy( org, footEnd ); footWhy = 9;
	return qfalse;
}

static qboolean BotPredictFooting( bot_state_t *bs, playerState_t *ps, vec3_t wishdir, float wishspeed, qboolean jump, int frames, int airwish ) {
	int			saved = footSkipPad;
	qboolean	safe;

	footPadFlight = 0;
	safe = BotPredictFootingRun( bs, ps, wishdir, wishspeed, jump, frames, airwish );
	footSkipPad = saved;
	return safe;
}

/*
==================
BotFootSafe

Die Frage vom Boden aus: traegt dieser Befehl?

Was der Bot in der Luft tun wird, weiss die Rechnung nicht - sie kennt nur den
Befehl dieses Bildes. Also rechnet sie mit den beiden Moeglichkeiten, die es
gibt, und welche zaehlt, haengt davon ab, wer den Bot gerade fuehrt.

Die freie Bewegung (Kampf, Ausweichen) gibt in der Luft gar keinen Befehl:
BotWalkInDirection kehrt dort ohne EA_Move zurueck. Gemessen auf q3dm18 - ein
Bot wechselt im Gefecht die Seite und springt; die Rechnung nahm an, er halte
im Flug gegen seinen Schwung, und sah ihn 40 Einheiten vor der Kante landen.
Er hielt nicht dagegen und flog darueber hinaus. Ohne Weg muss die Landung
deshalb mit und ohne Steuerung eine sein.

Der Weg nach der Karte steuert im Flug weiter auf das Ende der Strecke zu, und
zwar mit dem Tempo, das es braucht (BotAirControl) - nicht mit dem, das am
Boden befohlen war. Gemessen auf q3dm9: vor dem Schritt vom Steg drosselt
botlib auf 152, beschleunigt im Fall auf 212 und landet auf der Plattform; mit
152 gerechnet, faellt der Bot daneben, und der Tritt hielt ihn an der Kante
fest. Auf dem Weg genuegt es deshalb, wenn eine der beiden Annahmen traegt:
der Befehl bleibt, oder er wird zu vollem Tempo.
==================
*/
static qboolean BotFootSafe( bot_state_t *bs, playerState_t *ps, vec3_t wishdir, float wishspeed, qboolean jump, int frames ) {
	if ( bs->travel_type ) {
		if ( BotPredictFooting( bs, ps, wishdir, wishspeed, jump, frames, 1 ) ) {
			return qtrue;
		}
		return BotPredictFooting( bs, ps, wishdir, wishspeed, jump, frames, 2 );
	}
	if ( !BotPredictFooting( bs, ps, wishdir, wishspeed, jump, frames, 1 ) ) {
		return qfalse;
	}
	// blieb er auf dem Boden, gibt es keinen Flug, ueber den man sich irren kann
	if ( footWhy == 1 || footWhy == 2 ) {
		return qtrue;
	}
	return BotPredictFooting( bs, ps, wishdir, wishspeed, jump, frames, 0 );
}

/*
==================
BotFootWish

Was aus dem Befehl des Bots im Spiel wird: Richtung und Tempo, so wie
BotInputToUserCommand und PM_CmdScale sie ausrechnen.
==================
*/
static float BotFootWish( bot_input_t *bi, playerState_t *ps, vec3_t want ) {
	vec3_t		angles, forward, right;
	float		f, r, m, speed;

	speed = bi->speed;
	if ( speed > 400.0f ) {
		speed = 400.0f;
	}
	if ( speed < 0.0f ) {
		speed = 0.0f;
	}
	VectorSet( angles, 0, bi->viewangles[YAW], 0 );
	AngleVectors( angles, forward, right, NULL );
	f = DotProduct( forward, bi->dir );
	r = DotProduct( right, bi->dir );
	m = fabs( f );
	if ( fabs( r ) > m ) {
		m = fabs( r );
	}
	if ( m > 0.0f ) {
		f *= speed * 127.0f / ( 400.0f * m );
		r *= speed * 127.0f / ( 400.0f * m );
	} else {
		f = r = 0.0f;
	}
	if ( bi->actionflags & ACTION_MOVEFORWARD ) f = 127.0f;
	if ( bi->actionflags & ACTION_MOVEBACK ) f = -127.0f;
	if ( bi->actionflags & ACTION_MOVELEFT ) r = -127.0f;
	if ( bi->actionflags & ACTION_MOVERIGHT ) r = 127.0f;
	if ( bi->actionflags & ACTION_WALK ) {
		if ( f > 64.0f ) f = 64.0f;
		if ( f < -64.0f ) f = -64.0f;
		if ( r > 64.0f ) r = 64.0f;
		if ( r < -64.0f ) r = -64.0f;
	}
	VectorScale( forward, f, want );
	VectorMA( want, r, right, want );
	want[2] = 0.0f;
	if ( VectorNormalize( want ) <= 0.0f ) {
		return 0.0f;
	}
	m = fabs( f );
	if ( fabs( r ) > m ) {
		m = fabs( r );
	}
	speed = ps->speed * m / 127.0f;
	if ( bi->actionflags & ACTION_CROUCH ) {
		speed *= 0.25f;
	}
	return speed;
}

/*
==================
BotFooting

Der Tritt: nicht ins Leere laufen, und in der Luft zurueck ans Land steuern.

Auf dem Boden (g_botEdgeCare). Fuehrt das, was der Bot gerade vorhat, in den
naechsten vier Zehntelsekunden zu einer Landung, die er nicht ueberlebt, wird
der Befehl ersetzt - durch das Mildeste, was hilft:
  1  derselbe Schritt ohne den Sprung,
  2  dieselbe Richtung im Schritttempo,
  3  stehenbleiben,
  4  gegen den Schwung,
  5  zurueck dorthin, wo er zuletzt sicher stand.
Hilft nichts davon, bleibt der Befehl, wie er ist.

Was davon erlaubt ist, haengt wieder davon ab, wer fuehrt. Ohne Weg - im
Kampf - alles ausser dem Schritttempo: dort gibt es nichts, wo der Bot
hinmuesste. Auf dem Weg nach der Karte dagegen wird nie angehalten. Die erste
Fassung der Bremse hat gezeigt, was sonst passiert (8,7 Sekunden auf zehn
Einheiten), und die erste Fassung dieser Funktion hat es bestaetigt: auf q3dm9
standen die Bots je Lauf fuenfzig Sekunden an Kanten, ueber die ihr Weg
fuehrte, weil die Rechnung die Landung nicht fand. Steht die Rechnung gegen
den Weg, gilt der Weg - mit zwei Ausnahmen, in denen nicht der Weg das Problem
ist:
- das Schritttempo, wenn der Bot damit wirklich landet (nicht bloss noch nicht
  an der Kante ist). Das ist die Treppe aus schwebenden Absaetzen auf q3dm18,
  ueber deren erste Stufe die Bots mit vollem Tempo hinausfliegen;
- gegen den Schwung oder zurueck, wenn der Schwung woandershin zeigt als der
  Befehl. Dann traegt ihn ein Rueckstoss oder eine zu schnell genommene Kurve,
  und die Rechnung hat schon beruecksichtigt, wie schnell der Befehl das
  auffaengt.

Nicht angefasst werden die Reisearten, bei denen botlib den Absprung selbst
abpasst: der Sprung ueber eine Luecke (der Anlauf SOLL auf die Kante zufuehren,
gedrueckt wird erst im letzten Bild), Raketen- und BFG-Sprung, Aufzug und
schwebende Plattform.

In der Luft (g_botAirControl). Im Quelltext von id steht an der Stelle seit
1999 "FIXME: do air control to avoid hazards". Ein Mensch, den eine Rakete
ueber die Kante wirft, haelt dagegen; der Bot laesst sich fallen. Hier wird
gerechnet, wo er mit seinem jetzigen Befehl landet, und wenn das keine Landung
ist, die Richtung gesucht, mit der es eine wird: die zuletzt gewaehlte, zurueck
zum letzten sicheren Stand, gegen den Schwung, dann die acht Himmelsrichtungen.
Eine Sekunde Gegensteuern nimmt 320 Einheiten je Sekunde aus dem Flug.

Einen geplanten Flug - Sprungfeld, Sprung, Schritt vom Sims - laesst die
Luftsteuerung in Ruhe, solange nichts dazwischenkommt. Auch das ist gemessen:
das Sprungfeld auf q3dm9 wirft durch einen Torbogen, die Rechnung streifte im
ersten Bild dessen Kante, steuerte sechzehn Einheiten zur Seite - und der Bot
schlug mit dem Kopf an den Bogen und fiel in die Grube. Dazwischengekommen ist
etwas, wenn ein Treffer ihn geworfen hat (PMF_TIME_KNOCKBACK) oder er im Flug
schlagartig Tempo verliert, also mit jemandem zusammengestossen ist. Und die
Landung muss zwei Bilder hintereinander fehlen, bevor umgesteuert wird.
==================
*/
static qboolean BotFooting( bot_state_t *bs, bot_input_t *bi ) {
	playerState_t	*ps;
	gentity_t		*ent;
	vec3_t			want, cand, vel, veldir;
	float			wishspeed, speed, now;
	int				i, kind, ownwhy, padfail;
	qboolean		jump, routed;
	vec3_t			ownend;

	bs->foot_kind = 0;
	ent = &g_entities[bs->client];
	if ( !ent->client ) {
		return qfalse;
	}
	// der lebende Zustand, nicht die Kopie vom letzten Denkschritt: gerechnet
	// wird in jedem Bild des Servers, gedacht nur in jedem zweiten
	ps = &ent->client->ps;
	if ( ps->pm_type != PM_NORMAL || ent->waterlevel >= 2 || ps->speed <= 0 ) {
		bs->foot_void = 0;
		return qfalse;
	}
	// Auf einem Sprungfeld: der Flug, der jetzt kommt, ist geplant, auch wenn
	// ihn gerade kein Weg fuehrt (im Kampf). jumppad_ent steht nur, solange der
	// Bot das Feld beruehrt, also hier merken.
	footSkipPad = -1;
	if ( ps->jumppad_ent ) {
		// Werkbank: fuer das Protokoll, was die Rechnung zu diesem Flug sagt -
		// damit laesst sich pruefen, ob sie Sprungfelder richtig vorhersagt
		if ( !bs->foot_pad && g_botLog.integer ) {
			playerState_t	fly;
			vec3_t			none = { 0, 0, 0 };
			qboolean		lands;

			fly = *ps;
			fly.groundEntityNum = ENTITYNUM_NONE;
			footSkipPad = ps->jumppad_ent;
			lands = BotPredictFooting( bs, &fly, none, 0.0f, qfalse, 0, 0 );
			footSkipPad = -1;
			BotLogPrintf( "P %i %i %i %i %i %.0f %.0f %.0f %.0f %.0f %.0f\n", level.time, bs->client,
				ps->jumppad_ent, lands, footWhy, footEnd[0], footEnd[1], footEnd[2],
				ps->origin[0], ps->origin[1], ps->origin[2] );
		}
		bs->foot_pad = qtrue;
	} else if ( ps->groundEntityNum != ENTITYNUM_NONE ) {
		bs->foot_pad = qfalse;
	}
	now = FloatTime();
	wishspeed = BotFootWish( bi, ps, want );
	VectorCopy( ps->velocity, vel );
	vel[2] = 0.0f;
	speed = VectorLength( vel );
	if ( speed > 0.0f ) {
		VectorScale( vel, 1.0f / speed, veldir );
	} else {
		VectorClear( veldir );
	}

	if ( ps->groundEntityNum == ENTITYNUM_NONE ) {
		// vom Plan abgekommen?
		if ( ( ps->pm_flags & PMF_TIME_KNOCKBACK ) || now - bs->foot_knock < 0.4f
			|| ( bs->foot_lastspeed > 200.0f && speed < bs->foot_lastspeed * 0.6f ) ) {
			bs->foot_astray = qtrue;
		}
		bs->foot_lastspeed = speed;
		if ( !BotSw(bs, g_botAirControl) || now < bs->foot_release ) {
			return qfalse;
		}
		// der eigene Raketensprung wirft auch - und ist trotzdem der Plan
		if ( bs->travel_type == TRAVEL_ROCKETJUMP || bs->travel_type == TRAVEL_BFGJUMP ) {
			return qfalse;
		}
		if ( !bs->foot_astray
			&& ( bs->foot_pad || ( bs->travel_type && bs->travel_type != TRAVEL_WALK ) ) ) {
			return qfalse;
		}
		if ( BotPredictFooting( bs, ps, want, wishspeed, qfalse, 0, 1 ) ) {
			bs->foot_void = 0;
			return qfalse;
		}
		if ( ++bs->foot_void < 2 ) {
			return qfalse;
		}
		kind = 0;
		// die Richtung vom letzten Bild zuerst, damit die Wahl nicht springt
		if ( now - bs->foot_air_time < 0.25f
			&& BotPredictFooting( bs, ps, bs->foot_air_dir, ps->speed, qfalse, 0, 1 ) ) {
			VectorCopy( bs->foot_air_dir, cand );
			kind = 4;
		}
		// zurueck zum letzten sicheren Stand
		if ( !kind ) {
			VectorSubtract( bs->foot_origin, ps->origin, cand );
			cand[2] = 0.0f;
			if ( VectorNormalize( cand ) > 16.0f && BotPredictFooting( bs, ps, cand, ps->speed, qfalse, 0, 1 ) ) {
				kind = 1;
			}
		}
		// gegen den Schwung
		if ( !kind && speed > 30.0f ) {
			VectorNegate( veldir, cand );
			if ( BotPredictFooting( bs, ps, cand, ps->speed, qfalse, 0, 1 ) ) {
				kind = 2;
			}
		}
		for ( i = 0; i < 8 && !kind; i++ ) {
			VectorSet( cand, cos( i * ( M_PI / 4.0 ) ), sin( i * ( M_PI / 4.0 ) ), 0 );
			if ( BotPredictFooting( bs, ps, cand, ps->speed, qfalse, 0, 1 ) ) {
				kind = 3;
			}
		}
		if ( !kind ) {
			// nichts zu retten - und nicht in jedem Bild neu nachrechnen
			bs->foot_release = now + 0.3f;
			return qfalse;
		}
		VectorCopy( cand, bs->foot_air_dir );
		bs->foot_air_time = now;
		VectorCopy( cand, bi->dir );
		bi->speed = 400.0f;
		bi->actionflags &= ~( ACTION_MOVEFORWARD | ACTION_MOVEBACK | ACTION_MOVELEFT
			| ACTION_MOVERIGHT | ACTION_WALK | ACTION_CROUCH );
		if ( !( bs->log_flags & BOTLOG_AIR ) ) {
			BotLogPrintf( "A %i %i %.0f %.0f %.0f %.0f %.0f %.2f %.2f %i\n", level.time, bs->client,
				ps->origin[0], ps->origin[1], ps->origin[2], speed, ps->velocity[2],
				cand[0], cand[1], kind );
		}
		bs->log_flags |= BOTLOG_AIR;
		bs->foot_kind = 10 + kind;
		return qtrue;
	}

	bs->foot_astray = qfalse;
	bs->foot_void = 0;
	bs->foot_lastspeed = speed;
	if ( ps->pm_flags & PMF_TIME_KNOCKBACK ) {
		bs->foot_knock = now;
	}
	if ( !BotSw(bs, g_botEdgeCare) ) {
		return qfalse;
	}
	// ACTION_DELAYEDJUMP: einen Sprung ueber eine Luecke kuendigt botlib damit an
	// und drueckt erst ein Bild spaeter wirklich. Waehrend des Anlaufs steht also
	// nur diese Flagge - und vor dem Bot ist naturgemaess nichts.
	if ( bi->actionflags & ACTION_DELAYEDJUMP ) {
		return qfalse;
	}
	switch ( bs->travel_type ) {
		case TRAVEL_JUMP:
		case TRAVEL_ROCKETJUMP:
		case TRAVEL_BFGJUMP:
		case TRAVEL_GRAPPLEHOOK:
		case TRAVEL_ELEVATOR:
		case TRAVEL_FUNCBOB:
			return qfalse;
	}
	// mit Auto-Hop springt auch, wer die Taste noch haelt (PM_CheckJump)
	jump = ( bi->actionflags & ACTION_JUMP )
		&& ( pmove_AutoHop.integer || !( ps->pm_flags & PMF_JUMP_HELD ) );
	if ( speed < 30.0f && wishspeed <= 0.0f && !jump ) {
		VectorCopy( ps->origin, bs->foot_origin );
		return qfalse;				// steht
	}
	if ( BotFootSafe( bs, ps, want, wishspeed, jump, FOOT_GROUNDFRAMES ) ) {
		VectorCopy( ps->origin, bs->foot_origin );
		if ( jump && g_botLog.integer > 1 ) {
			BotLogPrintf( "J %i %i %.0f %.0f %.0f %.0f %.2f %.2f %.0f %i %.0f %.0f %.0f\n", level.time, bs->client,
				ps->origin[0], ps->origin[1], ps->origin[2], speed, want[0], want[1], wishspeed,
				footWhy, footEnd[0], footEnd[1], footEnd[2] );
		}
		return qfalse;
	}

	ownwhy = footWhy;
	VectorCopy( footEnd, ownend );
	// Gewartet wird nur auf das, was einen Bot wirklich umbringt: eine
	// Todeszone, Lava, ein Aufprall, den er nicht uebersteht, oder gar keine
	// Landung. Eine Landung auf der Kante zaehlt hier nicht - ein Sprungfeld
	// zielt auf seinen Zielpunkt, und die erste Fassung liess die Bots auf
	// q3dm17 sonst sieben Sekunden je Lauf vor Sprungfeldern stehen.
	//
	// Zweimal nachgemessen. Ein Aufprall, den der Bot mit zehn Leben nicht
	// uebersteht, ist kein Grund zu warten: davon wird er nicht gesuender, und
	// auf q3dm17 standen so Bots mit wenig Leben sechs Sekunden an der Kante.
	// Und "keine Landung" heisst nur dann Absturz, wenn die Rechnung ihn dabei
	// tief fallen sieht - sonst hing sie an einer Kante fest (sie rechnet eine
	// Wand je Bild, das Spiel bis zu vier), und auf q3dm9 wartete darum jeder
	// vor dem Sprungfeld auf den Sims.
	padfail = footPadFlight && ( ownwhy == 5
		|| ( ownwhy == 9 && ownend[2] < ps->origin[2] - 400.0f ) );
	routed = ( bs->travel_type != 0 );
	kind = 0;
	VectorCopy( want, cand );
	if ( now - bs->foot_padhold > 0.5f ) {
		bs->foot_padstart = now;
	}
	if ( jump && BotFootSafe( bs, ps, want, wishspeed, qfalse, FOOT_GROUNDFRAMES ) ) {
		kind = 1;
	} else if ( padfail && now - bs->foot_padstart < 6.0f
		&& BotFootSafe( bs, ps, want, 0.0f, qfalse, FOOT_GROUNDFRAMES ) ) {
		// Der Flug von diesem Sprungfeld geht jetzt nicht auf: davor warten,
		// auch auf dem eigenen Weg - wie ein Mensch, der sieht, dass die Falle
		// unten ist. Hoechstens sechs Sekunden, dann gilt wieder der Weg: die
		// Rechnung kann sich irren, und die Falle auf q3tourney6 bleibt fuenf
		// Sekunden unten.
		kind = 6;
		bs->foot_padhold = now;
	} else if ( routed ) {
		// nur, wenn er damit wirklich irgendwo ankommt
		if ( wishspeed > FOOT_SLOW + 10.0f
			&& BotFootSafe( bs, ps, want, FOOT_SLOW, qfalse, FOOT_SLOWFRAMES )
			&& ( footWhy == 3 || footWhy == 6 ) ) {
			kind = 2;
		}
	} else if ( BotFootSafe( bs, ps, want, 0.0f, qfalse, FOOT_GROUNDFRAMES ) ) {
		kind = 3;
	}
	// Schneller, als der eigene Befehl ihn machen kann, ist er nach einer
	// Landung vom Sprungfeld oder nach einem Treffer: dann ist der Schwung
	// nicht sein Weg, auch wenn der in dieselbe Richtung zeigt. Gemessen auf
	// q3tourney6: gelandet mit 700 u/s, und ueber die Kante gerutscht.
	if ( !kind && speed > 30.0f
		&& ( !routed || wishspeed <= 0.0f || DotProduct( want, veldir ) <= 0.7f
			|| speed > ps->speed + 60.0f ) ) {
		VectorNegate( veldir, cand );
		if ( BotFootSafe( bs, ps, cand, ps->speed, qfalse, FOOT_GROUNDFRAMES ) ) {
			kind = 4;
		} else {
			VectorSubtract( bs->foot_origin, ps->origin, cand );
			cand[2] = 0.0f;
			if ( VectorNormalize( cand ) > 8.0f
				&& BotFootSafe( bs, ps, cand, ps->speed, qfalse, FOOT_GROUNDFRAMES ) ) {
				kind = 5;
			}
		}
	}
	// Art 0: die Rechnung sieht keine Landung und kennt keinen Ausweg
	if ( g_botLog.integer && ( kind ? !( bs->log_flags & BOTLOG_BRAKE ) : g_botLog.integer > 1 ) ) {
		BotLogPrintf( "B %i %i %.0f %.0f %.0f %.0f %.2f %.2f %.2f %.2f %.0f %i %i %i %i %.0f %.0f %.0f\n",
			level.time, bs->client, ps->origin[0], ps->origin[1], ps->origin[2], speed,
			veldir[0], veldir[1], want[0], want[1], wishspeed, bs->travel_type, kind, jump,
			ownwhy, ownend[0], ownend[1], ownend[2] );
	}
	if ( !kind ) {
		return qfalse;
	}
	if ( BotSw(bs, g_botEdgeCare) > 1 ) {
		G_Printf( "bot edge: client %i held (%i) at %.0f %.0f %.0f, %.0f ups\n",
			bs->client, kind, ps->origin[0], ps->origin[1], ps->origin[2], speed );
	}
	bs->log_flags |= BOTLOG_BRAKE;
	bs->foot_kind = kind;

	// Die vier Richtungsflaggen weg: BotInputToUserCommand ueberschreibt
	// forward- und rightmove damit stumpf, nachdem es die Richtung ausgerechnet
	// hat; der Tritt waere sonst je nach Laune der KI wirkungslos.
	bi->actionflags &= ~( ACTION_JUMP | ACTION_MOVEFORWARD | ACTION_MOVEBACK
		| ACTION_MOVELEFT | ACTION_MOVERIGHT );
	switch ( kind ) {
		case 1:
			VectorCopy( want, bi->dir );
			bi->speed = wishspeed * 400.0f / ps->speed;
			break;
		case 2:
			VectorCopy( want, bi->dir );
			bi->speed = FOOT_SLOW * 400.0f / ps->speed;
			break;
		case 3:
		case 6:
			bi->speed = 0.0f;
			break;
		default:
			VectorCopy( cand, bi->dir );
			bi->speed = 400.0f;
			bi->actionflags &= ~( ACTION_WALK | ACTION_CROUCH );
			break;
	}
	return qtrue;
}

/*
==================
BotDodgeThink

Einer Rakete ausweichen, die auf den Bot zukommt.

Die Bots kennen im Original genau ein Geschoss: die Granate, um die sie einen
Bogen machen (BotCheckForGrenades). Raketen sehen sie nicht - sie werden in
BotAIStartFrame ausdruecklich aus der Welt der Bots herausgenommen.

Gerechnet wird mit dem, was auch ein Mensch abschaetzt: wo kommt die Rakete
vorbei, und wann. Liegt der naechste Punkt ihrer Bahn naeher als der
Wirkradius und ist sie in weniger als sechs Zehntelsekunden da, geht der Bot
einen Schritt quer zur Bahn - weg von ihr, und nie ueber eine Kante. Sehen
muss er sie dafuer, und nicht jeder sieht jede rechtzeitig: die Aussicht
waechst mit dem Koennen, von 44 Prozent auf Stufe eins bis alle auf Stufe fuenf.

Gemessen, zehn Bots auf Stufe drei, nur Raketenwerfer, je drei Laeufe von fuenf
Minuten, sonst alles gleich: 148 statt 171 Tode, davon 79 statt 100 durch
Raketen - und 14 statt 19 in der Grube, der Schritt kostet also keine Stuerze.
==================
*/
static void BotDodgeThink( bot_state_t *bs ) {
	int			i, chosen;
	gentity_t	*ent;
	vec3_t		rel, vel, closest, away, bestaway, bestvel, up = { 0, 0, 1 };
	float		speed2, t, miss, best, bestmiss;
	trace_t		tr;

	// Gemerkt wird die Nummer der Entitaet, und die vergibt das Spiel nach einer
	// Sekunde neu. Ist die gemerkte Rakete nicht mehr unterwegs, ist auch der
	// Entschluss ueber sie hinfaellig - sonst gaelte er fuer die naechste, die
	// zufaellig dieselbe Nummer bekommt, samt der Richtung der alten.
	if ( bs->dodge_missile >= MAX_CLIENTS ) {
		ent = &g_entities[bs->dodge_missile];
		if ( !ent->inuse || ent->s.eType != ET_MISSILE ) {
			bs->dodge_missile = 0;
			bs->dodge_decided = qfalse;
		}
	}
	if ( !BotSw(bs, g_botDodge) || BotIsDead( bs ) || BotIsObserver( bs ) ) {
		return;
	}
	chosen = -1;
	best = 0.6f;
	bestmiss = 0.0f;
	for ( i = MAX_CLIENTS; i < level.num_entities; i++ ) {
		ent = &g_entities[i];
		if ( !ent->inuse || ent->s.eType != ET_MISSILE ) {
			continue;
		}
		if ( ent->s.weapon != WP_ROCKET_LAUNCHER && ent->s.weapon != WP_BFG ) {
			continue;
		}
		if ( ent->r.ownerNum == bs->client ) {
			continue;
		}
		if ( ent->r.ownerNum >= 0 && ent->r.ownerNum < MAX_CLIENTS
			&& BotSameTeam( bs, ent->r.ownerNum ) ) {
			continue;
		}
		VectorCopy( ent->s.pos.trDelta, vel );
		speed2 = DotProduct( vel, vel );
		if ( speed2 < 1.0f ) {
			continue;
		}
		VectorSubtract( bs->origin, ent->r.currentOrigin, rel );
		t = DotProduct( rel, vel ) / speed2;
		// Sechs Zehntelsekunden und neunzig Einheiten: die erste Fassung nahm
		// acht Zehntel und hundertfuenfzig, und damit wich ein Bot auch dem
		// aus, was ihn gar nicht getroffen haette - gemessen sechs Prozent
		// seiner Lebenszeit.
		if ( t < 0.0f || t >= best ) {
			continue;			// schon vorbei, oder eine andere ist frueher da
		}
		VectorMA( ent->r.currentOrigin, t, vel, closest );
		VectorSubtract( bs->origin, closest, away );
		miss = VectorLength( away );
		if ( miss > 90.0f ) {
			continue;
		}
		trap_Trace( &tr, bs->eye, NULL, NULL, ent->r.currentOrigin, bs->entitynum, MASK_SOLID );
		if ( tr.fraction < 1.0f ) {
			continue;
		}
		best = t;
		bestmiss = miss;
		chosen = i;
		VectorCopy( away, bestaway );
		VectorCopy( vel, bestvel );
	}
	if ( chosen < 0 ) {
		return;
	}
	// Ueber jede Rakete wird einmal entschieden, nicht bei jedem Denkschritt
	// neu - ob ueberhaupt, und nach welcher Seite. Sonst wechselt die Seite mit
	// jeder Nachrechnung, und aus dem Ausweichschritt wird ein Zittern.
	if ( bs->dodge_missile == chosen ) {
		if ( bs->dodge_decided ) {
			bs->dodge_time = FloatTime() + 0.25f;
		}
		return;
	}
	bs->dodge_missile = chosen;
	bs->dodge_decided = qfalse;
	if ( random() >= 0.3f + 0.14f * bs->settings.skill ) {
		return;
	}
	bestaway[2] = 0.0f;
	if ( VectorNormalize( bestaway ) < 8.0f ) {
		// Genau auf Kurs: quer zur Bahn, und zwar auf die Seite, auf die der
		// Bot schon laeuft - sonst muss er erst seinen Schwung abbremsen und
		// steht, wenn die Rakete kommt. Die erste Fassung nahm dafuer die
		// Ausweichflagge der Kampfbewegung, rechnete aber von der Rakete aus
		// statt vom Bot aus und kam so genau auf die falsche Seite.
		bestvel[2] = 0.0f;
		CrossProduct( bestvel, up, bestaway );
		if ( VectorNormalize( bestaway ) < 0.1f ) {
			return;				// sie kommt senkrecht von oben
		}
		VectorCopy( bs->cur_ps.velocity, rel );
		rel[2] = 0.0f;
		speed2 = DotProduct( rel, bestaway );
		if ( speed2 < -20.0f || ( speed2 <= 20.0f && !( bs->flags & BFL_STRAFERIGHT ) ) ) {
			VectorNegate( bestaway, bestaway );
		}
	}
	if ( !BotGroundAhead( bs, bestaway, 140.0f ) ) {
		// dort ist nichts. Auf die andere Seite nur, wenn die Bahn so dicht am
		// Bot vorbeigeht, dass er sie dabei gleich hinter sich hat.
		if ( bestmiss > 40.0f ) {
			return;
		}
		VectorNegate( bestaway, bestaway );
		if ( !BotGroundAhead( bs, bestaway, 140.0f ) ) {
			return;
		}
	}
	VectorCopy( bestaway, bs->dodge_dir );
	bs->dodge_time = FloatTime() + 0.25f;
	bs->dodge_decided = qtrue;
}

/*
==================
BotStrandedThink

Von einem Fleck ohne Wegnetz herunterkommen.

Ein Bot kennt die Karte nur als Netz von Feldern und Verbindungen. Landet er
daneben - auf einem Sims, einem Zierrat, einer Kante, die der Kartenbauer nie
zum Betreten gedacht hat -, fuehrt von dort kein Weg zu irgendeinem Ziel. Die
Zielwahl findet nichts, der Knoten gibt keinen Bewegungsbefehl, und der Bot
steht, bis ihn jemand abschiesst. Gemessen: achtzig Sekunden auf einem Sims
von q3dm17, auf das ihn ein Kampfsprung gebracht hatte; 517 Zielwahlen ohne
Ergebnis.

Ein Mensch springt herunter. Das tut der Bot hier auch: hat er zehn
Denkschritte lang kein Ziel gefunden oder keinen Weg, ohne dass es dazwischen
einmal voranging, sucht er in sechzehn Richtungen den naechsten Boden, auf dem
das Wegnetz weitergeht, und geht dorthin - auch ueber die Kante. Findet er
keinen, probiert er es auf gut Glueck.

Erkannt wird es an dem, was die KI selbst meldet, und nicht am Feld, auf dem
der Bot steht. Das Sims von q3dm17 HAT Verbindungen im Wegnetz - sie fuehren
nur nirgends hin, wo etwas liegt. Eine erste Fassung fragte das Feld und tat
deshalb genau dort nichts.
==================
*/
// Liegt der Punkt in einem trigger_hurt - der Grube unter der Karte?
static qboolean BotInHurtTrigger( vec3_t point ) {
	vec3_t	mins, maxs;

	VectorSet( mins, point[0] - 16, point[1] - 16, point[2] - 24 );
	VectorSet( maxs, point[0] + 16, point[1] + 16, point[2] + 32 );
	return BotHurtTriggerInBox( mins, maxs );
}

static void BotStrandedThink( bot_state_t *bs ) {
	int			i, area;
	float		dist, score, best;
	vec3_t		dir, spot, below, vel;
	vec3_t		mins = { -15, -15, -24 }, maxs = { 15, 15, 32 };
	trace_t		tr;

	if ( !BotSw(bs, g_botUnstuck) || BotIsDead( bs ) || BotIsObserver( bs ) || BotIntermission( bs ) ) {
		bs->stranded_count = 0;
		return;
	}
	// es ging voran: ein Weg nach der Karte, der nicht gescheitert ist
	if ( bs->travel_type && !( bs->log_flags & BOTLOG_FAILURE ) ) {
		bs->stranded_count = 0;
		return;
	}
	// Kein Ziel oder kein Weg. Ein Denkschritt im Kampf zaehlt weder so noch
	// so: der Bot umkreist dort seinen Gegner und fragt gar nicht nach dem Weg.
	if ( bs->log_flags & ( BOTLOG_NOGOAL | BOTLOG_FAILURE ) ) {
		if ( !bs->stranded_count ) {
			bs->stranded_search = 0.0f;
		}
		bs->stranded_count++;
	}
	if ( bs->stranded_count < 10 || bs->cur_ps.groundEntityNum == ENTITYNUM_NONE ) {
		return;
	}
	if ( bs->stranded_search < FloatTime() ) {
		bs->stranded_search = FloatTime() + 0.7f;
		best = 99999.0f;
		for ( i = 0; i < 16; i++ ) {
			dir[0] = cos( i * ( M_PI / 8.0 ) );
			dir[1] = sin( i * ( M_PI / 8.0 ) );
			dir[2] = 0.0f;
			for ( dist = 48.0f; dist <= 432.0f; dist += 48.0f ) {
				VectorMA( bs->origin, dist, dir, spot );
				// steht etwas im Weg, geht es in dieser Richtung nicht weiter
				trap_Trace( &tr, bs->origin, mins, maxs, spot, bs->entitynum, MASK_PLAYERSOLID );
				if ( tr.fraction < 1.0f ) {
					break;
				}
				VectorCopy( spot, below );
				below[2] -= 1024.0f;
				trap_Trace( &tr, spot, mins, maxs, below, bs->entitynum, MASK_PLAYERSOLID );
				if ( tr.startsolid || tr.fraction >= 1.0f ) {
					continue;			// darunter ist nichts
				}
				if ( trap_PointContents( tr.endpos, bs->entitynum ) & ( CONTENTS_LAVA | CONTENTS_SLIME ) ) {
					continue;
				}
				area = BotPointAreaNum( tr.endpos );
				if ( !area || area == bs->areanum || !trap_AAS_AreaReachability( area ) ) {
					continue;			// auch dort kein Wegnetz, oder dasselbe Sims
				}
				// Das Sims selbst hat Verbindungen, sie fuehren nur nirgends
				// hin. Boden ist erst, was zurueck ins Spiel fuehrt: zu dem
				// Feld, von dem aus der Bot zuletzt einen Weg hatte.
				if ( bs->lastgood_area && !trap_AAS_AreaTravelTimeToGoalArea( area, tr.endpos, bs->lastgood_area, TFL_DEFAULT ) ) {
					continue;
				}
				if ( BotInHurtTrigger( tr.endpos ) ) {
					continue;			// die Grube hat auch einen Boden
				}
				// nah ist gut, tief ist weniger gut
				score = dist + ( bs->origin[2] - tr.endpos[2] ) * 0.5f;
				if ( score < best ) {
					best = score;
					VectorCopy( dir, bs->stranded_dir );
				}
				break;
			}
		}
		bs->stranded_blind = ( best >= 99999.0f );
		if ( bs->stranded_blind ) {
			// Nirgends Boden mit Wegnetz in Reichweite. Dann irgendwohin -
			// auch das ist, was ein Mensch tut, der auf einem Sims steht:
			// er springt, und sei es in die Grube, statt dort zu warten.
			i = (int)( random() * 16 ) & 15;
			bs->stranded_dir[0] = cos( i * ( M_PI / 8.0 ) );
			bs->stranded_dir[1] = sin( i * ( M_PI / 8.0 ) );
			bs->stranded_dir[2] = 0.0f;
		}
		BotLogPrintf( "U %i %i %.0f %.0f %.0f %.2f %.2f %s\n", level.time, bs->client,
			bs->origin[0], bs->origin[1], bs->origin[2], bs->stranded_dir[0], bs->stranded_dir[1],
			bs->stranded_blind ? "blind" : "floor" );
	}
	bs->log_flags |= BOTLOG_STRANDED;
	// Ins Ungewisse erst nach drei Sekunden: bis dahin kann sich der Gegner
	// zeigen, und vom Sims aus laesst sich durchaus schiessen.
	if ( bs->stranded_blind && bs->stranded_count < 30 ) {
		return;
	}
	trap_EA_Move( bs->client, bs->stranded_dir, 400 );
	// Kommt er nicht vom Fleck, steht ein Rand oder eine Stufe im Weg: darueber.
	// Nicht gleich beim ersten Schritt - ein Sprung traegt weiter als ein
	// Schritt, und der gefundene Boden kann schmal sein.
	VectorCopy( bs->cur_ps.velocity, vel );
	vel[2] = 0.0f;
	if ( bs->stranded_count > 15 && VectorLength( vel ) < 40.0f ) {
		trap_EA_Jump( bs->client );
	}
}

static void BotDodge( bot_state_t *bs, bot_input_t *bi ) {
	if ( !BotSw(bs, g_botDodge) || bs->dodge_time <= FloatTime() ) {
		return;
	}
	if ( bs->cur_ps.groundEntityNum == ENTITYNUM_NONE
		|| ( bi->actionflags & ( ACTION_JUMP | ACTION_DELAYEDJUMP ) ) ) {
		return;
	}
	// Der Schritt darf nirgends hinfuehren, wo nichts ist - und das wird in
	// jedem Bild neu geprueft, nicht nur beim Entschluss. Die Bremse hilft hier
	// nicht: sie traut dem, was der Bot selbst will, und das hier will er.
	// Die erste Fassung pruefte nur beim Entschluss; gemessen stiegen die Tode
	// in der Grube damit von 8,9 auf 12,9 Prozent.
	if ( !BotGroundAhead( bs, bs->dodge_dir, 110.0f ) ) {
		bs->dodge_time = 0.0f;
		bs->dodge_decided = qfalse;
		return;
	}
	VectorCopy( bs->dodge_dir, bi->dir );
	bi->speed = 400.0f;
	bi->actionflags &= ~( ACTION_MOVEFORWARD | ACTION_MOVEBACK
		| ACTION_MOVELEFT | ACTION_MOVERIGHT );
	bs->log_flags |= BOTLOG_DODGE;
}

/*
==================
BotMoverGuard

Nicht unter ein Pendel laufen und nicht unter einer Plattform stehen, die
gleich herunterkommt.

Pendel und schwebende Plattformen (func_pendulum, func_bobbing) werden nie
aufgehalten: was sie nicht wegschieben koennen, toeten sie sofort
(G_MoverPush, "bobbing entities are instant-kill"). Die Bots wissen davon
nichts. Gemessen, zwoelf Bots, je drei Laeufe von vier Minuten: auf q3dm15
starben zehn unter den drei Pendeln des Gangs, jeder an der Stelle eines
Pendels, und alle waren einfach durchgelaufen; auf q3dm19 standen sie an der
Stelle, an der botlib auf die Plattform wartet (TRAVEL_FUNCBOB), halb unter
deren Rand, und die Plattform kam auf sie herunter.

Hier wird eine halbe Sekunde vorausgerechnet, wo die Falle sein wird
(BG_EvaluateTrajectory, fuer Pendel die Drehung, fuer Plattformen der Weg) und
wo der Bot mit seinem Befehl ist - und ob sich beide treffen. Getestet wird mit
dem wirklichen Modell der Falle (trap_EntityContact), nicht mit ihrem Kasten:
dazu wird statt der Falle der Bot bewegt, in ihr Bezugssystem zurueck. Trifft
es, bekommt er das Erste, das nicht trifft und keinen Absturz bedeutet:
warten, zurueck, oder weg von der Falle. Wie ein Mensch, der den Takt abpasst.

Dazu Tueren und Plattformen, die sich gerade bewegen und dabei Schaden von
hundert und mehr machen: die Quetschfalle auf q3tourney6. Die ist schnell
(tausend Einheiten je Sekunde), und wer darunter steht, wenn jemand den Knopf
trifft, kommt nicht mehr heraus - wer hineinlaufen will, waehrend sie faehrt,
schon.
==================
*/
#define MOVER_HORIZON		0.6f		// so weit voraus, in Sekunden
#define MOVER_STEP			0.1f

// Etwas breiter als der Bot, aber die Sohle bleibt frei: wer auf eine
// Plattform steigt oder auf ihr steht, beruehrt sie mit den Fuessen, und das
// ist kein Treffer. Die erste Fassung reichte zwei Einheiten unter die Fuesse -
// der Bot verpasste auf q3dm19 den Moment zum Aufsteigen und wartete eine
// ganze Runde der Plattform: Stehen 13 statt 8,7 Prozent seiner Zeit.
static vec3_t	moverMins = { -18, -18, -14 };
static vec3_t	moverMaxs = { 18, 18, 34 };

// Trifft die Falle den Bot, wenn er sich so bewegt? Die Bahn des Bots: die
// ersten Zehntel mit dem Schwung, den er hat, danach mit dem Befehl.
static qboolean BotMoverHits( gentity_t *mover, vec3_t origin, vec3_t vel, vec3_t wish, float wishspeed ) {
	vec3_t		pos, o0, o1, a0, a1, ax0[3], ax1[3], rel, local, test, mins, maxs;
	float		t, t1, t2;
	int			i;
	qboolean	turns;

	turns = ( mover->s.apos.trType != TR_STATIONARY );
	// das Modell steht dort, wo es der Server zuletzt hingestellt hat
	VectorCopy( mover->r.currentOrigin, o0 );
	VectorCopy( mover->r.currentAngles, a0 );
	if ( turns ) {
		AnglesToAxis( a0, ax0 );
	}
	for ( t = MOVER_STEP; t <= MOVER_HORIZON + 0.01f; t += MOVER_STEP ) {
		t1 = ( t < 0.15f ) ? t : 0.15f;
		t2 = t - t1;
		VectorMA( origin, t1, vel, pos );
		VectorMA( pos, t2 * wishspeed, wish, pos );
		BG_EvaluateTrajectory( &mover->s.pos, level.time + (int)( t * 1000 ), o1 );
		if ( turns ) {
			// in das Bezugssystem der Falle zu diesem Zeitpunkt, dann zurueck in
			// das von jetzt - dort steht ihr Modell
			BG_EvaluateTrajectory( &mover->s.apos, level.time + (int)( t * 1000 ), a1 );
			AnglesToAxis( a1, ax1 );
			VectorSubtract( pos, o1, rel );
			for ( i = 0; i < 3; i++ ) {
				local[i] = DotProduct( rel, ax1[i] );
			}
			VectorCopy( o0, test );
			for ( i = 0; i < 3; i++ ) {
				VectorMA( test, local[i], ax0[i], test );
			}
		} else {
			VectorSubtract( pos, o1, rel );
			VectorAdd( o0, rel, test );
		}
		VectorAdd( test, moverMins, mins );
		VectorAdd( test, moverMaxs, maxs );
		if ( trap_EntityContact( mins, maxs, mover ) ) {
			return qtrue;
		}
	}
	return qfalse;
}

static qboolean BotMoverDeadly( gentity_t *ent ) {
	if ( !ent->inuse || ent->s.eType != ET_MOVER || !ent->r.bmodel ) {
		return qfalse;
	}
	if ( ent->s.pos.trType == TR_SINE || ent->s.apos.trType == TR_SINE ) {
		return qtrue;			// Pendel und schwebende Plattform
	}
	// eine Quetschfalle, waehrend sie faehrt
	return ( ent->damage >= 100
		&& ( ent->s.pos.trType == TR_LINEAR_STOP || ent->s.pos.trType == TR_LINEAR ) );
}

static qboolean BotMoverGuard( bot_state_t *bs, bot_input_t *bi ) {
	playerState_t	*ps;
	gentity_t		*ent, *mover;
	vec3_t			want, vel, mins, maxs, cand, away, center;
	float			wishspeed;
	int				touch[64], n, i, kind;

	if ( !BotSw(bs, g_botEdgeCare) ) {
		return qfalse;
	}
	ent = &g_entities[bs->client];
	if ( !ent->client ) {
		return qfalse;
	}
	ps = &ent->client->ps;
	if ( ps->pm_type != PM_NORMAL ) {
		return qfalse;
	}
	VectorSet( mins, ps->origin[0] - 320, ps->origin[1] - 320, ps->origin[2] - 320 );
	VectorSet( maxs, ps->origin[0] + 320, ps->origin[1] + 320, ps->origin[2] + 320 );
	n = trap_EntitiesInBox( mins, maxs, touch, 64 );
	wishspeed = BotFootWish( bi, ps, want );
	VectorCopy( ps->velocity, vel );
	for ( i = 0; i < n; i++ ) {
		mover = &g_entities[touch[i]];
		if ( !BotMoverDeadly( mover ) || ps->groundEntityNum == mover->s.number ) {
			continue;
		}
		if ( !BotMoverHits( mover, ps->origin, vel, want, wishspeed ) ) {
			continue;
		}
		// Die Falle trifft ihn. Was hilft?
		kind = 0;
		VectorClear( cand );
		if ( !BotMoverHits( mover, ps->origin, vel, cand, 0.0f )
			&& ( ps->groundEntityNum == ENTITYNUM_NONE
				|| BotFootSafe( bs, ps, cand, 0.0f, qfalse, FOOT_GROUNDFRAMES ) ) ) {
			kind = 1;			// warten
		}
		if ( !kind && wishspeed > 0.0f ) {
			VectorNegate( want, cand );
			if ( !BotMoverHits( mover, ps->origin, vel, cand, ps->speed )
				&& ( ps->groundEntityNum == ENTITYNUM_NONE
					|| BotFootSafe( bs, ps, cand, ps->speed, qfalse, FOOT_GROUNDFRAMES ) ) ) {
				kind = 2;		// zurueck
			}
		}
		if ( !kind ) {
			VectorAdd( mover->r.absmin, mover->r.absmax, center );
			VectorScale( center, 0.5f, center );
			VectorSubtract( ps->origin, center, away );
			away[2] = 0.0f;
			if ( VectorNormalize( away ) > 0.0f
				&& !BotMoverHits( mover, ps->origin, vel, away, ps->speed )
				&& ( ps->groundEntityNum == ENTITYNUM_NONE
					|| BotFootSafe( bs, ps, away, ps->speed, qfalse, FOOT_GROUNDFRAMES ) ) ) {
				VectorCopy( away, cand );
				kind = 3;		// weg von der Falle
			}
		}
		if ( !kind ) {
			continue;			// nichts hilft: dann wenigstens nicht stehenbleiben
		}
		if ( g_botLog.integer && !( bs->log_flags & BOTLOG_MOVER ) ) {
			BotLogPrintf( "M %i %i %i %i %.0f %.0f %.0f\n", level.time, bs->client,
				mover->s.number, kind, ps->origin[0], ps->origin[1], ps->origin[2] );
		}
		bs->log_flags |= BOTLOG_MOVER;
		bs->foot_kind = 20 + kind;
		bi->actionflags &= ~( ACTION_JUMP | ACTION_MOVEFORWARD | ACTION_MOVEBACK
			| ACTION_MOVELEFT | ACTION_MOVERIGHT | ACTION_WALK | ACTION_CROUCH );
		if ( kind == 1 ) {
			bi->speed = 0.0f;
		} else {
			VectorCopy( cand, bi->dir );
			bi->speed = 400.0f;
		}
		return qtrue;
	}
	return qfalse;
}

/*
==================
BotJink

Beim Rueckzug Haken schlagen.

Im Kampf weicht ein Bot seitlich aus (BotAttackMove), auf dem Rueckzug und auf
dem Weg zu einem Gegenstand mitten im Gefecht laeuft er stur seinen Weg ab -
geradeaus, gleichmaessig, und damit fuer jede Vorhersage ein leichtes Ziel.
Gemessen auf q3dm17, Stufe 5: von den Bots, die ein starker Spieler erledigte,
starben vier von fuenf auf dem Rueckzug, zwei Drittel davon mit vollem Leben.

Hier wird, solange der Gegner zu sehen ist, die Laufrichtung abwechselnd um
etwa 35 Grad nach links und rechts gedreht, im zufaelligen Takt von drei bis
sieben Zehntelsekunden. Er kommt dabei mit vier Fuenfteln des Tempos voran.
Nur auf dem Boden, nur auf ebenem Weg (TRAVEL_WALK), und nur, wo die
Landevorhersage sagt, dass der Haken nicht ueber eine Kante fuehrt.
==================
*/
static void BotJink( bot_state_t *bs, bot_input_t *bi ) {
	playerState_t	*ps;
	vec3_t			want, cand, up = { 0, 0, 1 }, side;
	float			wishspeed, now, c, s;

	if ( !BotSw(bs, g_botJink) || !g_entities[bs->client].client ) {
		return;
	}
	if ( bs->ainode != AINode_Battle_Retreat && bs->ainode != AINode_Battle_NBG ) {
		return;
	}
	now = FloatTime();
	if ( bs->enemy < 0 || bs->enemyvisible_time < now - 0.5f ) {
		return;
	}
	ps = &g_entities[bs->client].client->ps;
	if ( ps->groundEntityNum == ENTITYNUM_NONE || bs->travel_type != TRAVEL_WALK
		|| ( bi->actionflags & ( ACTION_JUMP | ACTION_DELAYEDJUMP | ACTION_CROUCH ) ) ) {
		return;
	}
	wishspeed = BotFootWish( bi, ps, want );
	if ( wishspeed < 200.0f ) {
		return;
	}
	if ( now > bs->jink_time ) {
		bs->jink_side = ( bs->jink_side > 0 ) ? -1 : 1;
		bs->jink_time = now + 0.3f + random() * 0.4f;
	}
	CrossProduct( want, up, side );
	c = 0.82f;
	s = 0.57f * bs->jink_side;
	VectorScale( want, c, cand );
	VectorMA( cand, s, side, cand );
	if ( !BotFootSafe( bs, ps, cand, wishspeed, qfalse, FOOT_GROUNDFRAMES ) ) {
		return;
	}
	VectorCopy( cand, bi->dir );
	bi->actionflags &= ~( ACTION_MOVEFORWARD | ACTION_MOVEBACK | ACTION_MOVELEFT | ACTION_MOVERIGHT );
}

/*
==================
BotSpeedJump

Springen, um schneller voranzukommen.

In Quake laeuft man beim Springen ohne Bodenreibung weiter - wer huepft,
behaelt sein Tempo, statt es in jedem Bild ein Stueck zu verlieren. Die Bots
machen das von sich aus nie: sie springen nur, wo die Karte es verlangt
(TRAVEL_JUMP, Sprungfeld, Hindernis) oder zufaellig im Gefecht.

Gesprungen wird nur, wo es wirklich hilft, und das heisst hier: der Bot ist
auf dem Boden, laeuft schon schnell, will weiter in dieselbe Richtung, und da
vorne ist auch Boden. Kein Huepfen beim Ausweichen - ein Bot in der Luft ist
auf einer Wurfparabel und damit leichter zu treffen, nicht schwerer.

PM_CheckJump verlangt, dass die Taste zwischendurch losgelassen wird
(PMF_JUMP_HELD), sonst bleibt es beim ersten Sprung. Weil hier nur auf dem
Boden gedrueckt wird und in der Luft nicht, loest sich das von selbst: beim
Absprung gesetzt, waehrend des Flugs nicht mehr, bei der Landung wieder.
==================
*/
static void BotSpeedJump( bot_state_t *bs, bot_input_t *bi ) {
	vec3_t		vel, dir;
	float		speed;

	if ( !BotSw(bs, g_botJump) ) {
		return;
	}
	if ( bs->cur_ps.groundEntityNum == ENTITYNUM_NONE
		|| ( bi->actionflags & ( ACTION_JUMP | ACTION_DELAYEDJUMP | ACTION_CROUCH ) )
		|| ( bs->cur_ps.pm_flags & PMF_JUMP_HELD ) ) {
		return;					// fliegt schon, springt schon, oder haelt noch
	}

	VectorCopy( bs->cur_ps.velocity, vel );
	vel[2] = 0.0f;
	speed = VectorLength( vel );
	// unter zweihundert lohnt es nicht - und wer kaum Tempo hat, will meist
	// gerade zielen und nicht reisen
	if ( speed < 200.0f || bi->speed < 300.0f ) {
		return;
	}
	VectorScale( vel, 1.0f / speed, dir );

	// Nur geradeaus: zeigt der gewollte Weg woandershin, wird gerade
	// ausgewichen oder gewendet, und ein Sprung macht beides schlechter.
	if ( DotProduct( dir, bi->dir ) < 0.9f ) {
		return;
	}
	// Und nicht ins Leere huepfen. Weiter voraus als beim Bremsen, weil ein
	// Sprung laenger traegt als ein Schritt.
	if ( !BotGroundAhead( bs, dir, 64.0f + speed * 0.6f ) ) {
		return;
	}

	bi->actionflags |= ACTION_JUMP;
}

void BotUpdateInput(bot_state_t *bs, int time, int elapsed_time) {
	bot_input_t bi;
	int j;

	//add the delta angles to the bot's current view angles
	for (j = 0; j < 3; j++) {
		bs->viewangles[j] = AngleMod(bs->viewangles[j] + SHORT2ANGLE(bs->cur_ps.delta_angles[j]));
	}
	//change the bot view angles
	BotChangeViewAngles(bs, (float) elapsed_time / 1000);
	//retrieve the bot input
	trap_EA_GetInput(bs->client, (float) time / 1000, &bi);
	//respawn hack
	if (bi.actionflags & ACTION_RESPAWN) {
		if (bs->lastucmd.buttons & BUTTON_ATTACK) bi.actionflags &= ~(ACTION_RESPAWN|ACTION_ATTACK);
	}
	// Werkbank: erst, was der Bot will - der Rakete ausweichen -, dann der
	// Tritt, der das letzte Wort hat. Und wenn er nicht eingegriffen hat, darf
	// gesprungen werden, um Tempo zu halten.
	BotDodge(bs, &bi);
	BotJink(bs, &bi);
	if ( !BotFooting(bs, &bi) ) {
		BotSpeedJump(bs, &bi);
	}
	// und zuletzt: nicht unter ein Pendel oder eine Plattform
	BotMoverGuard(bs, &bi);
	// Werkbank: g_botLog 3 schreibt jedes Bild des Servers mit - Zittern, das
	// schneller ist als ein Denkschritt, sieht man sonst nicht
	if ( g_botLog.integer >= 3 && g_entities[bs->client].client ) {
		playerState_t *fps = &g_entities[bs->client].client->ps;

		BotLogPrintf( "F %i %i %.0f %.0f %.0f %.0f %.0f %.0f %.2f %.2f %.0f %i %i %i %i\n",
			level.time, bs->client, fps->origin[0], fps->origin[1], fps->origin[2],
			fps->velocity[0], fps->velocity[1], bi.viewangles[YAW], bi.dir[0], bi.dir[1],
			bi.speed, bs->foot_kind, fps->groundEntityNum != ENTITYNUM_NONE,
			bs->travel_type, fps->pm_type );
	}
	//convert the bot input to a usercmd
	BotInputToUserCommand(&bi, &bs->lastucmd, bs->cur_ps.delta_angles, time);
	//subtract the delta angles
	for (j = 0; j < 3; j++) {
		bs->viewangles[j] = AngleMod(bs->viewangles[j] - SHORT2ANGLE(bs->cur_ps.delta_angles[j]));
	}
}

/*
==============
BotAIRegularUpdate
==============
*/
void BotAIRegularUpdate(void) {
	if (regularupdate_time < FloatTime()) {
		trap_BotUpdateEntityItems();
		regularupdate_time = FloatTime() + 0.3;
	}
}

/*
==============
RemoveColorEscapeSequences
==============
*/
void RemoveColorEscapeSequences( char *text ) {
	int i, l;

	l = 0;
	for ( i = 0; text[i]; i++ ) {
		if (Q_IsColorString(&text[i])) {
			i++;
			continue;
		}
		if (text[i] > 0x7E)
			continue;
		text[l++] = text[i];
	}
	text[l] = '\0';
}

/*
==================
BotTravelToGoal

Der Weg nach der Karte, und dazu das Wissen, welche Reiseart es war. Die
Knoten rufen sonst trap_BotMoveToGoal direkt und werfen das Ergebnis danach
weg; die Kantenbremse sitzt aber spaeter, in BotUpdateInput, und muss wissen,
ob der Bot gerade ueber eine Kante SOLL.
==================
*/
void BotTravelToGoal( bot_state_t *bs, bot_moveresult_t *moveresult, bot_goal_t *goal, int tfl ) {
	trap_BotMoveToGoal( moveresult, bs->ms, goal, tfl );
	bs->travel_type = moveresult->traveltype & TRAVELTYPE_MASK;
	if ( bs->travel_type && !moveresult->failure && bs->areanum ) {
		bs->lastgood_area = bs->areanum;
	}
	bs->log_goal = goal ? goal->number : 0;
	if ( moveresult->failure ) bs->log_flags |= BOTLOG_FAILURE;
	if ( moveresult->blocked ) bs->log_flags |= BOTLOG_BLOCKED;
	if ( goal && ( goal->flags & GFL_DROPPED ) ) bs->log_flags |= BOTLOG_DROPPED;
}

/*
==================
BotLogThink

Die Zeile je Denkschritt. Steht ein Bot laenger an einer Stelle oder laeuft er
im Kreis, laesst sich hier ablesen, in welchem Knoten, mit welchem Ziel und mit
welcher Reiseart - und ob die Kantenbremse dabei war.
==================
*/
static void BotLogThink( bot_state_t *bs ) {
	vec3_t	vel;

	if ( !g_botLog.integer ) {
		bs->log_flags = 0;
		bs->log_switches = 0;
		return;
	}
	VectorCopy( bs->cur_ps.velocity, vel );
	vel[2] = 0;
	BotLogPrintf( "T %i %i %s %.0f %.0f %.0f %.0f %.0f %i %i %i %i %i %i %i %i %i\n",
		level.time, bs->client, BotNodeName( bs ),
		bs->origin[0], bs->origin[1], bs->origin[2],
		VectorLength( vel ), bs->cur_ps.velocity[2],
		bs->cur_ps.groundEntityNum != ENTITYNUM_NONE,
		bs->inventory[INVENTORY_HEALTH], bs->inventory[INVENTORY_ARMOR],
		bs->cur_ps.weapon, bs->enemy,
		bs->log_goal, bs->travel_type, bs->log_flags, bs->log_switches );
	bs->log_flags = 0;
	bs->log_switches = 0;
}

/*
==============
BotAI
==============
*/
int BotAI(int client, float thinktime) {
	bot_state_t *bs;
	char buf[1024], *args;
	int j;

	trap_EA_ResetInput(client);
	//
	bs = botstates[client];
	if (!bs || !bs->inuse) {
		BotAI_Print(PRT_FATAL, "BotAI: client %d is not setup\n", client);
		return qfalse;
	}

	//retrieve the current client state
	if (!BotAI_GetClientState(client, &bs->cur_ps)) {
		BotAI_Print(PRT_FATAL, "BotAI: failed to get player state for player %d\n", client);
		return qfalse;
	}
	//retrieve any waiting server commands
	while( trap_BotGetServerCommand(client, buf, sizeof(buf)) ) {
		//have buf point to the command and args to the command arguments
		args = strchr( buf, ' ');
		if (!args) continue;
		*args++ = '\0';

		//remove color espace sequences from the arguments
		RemoveColorEscapeSequences( args );

		if (!Q_stricmp(buf, "cp "))
			{ /*CenterPrintf*/ }
		else if (!Q_stricmp(buf, "cs"))
			{ /*ConfigStringModified*/ }
		else if (!Q_stricmp(buf, "print")) {
			//remove first and last quote from the chat message
			memmove(args, args+1, strlen(args));
			args[strlen(args)-1] = '\0';
			trap_BotQueueConsoleMessage(bs->cs, CMS_NORMAL, args);
		}
		else if (!Q_stricmp(buf, "chat")) {
			//remove first and last quote from the chat message
			memmove(args, args+1, strlen(args));
			args[strlen(args)-1] = '\0';
			trap_BotQueueConsoleMessage(bs->cs, CMS_CHAT, args);
		}
		else if (!Q_stricmp(buf, "tchat")) {
			//remove first and last quote from the chat message
			memmove(args, args+1, strlen(args));
			args[strlen(args)-1] = '\0';
			trap_BotQueueConsoleMessage(bs->cs, CMS_CHAT, args);
		}
#ifdef MISSIONPACK
		else if (!Q_stricmp(buf, "vchat")) {
			BotVoiceChatCommand(bs, SAY_ALL, args);
		}
		else if (!Q_stricmp(buf, "vtchat")) {
			BotVoiceChatCommand(bs, SAY_TEAM, args);
		}
		else if (!Q_stricmp(buf, "vtell")) {
			BotVoiceChatCommand(bs, SAY_TELL, args);
		}
#endif
		else if (!Q_stricmp(buf, "scores"))
			{ /*FIXME: parse scores?*/ }
		else if (!Q_stricmp(buf, "clientLevelShot"))
			{ /*ignore*/ }
	}
	//add the delta angles to the bot's current view angles
	for (j = 0; j < 3; j++) {
		bs->viewangles[j] = AngleMod(bs->viewangles[j] + SHORT2ANGLE(bs->cur_ps.delta_angles[j]));
	}
	//increase the local time of the bot
	bs->ltime += thinktime;
	//
	bs->thinktime = thinktime;
	//origin of the bot
	VectorCopy(bs->cur_ps.origin, bs->origin);
	//eye coordinates of the bot
	VectorCopy(bs->cur_ps.origin, bs->eye);
	bs->eye[2] += bs->cur_ps.viewheight;
	//get the area the bot is in
	bs->areanum = BotPointAreaNum(bs->origin);
	// Werkbank: ein Wegbefehl aus dem vorigen Denkschritt zaehlt nicht mehr
	bs->travel_type = 0;
	bs->log_goal = 0;
	//the real AI
	BotDeathmatchAI(bs, thinktime);
	// Werkbank: kommt etwas geflogen, und steht er irgendwo, wo es nicht weitergeht
	BotDodgeThink(bs);
	BotStrandedThink(bs);
	// Werkbank: was dieser Denkschritt getan hat
	BotLogThink(bs);
	//set the weapon selection every AI frame
	trap_EA_SelectWeapon(bs->client, bs->weaponnum);
	//subtract the delta angles
	for (j = 0; j < 3; j++) {
		bs->viewangles[j] = AngleMod(bs->viewangles[j] - SHORT2ANGLE(bs->cur_ps.delta_angles[j]));
	}
	//everything was ok
	return qtrue;
}

/*
==================
BotScheduleBotThink
==================
*/
void BotScheduleBotThink(void) {
	int i, botnum;

	botnum = 0;

	for( i = 0; i < MAX_CLIENTS; i++ ) {
		if( !botstates[i] || !botstates[i]->inuse ) {
			continue;
		}
		//initialize the bot think residual time
		botstates[i]->botthink_residual = bot_thinktime.integer * botnum / numbots;
		botnum++;
	}
}

/*
==============
BotWriteSessionData
==============
*/
void BotWriteSessionData(bot_state_t *bs) {
	const char	*s;
	const char	*var;

	s = va(
			"%i %i %i %i %i %i %i %i"
			" %f %f %f"
			" %f %f %f"
			" %f %f %f"
			" %f",
		bs->lastgoal_decisionmaker,
		bs->lastgoal_ltgtype,
		bs->lastgoal_teammate,
		bs->lastgoal_teamgoal.areanum,
		bs->lastgoal_teamgoal.entitynum,
		bs->lastgoal_teamgoal.flags,
		bs->lastgoal_teamgoal.iteminfo,
		bs->lastgoal_teamgoal.number,
		bs->lastgoal_teamgoal.origin[0],
		bs->lastgoal_teamgoal.origin[1],
		bs->lastgoal_teamgoal.origin[2],
		bs->lastgoal_teamgoal.mins[0],
		bs->lastgoal_teamgoal.mins[1],
		bs->lastgoal_teamgoal.mins[2],
		bs->lastgoal_teamgoal.maxs[0],
		bs->lastgoal_teamgoal.maxs[1],
		bs->lastgoal_teamgoal.maxs[2],
		bs->formation_dist
		);

	var = va( "botsession%i", bs->client );

	trap_Cvar_Set( var, s );
}

/*
==============
BotReadSessionData
==============
*/
void BotReadSessionData(bot_state_t *bs) {
	char	s[MAX_STRING_CHARS];
	const char	*var;

	var = va( "botsession%i", bs->client );
	trap_Cvar_VariableStringBuffer( var, s, sizeof(s) );

	sscanf(s,
			"%i %i %i %i %i %i %i %i"
			" %f %f %f"
			" %f %f %f"
			" %f %f %f"
			" %f",
		&bs->lastgoal_decisionmaker,
		&bs->lastgoal_ltgtype,
		&bs->lastgoal_teammate,
		&bs->lastgoal_teamgoal.areanum,
		&bs->lastgoal_teamgoal.entitynum,
		&bs->lastgoal_teamgoal.flags,
		&bs->lastgoal_teamgoal.iteminfo,
		&bs->lastgoal_teamgoal.number,
		&bs->lastgoal_teamgoal.origin[0],
		&bs->lastgoal_teamgoal.origin[1],
		&bs->lastgoal_teamgoal.origin[2],
		&bs->lastgoal_teamgoal.mins[0],
		&bs->lastgoal_teamgoal.mins[1],
		&bs->lastgoal_teamgoal.mins[2],
		&bs->lastgoal_teamgoal.maxs[0],
		&bs->lastgoal_teamgoal.maxs[1],
		&bs->lastgoal_teamgoal.maxs[2],
		&bs->formation_dist
		);
}

/*
==============
BotAISetupClient
==============
*/
int BotAISetupClient(int client, struct bot_settings_s *settings, qboolean restart) {
	char filename[144], name[144], gender[144];
	bot_state_t *bs;
	int errnum;

	if (!botstates[client]) botstates[client] = G_Alloc(sizeof(bot_state_t));
	bs = botstates[client];

	if (!bs) {
		return qfalse;
	}

	if (bs && bs->inuse) {
		BotAI_Print(PRT_FATAL, "BotAISetupClient: client %d already setup\n", client);
		return qfalse;
	}

	if (!trap_AAS_Initialized()) {
		BotAI_Print(PRT_FATAL, "AAS not initialized\n");
		return qfalse;
	}

	//load the bot character
	bs->character = trap_BotLoadCharacter(settings->characterfile, settings->skill);
	if (!bs->character) {
		BotAI_Print(PRT_FATAL, "couldn't load skill %f from %s\n", settings->skill, settings->characterfile);
		return qfalse;
	}
	//copy the settings
	memcpy(&bs->settings, settings, sizeof(bot_settings_t));
	//allocate a goal state
	bs->gs = trap_BotAllocGoalState(client);
	//load the item weights
	trap_Characteristic_String(bs->character, CHARACTERISTIC_ITEMWEIGHTS, filename, sizeof(filename));
	errnum = trap_BotLoadItemWeights(bs->gs, filename);
	if (errnum != BLERR_NOERROR) {
		trap_BotFreeGoalState(bs->gs);
		return qfalse;
	}
	//allocate a weapon state
	bs->ws = trap_BotAllocWeaponState();
	//load the weapon weights
	trap_Characteristic_String(bs->character, CHARACTERISTIC_WEAPONWEIGHTS, filename, sizeof(filename));
	errnum = trap_BotLoadWeaponWeights(bs->ws, filename);
	if (errnum != BLERR_NOERROR) {
		trap_BotFreeGoalState(bs->gs);
		trap_BotFreeWeaponState(bs->ws);
		return qfalse;
	}
	//allocate a chat state
	bs->cs = trap_BotAllocChatState();
	//load the chat file
	trap_Characteristic_String(bs->character, CHARACTERISTIC_CHAT_FILE, filename, sizeof(filename));
	trap_Characteristic_String(bs->character, CHARACTERISTIC_CHAT_NAME, name, sizeof(name));
	errnum = trap_BotLoadChatFile(bs->cs, filename, name);
	if (errnum != BLERR_NOERROR) {
		trap_BotFreeChatState(bs->cs);
		trap_BotFreeGoalState(bs->gs);
		trap_BotFreeWeaponState(bs->ws);
		return qfalse;
	}
	//get the gender characteristic
	trap_Characteristic_String(bs->character, CHARACTERISTIC_GENDER, gender, sizeof(gender));
	//set the chat gender
	if (*gender == 'f' || *gender == 'F') trap_BotSetChatGender(bs->cs, CHAT_GENDERFEMALE);
	else if (*gender == 'm' || *gender == 'M') trap_BotSetChatGender(bs->cs, CHAT_GENDERMALE);
	else trap_BotSetChatGender(bs->cs, CHAT_GENDERLESS);

	bs->inuse = qtrue;
	bs->client = client;
	bs->entitynum = client;
	bs->setupcount = 4;
	bs->entergame_time = FloatTime();
	bs->ms = trap_BotAllocMoveState();
	bs->walker = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_WALKER, 0, 1);
	numbots++;

	if (trap_Cvar_VariableIntegerValue("bot_testichat")) {
		trap_BotLibVarSet("bot_testichat", "1");
		BotChatTest(bs);
	}
	//NOTE: reschedule the bot thinking
	BotScheduleBotThink();
	//if interbreeding start with a mutation
	if (bot_interbreed) {
		trap_BotMutateGoalFuzzyLogic(bs->gs, 1);
	}
	// if we kept the bot client
	if (restart) {
		BotReadSessionData(bs);
	}
	//bot has been setup successfully
	return qtrue;
}

/*
==============
BotAIShutdownClient
==============
*/
int BotAIShutdownClient(int client, qboolean restart) {
	bot_state_t *bs;

	bs = botstates[client];
	if (!bs || !bs->inuse) {
		//BotAI_Print(PRT_ERROR, "BotAIShutdownClient: client %d already shutdown\n", client);
		return qfalse;
	}

	if (restart) {
		BotWriteSessionData(bs);
	}

	if (BotChat_ExitGame(bs)) {
		trap_BotEnterChat(bs->cs, bs->client, CHAT_ALL);
	}

	trap_BotFreeMoveState(bs->ms);
	//free the goal state
	trap_BotFreeGoalState(bs->gs);
	//free the chat file
	trap_BotFreeChatState(bs->cs);
	//free the weapon weights
	trap_BotFreeWeaponState(bs->ws);
	//free the bot character
	trap_BotFreeCharacter(bs->character);
	//
	BotFreeWaypoints(bs->checkpoints);
	BotFreeWaypoints(bs->patrolpoints);
	//clear activate goal stack
	BotClearActivateGoalStack(bs);
	//clear the bot state
	memset(bs, 0, sizeof(bot_state_t));
	//set the inuse flag to qfalse
	bs->inuse = qfalse;
	//there's one bot less
	numbots--;
	//everything went ok
	return qtrue;
}

/*
==============
BotResetState

called when a bot enters the intermission or observer mode and
when the level is changed
==============
*/
void BotResetState(bot_state_t *bs) {
	int client, entitynum, inuse;
	int movestate, goalstate, chatstate, weaponstate;
	bot_settings_t settings;
	int character;
	playerState_t ps;							//current player state
	float entergame_time;

	//save some things that should not be reset here
	memcpy(&settings, &bs->settings, sizeof(bot_settings_t));
	memcpy(&ps, &bs->cur_ps, sizeof(playerState_t));
	inuse = bs->inuse;
	client = bs->client;
	entitynum = bs->entitynum;
	character = bs->character;
	movestate = bs->ms;
	goalstate = bs->gs;
	chatstate = bs->cs;
	weaponstate = bs->ws;
	entergame_time = bs->entergame_time;
	//free checkpoints and patrol points
	BotFreeWaypoints(bs->checkpoints);
	BotFreeWaypoints(bs->patrolpoints);
	//reset the whole state
	memset(bs, 0, sizeof(bot_state_t));
	//copy back some state stuff that should not be reset
	bs->ms = movestate;
	bs->gs = goalstate;
	bs->cs = chatstate;
	bs->ws = weaponstate;
	memcpy(&bs->cur_ps, &ps, sizeof(playerState_t));
	memcpy(&bs->settings, &settings, sizeof(bot_settings_t));
	bs->inuse = inuse;
	bs->client = client;
	bs->entitynum = entitynum;
	bs->character = character;
	bs->entergame_time = entergame_time;
	//reset several states
	if (bs->ms) trap_BotResetMoveState(bs->ms);
	if (bs->gs) trap_BotResetGoalState(bs->gs);
	if (bs->ws) trap_BotResetWeaponState(bs->ws);
	if (bs->gs) trap_BotResetAvoidGoals(bs->gs);
	if (bs->ms) trap_BotResetAvoidReach(bs->ms);
}

static void BotLibItemSetup(void);

/*
==============
BotAILoadMap
==============
*/
int BotAILoadMap( int restart ) {
	int			i;
	vmCvar_t	mapname;

	// Werkbank: vor dem Laden, damit die Gegenstaende gleich richtig zugeordnet werden
	BotLibItemSetup();
	if (!restart) {
		trap_Cvar_Register( &mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM );
		trap_BotLibLoadMap( mapname.string );
	}

	for (i = 0; i < MAX_CLIENTS; i++) {
		if (botstates[i] && botstates[i]->inuse) {
			BotResetState( botstates[i] );
			botstates[i]->setupcount = 4;
		}
	}

	BotSetupDeathmatchAI();

	return qtrue;
}

#ifdef MISSIONPACK
void ProximityMine_Trigger( gentity_t *trigger, gentity_t *other, trace_t *trace );
#endif

/*
==================
BotAIStartFrame
==================
*/
int BotAIStartFrame(int time) {
	int i;
	gentity_t	*ent;
	bot_entitystate_t state;
	int elapsed_time, thinktime;
	static int local_time;
	static int botlib_residual;
	static int lastbotthink_time;

	G_CheckBotSpawn();

	trap_Cvar_Update(&bot_rocketjump);
	trap_Cvar_Update(&bot_grapple);
	trap_Cvar_Update(&bot_fastchat);
	trap_Cvar_Update(&bot_nochat);
	trap_Cvar_Update(&bot_testrchat);
	trap_Cvar_Update(&bot_thinktime);
	trap_Cvar_Update(&bot_memorydump);
	trap_Cvar_Update(&bot_saveroutingcache);
	trap_Cvar_Update(&bot_pause);
	trap_Cvar_Update(&bot_report);

	if (bot_report.integer) {
//		BotTeamplayReport();
//		trap_Cvar_Set("bot_report", "0");
		BotUpdateInfoConfigStrings();
	}

	if (bot_pause.integer) {
		// execute bot user commands every frame
		for( i = 0; i < MAX_CLIENTS; i++ ) {
			if( !botstates[i] || !botstates[i]->inuse ) {
				continue;
			}
			if( g_entities[i].client->pers.connected != CON_CONNECTED ) {
				continue;
			}
			botstates[i]->lastucmd.forwardmove = 0;
			botstates[i]->lastucmd.rightmove = 0;
			botstates[i]->lastucmd.upmove = 0;
			botstates[i]->lastucmd.buttons = 0;
			botstates[i]->lastucmd.serverTime = time;
			trap_BotUserCommand(botstates[i]->client, &botstates[i]->lastucmd);
		}
		return qtrue;
	}

	if (bot_memorydump.integer) {
		trap_BotLibVarSet("memorydump", "1");
		trap_Cvar_Set("bot_memorydump", "0");
	}
	if (bot_saveroutingcache.integer) {
		trap_BotLibVarSet("saveroutingcache", "1");
		trap_Cvar_Set("bot_saveroutingcache", "0");
	}
	//check if bot interbreeding is activated
	BotInterbreeding();
	//cap the bot think time
	if (bot_thinktime.integer > 200) {
		trap_Cvar_Set("bot_thinktime", "200");
	}
	//if the bot think time changed we should reschedule the bots
	if (bot_thinktime.integer != lastbotthink_time) {
		lastbotthink_time = bot_thinktime.integer;
		BotScheduleBotThink();
	}

	elapsed_time = time - local_time;
	local_time = time;

	botlib_residual += elapsed_time;

	if (elapsed_time > bot_thinktime.integer) thinktime = elapsed_time;
	else thinktime = bot_thinktime.integer;

	// update the bot library
	if ( botlib_residual >= thinktime ) {
		botlib_residual -= thinktime;

		trap_BotLibStartFrame((float) time / 1000);

		if (!trap_AAS_Initialized()) return qfalse;

		//update entities in the botlib
		for (i = 0; i < MAX_GENTITIES; i++) {
			ent = &g_entities[i];
			if (!ent->inuse) {
				trap_BotLibUpdateEntity(i, NULL);
				continue;
			}
			if (!ent->r.linked) {
				trap_BotLibUpdateEntity(i, NULL);
				continue;
			}
			if (ent->r.svFlags & SVF_NOCLIENT) {
				trap_BotLibUpdateEntity(i, NULL);
				continue;
			}
			// do not update missiles
			if (ent->s.eType == ET_MISSILE && ent->s.weapon != WP_GRAPPLING_HOOK) {
				trap_BotLibUpdateEntity(i, NULL);
				continue;
			}
			// do not update event only entities
			if (ent->s.eType > ET_EVENTS) {
				trap_BotLibUpdateEntity(i, NULL);
				continue;
			}
#ifdef MISSIONPACK
			// never link prox mine triggers
			if (ent->r.contents == CONTENTS_TRIGGER) {
				if (ent->touch == ProximityMine_Trigger) {
					trap_BotLibUpdateEntity(i, NULL);
					continue;
				}
			}
#endif
			//
			memset(&state, 0, sizeof(bot_entitystate_t));
			//
			VectorCopy(ent->r.currentOrigin, state.origin);
			if (i < MAX_CLIENTS) {
				VectorCopy(ent->s.apos.trBase, state.angles);
			} else {
				VectorCopy(ent->r.currentAngles, state.angles);
			}
			VectorCopy(ent->s.origin2, state.old_origin);
			VectorCopy(ent->r.mins, state.mins);
			VectorCopy(ent->r.maxs, state.maxs);
			state.type = ent->s.eType;
			state.flags = ent->s.eFlags;
			if (ent->r.bmodel) state.solid = SOLID_BSP;
			else state.solid = SOLID_BBOX;
			state.groundent = ent->s.groundEntityNum;
			state.modelindex = ent->s.modelindex;
			state.modelindex2 = ent->s.modelindex2;
			state.frame = ent->s.frame;
			state.event = ent->s.event;
			state.eventParm = ent->s.eventParm;
			state.powerups = ent->s.powerups;
			state.legsAnim = ent->s.legsAnim;
			state.torsoAnim = ent->s.torsoAnim;
			state.weapon = ent->s.weapon;
			//
			trap_BotLibUpdateEntity(i, &state);
		}

		BotAIRegularUpdate();
	}

	floattime = trap_AAS_Time();

	// execute scheduled bot AI
	for( i = 0; i < MAX_CLIENTS; i++ ) {
		if( !botstates[i] || !botstates[i]->inuse ) {
			continue;
		}
		//
		botstates[i]->botthink_residual += elapsed_time;
		//
		if ( botstates[i]->botthink_residual >= thinktime ) {
			botstates[i]->botthink_residual -= thinktime;

			if (!trap_AAS_Initialized()) return qfalse;

			if (g_entities[i].client->pers.connected == CON_CONNECTED) {
				BotAI(i, (float) thinktime / 1000);
			}
		}
	}


	// execute bot user commands every frame
	for( i = 0; i < MAX_CLIENTS; i++ ) {
		if( !botstates[i] || !botstates[i]->inuse ) {
			continue;
		}
		if( g_entities[i].client->pers.connected != CON_CONNECTED ) {
			continue;
		}

		BotUpdateInput(botstates[i], time, elapsed_time);
		trap_BotUserCommand(botstates[i]->client, &botstates[i]->lastucmd);
	}

	return qtrue;
}

/*
==============
BotInitLibrary
==============
*/
int BotInitLibrary(void) {
	char buf[144];

	//set the maxclients and maxentities library variables before calling BotSetupLibrary
	Com_sprintf(buf, sizeof(buf), "%d", level.maxclients);
	trap_BotLibVarSet("maxclients", buf);
	Com_sprintf(buf, sizeof(buf), "%d", MAX_GENTITIES);
	trap_BotLibVarSet("maxentities", buf);
	//bsp checksum
	trap_Cvar_VariableStringBuffer("sv_mapChecksum", buf, sizeof(buf));
	if (strlen(buf)) trap_BotLibVarSet("sv_mapChecksum", buf);
	//maximum number of aas links
	trap_Cvar_VariableStringBuffer("max_aaslinks", buf, sizeof(buf));
	if (strlen(buf)) trap_BotLibVarSet("max_aaslinks", buf);
	//maximum number of items in a level
	trap_Cvar_VariableStringBuffer("max_levelitems", buf, sizeof(buf));
	if (strlen(buf)) trap_BotLibVarSet("max_levelitems", buf);
	//game type
	trap_Cvar_VariableStringBuffer("g_gametype", buf, sizeof(buf));
	if (!strlen(buf)) strcpy(buf, "0");
	trap_BotLibVarSet("g_gametype", buf);
	//bot developer mode and log file
	trap_BotLibVarSet("bot_developer", bot_developer.string);
	trap_Cvar_VariableStringBuffer("logfile", buf, sizeof(buf));
	trap_BotLibVarSet("log", buf);
	//no chatting
	trap_Cvar_VariableStringBuffer("bot_nochat", buf, sizeof(buf));
	if (strlen(buf)) trap_BotLibVarSet("nochat", buf);
	//visualize jump pads
	trap_Cvar_VariableStringBuffer("bot_visualizejumppads", buf, sizeof(buf));
	if (strlen(buf)) trap_BotLibVarSet("bot_visualizejumppads", buf);
	//forced clustering calculations
	trap_Cvar_VariableStringBuffer("bot_forceclustering", buf, sizeof(buf));
	if (strlen(buf)) trap_BotLibVarSet("forceclustering", buf);
	//forced reachability calculations
	trap_Cvar_VariableStringBuffer("bot_forcereachability", buf, sizeof(buf));
	if (strlen(buf)) trap_BotLibVarSet("forcereachability", buf);
	//force writing of AAS to file
	trap_Cvar_VariableStringBuffer("bot_forcewrite", buf, sizeof(buf));
	if (strlen(buf)) trap_BotLibVarSet("forcewrite", buf);
	//no AAS optimization
	trap_Cvar_VariableStringBuffer("bot_aasoptimize", buf, sizeof(buf));
	if (strlen(buf)) trap_BotLibVarSet("aasoptimize", buf);
	//
	trap_Cvar_VariableStringBuffer("bot_saveroutingcache", buf, sizeof(buf));
	if (strlen(buf)) trap_BotLibVarSet("saveroutingcache", buf);
	//reload instead of cache bot character files
	trap_Cvar_VariableStringBuffer("bot_reloadcharacters", buf, sizeof(buf));
	if (!strlen(buf)) strcpy(buf, "0");
	trap_BotLibVarSet("bot_reloadcharacters", buf);
	//base directory
	trap_Cvar_VariableStringBuffer("fs_basepath", buf, sizeof(buf));
	if (strlen(buf)) trap_BotLibVarSet("basedir", buf);
	//game directory
	trap_Cvar_VariableStringBuffer("fs_game", buf, sizeof(buf));
	if (strlen(buf)) trap_BotLibVarSet("gamedir", buf);
	//
	// Wird die Karte auf wenige Waffen eingeengt (g_weaponSpawns, siehe
	// G_SubstituteSpawnItem in g_items.c), liegt auf einem Sockel etwas
	// anderes, als die AAS-Datei dort verzeichnet hat. Die Botbibliothek
	// erkennt ihre Gegenstaende am Modelindex (BotUpdateEntityItems in
	// be_ai_goal.c) und haelt jeden, den sie so nicht wiederfindet, fuer einen
	// fallengelassenen - und fallengelassene sind ihr per Aufschlag von
	// tausend Punkten besonders wertvoll (be_ai_goal.c:1362). Die Bots wuerden
	// also auf jeden ersetzten Sockel zustuermen, und genau sie sind in diesem
	// Labor die Kontrollgruppe: ihr Verhalten darf sich durch eine
	// Messeinstellung nicht aendern. Der Aufschlag faellt deshalb weg, solange
	// eingeengt wird.
	//
	// Seit die Bibliothek solche Sockel umwidmen kann (BotRelinkSocket), ist
	// das nur noch der Behelf fuer eine aeltere ioquake3.exe, und er steht in
	// BotLibItemSetup - dort, wo er auch bei einem Neustart der Karte laeuft.
	//
#ifdef MISSIONPACK
	trap_BotLibDefine("MISSIONPACK");
#endif
	//setup the bot library
	return trap_BotLibSetup();
}

/*
==============
BotLibItemSetup

Was die Bibliothek ueber die Gegenstaende dieser Karte wissen muss: ob die
Sockel etwas anderes tragen als im BSP steht, und was Fallengelassenes wert
ist. Laeuft bei jedem Laden UND bei jedem Neustart der Karte - die Bibliothek
wird bei einem Neustart nicht abgebaut, und was hier einmal gesetzt wurde,
stuende sonst bis zum naechsten "map" weiter, auch wenn die Cvars laengst
wieder beim Original sind.
==============
*/
static void BotLibItemSetup(void) {
	char buf[144];
	qboolean narrowed;

	trap_Cvar_VariableStringBuffer("g_weaponSpawns", buf, sizeof(buf));
	narrowed = (strlen(buf) > 0);
	trap_BotLibVarSet("relinksockets", narrowed ? "1" : "0");
	// in beide Richtungen: tausend ist die Vorgabe der Bibliothek
	if (g_botDroppedWeight.value >= 0) {
		trap_BotLibVarSet("droppedweight", va("%i", g_botDroppedWeight.integer));
	} else {
		trap_BotLibVarSet("droppedweight", "1000");
	}
	if (narrowed) {
		buf[0] = '\0';
		trap_BotLibVarGet("relinksockets_ok", buf, sizeof(buf));
		if (buf[0] != '1') {
			trap_BotLibVarSet("droppedweight", "0");
			G_Printf("bots: diese ioquake3.exe kennt relinksockets nicht, droppedweight 0 als Behelf\n");
		}
	}
}

/*
==============
BotAISetup
==============
*/
int BotAISetup( int restart ) {
	int			errnum;

	trap_Cvar_Register(&bot_thinktime, "bot_thinktime", "100", CVAR_CHEAT);
	trap_Cvar_Register(&bot_memorydump, "bot_memorydump", "0", CVAR_CHEAT);
	trap_Cvar_Register(&bot_saveroutingcache, "bot_saveroutingcache", "0", CVAR_CHEAT);
	trap_Cvar_Register(&bot_pause, "bot_pause", "0", CVAR_CHEAT);
	trap_Cvar_Register(&bot_report, "bot_report", "0", CVAR_CHEAT);
	trap_Cvar_Register(&bot_testsolid, "bot_testsolid", "0", CVAR_CHEAT);
	trap_Cvar_Register(&bot_testclusters, "bot_testclusters", "0", CVAR_CHEAT);
	trap_Cvar_Register(&bot_developer, "bot_developer", "0", CVAR_CHEAT);
	trap_Cvar_Register(&bot_interbreedchar, "bot_interbreedchar", "", 0);
	trap_Cvar_Register(&bot_interbreedbots, "bot_interbreedbots", "10", 0);
	trap_Cvar_Register(&bot_interbreedcycle, "bot_interbreedcycle", "20", 0);
	trap_Cvar_Register(&bot_interbreedwrite, "bot_interbreedwrite", "", 0);

	//if the game is restarted for a tournament
	if (restart) {
		return qtrue;
	}

	//initialize the bot states
	memset( botstates, 0, sizeof(botstates) );

	errnum = BotInitLibrary();
	if (errnum != BLERR_NOERROR) return qfalse;
	return qtrue;
}

/*
==============
BotAIShutdown
==============
*/
int BotAIShutdown( int restart ) {

	int i;

	BotLogClose();

	//if the game is restarted for a tournament
	if ( restart ) {
		//shutdown all the bots in the botlib
		for (i = 0; i < MAX_CLIENTS; i++) {
			if (botstates[i] && botstates[i]->inuse) {
				BotAIShutdownClient(botstates[i]->client, restart);
			}
		}
		//don't shutdown the bot library
	}
	else {
		trap_BotLibShutdown();
	}
	return qtrue;
}

