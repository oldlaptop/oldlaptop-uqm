//Copyright Paul Reiche, Fred Ford. 1992-2002

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "ships/ship.h"
#include "ships/zoqfot/resinst.h"

#include "colors.h"
#include "globdata.h"
#include "libs/mathlib.h"


#define MAX_CREW 6 //10
#define MAX_ENERGY 10
#define ENERGY_REGENERATION 1
#define WEAPON_ENERGY_COST 2 //1
#define SPECIAL_ENERGY_COST 6 //(MAX_ENERGY * 3 / 4)
#define ENERGY_WAIT 4
#define MAX_THRUST 40
#define THRUST_INCREMENT 6 //10
#define TURN_WAIT 1
#define THRUST_WAIT 1 //0
#define WEAPON_WAIT 0
#define SPECIAL_WAIT 6

#define SHIP_MASS 5
#define MISSILE_SPEED DISPLAY_TO_WORLD (10)
#define MISSILE_LIFE 100 //20 //50 //10
#define MISSILE_RANGE (MISSILE_SPEED * MISSILE_LIFE)

static RACE_DESC zoqfotpik_desc =
{
	{
		FIRES_FORE,
		30, /* Super Melee cost */
		320 / SPHERE_RADIUS_INCREMENT, /* Initial sphere of influence radius */
		MAX_CREW, MAX_CREW,
		MAX_ENERGY, MAX_ENERGY,
		{
			3761, 5333,
		},
		(STRING)ZOQFOTPIK_RACE_STRINGS,
		(FRAME)ZOQFOTPIK_ICON_MASK_PMAP_ANIM,
		(FRAME)ZOQFOTPIK_MICON_MASK_PMAP_ANIM,
	},
	{
		MAX_THRUST,
		THRUST_INCREMENT,
		ENERGY_REGENERATION,
		WEAPON_ENERGY_COST,
		SPECIAL_ENERGY_COST,
		ENERGY_WAIT,
		TURN_WAIT,
		THRUST_WAIT,
		WEAPON_WAIT,
		SPECIAL_WAIT,
		SHIP_MASS,
	},
	{
		{
			(FRAME)ZOQFOTPIK_BIG_MASK_PMAP_ANIM,
			(FRAME)ZOQFOTPIK_MED_MASK_PMAP_ANIM,
			(FRAME)ZOQFOTPIK_SML_MASK_PMAP_ANIM,
		},
		{
			(FRAME)SPIT_BIG_MASK_PMAP_ANIM,
			(FRAME)SPIT_MED_MASK_PMAP_ANIM,
			(FRAME)SPIT_SML_MASK_PMAP_ANIM,
		},
		{
			(FRAME)STINGER_BIG_MASK_PMAP_ANIM,
			(FRAME)STINGER_MED_MASK_PMAP_ANIM,
			(FRAME)STINGER_SML_MASK_PMAP_ANIM,
		},
		{
			(FRAME)ZOQFOTPIK_CAPTAIN_MASK_PMAP_ANIM,
			(FRAME)0,
			(FRAME)0,
			(FRAME)0,
			(FRAME)0,
		},
		(SOUND)ZOQFOTPIK_VICTORY_SONG,
		(SOUND)ZOQFOTPIK_SHIP_SOUNDS,
	},
	{
		0,
		MISSILE_RANGE,
		NULL_PTR,
	},
	(UNINIT_FUNC *) NULL,
	(PREPROCESS_FUNC *) NULL,
	(POSTPROCESS_FUNC *) NULL,
	(INIT_WEAPON_FUNC *) NULL,
	0,
};

#define ZOQFOTPIK_OFFSET 13
#define SPIT_WAIT 4

static void
spit_preprocess (PELEMENT ElementPtr)
{
		COUNT index, angle, speed, facing;

		if(ElementPtr->life_span % 10 == 0) ElementPtr->next.image.frame =
				IncFrameIndex (ElementPtr->next.image.frame);
		angle = GetVelocityTravelAngle (&ElementPtr->velocity);
		facing = ANGLE_TO_FACING (angle);
		/*if ((*/index = GetFrameIndex (ElementPtr->next.image.frame)/*) == 1)
			angle = angle + (((COUNT)TFB_Random () % 3) - 1)*/;
		if(ElementPtr->life_span % 4 == 0)TrackShip(ElementPtr, &facing);

		speed = WORLD_TO_VELOCITY (DISPLAY_TO_WORLD (
				GetFrameCount (ElementPtr->next.image.frame) - index) << 1);
		SetVelocityComponents (&ElementPtr->velocity,
				(SIZE)COSINE (FACING_TO_ANGLE(facing), speed),
				(SIZE)SINE (FACING_TO_ANGLE(facing), speed));

		ElementPtr->turn_wait = SPIT_WAIT;
		ElementPtr->state_flags |= CHANGING;
}


