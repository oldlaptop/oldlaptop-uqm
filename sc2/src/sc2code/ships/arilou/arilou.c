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
#include "ships/arilou/resinst.h"

#include "libs/mathlib.h"


#define MAX_CREW 6
#define MAX_ENERGY 20
#define ENERGY_REGENERATION 1
#define WEAPON_ENERGY_COST 2
#define SPECIAL_ENERGY_COST 3
#define ENERGY_WAIT 6
#define MAX_THRUST /* DISPLAY_TO_WORLD (10) */ 40
#define THRUST_INCREMENT MAX_THRUST
#define TURN_WAIT 0
#define THRUST_WAIT 0
#define WEAPON_WAIT 1
#define SPECIAL_WAIT 2

#define SHIP_MASS 1
#define ARILOU_OFFSET 9
#define LASER_RANGE DISPLAY_TO_WORLD (100 + ARILOU_OFFSET)

static RACE_DESC arilou_desc =
{
	{
		/* FIRES_FORE | */ IMMEDIATE_WEAPON,
		50, /* Super Melee cost */
		250 / SPHERE_RADIUS_INCREMENT, /* Initial sphere of influence radius */
		MAX_CREW, MAX_CREW,
		MAX_ENERGY, MAX_ENERGY,
		{
			438, 6372,
		},
		(STRING)ARILOU_RACE_STRINGS,
		(FRAME)ARILOU_ICON_MASK_PMAP_ANIM,
		(FRAME)ARILOU_MICON_MASK_PMAP_ANIM,
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
			(FRAME)ARILOU_BIG_MASK_PMAP_ANIM,
			(FRAME)ARILOU_MED_MASK_PMAP_ANIM,
			(FRAME)ARILOU_SML_MASK_PMAP_ANIM,
		},
		{
			(FRAME)0,
			(FRAME)0,
			(FRAME)0,
		},
		{
			(FRAME)WARP_BIG_MASK_PMAP_ANIM,
			(FRAME)WARP_MED_MASK_PMAP_ANIM,
			(FRAME)WARP_SML_MASK_PMAP_ANIM,
		},
		{
			(FRAME)ARILOU_CAPTAIN_MASK_PMAP_ANIM,
			(FRAME)0,
			(FRAME)0,
			(FRAME)0,
			(FRAME)0,
		},
		(SOUND)ARILOU_VICTORY_SONG,
		(SOUND)ARILOU_SHIP_SOUNDS,
	},
	{
		0,
		LASER_RANGE >> 1,
		NULL_PTR,
	},
	(UNINIT_FUNC *) NULL,
	(PREPROCESS_FUNC *) NULL,
	(POSTPROCESS_FUNC *) NULL,
	(INIT_WEAPON_FUNC *) NULL,
	0,
};

static COUNT
initialize_autoaim_laser (PELEMENT ShipPtr, HELEMENT LaserArray[])
{
	COUNT orig_facing;
	SIZE delta_facing;
	STARSHIPPTR StarShipPtr;
	LASER_BLOCK LaserBlock;

	GetElementStarShip (ShipPtr, &StarShipPtr);
	LaserBlock.face = orig_facing = StarShipPtr->ShipFacing;
	if ((delta_facing = TrackShip (ShipPtr, &LaserBlock.face)) > 0)
		LaserBlock.face = NORMALIZE_FACING (orig_facing + delta_facing);
	//ShipPtr->hTarget = 0;

	LaserBlock.cx = ShipPtr->next.location.x;
	LaserBlock.cy = ShipPtr->next.location.y;
	LaserBlock.ex = COSINE (FACING_TO_ANGLE (LaserBlock.face), LASER_RANGE);
	LaserBlock.ey = SINE (FACING_TO_ANGLE (LaserBlock.face), LASER_RANGE);
	LaserBlock.sender = (ShipPtr->state_flags & (GOOD_GUY | BAD_GUY))
			| IGNORE_SIMILAR;
	LaserBlock.pixoffs = ARILOU_OFFSET;
	LaserBlock.color = BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x0A), 0x0E);
	LaserArray[0] = initialize_laser (&LaserBlock);
	
	return (1);
}

