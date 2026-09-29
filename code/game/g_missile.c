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
#include "g_local.h"

#define	MISSILE_PRESTEP_TIME	50

/*
================
G_BounceMissile

================
*/
void G_BounceMissile( gentity_t *ent, trace_t *trace ) {
	vec3_t	velocity;
	float	dot;
	int		hitTime;

	// reflect the velocity on the trace plane
	hitTime = level.previousTime + ( level.time - level.previousTime ) * trace->fraction;
	BG_EvaluateTrajectoryDelta( &ent->s.pos, hitTime, velocity );
	dot = DotProduct( velocity, trace->plane.normal );
	VectorMA( velocity, -2*dot, trace->plane.normal, ent->s.pos.trDelta );

	if ( ent->s.eFlags & EF_BOUNCE_HALF ) {
		VectorScale( ent->s.pos.trDelta, 0.65, ent->s.pos.trDelta );
		// check for stop
		if ( trace->plane.normal[2] > 0.2 && VectorLength( ent->s.pos.trDelta ) < 40 ) {
			G_SetOrigin( ent, trace->endpos );
			ent->s.time = level.time / 4;
			return;
		}
	}

	VectorAdd( ent->r.currentOrigin, trace->plane.normal, ent->r.currentOrigin);
	VectorCopy( ent->r.currentOrigin, ent->s.pos.trBase );
	ent->s.pos.trTime = level.time;
}


/*
================
G_ExplodeMissile

Explode a missile without an impact
================
*/
void G_ExplodeMissile( gentity_t *ent ) {
	vec3_t		dir;
	vec3_t		origin;

	BG_EvaluateTrajectory( &ent->s.pos, level.time, origin );
	SnapVector( origin );
	G_SetOrigin( ent, origin );

	// we don't have a valid direction, so just point straight up
	dir[0] = dir[1] = 0;
	dir[2] = 1;

	ent->s.eType = ET_GENERAL;
	G_AddEvent( ent, EV_MISSILE_MISS, DirToByte( dir ) );

	ent->freeAfterEvent = qtrue;

	// splash damage
	if ( ent->splashDamage ) {
		if( G_RadiusDamage( ent->r.currentOrigin, ent->parent, ent->splashDamage, ent->splashRadius, ent
			, ent->splashMethodOfDeath ) ) {
			g_entities[ent->r.ownerNum].client->accuracy_hits++;
		}
	}

	trap_LinkEntity( ent );
}


#ifdef MISSIONPACK
/*
================
ProximityMine_Explode
================
*/
static void ProximityMine_Explode( gentity_t *mine ) {
	G_ExplodeMissile( mine );
	// if the prox mine has a trigger free it
	if (mine->activator) {
		G_FreeEntity(mine->activator);
		mine->activator = NULL;
	}
}

/*
================
ProximityMine_Die
================
*/
static void ProximityMine_Die( gentity_t *ent, gentity_t *inflictor, gentity_t *attacker, int damage, int mod ) {
	ent->think = ProximityMine_Explode;
	ent->nextthink = level.time + 1;
}

/*
================
ProximityMine_Trigger
================
*/
void ProximityMine_Trigger( gentity_t *trigger, gentity_t *other, trace_t *trace ) {
	vec3_t		v;
	gentity_t	*mine;

	if( !other->client ) {
		return;
	}

	// trigger is a cube, do a distance test now to act as if it's a sphere
	VectorSubtract( trigger->s.pos.trBase, other->s.pos.trBase, v );
	if( VectorLength( v ) > trigger->parent->splashRadius ) {
		return;
	}


	if ( g_gametype.integer >= GT_TEAM ) {
		// don't trigger same team mines
		if (trigger->parent->s.generic1 == other->client->sess.sessionTeam) {
			return;
		}
	}

	// ok, now check for ability to damage so we don't get triggered through walls, closed doors, etc...
	if( !CanDamage( other, trigger->s.pos.trBase ) ) {
		return;
	}

	// trigger the mine!
	mine = trigger->parent;
	mine->s.loopSound = 0;
	G_AddEvent( mine, EV_PROXIMITY_MINE_TRIGGER, 0 );
	mine->nextthink = level.time + 500;

	G_FreeEntity( trigger );
}

/*
================
ProximityMine_Activate
================
*/
static void ProximityMine_Activate( gentity_t *ent ) {
	gentity_t	*trigger;
	float		r;

	ent->think = ProximityMine_Explode;
	ent->nextthink = level.time + g_proxMineTimeout.integer;

	ent->takedamage = qtrue;
	ent->health = 1;
	ent->die = ProximityMine_Die;

	ent->s.loopSound = G_SoundIndex( "sound/weapons/proxmine/wstbtick.wav" );

	// build the proximity trigger
	trigger = G_Spawn ();

	trigger->classname = "proxmine_trigger";

	r = ent->splashRadius;
	VectorSet( trigger->r.mins, -r, -r, -r );
	VectorSet( trigger->r.maxs, r, r, r );

	G_SetOrigin( trigger, ent->s.pos.trBase );

	trigger->parent = ent;
	trigger->r.contents = CONTENTS_TRIGGER;
	trigger->touch = ProximityMine_Trigger;

	trap_LinkEntity (trigger);

	// set pointer to trigger so the entity can be freed when the mine explodes
	ent->activator = trigger;
}

/*
================
ProximityMine_ExplodeOnPlayer
================
*/
static void ProximityMine_ExplodeOnPlayer( gentity_t *mine ) {
	gentity_t	*player;

	player = mine->enemy;
	player->client->ps.eFlags &= ~EF_TICKING;

	if ( player->client->invulnerabilityTime > level.time ) {
		G_Damage( player, mine->parent, mine->parent, vec3_origin, mine->s.origin, 1000, DAMAGE_NO_KNOCKBACK, MOD_JUICED );
		player->client->invulnerabilityTime = 0;
		G_TempEntity( player->client->ps.origin, EV_JUICED );
	}
	else {
		G_SetOrigin( mine, player->s.pos.trBase );
		// make sure the explosion gets to the client
		mine->r.svFlags &= ~SVF_NOCLIENT;
		mine->splashMethodOfDeath = MOD_PROXIMITY_MINE;
		G_ExplodeMissile( mine );
	}
}

/*
================
ProximityMine_Player
================
*/
static void ProximityMine_Player( gentity_t *mine, gentity_t *player ) {
	if( mine->s.eFlags & EF_NODRAW ) {
		return;
	}

	G_AddEvent( mine, EV_PROXIMITY_MINE_STICK, 0 );

	if( player->s.eFlags & EF_TICKING ) {
		player->activator->splashDamage += mine->splashDamage;
		player->activator->splashRadius *= 1.50;
		mine->think = G_FreeEntity;
		mine->nextthink = level.time;
		return;
	}

	player->client->ps.eFlags |= EF_TICKING;
	player->activator = mine;

	mine->s.eFlags |= EF_NODRAW;
	mine->r.svFlags |= SVF_NOCLIENT;
	mine->s.pos.trType = TR_LINEAR;
	VectorClear( mine->s.pos.trDelta );

	mine->enemy = player;
	mine->think = ProximityMine_ExplodeOnPlayer;
	if ( player->client->invulnerabilityTime > level.time ) {
		mine->nextthink = level.time + 2 * 1000;
	}
	else {
		mine->nextthink = level.time + 10 * 1000;
	}
}
#endif