static COUNT
initialize_spit (PELEMENT ShipPtr, HELEMENT SpitArray[])
{
#define MISSILE_DAMAGE 1
#define MISSILE_HITS 1
#define MISSILE_OFFSET 0
	STARSHIPPTR StarShipPtr;
	MISSILE_BLOCK MissileBlock;

	GetElementStarShip (ShipPtr, &StarShipPtr);
	MissileBlock.cx = ShipPtr->next.location.x;
	MissileBlock.cy = ShipPtr->next.location.y;
	MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.weapon;
	MissileBlock.face = StarShipPtr->ShipFacing;
	MissileBlock.index = 0;
	MissileBlock.sender = (ShipPtr->state_flags & (GOOD_GUY | BAD_GUY))
			| IGNORE_SIMILAR | PERSISTENT;
	MissileBlock.pixoffs = ZOQFOTPIK_OFFSET;
	MissileBlock.speed = DISPLAY_TO_WORLD (
			GetFrameCount (StarShipPtr->RaceDescPtr->ship_data.weapon[0])) << 1;
	MissileBlock.hit_points = MISSILE_HITS;
	MissileBlock.damage = MISSILE_DAMAGE;
	MissileBlock.life = MISSILE_LIFE;
	MissileBlock.preprocess_func = spit_preprocess;
	MissileBlock.blast_offs = MISSILE_OFFSET;
	SpitArray[0] = initialize_missile (&MissileBlock);

	return (1);
}

/*#define SPIT_WAIT 4 //2

static void
spit_preprocess (PELEMENT ElementPtr)
{
	if (ElementPtr->turn_wait > 0)
		--ElementPtr->turn_wait;
	else
	{
		COUNT index, angle, speed;

		ElementPtr->next.image.frame =
				IncFrameIndex (ElementPtr->next.image.frame);
		angle = GetVelocityTravelAngle (&ElementPtr->velocity);

		index = GetFrameIndex (ElementPtr->next.image.frame);
		//if ((index = GetFrameIndex (ElementPtr->next.image.frame)) == 1)
			//angle = angle + (((COUNT)TFB_Random () % 3) - 1);

		speed = WORLD_TO_VELOCITY (DISPLAY_TO_WORLD (
				GetFrameCount (ElementPtr->next.image.frame) - index) << 1);
		SetVelocityComponents (&ElementPtr->velocity,
				(SIZE)COSINE (angle, speed),
				(SIZE)SINE (angle, speed));

		ElementPtr->turn_wait = SPIT_WAIT;
		ElementPtr->state_flags |= CHANGING;
	}
}*/

/*static void
spit_collision (PELEMENT ElementPtr0, PPOINT pPt0, PELEMENT ElementPtr1, PPOINT pPt1)
{
	COUNT original_mass;
	original_mass = ElementPtr0->mass_points;
	if(!(ElementPtr1->state_flags & PLAYER_SHIP))
	{
		ElementPtr0->mass_points = 5;
	}
	weapon_collision(ElementPtr0, pPt0, ElementPtr1, pPt1);
	ElementPtr0->mass_points = original_mass;
}*/

/*static COUNT
initialize_spit (PELEMENT ShipPtr, HELEMENT SpitArray[])
{
#define MISSILE_DAMAGE 1
#define MISSILE_HITS 1
#define MISSILE_OFFSET 0
	STARSHIPPTR StarShipPtr;
	MISSILE_BLOCK MissileBlock;
	COUNT i;

	GetElementStarShip (ShipPtr, &StarShipPtr);
	MissileBlock.cx = ShipPtr->next.location.x;
	MissileBlock.cy = ShipPtr->next.location.y;
	MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.special;
	MissileBlock.face = MissileBlock.index = StarShipPtr->ShipFacing;
	MissileBlock.sender = (ShipPtr->state_flags & (GOOD_GUY | BAD_GUY))
			| IGNORE_SIMILAR | PERSISTENT;
	MissileBlock.pixoffs = ZOQFOTPIK_OFFSET;
	MissileBlock.speed = DISPLAY_TO_WORLD (
			GetFrameCount (StarShipPtr->RaceDescPtr->ship_data.weapon[0])) << 1;
	MissileBlock.hit_points = MISSILE_HITS;
	MissileBlock.damage = MISSILE_DAMAGE;
	MissileBlock.life = MISSILE_LIFE;
	MissileBlock.preprocess_func = NULL_PTR; //spit_preprocess;
	MissileBlock.blast_offs = MISSILE_OFFSET;
	//SpitArray[0] = initialize_missile (&MissileBlock);
	
	for(i = 0; i < 5; ++i)
	{
		SpitArray[i] = initialize_missile (&MissileBlock);

		if(SpitArray[i])
		{
			ELEMENTPTR SpitPtr;
			LockElement(SpitArray[i], &SpitPtr);

			SetVelocityComponents (&SpitPtr->velocity,
					COSINE (NORMALIZE_ANGLE(FACING_TO_ANGLE(MissileBlock.face) + i - 2), WORLD_TO_VELOCITY(MissileBlock.speed)),
					SINE (NORMALIZE_ANGLE(FACING_TO_ANGLE(MissileBlock.face) + i - 2), WORLD_TO_VELOCITY(MissileBlock.speed)));

			//SpitPtr->collision_func = spit_collision;

			UnlockElement(SpitArray[i]);
		}
	}

	return (5);
}*/