static void
arilou_intelligence (PELEMENT ShipPtr, PEVALUATE_DESC ObjectsOfConcern,
		COUNT ConcernCounter)
{
	STARSHIPPTR StarShipPtr;

	GetElementStarShip (ShipPtr, &StarShipPtr);
	StarShipPtr->ship_input_state |= THRUST;

	 ObjectsOfConcern[ENEMY_SHIP_INDEX].MoveState = ENTICE;
	ship_intelligence (ShipPtr, ObjectsOfConcern, ConcernCounter);

	if (StarShipPtr->special_counter == 0)
	{
		PEVALUATE_DESC lpEvalDesc;

		StarShipPtr->ship_input_state &= ~SPECIAL;

		lpEvalDesc = &ObjectsOfConcern[ENEMY_WEAPON_INDEX];
		if (lpEvalDesc->ObjectPtr && lpEvalDesc->which_turn <= 6)
		{
			BOOLEAN IsTrackingWeapon;
			STARSHIPPTR EnemyStarShipPtr;

			GetElementStarShip (lpEvalDesc->ObjectPtr, &EnemyStarShipPtr);
			if (((EnemyStarShipPtr->RaceDescPtr->ship_info.ship_flags
					& SEEKING_WEAPON) &&
					lpEvalDesc->ObjectPtr->next.image.farray ==
					EnemyStarShipPtr->RaceDescPtr->ship_data.weapon) ||
					((EnemyStarShipPtr->RaceDescPtr->ship_info.ship_flags
					& SEEKING_SPECIAL) &&
					lpEvalDesc->ObjectPtr->next.image.farray ==
					EnemyStarShipPtr->RaceDescPtr->ship_data.special))
				IsTrackingWeapon = TRUE;
			else
				IsTrackingWeapon = FALSE;

			if (((lpEvalDesc->ObjectPtr->state_flags & PLAYER_SHIP) /* means IMMEDIATE WEAPON */
					|| (IsTrackingWeapon && (lpEvalDesc->which_turn == 1
					|| (lpEvalDesc->ObjectPtr->state_flags & CREW_OBJECT))) /* FIGHTERS!!! */
					|| PlotIntercept (lpEvalDesc->ObjectPtr, ShipPtr, 3, 0))
					&& !(TFB_Random () & 3))
			{
				StarShipPtr->ship_input_state &= ~(LEFT | RIGHT | THRUST | WEAPON);
				StarShipPtr->ship_input_state |= SPECIAL;
			}
		}
	}
	if (StarShipPtr->RaceDescPtr->ship_info.energy_level <= SPECIAL_ENERGY_COST << 1)
		StarShipPtr->ship_input_state &= ~WEAPON;
}

/*static void
arilou_collision (PELEMENT ElementPtr0, PPOINT pPt0,
		PELEMENT ElementPtr1, PPOINT pPt1)
{
	//Don't collide!
}*/