/*
================
G_MissileImpact
================
*/
void G_MissileImpact( gentity_t *ent, trace_t *trace ) {
	gentity_t		*other;
	qboolean		hitClient = qfalse;
#ifdef MISSIONPACK
	vec3_t			forward, impactpoint, bouncedir;
	int				eFlags;
#endif
	other = &g_entities[trace->entityNum];

	// check for bounce
	if ( !other->takedamage &&
		( ent->s.eFlags & ( EF_BOUNCE | EF_BOUNCE_HALF ) ) ) {
		G_BounceMissile( ent, trace );
		G_AddEvent( ent, EV_GRENADE_BOUNCE, 0 );
		return;
	}

#ifdef MISSIONPACK
	if ( other->takedamage ) {
		if ( ent->s.weapon != WP_PROX_LAUNCHER ) {
			if ( other->client && other->client->invulnerabilityTime > level.time ) {
				//
				VectorCopy( ent->s.pos.trDelta, forward );
				VectorNormalize( forward );
				if (G_InvulnerabilityEffect( other, forward, ent->s.pos.trBase, impactpoint, bouncedir )) {
					VectorCopy( bouncedir, trace->plane.normal );
					eFlags = ent->s.eFlags & EF_BOUNCE_HALF;
					ent->s.eFlags &= ~EF_BOUNCE_HALF;
					G_BounceMissile( ent, trace );
					ent->s.eFlags |= eFlags;
				}
				ent->target_ent = other;
				return;
			}
		}
	}
#endif
	// impact damage
	if (other->takedamage) {
		// FIXME: wrong damage direction?
		if ( ent->damage ) {
			vec3_t	velocity;

			if( LogAccuracyHit( other, &g_entities[ent->r.ownerNum] ) ) {
				g_entities[ent->r.ownerNum].client->accuracy_hits++;
				hitClient = qtrue;
			}
			BG_EvaluateTrajectoryDelta( &ent->s.pos, level.time, velocity );
			if ( VectorLength( velocity ) == 0 ) {
				velocity[2] = 1;	// stepped on a grenade
			}
			G_Damage (other, ent, &g_entities[ent->r.ownerNum], velocity,
				ent->s.origin, ent->damage, 
				0, ent->methodOfDeath);
		}
	}

#ifdef MISSIONPACK
	if( ent->s.weapon == WP_PROX_LAUNCHER ) {
		if( ent->s.pos.trType != TR_GRAVITY ) {
			return;
		}

		// if it's a player, stick it on to them (flag them and remove this entity)
		if( other->s.eType == ET_PLAYER && other->health > 0 ) {
			ProximityMine_Player( ent, other );
			return;
		}

		SnapVectorTowards( trace->endpos, ent->s.pos.trBase );
		G_SetOrigin( ent, trace->endpos );
		ent->s.pos.trType = TR_STATIONARY;
		VectorClear( ent->s.pos.trDelta );

		G_AddEvent( ent, EV_PROXIMITY_MINE_STICK, trace->surfaceFlags );

		ent->think = ProximityMine_Activate;
		ent->nextthink = level.time + 2000;

		vectoangles( trace->plane.normal, ent->s.angles );
		ent->s.angles[0] += 90;

		// link the prox mine to the other entity
		ent->enemy = other;
		ent->die = ProximityMine_Die;
		VectorCopy(trace->plane.normal, ent->movedir);
		VectorSet(ent->r.mins, -4, -4, -4);
		VectorSet(ent->r.maxs, 4, 4, 4);
		trap_LinkEntity(ent);

		return;
	}
#endif

	if (!strcmp(ent->classname, "hook")) {
		gentity_t *nent;
		vec3_t v;

		nent = G_Spawn();
		if ( other->takedamage && other->client ) {

			G_AddEvent( nent, EV_MISSILE_HIT, DirToByte( trace->plane.normal ) );
			nent->s.otherEntityNum = other->s.number;

			ent->enemy = other;

			v[0] = other->r.currentOrigin[0] + (other->r.mins[0] + other->r.maxs[0]) * 0.5;
			v[1] = other->r.currentOrigin[1] + (other->r.mins[1] + other->r.maxs[1]) * 0.5;
			v[2] = other->r.currentOrigin[2] + (other->r.mins[2] + other->r.maxs[2]) * 0.5;

			SnapVectorTowards( v, ent->s.pos.trBase );	// save net bandwidth
		} else {
			VectorCopy(trace->endpos, v);
			G_AddEvent( nent, EV_MISSILE_MISS, DirToByte( trace->plane.normal ) );
			ent->enemy = NULL;
		}

		SnapVectorTowards( v, ent->s.pos.trBase );	// save net bandwidth

		nent->freeAfterEvent = qtrue;
		// change over to a normal entity right at the point of impact
		nent->s.eType = ET_GENERAL;
		ent->s.eType = ET_GRAPPLE;

		G_SetOrigin( ent, v );
		G_SetOrigin( nent, v );

		ent->think = Weapon_HookThink;
		ent->nextthink = level.time + FRAMETIME;

		ent->parent->client->ps.pm_flags |= PMF_GRAPPLE_PULL;
		VectorCopy( ent->r.currentOrigin, ent->parent->client->ps.grapplePoint);

		trap_LinkEntity( ent );
		trap_LinkEntity( nent );

		return;
	}

	// is it cheaper in bandwidth to just remove this ent and create a new
	// one, rather than changing the missile into the explosion?

	if ( other->takedamage && other->client ) {
		G_AddEvent( ent, EV_MISSILE_HIT, DirToByte( trace->plane.normal ) );
		ent->s.otherEntityNum = other->s.number;
	} else if( trace->surfaceFlags & SURF_METALSTEPS ) {
		G_AddEvent( ent, EV_MISSILE_MISS_METAL, DirToByte( trace->plane.normal ) );
	} else {
		G_AddEvent( ent, EV_MISSILE_MISS, DirToByte( trace->plane.normal ) );
	}

	ent->freeAfterEvent = qtrue;

	// change over to a normal entity right at the point of impact
	ent->s.eType = ET_GENERAL;

	SnapVectorTowards( trace->endpos, ent->s.pos.trBase );	// save net bandwidth

	G_SetOrigin( ent, trace->endpos );

	// splash damage (doesn't apply to person directly hit)
	if ( ent->splashDamage ) {
		if( G_RadiusDamage( trace->endpos, ent->parent, ent->splashDamage, ent->splashRadius, 
			other, ent->splashMethodOfDeath ) ) {
			if( !hitClient ) {
				g_entities[ent->r.ownerNum].client->accuracy_hits++;
			}
		}
	}

	trap_LinkEntity( ent );
}

/*
================
Zielsuch-Raketen

Nachgebaut nach Anup Shindes Mod von 2007, aber nicht aus dessen Code - der hat
Fehler, die hier nicht mitkommen durften:

- Er hing die Lenkung an ent->think und setzte nextthink jedes Bild neu. Damit
  verschwand der Selbstzuender nach fuenfzehn Sekunden: eine Rakete, die kein
  Ziel fand, flog ewig. Auf q3dm17 geht jeder Fehlschuss ins All, und mit
  unbegrenzter Munition ist die Tabelle der 1024 Entitaeten bald voll -
  "G_Spawn: no free entities", Absturz. Hier laeuft die Lenkung in
  G_RunMissile, think und nextthink bleiben unberuehrt.
- Er las self->client ohne Pruefung, auch in fire_rocket. Kartenschuetzen
  (shooter_rocket) haben keinen client - Absturz auf jeder Karte mit einem.
- Er lenkte mit VectorMA( forward, 0.05, targetdir ), aber targetdir war nicht
  normiert, sondern so lang wie die Entfernung. Bei 500 Einheiten wog das Ziel
  also 25-mal so viel wie die Flugrichtung: die Rakete schnappte auf weite
  Entfernung sofort herum und lenkte nur aus der Naehe sanft - verkehrt herum.
  Hier dreht sie mit einer festen Rate in Grad je Sekunde, das ist ein echter
  Wendekreis und unabhaengig von der Entfernung.
- Die "Hysterese" beim Zielwechsel (Entfernung < bisher + 100) bevorzugte
  Kandidaten, die bis zu hundert Einheiten WEITER weg waren, und merkte sich
  zwischen zwei Bildern gar nichts. Hier behaelt die Rakete ihr Ziel, solange
  es gueltig und sichtbar bleibt, und sucht erst dann ein neues.
- Die Sicht pruefte er ueber trace.contents & CONTENTS_SOLID zu den Fuessen.
  Hier zur Koerpermitte, und sichtbar heisst: nichts dazwischen oder das Ziel
  selbst getroffen.

Die "variable Geschwindigkeit" und das "Feuerwerk" kamen nicht aus dem Mod mit:
die erste normierte Weltpositionen und mass den Abstand zwischen den Einheits-
vektoren - also den Winkel zweier Orte, gesehen vom Kartenursprung aus; die
schnellen Stufen konnten nie greifen. Das zweite schoss aus der Lenkung heraus
weitere Raketen ab, ueber eine gemeinsame globale Uhr fuer alle, ohne Grenze.
Beides gibt es hier neu und begrenzt: ein Tempoprofil ueber das Alter der
Rakete (g_homingSpeedStart/End/Ramp) und Splitter nach Ablauf der Lebensdauer
(g_homingSplit), die selbst nicht mehr zerfallen.

Was sonst dazukam, jeweils mit der Vorgabe "wie bisher":
- g_homingProximity: Naeherungszuender, zuendet im Vorbeiflug.
- g_homingLead: Vorhalt - auf den Treffpunkt statt auf das Ziel.
- g_homingArm: die ersten Millisekunden geradeaus, ohne Zielsuche und Zuender.
- g_homingFuel: die Lenkkraft nimmt ab und ist nach so vielen Sekunden weg.
- g_homingDrag: enge Kurven kosten Tempo, der Motor holt es wieder auf.
- g_homingPick, g_homingAir: welches Ziel, und nur in der Luft oder am Boden.
- g_homingWarn: ein Warnton, den nur der Verfolgte hoert.
- g_homingMissiles: Raketen jagen Raketen.
================
*/
#define HOMING_RANGE		5000.0f
// Rakete gegen Rakete: so nah heisst getroffen. Raketen haben keinen Koerper,
// die Spur in G_RunMissile bleibt an einer anderen Rakete nie haengen.
#define HOMING_CONTACT		40.0f
#define HOMING_MIN_SPEED	100.0f
#define HOMING_MAX_SPEED	3000.0f
#define HOMING_LEAD_MAX		2.0f	// hoechstens so viele Sekunden vorhalten