/*static void
spawn_beam (PELEMENT ShipPtr)
{
#define BEAM_DAMAGE 4
#define BEAM_HITS 6
#define BEAM_OFFSET 0
#define BEAM_SPEED DISPLAY_TO_WORLD (24)
#define BEAM_LIFE 50
	STARSHIPPTR StarShipPtr;
	MISSILE_BLOCK MissileBlock;
	HELEMENT Missile;

	GetElementStarShip (ShipPtr, &StarShipPtr);
	MissileBlock.cx = ShipPtr->next.location.x;
	MissileBlock.cy = ShipPtr->next.location.y;
	MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.special;
	MissileBlock.face = MissileBlock.index = StarShipPtr->ShipFacing;
	MissileBlock.sender = (ShipPtr->state_flags & (GOOD_GUY | BAD_GUY))
			| IGNORE_SIMILAR | PERSISTENT;
	MissileBlock.pixoffs = ZOQFOTPIK_OFFSET;
	MissileBlock.speed = BEAM_SPEED;
	MissileBlock.hit_points = BEAM_HITS;
	MissileBlock.damage = BEAM_DAMAGE;
	MissileBlock.life = BEAM_LIFE;
	MissileBlock.preprocess_func = NULL_PTR; //spit_preprocess;
	MissileBlock.blast_offs = BEAM_OFFSET;
	Missile = initialize_missile (&MissileBlock);
	
	if(Missile)
	{
		ELEMENTPTR MissilePtr;
		LockElement(Missile, &MissilePtr);

		SetElementStarShip (MissilePtr, StarShipPtr);

		UnlockElement(Missile);
		PutElement(Missile);
	}
}*/


static void spawn_tongue (PELEMENT ElementPtr);

static void
tongue_postprocess (PELEMENT ElementPtr)
{
	if (ElementPtr->turn_wait)
		spawn_tongue (ElementPtr);
}

static void
tongue_collision (PELEMENT ElementPtr0, PPOINT pPt0, PELEMENT ElementPtr1, PPOINT pPt1)
{
	STARSHIPPTR StarShipPtr;

	GetElementStarShip (ElementPtr0, &StarShipPtr);
	if (StarShipPtr->special_counter ==
			StarShipPtr->RaceDescPtr->characteristics.special_wait)
		weapon_collision (ElementPtr0, pPt0, ElementPtr1, pPt1);

	//StarShipPtr->special_counter -= ElementPtr0->turn_wait / 2;
	if ((ElementPtr1->state_flags & PLAYER_SHIP) || GRAVITY_MASS(ElementPtr1->mass_points))
	{
		ElementPtr0->turn_wait = 0;
		ElementPtr0->state_flags |= NONSOLID;
	}
	else
	{
		ElementPtr0->state_flags &= ~DISAPPEARING;
	}
}

static void
spawn_tongue (PELEMENT ElementPtr)
{
#define TONGUE_SPEED 0
#define TONGUE_HITS 1
#define TONGUE_DAMAGE 12
#define TONGUE_OFFSET 4
	STARSHIPPTR StarShipPtr;
	MISSILE_BLOCK TongueBlock;
	HELEMENT Tongue;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	TongueBlock.cx = ElementPtr->next.location.x;
	TongueBlock.cy = ElementPtr->next.location.y;
	TongueBlock.farray = StarShipPtr->RaceDescPtr->ship_data.special;
	TongueBlock.face = TongueBlock.index = StarShipPtr->ShipFacing;
	TongueBlock.sender = (ElementPtr->state_flags & (GOOD_GUY | BAD_GUY))
			| IGNORE_SIMILAR;
	TongueBlock.pixoffs = 0;
	TongueBlock.speed = TONGUE_SPEED;
	TongueBlock.hit_points = TONGUE_HITS;
	TongueBlock.damage = TONGUE_DAMAGE;
	TongueBlock.life = 1;
	TongueBlock.preprocess_func = 0;
	TongueBlock.blast_offs = TONGUE_OFFSET;
	Tongue = initialize_missile (&TongueBlock);
	if (Tongue)
	{
		ELEMENTPTR TonguePtr;

		LockElement (Tongue, &TonguePtr);
		TonguePtr->postprocess_func = tongue_postprocess;
		TonguePtr->collision_func = tongue_collision;
		if (ElementPtr->state_flags & PLAYER_SHIP)
			TonguePtr->turn_wait = StarShipPtr->special_counter * 2;
		else
		{
			COUNT angle;
			//RECT r;
			//SIZE x_offs, y_offs;

			TonguePtr->turn_wait = ElementPtr->turn_wait - 1;

			/*GetFrameRect (TonguePtr->current.image.frame, &r);
			x_offs = DISPLAY_TO_WORLD (r.extent.width);
			y_offs = DISPLAY_TO_WORLD (r.extent.height);*/

			angle = FACING_TO_ANGLE (StarShipPtr->ShipFacing);
			/*if (angle > HALF_CIRCLE && angle < FULL_CIRCLE)
				TonguePtr->current.location.x -= x_offs;
			else if (angle > 0 && angle < HALF_CIRCLE)
				TonguePtr->current.location.x += x_offs;
			if (angle < QUADRANT || angle > FULL_CIRCLE - QUADRANT)
				TonguePtr->current.location.y -= y_offs;
			else if (angle > QUADRANT && angle < FULL_CIRCLE - QUADRANT)
				TonguePtr->current.location.y += y_offs;*/
			TonguePtr->current.location.x += COSINE(angle, DISPLAY_TO_WORLD(10));
			TonguePtr->current.location.y += SINE(angle, DISPLAY_TO_WORLD(10));
		}

		SetElementStarShip (TonguePtr, StarShipPtr);
		UnlockElement (Tongue);
		PutElement (Tongue);
	}
}