static void
arilou_preprocess (PELEMENT ElementPtr)
{
	//ElementPtr->collision_func = arilou_collision;

	STARSHIPPTR StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	if (!(ElementPtr->state_flags & NONSOLID))
	{
		HELEMENT hShip, hNextShip;
		ELEMENTPTR Enemy;
		STARSHIPPTR EnemyStarShip;
		BOOLEAN found_an_enemy = false;
		for (hShip = GetHeadElement (); hShip != 0; hShip = hNextShip)
		{
			LockElement (hShip, &Enemy);
			hNextShip = GetSuccElement (Enemy);
			if (Enemy->state_flags & PLAYER_SHIP
				&& (Enemy->state_flags & (GOOD_GUY | BAD_GUY)) !=
				(ElementPtr->state_flags & (GOOD_GUY | BAD_GUY)))
			{
				found_an_enemy = true;
				GetElementStarShip (Enemy, &EnemyStarShip);
				break;
			}
		}

		if (ElementPtr->thrust_wait == 0)
		{
			ZeroVelocityComponents (&ElementPtr->velocity);
			StarShipPtr->cur_status_flags &= ~SHIP_AT_MAX_SPEED;
		}

		if ((StarShipPtr->cur_status_flags & SPECIAL)
				&& StarShipPtr->special_counter == 0
				&& !(StarShipPtr->cur_status_flags & PLAY_VICTORY_DITTY)
				&& (!found_an_enemy || !(EnemyStarShip->RaceResIndex == StarShipPtr->RaceResIndex))
				&& DeltaEnergy (ElementPtr, -SPECIAL_ENERGY_COST))
		{
			//can't be used during victory ditty
			//that causes bugs..
			//can't be used against another arilou
			//otherwise, there could be a nasty standoff...

			//COPIED from DoRunAway:
			//
			if (GetPrimType (&DisplayArray[ElementPtr->PrimIndex]) == STAMP_PRIM
					&& ElementPtr->life_span == NORMAL_LIFE
					&& !(ElementPtr->state_flags & FINITE_LIFE)
					&& ElementPtr->mass_points != MAX_SHIP_MASS * 10
					&& !(ElementPtr->state_flags & APPEARING))
			{
				extern void flee_preprocess (PELEMENT);
				extern void ship_transition (PELEMENT);
				extern void new_ship (PELEMENT);
	
				ElementPtr->turn_wait = 3;
				ElementPtr->thrust_wait = MAKE_BYTE (4, 0);
				ElementPtr->preprocess_func = flee_preprocess;
				ElementPtr->mass_points = MAX_SHIP_MASS * 10;
				ZeroVelocityComponents (&ElementPtr->velocity);
				StarShipPtr->cur_status_flags &=
						~(SHIP_AT_MAX_SPEED | SHIP_BEYOND_MAX_SPEED);

				SetPrimColor (&DisplayArray[ElementPtr->PrimIndex],
						BUILD_COLOR (MAKE_RGB15 (0x0B, 0x00, 0x00), 0x2E));
				SetPrimType (&DisplayArray[ElementPtr->PrimIndex], STAMPFILL_PRIM);
			
				CyborgDescPtr->ship_input_state = 0;

				//copied from flee_preprocess
				ElementPtr->death_func = new_ship;
				ElementPtr->crew_level = 0;
	
				ElementPtr->life_span = HYPERJUMP_LIFE + 1;
				ElementPtr->preprocess_func = ship_transition;
				ElementPtr->postprocess_func = NULL_PTR;
				SetPrimType (&DisplayArray[ElementPtr->PrimIndex], NO_PRIM);
				ElementPtr->state_flags |= NONSOLID | FINITE_LIFE | CHANGING;
				//end copied from flee_preprocess
			}
			//END COPIED FROM DoRunAway
		}

		UnlockElement (ElementPtr->hTarget);
	}
	else if (ElementPtr->next.image.farray == StarShipPtr->RaceDescPtr->ship_data.special)
	{
		COUNT life_span;

		StarShipPtr->cur_status_flags =
				(StarShipPtr->cur_status_flags
				& ~(LEFT | RIGHT | THRUST | WEAPON | SPECIAL))
				| (StarShipPtr->old_status_flags
				& (LEFT | RIGHT | THRUST | WEAPON | SPECIAL));
		++StarShipPtr->weapon_counter;
		++StarShipPtr->special_counter;
		++StarShipPtr->energy_counter;
		++ElementPtr->turn_wait;
		++ElementPtr->thrust_wait;

		if ((life_span = ElementPtr->life_span) == NORMAL_LIFE)
		{
			ElementPtr->state_flags &= ~(NONSOLID | FINITE_LIFE);
			ElementPtr->state_flags |= APPEARING;
			ElementPtr->current.image.farray =
					ElementPtr->next.image.farray =
					StarShipPtr->RaceDescPtr->ship_data.ship;
			ElementPtr->current.image.frame =
					ElementPtr->next.image.frame =
					SetAbsFrameIndex (StarShipPtr->RaceDescPtr->ship_data.ship[0],
					StarShipPtr->ShipFacing);
			InitIntersectStartPoint (ElementPtr);
		}
		else
		{
			--life_span;
			if (life_span != 2)
			{
				if (life_span < 2)
					ElementPtr->next.image.frame =
							DecFrameIndex (ElementPtr->next.image.frame);
				else
					ElementPtr->next.image.frame =
							IncFrameIndex (ElementPtr->next.image.frame);
			}
			else
			{
				ElementPtr->next.location.x =
						WRAP_X (DISPLAY_ALIGN_X (TFB_Random ()));
				ElementPtr->next.location.y =
						WRAP_Y (DISPLAY_ALIGN_Y (TFB_Random ()));
			}
		}

		ElementPtr->state_flags |= CHANGING;
	}
}

RACE_DESCPTR
init_arilou (void)
{
	RACE_DESCPTR RaceDescPtr;

	arilou_desc.preprocess_func = arilou_preprocess;
	arilou_desc.init_weapon_func = initialize_autoaim_laser;
	arilou_desc.cyborg_control.intelligence_func =
			(void (*) (PVOID ShipPtr, PVOID ObjectsOfConcern, COUNT
					ConcernCounter)) arilou_intelligence;

	RaceDescPtr = &arilou_desc;

	return (RaceDescPtr);
}