// Wo das Ziel ist und wie schnell es sich bewegt - fuer Spieler die
// Koerpermitte, fuer Raketen die Bahn zu genau diesem Zeitpunkt. Die Bahn und
// nicht currentOrigin: eine Rakete, die in diesem Bild noch nicht bewegt
// wurde, stuende sonst ein Bild zurueck.
static void G_HomingTargetState( gentity_t *target, vec3_t pos, vec3_t vel ) {
	if ( target->client ) {
		VectorAdd( target->r.mins, target->r.maxs, pos );
		VectorMA( target->r.currentOrigin, 0.5f, pos, pos );
		VectorCopy( target->client->ps.velocity, vel );
	} else {
		BG_EvaluateTrajectory( &target->s.pos, level.time, pos );
		BG_EvaluateTrajectoryDelta( &target->s.pos, level.time, vel );
	}
}

// Darf diese Rakete dieses Ziel verfolgen? Nur die Regeln, keine Geometrie.
// fresh heisst: das Ziel wird gerade neu gewaehlt. Nur dann gilt der Filter
// Luft oder Boden - wer schon verfolgt wird und kurz aufsetzt, bleibt verfolgt.
static qboolean G_HomingAllowed( gentity_t *missile, gentity_t *target, qboolean fresh ) {
	gentity_t	*owner, *other;
	qboolean	airborne;

	if ( !target || !target->inuse || target == missile ) {
		return qfalse;
	}
	// Mitspieler nur, wenn der Schuetze noch da ist und eine Mannschaft hat.
	// Ist er gegangen, zeigt parent auf ein freies oder neu belegtes Feld.
	owner = missile->parent;
	if ( owner && ( !owner->inuse || !owner->client ) ) {
		owner = NULL;
	}

	if ( target->client ) {
		if ( g_homingMissiles.integer >= 2 ) {
			return qfalse;		// nur Raketen
		}
		if ( target->s.number == missile->r.ownerNum ) {
			return qfalse;		// nie den Schuetzen selbst
		}
		if ( target->health <= 0 || target->client->sess.sessionTeam == TEAM_SPECTATOR ) {
			return qfalse;
		}
		if ( target->client->ps.powerups[PW_INVIS] ) {
			return qfalse;		// wer unsichtbar ist, bleibt es auch fuer die Rakete
		}
		if ( owner && OnSameTeam( target, owner ) ) {
			return qfalse;
		}
		if ( fresh && g_homingAir.integer ) {
			airborne = target->client->ps.groundEntityNum == ENTITYNUM_NONE;
			if ( g_homingAir.integer == 1 ? !airborne : airborne ) {
				return qfalse;
			}
		}
		return qtrue;
	}

	// Raketen als Ziel: nur fremde und nur echte Raketen - keine Granaten, kein
	// Plasma, kein Enterhaken. Die eigenen nie, auch nicht die Splitter, die
	// denselben Schuetzen haben.
	if ( !g_homingMissiles.integer || target->s.eType != ET_MISSILE
		|| target->s.weapon != WP_ROCKET_LAUNCHER ) {
		return qfalse;
	}
	if ( target->r.ownerNum == missile->r.ownerNum ) {
		return qfalse;
	}
	other = target->parent;
	if ( owner && other && other->inuse && other->client && OnSameTeam( other, owner ) ) {
		return qfalse;
	}
	return qtrue;
}

// Liegt pos in Reichweite und im Blickkegel der Rakete?
static qboolean G_HomingInCone( gentity_t *missile, const vec3_t forward, float coneCos,
		const vec3_t pos, float *distOut, float *dotOut ) {
	vec3_t	dir;
	float	dist, dot;

	VectorSubtract( pos, missile->r.currentOrigin, dir );
	dist = VectorNormalize( dir );
	if ( dist > HOMING_RANGE ) {
		return qfalse;
	}
	dot = dist > 0.0f ? DotProduct( forward, dir ) : 1.0f;
	if ( dot < coneCos ) {
		return qfalse;			// nicht im Blickkegel der Rakete
	}
	*distOut = dist;
	*dotOut = dot;
	return qtrue;
}

// Freie Sicht: nichts dazwischen, oder das Ziel selbst getroffen. Uebergangen
// werden nur die Rakete und was ihr Schuetze besitzt (seine Leichen); sein
// eigener Koerper verdeckt die Sicht wie vor den weiteren Reglern. Beim Start
// in seiner Box schadet das nicht: eine Spur, die in einer Box beginnt, kommt
// mit fraction 1 heraus.
static qboolean G_HomingSees( gentity_t *missile, gentity_t *target, const vec3_t pos ) {
	trace_t	tr;

	trap_Trace( &tr, missile->r.currentOrigin, NULL, NULL, pos, missile->s.number, MASK_SHOT );
	return tr.fraction >= 1.0f || tr.entityNum == target->s.number;
}

// Kleiner ist besser. g_homingPick sagt, was "besser" heisst.
static float G_HomingScore( gentity_t *missile, gentity_t *target, const vec3_t pos,
		const vec3_t forward, float dist, float dot ) {
	gentity_t	*owner;
	float		health, effective;
	vec3_t		d;

	switch ( g_homingPick.integer ) {
	case 1:
		// Das im Fadenkreuz: der kleinste Winkel zur Flugrichtung. Bis zur
		// ersten Wahl fliegt die Rakete noch auf der Blicklinie - dann vom
		// Abschusspunkt aus messen, sonst blaeht die Parallaxe nahe Ziele auf:
		// 3 Grad neben dem Fadenkreuz auf 200 Einheiten sind von der 90
		// Einheiten weiter geflogenen Rakete aus 6 Grad.
		if ( !missile->homingLocked ) {
			VectorSubtract( pos, missile->homingFrom, d );
			if ( VectorNormalize( d ) > 0.0f ) {
				dot = DotProduct( forward, d );
			}
		}
		return 1.0f - dot;
	case 2:
		// Am leichtesten zu toeten. Ruestung schluckt ARMOR_PROTECTION des
		// Schadens, solange sie reicht - mehr als Leben / (1 - 0,66) braucht
		// es also nie. Raketen haben kein Leben und kommen nach allen Spielern;
		// der Abstand entscheidet nur bei Gleichstand.
		if ( !target->client ) {
			return 100000.0f + dist;
		}
		health = target->health;
		effective = health + target->client->ps.stats[STAT_ARMOR];
		if ( effective > health / ( 1.0f - ARMOR_PROTECTION ) ) {
			effective = health / ( 1.0f - ARMOR_PROTECTION );
		}
		return effective * 10.0f + dist * 0.001f;
	case 3:
		// Wer den Schuetzen zuletzt getroffen hat, vor allen anderen - und nur
		// ein Gegner. lasthurt_client taugt dafuer nicht: eigener Splash, ein
		// Raketensprung oder ein Sturz ueberschreiben ihn, und danach waere
		// die Rache vergessen. lastEnemyHurtTime 0 heisst: nie getroffen.
		owner = missile->parent;
		if ( target->client && owner && owner->inuse && owner->client
			&& owner->client->lastEnemyHurtTime
			&& owner->client->lastEnemyHurtClient == target->s.number ) {
			return dist - 100000.0f;
		}
		return dist;
	default:
		return dist;
	}
}