static void
zoqfotpik_intelligence (PELEMENT ShipPtr, PEVALUATE_DESC ObjectsOfConcern, COUNT ConcernCounter)
{
	BOOLEAN GiveTongueJob;
	STARSHIPPTR StarShipPtr;

	GetElementStarShip (ShipPtr, &StarShipPtr);

	GiveTongueJob = FALSE;
	if (StarShipPtr->special_counter == 0)
	{
			PEVALUATE_DESC lpEnemyEvalDesc;

		StarShipPtr->ship_input_state &= ~SPECIAL;

		lpEnemyEvalDesc = &ObjectsOfConcern[ENEMY_SHIP_INDEX];
		if (lpEnemyEvalDesc->ObjectPtr
				&& lpEnemyEvalDesc->which_turn <= 4
#ifdef NEVER
				&& StarShipPtr->RaceDescPtr->ship_info.energy_level >= SPECIAL_ENERGY_COST
#endif /* NEVER */
				)
		{
			SIZE delta_x, delta_y;

			GiveTongueJob = TRUE;

			lpEnemyEvalDesc->MoveState = PURSUE;
			delta_x = lpEnemyEvalDesc->ObjectPtr->next.location.x
					- ShipPtr->next.location.x;
			delta_y = lpEnemyEvalDesc->ObjectPtr->next.location.y
					- ShipPtr->next.location.y;
			if (StarShipPtr->ShipFacing == NORMALIZE_FACING (
					ANGLE_TO_FACING (ARCTAN (delta_x, delta_y))
					))
				StarShipPtr->ship_input_state |= SPECIAL;
		}
	}

	++StarShipPtr->weapon_counter;
	ship_intelligence (ShipPtr, ObjectsOfConcern, ConcernCounter);
	--StarShipPtr->weapon_counter;

	if (StarShipPtr->weapon_counter == 0)
	{
		StarShipPtr->ship_input_state &= ~WEAPON;
		if (!GiveTongueJob)
		{
			ObjectsOfConcern += ConcernCounter;
			while (ConcernCounter--)
			{
				--ObjectsOfConcern;
				if (ObjectsOfConcern->ObjectPtr
						&& (ConcernCounter == ENEMY_SHIP_INDEX
						|| (ConcernCounter == ENEMY_WEAPON_INDEX
						&& ObjectsOfConcern->MoveState != AVOID
#ifdef NEVER
						&& !(PlayerControl[cur_player] & STANDARD_RATING)
#endif /* NEVER */
						))
						&& ship_weapons (ShipPtr,
						ObjectsOfConcern->ObjectPtr, DISPLAY_TO_WORLD (20)))
				{
					StarShipPtr->ship_input_state |= WEAPON;
					break;
				}
			}
		}
	}
}

static void
zoqfotpik_collision (PELEMENT ElementPtr0, PPOINT pPt0, PELEMENT ElementPtr1, PPOINT pPt1)
{
	if ((ElementPtr0->state_flags & PLAYER_SHIP))
	{
		collision(ElementPtr0, pPt0, ElementPtr1, pPt1);
	}
	else if (!(ElementPtr1->state_flags & FINITE_LIFE))
	{
		ElementPtr0->state_flags |= COLLISION;
	}

	/*if (ElementPtr0->life_span == NORMAL_LIFE)
	{
		collision(ElementPtr0, pPt0, ElementPtr1, pPt1);
	}
	else if (ElementPtr1->state_flags & FINITE_LIFE)
	{
		//get knocked back a little, so you can't shield your tail with your body
		SIZE dx, dy, mag, damage_effect;

		dx = ElementPtr0->next.location.x
			- ElementPtr1->next.location.x;
		dy = ElementPtr0->next.location.y
			- ElementPtr1->next.location.y;

		mag = square_root((long)dx*dx + (long)dy*dy);
		damage_effect = ElementPtr1->mass_points;
		if (damage_effect > 6) damage_effect = 6;

		DeltaVelocityComponents(&ElementPtr0->velocity,
			(dx * damage_effect * 250) / mag,
			(dy * damage_effect * 250) / mag);
	}
	else
	{
		ElementPtr0->state_flags |= COLLISION;
	}*/
}

static void
fix_tail (PELEMENT ElementPtr)
{
	HELEMENT hShip, hNextShip, hBestShip;
	ELEMENTPTR IterPtr;
	COUNT best_turn_wait;

	hBestShip = 0;
	best_turn_wait = 0;

	for (hShip = GetHeadElement (); hShip != 0; hShip = hNextShip)
	{
		LockElement (hShip, &IterPtr);
		hNextShip = GetSuccElement (IterPtr);
		if ((!(IterPtr->state_flags & FINITE_LIFE))
			&& IterPtr->crew_level > 0
			&& IterPtr->turn_wait < ElementPtr->turn_wait
			&& (IterPtr->state_flags & (GOOD_GUY | BAD_GUY)) ==
			(ElementPtr->state_flags & (GOOD_GUY | BAD_GUY)))
		{
			if(hBestShip == 0 || IterPtr->turn_wait > best_turn_wait)
			{
				hBestShip = hShip;
				best_turn_wait = IterPtr->turn_wait;
			}
		}
		UnlockElement (hShip);
	}

	ElementPtr->hTarget = hBestShip;
}

#define FOLLOW_DISTANCE 40
#define MAX_ACCELERATION 500

#include "libs/log.h"
static void
tail_follow (PELEMENT ElementPtr)
{
	ELEMENTPTR LeaderPtr;
	SIZE dx, dy;
	COUNT angle;

	LockElement(ElementPtr->hTarget, &LeaderPtr);

	if (!LeaderPtr)
	{
		fix_tail (ElementPtr);
	}
	else
	{
		dx = WRAP_DELTA_X (LeaderPtr->next.location.x
			- ElementPtr->next.location.x);
		dy = WRAP_DELTA_Y (LeaderPtr->next.location.y
			- ElementPtr->next.location.y);
		//if((long)dx*dx + (long)dy*dy > FOLLOW_DISTANCE*FOLLOW_DISTANCE)
		{
			SIZE mag;
			SIZE cur_dx, cur_dy;
			SIZE leader_dx, leader_dy;
			SIZE diff_dx, diff_dy;
			long excess_dist; 
			excess_dist = square_root((long)dx*dx + (long)dy*dy) - square_root(FOLLOW_DISTANCE*FOLLOW_DISTANCE);
			if(excess_dist > FOLLOW_DISTANCE * 5)excess_dist = FOLLOW_DISTANCE * 5;
	
			mag = square_root(dx*dx + dy*dy);
	
			GetCurrentVelocityComponents(&LeaderPtr->velocity, &leader_dx, &leader_dy);
			
			GetCurrentVelocityComponents(&ElementPtr->velocity, &cur_dx, &cur_dy);
			if(mag)
			{
				diff_dx = (dx * excess_dist * 15) / mag - cur_dx;
				diff_dy = (dy * excess_dist * 15) / mag - cur_dy;
				//diff_dx = (dx * excess_dist) / mag - (cur_dx - leader_dx);
				//diff_dy = (dy * excess_dist) / mag - (cur_dy - leader_dy);
			}
			else
			{
				diff_dx = -cur_dx;
				diff_dy = -cur_dy;
			}

			if((long)diff_dx * dx >= 0) // i.e. we're accelerating towards the leader
			{
				long dvx, dvy, dvx2, dvy2;

				//limit acceleration
				if((long)diff_dx*diff_dx + (long)diff_dy*diff_dy > MAX_ACCELERATION*MAX_ACCELERATION)
				{
					SIZE divisor;
					divisor = square_root((long)diff_dx*diff_dx + (long)diff_dy*diff_dy);
					diff_dx = (diff_dx * MAX_ACCELERATION) / divisor;
					diff_dy = (diff_dy * MAX_ACCELERATION) / divisor;
				}
				
				//limit insanity
				dvx = leader_dx - cur_dx;
				dvy = leader_dy - cur_dy;
				dvx2 = dvx + diff_dx;
				dvy2 = dvy + diff_dy;
				if(dvx2*dvx2 + dvy2*dvy2 > (long)MAX_ACCELERATION*MAX_ACCELERATION*24*24 && dvx2*dvx2 + dvy2*dvy2 > dvx*dvx + dvy*dvy)
				{
					diff_dx = 0;
					diff_dy = 0;
				}
			}

			if(!(ElementPtr->state_flags & DEFY_PHYSICS))
			{
				DeltaVelocityComponents(&ElementPtr->velocity,
					diff_dx,
					diff_dy);
			
				/*DeltaVelocityComponents(&LeaderPtr->velocity,
					-diff_dx,
					-diff_dy);*/
			}
		}
	
		angle = ARCTAN(dx, dy);
		ElementPtr->next.image.frame = SetAbsFrameIndex(ElementPtr->current.image.farray[0], ANGLE_TO_FACING(angle));
		ElementPtr->state_flags |= CHANGING;
	}
	
	UnlockElement(ElementPtr->hTarget);
}