// Das Tempoprofil aus g_homingSpeedStart/End/Ramp, so begrenzt, wie es gilt.
static void G_HomingProfile( float *v0, float *v1, float *ramp ) {
	*v0 = Com_Clamp( HOMING_MIN_SPEED, HOMING_MAX_SPEED, g_homingSpeedStart.value );
	*v1 = Com_Clamp( HOMING_MIN_SPEED, HOMING_MAX_SPEED, g_homingSpeedEnd.value );
	*ramp = Com_Clamp( 0.05f, 10.0f, g_homingSpeedRamp.value );
}

// Der Anteil des Profils an der Strecke bis zum Alter x, geteilt durch
// (Ende - Start): auf der Rampe x²/(2·Rampe), danach x - Rampe/2.
static float G_HomingRampDistance( float x, float ramp ) {
	return x <= ramp ? 0.5f * x * x / ramp : x - 0.5f * ramp;
}

// Das mittlere Tempo der naechsten t Sekunden nach dem Profil. Ein Rueckstand
// aus dem Kurvenverlust bleibt als fester Abzug. Ohne Profil - die Vorgabe
// 900 auf 900 - ist es genau das jetzige Tempo.
static float G_HomingMeanSpeed( float age, float t, float cur ) {
	float	v0, v1, ramp, now, mean;

	G_HomingProfile( &v0, &v1, &ramp );
	if ( v0 == v1 || t <= 0.0f ) {
		return cur;
	}
	now = v0 + ( v1 - v0 ) * Com_Clamp( 0.0f, 1.0f, age / ramp );
	mean = v0 + ( v1 - v0 )
		* ( G_HomingRampDistance( age + t, ramp ) - G_HomingRampDistance( age, ramp ) ) / t;
	if ( cur < now ) {
		mean -= now - cur;
	}
	return mean < HOMING_MIN_SPEED ? HOMING_MIN_SPEED : mean;
}

// Die Flugzeit bis zum Treffpunkt, wenn Rakete und Ziel geradeaus
// weiterfliegen: |d + v·t| = Tempo·t, nach t aufgeloest. Exakt, auch fuer zwei
// Raketen, die sich gleich schnell entgegenfliegen - dort liefe eine Naeherung
// im Kreis. Nicht einholbar: so lange, wie der Flug zum jetzigen Ort dauert.
// Hoechstens HOMING_LEAD_MAX.
static float G_HomingInterceptTime( const vec3_t d, const vec3_t vel, float speed ) {
	float	a, b, c, disc, root, t, t1, t2;

	a = DotProduct( vel, vel ) - speed * speed;
	b = 2.0f * DotProduct( d, vel );
	c = DotProduct( d, d );
	t = -1.0f;
	if ( a > -0.001f && a < 0.001f ) {
		if ( b < 0.0f ) {
			t = -c / b;			// gleich schnell: einholbar nur, was entgegenkommt
		}
	} else {
		disc = b * b - 4.0f * a * c;
		if ( disc >= 0.0f ) {
			root = sqrt( disc );
			t1 = ( -b - root ) / ( 2.0f * a );
			t2 = ( -b + root ) / ( 2.0f * a );
			if ( t1 > 0.0f && ( t2 <= 0.0f || t1 < t2 ) ) {
				t = t1;
			} else if ( t2 > 0.0f ) {
				t = t2;
			}
		}
	}
	if ( t <= 0.0f ) {
		t = sqrt( c ) / speed;
	}
	if ( t > HOMING_LEAD_MAX ) {
		t = HOMING_LEAD_MAX;
	}
	return t;
}

// Wohin lenken. Ohne Vorhalt auf das Ziel; mit g_homingLead auf den
// Treffpunkt. Der Prozentsatz kuerzt die Vorhaltezeit.
static void G_HomingAimPoint( gentity_t *missile, gentity_t *target, const vec3_t pos,
		const vec3_t vel, float speed, vec3_t aim ) {
	vec3_t	d, down;
	float	lead, t, mean, age, gravity, fall, drop, land;
	trace_t	tr;

	VectorCopy( pos, aim );
	lead = Com_Clamp( 0.0f, 100.0f, g_homingLead.value ) * 0.01f;
	if ( lead <= 0.0f ) {
		return;
	}

	// Die Flugzeit einmal mit dem jetzigen Tempo und noch einmal mit dem
	// mittleren Tempo des Profils ueber genau diese Zeit: eine Rakete, die von
	// 300 auf 2000 beschleunigt, holt ein, was mit 300 "nicht einholbar" waere,
	// und hielte sonst viel zu weit vor - das Ziel fiele dabei aus dem Kegel.
	VectorSubtract( pos, missile->r.currentOrigin, d );
	age = ( level.time - missile->homingBorn ) * 0.001f;
	t = G_HomingInterceptTime( d, vel, speed );
	mean = G_HomingMeanSpeed( age, t, speed );
	if ( mean != speed ) {
		t = G_HomingInterceptTime( d, vel, mean );
	}
	t *= lead;

	VectorMA( pos, t, vel, aim );
	if ( target->client ) {
		// Fallen laesst nur, wer wirklich faellt - kein Flug-Powerup, nicht im
		// Wasser - und nur bis zum Boden unter ihm; danach laeuft er dort
		// weiter. Ohne das lag der Punkt bei einem Hasensprung auf 1200
		// Einheiten Hunderte Einheiten unter dem Boden, und die Rakete schlug
		// davor ein. Die Spur geht senkrecht nach unten, nicht entlang der
		// Flugbahn - eine Naeherung, die fuer den Hasensprung reicht.
		gravity = target->client->ps.gravity;
		if ( target->client->ps.groundEntityNum == ENTITYNUM_NONE && gravity > 0.0f
			&& !target->client->ps.powerups[PW_FLIGHT] && target->waterlevel <= 1 ) {
			fall = t;
			VectorCopy( target->r.currentOrigin, down );
			down[2] -= 8192.0f;
			trap_Trace( &tr, target->r.currentOrigin, target->r.mins, target->r.maxs,
				down, target->s.number, MASK_PLAYERSOLID );
			if ( !tr.startsolid && tr.fraction < 1.0f ) {
				drop = target->r.currentOrigin[2] - tr.endpos[2];
				if ( drop < 0.0f ) {
					drop = 0.0f;
				}
				// z(t) = z0 + vz·t - g/2·t² = Boden: die positive Wurzel
				land = ( vel[2] + sqrt( vel[2] * vel[2] + 2.0f * gravity * drop ) ) / gravity;
				if ( land < fall ) {
					fall = land;
				}
			}
			// Kein Boden darunter (der Abgrund auf q3dm17): der volle Fall stimmt.
			aim[2] = pos[2] + vel[2] * fall - 0.5f * gravity * fall * fall;
		}
	} else if ( target->s.pos.trType == TR_GRAVITY ) {
		aim[2] -= 0.5f * DEFAULT_GRAVITY * t * t;
	}
}

// Kommen sich Rakete und Ziel bis zum naechsten Bild naeher als radius, wenn
// beide geradeaus weiterfliegen? Mit der Relativbewegung gerechnet: zwei
// Raketen, die sich mit 1800 u/s entgegenfliegen, legen in einem Bild 90
// Einheiten zurueck und wuerden sich zwischen zwei Pruefungen glatt verfehlen.
// *when ist der Zeitpunkt der groessten Naehe, in Sekunden ab jetzt.
static qboolean G_HomingClose( const vec3_t rel, const vec3_t relVel, float dt,
		float radius, float *when ) {
	vec3_t	p;
	float	vv, t;

	vv = DotProduct( relVel, relVel );
	t = vv > 0.001f ? -DotProduct( rel, relVel ) / vv : 0.0f;
	t = Com_Clamp( 0.0f, dt, t );
	VectorMA( rel, t, relVel, p );
	*when = t;
	return DotProduct( p, p ) < radius * radius;
}