static void
tail_death (PELEMENT ElementPtr)
{
	/*ELEMENTPTR LeaderPtr;

	LockElement(ElementPtr->hTarget, &LeaderPtr);
	if(LeaderPtr)LeaderPtr->life_span = NORMAL_LIFE;
	UnlockElement(ElementPtr->hTarget);*/

	ElementPtr->hTarget = 0;
	ElementPtr->preprocess_func = 0;
}

#define TAIL_SECTIONS 30

static void
spawn_tail (PELEMENT ElementPtr)
{
	STARSHIPPTR StarShipPtr;
	ELEMENTPTR ShipPtr;
	HELEMENT hTail[TAIL_SECTIONS];
	COUNT i;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	LockElement(StarShipPtr->hShip, &ShipPtr);

	//ShipPtr->life_span = NORMAL_LIFE + 1;
	
	for(i = 0; i < TAIL_SECTIONS; ++i)
	{
		hTail[i] = AllocElement();

		if(!hTail[i])
		{
			/*// uh oh, it could not be made
			// so make the last one vulnerable
			if(i==0)ShipPtr->life_span = NORMAL_LIFE;
			else
			{
				ELEMENTPTR TailPtr;
				LockElement (hTail[i - 1], &TailPtr);
				TailPtr->life_span = NORMAL_LIFE;
				UnlockElement (hTail[i - i]);
			}*/

			break;
		}
		else
		{
			ELEMENTPTR TailPtr;

			LockElement (hTail[i], &TailPtr);

			TailPtr->state_flags =
				IGNORE_SIMILAR | APPEARING
				| (ShipPtr->state_flags & (GOOD_GUY | BAD_GUY));
			/*if(i == TAIL_SECTIONS - 1)*/TailPtr->life_span = NORMAL_LIFE;
			//else TailPtr->life_span = NORMAL_LIFE + 1;
			TailPtr->hit_points = MAX_CREW;
			TailPtr->mass_points = SHIP_MASS;

			TailPtr->turn_wait = i;

			TailPtr->current.location.x = ShipPtr->current.location.x;
			TailPtr->current.location.y = ShipPtr->current.location.y;
			TailPtr->current.image.farray = ShipPtr->current.image.farray;
			TailPtr->current.image.frame = ShipPtr->current.image.frame;
			
			if(i == 0)TailPtr->hTarget = StarShipPtr->hShip;
			else TailPtr->hTarget = hTail[i - 1];
			TailPtr->preprocess_func = tail_follow;
			TailPtr->collision_func = zoqfotpik_collision;
			TailPtr->death_func = tail_death;

			SetElementStarShip(TailPtr, StarShipPtr);

			SetPrimType (&(GLOBAL (DisplayArray))[
				TailPtr->PrimIndex
				], STAMP_PRIM);

			UnlockElement (hTail[i]);
		}
	}

	for(i = 0; i < TAIL_SECTIONS; ++i)
	{
		if(hTail[i])
		{
			PutElement(hTail[i]);
		}
		else
		{
			break;
		}
	}
	
	UnlockElement(StarShipPtr->hShip);
}

static void
zoqfotpik_postprocess (PELEMENT ElementPtr)
{
	STARSHIPPTR StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	if ((StarShipPtr->cur_status_flags & SPECIAL)
			&& StarShipPtr->special_counter == 0
			&& DeltaEnergy (ElementPtr, -SPECIAL_ENERGY_COST))
	{
		ProcessSound (SetAbsSoundIndex (
					// STICK_OUT_TONGUE 
				StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1), ElementPtr);

		StarShipPtr->special_counter =
				StarShipPtr->RaceDescPtr->characteristics.special_wait;
	}

	if (StarShipPtr->special_counter)
		spawn_tongue (ElementPtr);

	/*if (StarShipPtr->cur_status_flags & SPECIAL)
	{
		++ElementPtr->turn_wait;
		++ElementPtr->thrust_wait;
		spawn_beam (ElementPtr);
	}*/
}

static HELEMENT
find_best_leader (PELEMENT ShipPtr)
{
	HELEMENT hShip, hNextShip, hBestShip;
	ELEMENTPTR IterPtr;
	COUNT best_turn_wait;

	hBestShip = 0;
	best_turn_wait = 255;

	for (hShip = GetHeadElement (); hShip != 0; hShip = hNextShip)
	{
		LockElement (hShip, &IterPtr);
		hNextShip = GetSuccElement (IterPtr);
		if ((!(IterPtr->state_flags & FINITE_LIFE))
			&& IterPtr->crew_level > 0
			&& (IterPtr->state_flags & (GOOD_GUY | BAD_GUY)) ==
			(ShipPtr->state_flags & (GOOD_GUY | BAD_GUY)))
		{
			if(hBestShip == 0 || IterPtr->turn_wait < best_turn_wait)
			{
				hBestShip = hShip;
				best_turn_wait = IterPtr->turn_wait;
			}
		}
		UnlockElement (hShip);
	}
	return hBestShip;
}

static void zoqfotpik_preprocess2 (PELEMENT ElementPtr);

static void
recontrol_zoqfotpik (PELEMENT ElementPtr)
{
	STARSHIPPTR StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	if (!(ElementPtr->state_flags & PLAYER_SHIP))
	{
		ELEMENTPTR ShipPtr;

		LockElement (StarShipPtr->hShip, &ShipPtr);
		ShipPtr->death_func = recontrol_zoqfotpik;
		UnlockElement (StarShipPtr->hShip);
	}
	else
	{
		HELEMENT hBestShip;
		ELEMENTPTR TakeOverPtr, NewShipPtr, OldShipPtr;

		if (!(hBestShip = find_best_leader(ElementPtr)))
		{
			//uh oh
			ElementPtr->death_func =
					(void (*) (PELEMENT ElementPtr))
						StarShipPtr->RaceDescPtr->init_weapon_func;
			ElementPtr->preprocess_func = StarShipPtr->RaceDescPtr->preprocess_func;
			ElementPtr->postprocess_func = StarShipPtr->RaceDescPtr->postprocess_func;
			StarShipPtr->RaceDescPtr->preprocess_func = zoqfotpik_preprocess2;
			StarShipPtr->RaceDescPtr->postprocess_func = zoqfotpik_postprocess;
			ElementPtr->state_flags &= ~DISAPPEARING;
			ElementPtr->life_span = 1;
			ElementPtr->mass_points = SHIP_MASS;
			return;
		}

		LockElement (hBestShip, &TakeOverPtr);

		NewShipPtr = TakeOverPtr;
		OldShipPtr = ElementPtr;

		NewShipPtr->state_flags = APPEARING | PLAYER_SHIP | IGNORE_SIMILAR
				| (ElementPtr->state_flags & (GOOD_GUY | BAD_GUY));

		NewShipPtr->mass_points = SHIP_MASS;
		NewShipPtr->preprocess_func = StarShipPtr->RaceDescPtr->preprocess_func;
		NewShipPtr->postprocess_func = StarShipPtr->RaceDescPtr->postprocess_func;
		NewShipPtr->death_func =
				(void (*) (PELEMENT ElementPtr))
						StarShipPtr->RaceDescPtr->init_weapon_func;
		StarShipPtr->RaceDescPtr->preprocess_func = zoqfotpik_preprocess2;
		StarShipPtr->RaceDescPtr->postprocess_func = zoqfotpik_postprocess;
		StarShipPtr->RaceDescPtr->init_weapon_func = initialize_spit;
		StarShipPtr->RaceDescPtr->ship_info.crew_level = TakeOverPtr->crew_level;
		StarShipPtr->RaceDescPtr->ship_info.energy_level = MAX_ENERGY;
					/* DONT fix [s]vux[/s] arilou impairment */
		/*StarShipPtr->RaceDescPtr->characteristics.max_thrust = MAX_THRUST;
		StarShipPtr->RaceDescPtr->characteristics.thrust_increment = THRUST_INCREMENT;*/
		StarShipPtr->RaceDescPtr->characteristics.turn_wait = TURN_WAIT;
		StarShipPtr->RaceDescPtr->characteristics.thrust_wait = THRUST_WAIT;
		StarShipPtr->RaceDescPtr->characteristics.special_wait = SPECIAL_WAIT;

		StarShipPtr->ship_input_state = 0;
		StarShipPtr->cur_status_flags =
				StarShipPtr->old_status_flags = 0;
		StarShipPtr->energy_counter =
				StarShipPtr->weapon_counter =
				StarShipPtr->special_counter = 0;
		NewShipPtr->crew_level =
				NewShipPtr->turn_wait = NewShipPtr->thrust_wait = 0;
		NewShipPtr->life_span = NORMAL_LIFE;

		NewShipPtr->current = TakeOverPtr->current;
		SetPrimType (&(GLOBAL (DisplayArray))[
				NewShipPtr->PrimIndex
				], STAMP_PRIM);
		StarShipPtr->ShipFacing = GetFrameIndex(NewShipPtr->current.image.frame);

		OldShipPtr->crew_level = 0;
		OldShipPtr->life_span = 0;
		OldShipPtr->state_flags |= DISAPPEARING | NONSOLID;

		StarShipPtr->hShip = hBestShip;
		NewShipPtr->hTarget = StarShipPtr->hShip;

		UnlockElement (hBestShip);
	}
}