// Zerlegen an einem gegebenen Punkt - wie G_ExplodeMissile, aber ohne dessen
// Annahme, der Schuetze sei ein Spieler: eine abgefangene Rakete kann von
// einem Kartenschuetzen stammen, und dort stuerzte accuracy_hits ab.
static void G_HomingBurst( gentity_t *ent, const vec3_t at ) {
	vec3_t		origin, dir;
	gentity_t	*owner;

	VectorCopy( at, origin );
	SnapVector( origin );
	G_SetOrigin( ent, origin );

	dir[0] = dir[1] = 0;
	dir[2] = 1;
	ent->s.eType = ET_GENERAL;
	G_AddEvent( ent, EV_MISSILE_MISS, DirToByte( dir ) );
	ent->freeAfterEvent = qtrue;

	owner = &g_entities[ent->r.ownerNum];
	if ( ent->splashDamage && G_RadiusDamage( ent->r.currentOrigin, ent->parent,
			ent->splashDamage, ent->splashRadius, ent, ent->splashMethodOfDeath )
		&& owner->client ) {
		owner->client->accuracy_hits++;
	}
	trap_LinkEntity( ent );
}

// Der Warnton fuer den Verfolgten: nur Menschen, nur er hoert ihn, und je
// naeher die Rakete, desto dichter - 800 ms ab 2000 Einheiten, 120 ms aus
// der Naehe, wie ein Abstandswarner. Sofort piept es nur fuer einen anderen
// Menschen als zuletzt; derselbe bleibt im Takt seines Abstands, auch nach
// einem Bild ohne Sicht oder einem Abstecher der Rakete zu einem Bot. Sonst
// piepte ein Wechsel hin und her alle 100 ms, egal wie weit weg sie ist.
static void G_HomingWarn( gentity_t *ent, gentity_t *target, float dist ) {
	gentity_t	*te;

	if ( !g_homingWarn.integer || !target->client || ( target->r.svFlags & SVF_BOT ) ) {
		return;
	}
	if ( target->s.number == ent->homingBeepTarget && level.time < ent->homingBeep ) {
		return;
	}
	te = G_TempEntity( target->r.currentOrigin, EV_GLOBAL_SOUND );
	te->s.eventParm = G_SoundIndex( "sound/homing/lock.wav" );
	te->r.svFlags |= SVF_SINGLECLIENT | SVF_BROADCAST;
	te->r.singleClient = target->s.number;
	ent->homingBeepTarget = target->s.number;
	ent->homingBeep = level.time + (int)Com_Clamp( 120.0f, 800.0f, dist * 0.4f );
}

// Nach Ablauf der Lebensdauer: statt sich nur zu zerlegen, in g_homingSplit
// Splitter zerfallen, die reihum 25 Grad aus der Flugrichtung weiterfliegen und
// selbst suchen. Jeder traegt den halben Schaden, und Splitter zerfallen nicht
// noch einmal - das Feuerwerk von 2007 hatte keine Grenze.
static void G_HomingSplit( gentity_t *ent ) {
	vec3_t		origin, forward, right, up, dir;
	gentity_t	*owner, *bolt;
	float		speed, angle, spread;
	int			i, n, room, made;

	BG_EvaluateTrajectory( &ent->s.pos, level.time, origin );
	owner = ent->parent;
	n = (int)Com_Clamp( 0.0f, 4.0f, g_homingSplit.value );
	made = 0;

	if ( n > 0 && ent->homingGen == 0 && owner && owner->inuse && owner->client ) {
		// Nur mit Platz in der Entitaetentabelle: lieber keine Splitter als
		// "G_Spawn: no free entities" und ein Absturz.
		room = ENTITYNUM_MAX_NORMAL - level.num_entities;
		for ( i = MAX_CLIENTS; i < level.num_entities; i++ ) {
			if ( !g_entities[i].inuse ) {
				room++;
			}
		}
		if ( room < 64 ) {
			n = 0;
		}

		speed = ent->homingSpeed > 0.0f ? ent->homingSpeed : VectorLength( ent->s.pos.trDelta );
		VectorNormalize2( ent->s.pos.trDelta, forward );
		PerpendicularVector( right, forward );
		CrossProduct( forward, right, up );
		spread = DEG2RAD( 25.0f );
		for ( i = 0; i < n; i++ ) {
			angle = 2.0f * M_PI * i / n;
			VectorScale( forward, cos( spread ), dir );
			VectorMA( dir, sin( spread ) * cos( angle ), right, dir );
			VectorMA( dir, sin( spread ) * sin( angle ), up, dir );

			bolt = fire_rocket( owner, origin, dir );
			// Auch wenn nur die Menschen suchen und der Schuetze inzwischen als
			// Bot gilt: der Splitter einer Zielsuch-Rakete sucht.
			bolt->homing = qtrue;
			bolt->homingGen = 1;
			// Ihr Alter, also laengst scharf, und das Tempoprofil laeuft weiter.
			// Die Treibstoffuhr dagegen hat fire_rocket eben neu gestellt: erbten
			// sie auch die, koennten sie mit g_homingFuel nie lenken.
			bolt->homingBorn = ent->homingBorn;
			bolt->homingSpeed = speed;
			bolt->s.pos.trTime = level.time;		// kein Vorlauf, sie entstehen im Flug
			VectorScale( dir, speed, bolt->s.pos.trDelta );
			SnapVector( bolt->s.pos.trDelta );
			bolt->damage = ent->damage / 2;
			bolt->splashDamage = ent->splashDamage / 2;
			// Eigene Lebensdauer, dann zerlegen - fire_rocket hat sie nur
			// gesetzt, wenn es den Schuetzen selbst fuer suchend hielt.
			bolt->think = G_ExplodeMissile;
			bolt->nextthink = level.time
				+ (int)( Com_Clamp( 0.5f, 15.0f, g_homingLifetime.value ) * 1000.0f );
			made++;
		}
	}

	// Die Huelle platzt sichtbar. Mit Splittern ohne Schaden - den tragen
	// jetzt die; ohne (kein Platz, Schuetze weg) wie jede Rakete.
	if ( made ) {
		ent->splashDamage = 0;
	}
	G_HomingBurst( ent, origin );
}

// Lenken, Tempo und Zuender - einmal je Bild, nach der Bewegung. Gibt qtrue
// zurueck, wenn die Rakete dabei gezuendet hat; dann darf G_RunMissile nicht
// weitermachen, der think liefe sonst auf einer Explosion.
static qboolean G_HomingSteer( gentity_t *ent ) {
	vec3_t		forward, dir, pos, vel, bestPos, bestVel, aim, want, perp, cross;
	vec3_t		mvel, rel, relVel, at;
	float		speed, cur, dist, dot, score, best, coneCos, maxTurn, cosAngle;
	float		s, a, dt, age, fuel, turned, profile, v0, v1, ramp, rate, drag;
	float		radius, when, x;
	gentity_t	*target, *cand;
	trace_t		tr;
	int			i, limit, hunter, hunted;

	dt = ( level.time - level.previousTime ) * 0.001f;
	speed = VectorLength( ent->s.pos.trDelta );
	if ( dt <= 0.0f || speed < 1.0f ) {
		return qfalse;
	}
	VectorScale( ent->s.pos.trDelta, 1.0f / speed, forward );
	VectorCopy( forward, dir );
	age = ( level.time - ent->homingBorn ) * 0.001f;
	cur = ent->homingSpeed > 0.0f ? ent->homingSpeed : speed;
	turned = 0.0f;
	target = NULL;

	// Die ersten g_homingArm Millisekunden fliegt sie geradeaus: keine
	// Zielsuche, kein Zuender. Splitter haben das Alter ihrer Mutter und sind
	// damit laengst scharf; ihr Treibstoff dagegen zaehlt ab dem Zerfall.
	if ( level.time - ent->homingBorn >= (int)Com_Clamp( 0.0f, 2000.0f, g_homingArm.value ) ) {
		coneCos = cos( DEG2RAD( Com_Clamp( 1.0f, 180.0f, g_homingCone.value ) ) );

		// Das bisherige Ziel behalten, solange es gueltig und sichtbar ist.
		// Erst wenn es wegfaellt, wird neu gesucht. Mit g_homingRetarget jedes
		// Bild neu: die Rakete schwenkt um, sobald ein anderes besser passt.
		if ( !g_homingRetarget.integer && G_HomingAllowed( ent, ent->homingTarget, qfalse ) ) {
			G_HomingTargetState( ent->homingTarget, bestPos, bestVel );
			if ( G_HomingInCone( ent, forward, coneCos, bestPos, &dist, &dot )
				&& G_HomingSees( ent, ent->homingTarget, bestPos ) ) {
				target = ent->homingTarget;
			}
		}
		if ( !target ) {
			// Spieler, und mit g_homingMissiles alles dahinter. Die Spur kommt
			// zuletzt und nur fuer den, der besser waere als der bisher beste -
			// sie ist das Teure, und mit Raketen als Ziel gibt es viele.
			best = 0.0f;
			limit = g_homingMissiles.integer ? level.num_entities : level.maxclients;
			for ( i = 0; i < limit; i++ ) {
				if ( i >= level.maxclients && i < MAX_CLIENTS ) {
					continue;
				}
				cand = &g_entities[i];
				// Wer schon verfolgt wird, faellt auch beim Umschwenken nicht
				// durch den Filter Luft/Boden, nur weil er kurz aufsetzt. Im
				// Haltemodus aendert das nichts: dort laeuft die Suche erst,
				// wenn das gehaltene Ziel Kegel oder Sicht schon verloren hat.
				if ( !G_HomingAllowed( ent, cand, cand != ent->homingTarget ) ) {
					continue;
				}
				G_HomingTargetState( cand, pos, vel );
				if ( !G_HomingInCone( ent, forward, coneCos, pos, &dist, &dot ) ) {
					continue;
				}
				score = G_HomingScore( ent, cand, pos, forward, dist, dot );
				if ( target && score >= best ) {
					continue;
				}
				if ( !G_HomingSees( ent, cand, pos ) ) {
					continue;
				}
				target = cand;
				best = score;
				VectorCopy( pos, bestPos );
				VectorCopy( vel, bestVel );
			}
			ent->homingTarget = target;
		}
		if ( target ) {
			ent->homingLocked = qtrue;		// ab jetzt misst g_homingPick 1 von der Rakete aus
		}
	}

	if ( target ) {
		G_HomingAimPoint( ent, target, bestPos, bestVel, cur, aim );
		VectorSubtract( aim, ent->r.currentOrigin, want );

		// Hoechstens g_homingTurn Grad je Sekunde drehen, gerechnet auf die
		// Zeit dieses Bildes. Das macht den Wendekreis: Radius = Tempo / Rate.
		maxTurn = DEG2RAD( Com_Clamp( 1.0f, 3600.0f, g_homingTurn.value ) ) * dt;
		// Treibstoff: die Lenkkraft nimmt gleichmaessig ab und ist nach
		// g_homingFuel Sekunden weg. Danach fliegt sie geradeaus weiter. Die
		// Uhr ist die eigene, nicht das Alter - Splitter tanken neu.
		fuel = Com_Clamp( 0.0f, 60.0f, g_homingFuel.value );
		if ( fuel > 0.0f ) {
			maxTurn *= Com_Clamp( 0.0f, 1.0f,
				1.0f - ( level.time - ent->homingFuelBorn ) * 0.001f / fuel );
		}
		// Mehr als eine halbe Drehung je Bild gibt es nicht: ab pi ist jedes
		// Ziel in Reichweite. Darueber liefe cos(maxTurn) wieder gegen 1, und
		// die Drehung unten zeigte vom Ziel weg (sv_fps 10, g_homingTurn 3600).
		if ( maxTurn > M_PI ) {
			maxTurn = M_PI;
		}

		if ( VectorNormalize( want ) > 1.0f && maxTurn > 0.0f ) {
			cosAngle = Com_Clamp( -1.0f, 1.0f, DotProduct( forward, want ) );
			if ( cosAngle >= cos( maxTurn ) ) {
				// In Reichweite: direkt drauf. Der Winkel dafuer ueber atan2 -
				// acos gibt es im QVM nicht.
				CrossProduct( forward, want, cross );
				turned = atan2( VectorLength( cross ), cosAngle );
				VectorCopy( want, dir );
			} else {
				// Um genau maxTurn drehen, in der Ebene aus forward und want. Die
				// Senkrechte dazu ist want ohne seinen forward-Anteil; damit gilt
				// dir = forward·cos + senkrecht·sin - exakt, ohne den Winkel
				// selbst zu kennen.
				VectorMA( want, -cosAngle, forward, perp );
				if ( VectorNormalize( perp ) >= 0.0001f ) {
					s = sin( maxTurn );
					a = cos( maxTurn );
					VectorScale( forward, a, dir );
					VectorMA( dir, s, perp, dir );
					VectorNormalize( dir );
					turned = maxTurn;
				}
				// sonst: Ziel genau hinter der Rakete - dieses Bild geradeaus,
				// im naechsten ist der Winkel schon ein anderer
			}
		}

		G_HomingWarn( ent, target, Distance( bestPos, ent->r.currentOrigin ) );
	}

	// Tempo: ein Profil von g_homingSpeedStart nach g_homingSpeedEnd ueber
	// g_homingSpeedRamp Sekunden. Enge Kurven kosten mit g_homingDrag Tempo,
	// das der Motor danach wieder aufholt.
	G_HomingProfile( &v0, &v1, &ramp );
	profile = v0 + ( v1 - v0 ) * Com_Clamp( 0.0f, 1.0f, age / ramp );
	drag = Com_Clamp( 0.0f, 100.0f, g_homingDrag.value ) * 0.01f;
	if ( drag > 0.0f && turned > 0.0f ) {
		// e^(-x) mit x = k·Winkel, k so, dass bei 100 % eine Vierteldrehung
		// ohne Nachschub das Tempo halbiert: k = 2·ln 2 / pi. exp gibt es im
		// QVM nicht; die Pade-Form (1 - x/2) / (1 + x/2) trifft es auch bei 36
		// Grad je Bild (720 °/s) auf ein Promille und ist unter 2 positiv.
		x = 0.4413f * drag * turned;
		if ( x > 1.9f ) {
			x = 1.9f;
		}
		cur *= ( 1.0f - 0.5f * x ) / ( 1.0f + 0.5f * x );
	}
	// Der Motor holt 900 u/s je Sekunde gegenueber dem Profil auf. Steigt das
	// Profil gerade, kommt dessen Steigung obendrauf - sonst liefe es der
	// gebremsten Rakete genau so schnell davon, wie sie aufholt. Ohne
	// Kurvenverlust haelt sie damit genau das Profil.
	rate = 900.0f;
	if ( v1 > v0 && age < ramp ) {
		rate += ( v1 - v0 ) / ramp;
	}
	if ( cur < profile ) {
		cur += rate * dt;
		if ( cur > profile ) {
			cur = profile;
		}
	} else {
		cur = profile;
	}
	if ( cur < HOMING_MIN_SPEED ) {
		cur = HOMING_MIN_SPEED;
	}
	ent->homingSpeed = cur;

	// Die Bahn nur neu ansetzen, wenn sich etwas geaendert hat: ein neuer
	// Ursprung geht jedes Mal uebers Netz.
	if ( turned > 0.0f || cur - speed > 2.0f || speed - cur > 2.0f ) {
		VectorCopy( ent->r.currentOrigin, ent->s.pos.trBase );
		ent->s.pos.trTime = level.time;
		VectorScale( dir, cur, ent->s.pos.trDelta );
		SnapVector( ent->s.pos.trDelta );			// wie in fire_rocket, spart Bandbreite
	}

	// Zuender. Gegen Spieler nur mit g_homingProximity; gegen Raketen immer -
	// anders als ein Spieler hat eine Rakete keinen Koerper, an dem die Spur in
	// G_RunMissile haengen bleiben koennte. Die gejagte Rakete platzt mit.
	if ( target ) {
		radius = Com_Clamp( 0.0f, 1000.0f, g_homingProximity.value );
		if ( !target->client ) {
			// Eine Rakete hat keinen Koerper und nimmt keinen Schaden: sie
			// gilt nur dort als abgeschossen, wo die eigene Explosion sie
			// erreicht - nie weiter als splashRadius, nie weniger als die
			// Beruehrung. Sonst "schoss" ein Zuender von 300 Raketen ab, an
			// die keine Explosion herankam.
			if ( radius > ent->splashRadius ) {
				radius = ent->splashRadius;
			}
			if ( radius < HOMING_CONTACT ) {
				radius = HOMING_CONTACT;
			}
		}
		if ( radius > 0.0f ) {
			VectorScale( dir, cur, mvel );
			VectorSubtract( bestPos, ent->r.currentOrigin, rel );
			VectorSubtract( bestVel, mvel, relVel );
			if ( G_HomingClose( rel, relVel, dt, radius, &when ) ) {
				// Beide am Punkt der groessten Naehe zuenden. Die gejagte Rakete
				// von ihrem letzten geprueften Ort aus: hat sie eine hoehere
				// Nummer, lief sie in diesem Bild noch nicht, und ihre Bahn kann
				// schon hinter einer Wand liegen. Trifft sie auf dem Weg dorthin
				// eine Wand oder einen Spieler, macht das ihr eigenes
				// G_RunMissile - hier dann kein Abfangen, sonst platzte sie auf
				// der falschen Seite der Wand. Die Jaegerin sucht sich im
				// naechsten Bild ein neues Ziel.
				if ( !target->client ) {
					VectorMA( bestPos, when, bestVel, at );
					trap_Trace( &tr, target->r.currentOrigin, target->r.mins, target->r.maxs,
						at, target->r.ownerNum, target->clipmask );
					if ( tr.startsolid || tr.fraction < 1.0f ) {
						return qfalse;
					}
					// Fuer die Auswertung, im Stil der Kill-Zeilen: wessen Rakete
					// wessen abgeschossen hat. Ohne die Zeile saehe man es nur
					// daran, dass weniger Raketen treffen.
					// Wie player_die: was kein Spielerplatz ist (ein
					// Kartenschuetze), steht als <world> mit 1022 da.
					hunter = ent->r.ownerNum >= 0 && ent->r.ownerNum < MAX_CLIENTS
						&& g_entities[ent->r.ownerNum].client ? ent->r.ownerNum : ENTITYNUM_WORLD;
					hunted = target->r.ownerNum >= 0 && target->r.ownerNum < MAX_CLIENTS
						&& g_entities[target->r.ownerNum].client ? target->r.ownerNum : ENTITYNUM_WORLD;
					G_LogPrintf( "Intercept: %i %i: %s shot down a rocket of %s\n", hunter, hunted,
						hunter != ENTITYNUM_WORLD ? g_entities[hunter].client->pers.netname : "<world>",
						hunted != ENTITYNUM_WORLD ? g_entities[hunted].client->pers.netname : "<world>" );
					G_HomingBurst( target, at );
				}
				// Die Jaegerin ebenso nicht hinter einer Wand: die Spur haelt an
				// der ersten festen Flaeche.
				VectorMA( ent->r.currentOrigin, when, mvel, at );
				trap_Trace( &tr, ent->r.currentOrigin, NULL, NULL, at, ent->r.ownerNum, MASK_SOLID );
				G_HomingBurst( ent, tr.endpos );
				return qtrue;
			}
		}
	}
	return qfalse;
}