static void
intercept_zoqfotpik_death (PELEMENT ElementPtr)
{
	STARSHIPPTR StarShipPtr;

	ElementPtr->state_flags &= ~DISAPPEARING;
	ElementPtr->life_span = 1;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	if (StarShipPtr->RaceDescPtr->ship_info.crew_level == 0)
	{
		ELEMENTPTR ShipPtr;

		LockElement (StarShipPtr->hShip, &ShipPtr);
		/*if (GRAVITY_MASS (ShipPtr->mass_points + 1))
		{
			ElementPtr->state_flags |= DISAPPEARING;
			ElementPtr->life_span = 0;
		}
		else
		{*/

			if (find_best_leader(ShipPtr))
			{
				ShipPtr->mass_points = MAX_SHIP_MASS + 1;
				StarShipPtr->RaceDescPtr->preprocess_func = ShipPtr->preprocess_func;
				StarShipPtr->RaceDescPtr->postprocess_func = ShipPtr->postprocess_func;
				StarShipPtr->RaceDescPtr->init_weapon_func =
						(COUNT (*) (PELEMENT ElementPtr,
								HELEMENT Weapon[]))
								ShipPtr->death_func;
	
				ElementPtr->death_func = recontrol_zoqfotpik;
			}
			else
			{
				ElementPtr->state_flags |= DISAPPEARING;
				ElementPtr->life_span = 0;
			}
		/*}*/
		UnlockElement (StarShipPtr->hShip);
	}
}

static void
zoqfotpik_preprocess2 (PELEMENT ElementPtr)
{
	STARSHIPPTR StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);

	if((ElementPtr->state_flags & APPEARING))
	{
		ELEMENTPTR PhoenixPtr;
		HELEMENT hPhoenix;

		if ((hPhoenix = AllocElement ()))
		{

			LockElement (hPhoenix, &PhoenixPtr);
			PhoenixPtr->state_flags =
					FINITE_LIFE | NONSOLID | IGNORE_SIMILAR
					| (ElementPtr->state_flags & (GOOD_GUY | BAD_GUY));
			PhoenixPtr->life_span = 1;

			PhoenixPtr->death_func = intercept_zoqfotpik_death; //intercept_pkunk_death;

			SetElementStarShip (PhoenixPtr, StarShipPtr);

			UnlockElement (hPhoenix);
			InsertElement (hPhoenix, GetHeadElement ());
		}
		StarShipPtr->RaceDescPtr->preprocess_func = 0;
	}
}

static void
zoqfotpik_preprocess (PELEMENT ElementPtr)
{
	HELEMENT hSpawner;
	STARSHIPPTR StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	hSpawner = AllocElement ();
	if (hSpawner)
	{
		ELEMENTPTR SpawnerPtr;
		STARSHIPPTR StarShipPtr;

		LockElement (hSpawner, &SpawnerPtr);
		SpawnerPtr->state_flags =
				FINITE_LIFE | NONSOLID | IGNORE_SIMILAR | APPEARING
				| (ElementPtr->state_flags & (GOOD_GUY | BAD_GUY));
		SpawnerPtr->life_span = HYPERJUMP_LIFE + 1;

		SpawnerPtr->death_func = spawn_tail;

		GetElementStarShip (ElementPtr, &StarShipPtr);
		SetElementStarShip (SpawnerPtr, StarShipPtr);

		SetPrimType (&(GLOBAL (DisplayArray))[
				SpawnerPtr->PrimIndex
				], NO_PRIM);

		UnlockElement (hSpawner);
		PutElement (hSpawner);
	}

	zoqfotpik_preprocess2(ElementPtr);

	StarShipPtr->RaceDescPtr->preprocess_func = 0;
	ElementPtr->collision_func = zoqfotpik_collision;
}

RACE_DESCPTR
init_zoqfotpik (void)
{
	RACE_DESCPTR RaceDescPtr;

	zoqfotpik_desc.preprocess_func = zoqfotpik_preprocess;
	zoqfotpik_desc.postprocess_func = zoqfotpik_postprocess;
	zoqfotpik_desc.init_weapon_func = initialize_spit;
	zoqfotpik_desc.cyborg_control.intelligence_func =
			(void (*) (PVOID ShipPtr, PVOID ObjectsOfConcern, COUNT
					ConcernCounter)) zoqfotpik_intelligence;

	RaceDescPtr = &zoqfotpik_desc;

	return (RaceDescPtr);
}