/*
================
G_RunMissile
================
*/
void G_RunMissile( gentity_t *ent ) {
	vec3_t		origin;
	trace_t		tr;
	int			passent;

	// get current position
	BG_EvaluateTrajectory( &ent->s.pos, level.time, origin );

	// if this missile bounced off an invulnerability sphere
	if ( ent->target_ent ) {
		passent = ent->target_ent->s.number;
	}
#ifdef MISSIONPACK
	// prox mines that left the owner bbox will attach to anything, even the owner
	else if (ent->s.weapon == WP_PROX_LAUNCHER && ent->count) {
		passent = ENTITYNUM_NONE;
	}
#endif
	else {
		// ignore interactions with the missile owner
		passent = ent->r.ownerNum;
	}
	// trace a line from the previous position to the current position
	trap_Trace( &tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, origin, passent, ent->clipmask );

	if ( tr.startsolid || tr.allsolid ) {
		// make sure the tr.entityNum is set to the entity we're stuck in
		trap_Trace( &tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, ent->r.currentOrigin, passent, ent->clipmask );
		tr.fraction = 0;
	}
	else {
		VectorCopy( tr.endpos, ent->r.currentOrigin );
	}

	trap_LinkEntity( ent );

	if ( tr.fraction != 1 ) {
		// never explode or bounce on sky
		if ( tr.surfaceFlags & SURF_NOIMPACT ) {
			// If grapple, reset owner
			if (ent->parent && ent->parent->client && ent->parent->client->hook == ent) {
				ent->parent->client->hook = NULL;
			}
			G_FreeEntity( ent );
			return;
		}
		G_MissileImpact( ent, &tr );
		if ( ent->s.eType != ET_MISSILE ) {
			return;		// exploded
		}
	}
#ifdef MISSIONPACK
	// if the prox mine wasn't yet outside the player body
	if (ent->s.weapon == WP_PROX_LAUNCHER && !ent->count) {
		// check if the prox mine is outside the owner bbox
		trap_Trace( &tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, ent->r.currentOrigin, ENTITYNUM_NONE, ent->clipmask );
		if (!tr.startsolid || tr.entityNum != ent->r.ownerNum) {
			ent->count = 1;
		}
	}
#endif
	// Zielsuch-Raketen lenken nach der Bewegung dieses Bildes und vor dem think
	// - der think bleibt der Selbstzuender. Hat der Naeherungszuender dabei
	// ausgeloest, ist sie keine Rakete mehr und der think darf nicht laufen.
	if ( ent->homing && G_HomingSteer( ent ) ) {
		return;
	}

	// check think function after bouncing
	G_RunThink( ent );
}


//=============================================================================

/*
=================
fire_plasma

=================
*/
gentity_t *fire_plasma (gentity_t *self, vec3_t start, vec3_t dir) {
	gentity_t	*bolt;

	VectorNormalize (dir);

	bolt = G_Spawn();
	bolt->classname = "plasma";
	bolt->nextthink = level.time + 10000;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_PLASMAGUN;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 20;
	bolt->splashDamage = 15;
	bolt->splashRadius = 20;
	bolt->methodOfDeath = MOD_PLASMA;
	bolt->splashMethodOfDeath = MOD_PLASMA_SPLASH;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = NULL;

	bolt->s.pos.trType = TR_LINEAR;
	bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;		// move a bit on the very first frame
	VectorCopy( start, bolt->s.pos.trBase );
	VectorScale( dir, 2000, bolt->s.pos.trDelta );
	SnapVector( bolt->s.pos.trDelta );			// save net bandwidth

	VectorCopy (start, bolt->r.currentOrigin);

	return bolt;
}	

//=============================================================================


/*
=================
fire_grenade
=================
*/
gentity_t *fire_grenade (gentity_t *self, vec3_t start, vec3_t dir) {
	gentity_t	*bolt;

	VectorNormalize (dir);

	bolt = G_Spawn();
	bolt->classname = "grenade";
	bolt->nextthink = level.time + 2500;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_GRENADE_LAUNCHER;
	bolt->s.eFlags = EF_BOUNCE_HALF;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 100;
	bolt->splashDamage = 100;
	bolt->splashRadius = 150;
	bolt->methodOfDeath = MOD_GRENADE;
	bolt->splashMethodOfDeath = MOD_GRENADE_SPLASH;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = NULL;

	bolt->s.pos.trType = TR_GRAVITY;
	bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;		// move a bit on the very first frame
	VectorCopy( start, bolt->s.pos.trBase );
	VectorScale( dir, 700, bolt->s.pos.trDelta );
	SnapVector( bolt->s.pos.trDelta );			// save net bandwidth

	VectorCopy (start, bolt->r.currentOrigin);

	return bolt;
}

//=============================================================================


/*
=================
fire_bfg
=================
*/
gentity_t *fire_bfg (gentity_t *self, vec3_t start, vec3_t dir) {
	gentity_t	*bolt;

	VectorNormalize (dir);

	bolt = G_Spawn();
	bolt->classname = "bfg";
	bolt->nextthink = level.time + 10000;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_BFG;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 100;
	bolt->splashDamage = 100;
	bolt->splashRadius = 120;
	bolt->methodOfDeath = MOD_BFG;
	bolt->splashMethodOfDeath = MOD_BFG_SPLASH;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = NULL;

	bolt->s.pos.trType = TR_LINEAR;
	bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;		// move a bit on the very first frame
	VectorCopy( start, bolt->s.pos.trBase );
	VectorScale( dir, 2000, bolt->s.pos.trDelta );
	SnapVector( bolt->s.pos.trDelta );			// save net bandwidth
	VectorCopy (start, bolt->r.currentOrigin);

	return bolt;
}

//=============================================================================


/*
=================
fire_rocket
=================
*/
gentity_t *fire_rocket (gentity_t *self, vec3_t start, vec3_t dir) {
	gentity_t	*bolt;

	VectorNormalize (dir);

	bolt = G_Spawn();
	bolt->classname = "rocket";
	bolt->nextthink = level.time + 15000;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_ROCKET_LAUNCHER;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 100;
	bolt->splashDamage = 100;
	bolt->splashRadius = 120;
	bolt->methodOfDeath = MOD_ROCKET;
	bolt->splashMethodOfDeath = MOD_ROCKET_SPLASH;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = NULL;

	bolt->s.pos.trType = TR_LINEAR;
	bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;		// move a bit on the very first frame
	VectorCopy( start, bolt->s.pos.trBase );
	VectorScale( dir, 900, bolt->s.pos.trDelta );
	SnapVector( bolt->s.pos.trDelta );			// save net bandwidth
	VectorCopy (start, bolt->r.currentOrigin);

	// Zielsuche beim Abschuss entscheiden. self->client zuerst: auch
	// Kartenschuetzen (shooter_rocket) rufen fire_rocket, und die haben keinen -
	// genau daran stuerzte der Mod von 2007 ab. 1 heisst nur Menschen, 2 alle.
	bolt->homing = g_homingRockets.integer && self->client
		&& ( g_homingRockets.integer >= 2 || !( self->r.svFlags & SVF_BOT ) );
	bolt->homingTarget = NULL;
	bolt->homingBorn = level.time;
	bolt->homingFuelBorn = level.time;
	bolt->homingSpeed = 0.0f;
	bolt->homingBeep = 0;
	// Ausdruecklich: G_Spawn nullt das Feld, und 0 ist ein echter Spieler.
	bolt->homingBeepTarget = ENTITYNUM_NONE;
	bolt->homingGen = 0;
	VectorCopy( start, bolt->homingFrom );
	bolt->homingLocked = qfalse;
	if ( bolt->homing ) {
		// Die Lebensdauer ersetzt nur die 15 s von oben - G_ExplodeMissile
		// bleibt, also zerlegt sie sich mit vollem Splash dort, wo sie gerade
		// ist. Nur fuer Zielsuch-Raketen: eine gerade fliegende trifft lange
		// vorher eine Wand, eine kreisende sonst erst nach einer Viertelminute.
		// Mit Splittern zerfaellt sie stattdessen.
		bolt->nextthink = level.time
			+ (int)( Com_Clamp( 0.5f, 15.0f, g_homingLifetime.value ) * 1000.0f );
		if ( g_homingSplit.integer > 0 ) {
			bolt->think = G_HomingSplit;
		}
		// Das Starttempo des Profils statt der festen 900. Der Vorlauf oben
		// (MISSILE_PRESTEP_TIME) rechnet mit diesem Tempo mit.
		bolt->homingSpeed = Com_Clamp( HOMING_MIN_SPEED, HOMING_MAX_SPEED, g_homingSpeedStart.value );
		VectorScale( dir, bolt->homingSpeed, bolt->s.pos.trDelta );
		SnapVector( bolt->s.pos.trDelta );
	}

	return bolt;
}

/*
=================
fire_grapple
=================
*/
gentity_t *fire_grapple (gentity_t *self, vec3_t start, vec3_t dir) {
	gentity_t	*hook;

	VectorNormalize (dir);

	hook = G_Spawn();
	hook->classname = "hook";
	hook->nextthink = level.time + 10000;
	hook->think = Weapon_HookFree;
	hook->s.eType = ET_MISSILE;
	hook->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	hook->s.weapon = WP_GRAPPLING_HOOK;
	hook->r.ownerNum = self->s.number;
	hook->methodOfDeath = MOD_GRAPPLE;
	hook->clipmask = MASK_SHOT;
	hook->parent = self;
	hook->target_ent = NULL;

	hook->s.pos.trType = TR_LINEAR;
	hook->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;		// move a bit on the very first frame
	hook->s.otherEntityNum = self->s.number; // use to match beam in client
	VectorCopy( start, hook->s.pos.trBase );
	VectorScale( dir, 800, hook->s.pos.trDelta );
	SnapVector( hook->s.pos.trDelta );			// save net bandwidth
	VectorCopy (start, hook->r.currentOrigin);

	self->client->hook = hook;

	return hook;
}


#ifdef MISSIONPACK
/*
=================
fire_nail
=================
*/
#define NAILGUN_SPREAD	500

gentity_t *fire_nail( gentity_t *self, vec3_t start, vec3_t forward, vec3_t right, vec3_t up ) {
	gentity_t	*bolt;
	vec3_t		dir;
	vec3_t		end;
	float		r, u, scale;

	bolt = G_Spawn();
	bolt->classname = "nail";
	bolt->nextthink = level.time + 10000;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_NAILGUN;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 20;
	bolt->methodOfDeath = MOD_NAIL;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = NULL;

	bolt->s.pos.trType = TR_LINEAR;
	bolt->s.pos.trTime = level.time;
	VectorCopy( start, bolt->s.pos.trBase );

	r = random() * M_PI * 2.0f;
	u = sin(r) * crandom() * NAILGUN_SPREAD * 16;
	r = cos(r) * crandom() * NAILGUN_SPREAD * 16;
	VectorMA( start, 8192 * 16, forward, end);
	VectorMA (end, r, right, end);
	VectorMA (end, u, up, end);
	VectorSubtract( end, start, dir );
	VectorNormalize( dir );

	scale = 555 + random() * 1800;
	VectorScale( dir, scale, bolt->s.pos.trDelta );
	SnapVector( bolt->s.pos.trDelta );

	VectorCopy( start, bolt->r.currentOrigin );

	return bolt;
}	


/*
=================
fire_prox
=================
*/
gentity_t *fire_prox( gentity_t *self, vec3_t start, vec3_t dir ) {
	gentity_t	*bolt;

	VectorNormalize (dir);

	bolt = G_Spawn();
	bolt->classname = "prox mine";
	bolt->nextthink = level.time + 3000;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_PROX_LAUNCHER;
	bolt->s.eFlags = 0;
	bolt->r.ownerNum = self->s.number;
	bolt->parent = self;
	bolt->damage = 0;
	bolt->splashDamage = 100;
	bolt->splashRadius = 150;
	bolt->methodOfDeath = MOD_PROXIMITY_MINE;
	bolt->splashMethodOfDeath = MOD_PROXIMITY_MINE;
	bolt->clipmask = MASK_SHOT;
	bolt->target_ent = NULL;
	// count is used to check if the prox mine left the player bbox
	// if count == 1 then the prox mine left the player bbox and can attack to it
	bolt->count = 0;

	//FIXME: we prolly wanna abuse another field
	bolt->s.generic1 = self->client->sess.sessionTeam;

	bolt->s.pos.trType = TR_GRAVITY;
	bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;		// move a bit on the very first frame
	VectorCopy( start, bolt->s.pos.trBase );
	VectorScale( dir, 700, bolt->s.pos.trDelta );
	SnapVector( bolt->s.pos.trDelta );			// save net bandwidth

	VectorCopy (start, bolt->r.currentOrigin);

	return bolt;
}
#endif
