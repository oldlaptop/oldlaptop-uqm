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
#include "ships/mycon/resinst.h"

#define MAX_CREW 20
#define MAX_ENERGY MAX_ENERGY_SIZE //40
#define INITIAL_ENERGY 14
#define ENERGY_REGENERATION 1
#define WEAPON_ENERGY_COST 6 //20
#define SPECIAL_ENERGY_COST 2 //MAX_ENERGY
#define ENERGY_WAIT 6 //4
#define MAX_THRUST /* DISPLAY_TO_WORLD (7) */ 0 //27
#define THRUST_INCREMENT /* DISPLAY_TO_WORLD (2) */ 0 //9
#define TURN_WAIT 9 //6
#define THRUST_WAIT 255 //6
#define WEAPON_WAIT 5
#define SPECIAL_WAIT 0

#define SHIP_MASS 10 //(MAX_SHIP_MASS * 10) //7

#define NUM_PLASMAS 11
#define NUM_GLOBALLS 8
#define PLASMA_DURATION 13
#define MISSILE_LIFE (NUM_PLASMAS * PLASMA_DURATION)
#define MISSILE_SPEED DISPLAY_TO_WORLD (8)
#define MISSILE_HELD_SPEED DISPLAY_TO_WORLD (16)

static RACE_DESC mycon_desc =
{
	{
		FIRES_FORE | SEEKING_WEAPON,
		37, /* Super Melee cost */
		1070 / SPHERE_RADIUS_INCREMENT, /* Initial sphere of influence radius */
		MAX_CREW, MAX_CREW,
		INITIAL_ENERGY, MAX_ENERGY,
		{
			6392, 2200,
		},
		(STRING)MYCON_RACE_STRINGS,
		(FRAME)MYCON_ICON_MASK_PMAP_ANIM,
		(FRAME)MYCON_MICON_MASK_PMAP_ANIM,
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
			(FRAME)MYCON_BIG_MASK_PMAP_ANIM,
			(FRAME)MYCON_MED_MASK_PMAP_ANIM,
			(FRAME)MYCON_SML_MASK_PMAP_ANIM,
		},
		{
			(FRAME)PLASMA_BIG_MASK_PMAP_ANIM,
			(FRAME)PLASMA_MED_MASK_PMAP_ANIM,
			(FRAME)PLASMA_SML_MASK_PMAP_ANIM,
		},
		{
			(FRAME)0,
			(FRAME)0,
			(FRAME)0,
		},
		{
			(FRAME)MYCON_CAPTAIN_MASK_PMAP_ANIM,
			(FRAME)0,
			(FRAME)0,
			(FRAME)0,
			(FRAME)0,
		},
		(SOUND)MYCON_VICTORY_SONG,
		(SOUND)MYCON_SHIP_SOUNDS,
	},
	{
		0,
		DISPLAY_TO_WORLD (800),
		NULL_PTR,
	},
	(UNINIT_FUNC *) NULL,
	(PREPROCESS_FUNC *) NULL,
	(POSTPROCESS_FUNC *) NULL,
	(INIT_WEAPON_FUNC *) NULL,
	0,
};


#define MISSILE_DAMAGE 10

#define TRACK_WAIT 1

static void
plasma_preprocess (PELEMENT ElementPtr)
{
	COUNT plasma_index;

	if (ElementPtr->mass_points > ElementPtr->hit_points)
		ElementPtr->life_span = ElementPtr->hit_points * PLASMA_DURATION;
	else
		ElementPtr->hit_points = (BYTE)((ElementPtr->life_span *
				MISSILE_DAMAGE + (MISSILE_LIFE - 1)) / MISSILE_LIFE);
	ElementPtr->mass_points = ElementPtr->hit_points;
	plasma_index = NUM_PLASMAS - ((ElementPtr->life_span +
			(PLASMA_DURATION - 1)) / PLASMA_DURATION);
	if (plasma_index != GetFrameIndex (ElementPtr->next.image.frame))
	{
		ElementPtr->next.image.frame =
				SetAbsFrameIndex (ElementPtr->next.image.frame,
				plasma_index);
		ElementPtr->state_flags |= CHANGING;
	}

	if (ElementPtr->turn_wait > 0)
		--ElementPtr->turn_wait;
	else
	{
		COUNT facing;

		facing = NORMALIZE_FACING (ANGLE_TO_FACING (
				GetVelocityTravelAngle (&ElementPtr->velocity)
				));
		if (TrackShip (ElementPtr, &facing) > 0)
			SetVelocityVector (&ElementPtr->velocity,
					MISSILE_SPEED, facing);

		ElementPtr->turn_wait = TRACK_WAIT;
	}
}

static void
plasma_held_preprocess (PELEMENT ElementPtr)
{
	STARSHIPPTR StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	if (StarShipPtr->cur_status_flags & WEAPON)
		++ElementPtr->life_span; /* keep it going while key pressed */
	else
	{
		ElementPtr->preprocess_func = plasma_preprocess;
	}
}


static void
plasma_blast_preprocess (PELEMENT ElementPtr)
{
	if (ElementPtr->life_span >= ElementPtr->thrust_wait)
		ElementPtr->next.image.frame =
				IncFrameIndex (ElementPtr->next.image.frame);
	else
		ElementPtr->next.image.frame =
				DecFrameIndex (ElementPtr->next.image.frame);
	if (ElementPtr->hTarget)
	{
		ELEMENTPTR ShipPtr;

		LockElement (ElementPtr->hTarget, &ShipPtr);
		ElementPtr->next.location = ShipPtr->next.location;
		UnlockElement (ElementPtr->hTarget);
	}

	ElementPtr->state_flags |= CHANGING;
}

static void
plasma_collision (PELEMENT ElementPtr0, PPOINT pPt0, PELEMENT ElementPtr1, PPOINT pPt1)
{
	SIZE old_mass;
	HELEMENT hBlastElement;

	old_mass = (SIZE)ElementPtr0->mass_points;
	if ((ElementPtr0->pParent != ElementPtr1->pParent
			|| (ElementPtr1->state_flags & PLAYER_SHIP))
			&& (hBlastElement =
			weapon_collision (ElementPtr0, pPt0, ElementPtr1, pPt1)))
	{
		SIZE num_animations;
		ELEMENTPTR BlastElementPtr;

		LockElement (hBlastElement, &BlastElementPtr);
		BlastElementPtr->pParent = ElementPtr0->pParent;
		if (!(ElementPtr1->state_flags & PLAYER_SHIP))
			BlastElementPtr->hTarget = 0;
		else
		{
			STARSHIPPTR StarShipPtr;

			GetElementStarShip (ElementPtr1, &StarShipPtr);
			BlastElementPtr->hTarget = StarShipPtr->hShip;
		}

		BlastElementPtr->current.location = ElementPtr1->current.location;

		if ((num_animations =
				(old_mass * NUM_GLOBALLS +
				(MISSILE_DAMAGE - 1)) / MISSILE_DAMAGE) == 0)
			num_animations = 1;

		BlastElementPtr->thrust_wait = (BYTE)num_animations;
		BlastElementPtr->life_span = (num_animations << 1) - 1;
		{
			BlastElementPtr->preprocess_func = plasma_blast_preprocess;
		}
		BlastElementPtr->current.image.farray = ElementPtr0->next.image.farray;
		BlastElementPtr->current.image.frame =
				SetAbsFrameIndex (BlastElementPtr->current.image.farray[0],
				NUM_PLASMAS);

		UnlockElement (hBlastElement);
	}
}

static void
mycon_intelligence (PELEMENT ShipPtr, PEVALUATE_DESC ObjectsOfConcern, COUNT ConcernCounter)
{
	STARSHIPPTR StarShipPtr;
	PEVALUATE_DESC lpEvalDesc;

	lpEvalDesc = &ObjectsOfConcern[ENEMY_WEAPON_INDEX];
	if (lpEvalDesc->ObjectPtr && lpEvalDesc->MoveState == ENTICE)
	{
		if ((lpEvalDesc->ObjectPtr->state_flags & FINITE_LIFE)
				&& !(lpEvalDesc->ObjectPtr->state_flags & CREW_OBJECT))
			lpEvalDesc->MoveState = AVOID;
		else
			lpEvalDesc->MoveState = PURSUE;
	}

	ship_intelligence (ShipPtr, ObjectsOfConcern, ConcernCounter);

	GetElementStarShip (ShipPtr, &StarShipPtr);
	if (ObjectsOfConcern[ENEMY_WEAPON_INDEX].MoveState == PURSUE)
		StarShipPtr->ship_input_state &= ~THRUST; /* don't pursue seekers */

	lpEvalDesc = &ObjectsOfConcern[ENEMY_SHIP_INDEX];
	if (StarShipPtr->weapon_counter == 0
			&& lpEvalDesc->ObjectPtr
			&& (lpEvalDesc->which_turn <= 16
			|| ShipPtr->crew_level == StarShipPtr->RaceDescPtr->ship_info.max_crew))
	{
		COUNT travel_facing, direction_facing;
		SIZE delta_x, delta_y;

		travel_facing = NORMALIZE_FACING (
				ANGLE_TO_FACING (GetVelocityTravelAngle (&ShipPtr->velocity)
				+ HALF_CIRCLE)
				);
		delta_x = lpEvalDesc->ObjectPtr->current.location.x
				- ShipPtr->current.location.x;
		delta_y = lpEvalDesc->ObjectPtr->current.location.y
				- ShipPtr->current.location.y;
		direction_facing = NORMALIZE_FACING (
				ANGLE_TO_FACING (ARCTAN (delta_x, delta_y))
				);

		if (NORMALIZE_FACING (direction_facing
				- StarShipPtr->ShipFacing
				+ ANGLE_TO_FACING (QUADRANT))
				<= ANGLE_TO_FACING (HALF_CIRCLE)
				&& (!(StarShipPtr->cur_status_flags &
				(SHIP_BEYOND_MAX_SPEED | SHIP_IN_GRAVITY_WELL))
				|| NORMALIZE_FACING (direction_facing
				- travel_facing + ANGLE_TO_FACING (OCTANT))
				<= ANGLE_TO_FACING (QUADRANT)))
			StarShipPtr->ship_input_state |= WEAPON;
	}

	if (StarShipPtr->special_counter == 0)
	{
		StarShipPtr->ship_input_state &= ~SPECIAL;
		StarShipPtr->RaceDescPtr->cyborg_control.WeaponRange = DISPLAY_TO_WORLD (800);
		if (ShipPtr->crew_level < StarShipPtr->RaceDescPtr->ship_info.max_crew)
		{
			StarShipPtr->RaceDescPtr->cyborg_control.WeaponRange = MISSILE_SPEED * MISSILE_LIFE;
			if (StarShipPtr->RaceDescPtr->ship_info.energy_level >= SPECIAL_ENERGY_COST
					&& !(StarShipPtr->ship_input_state & WEAPON))
				StarShipPtr->ship_input_state |= SPECIAL;
		}
	}
}

static COUNT
initialize_plasma (PELEMENT ShipPtr, HELEMENT PlasmaArray[])
{
#define MYCON_OFFSET 24
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
			| PERSISTENT;
	MissileBlock.pixoffs = MYCON_OFFSET;
	MissileBlock.speed = MISSILE_HELD_SPEED;
	MissileBlock.hit_points = MISSILE_DAMAGE;
	MissileBlock.damage = MISSILE_DAMAGE;
	MissileBlock.life = MISSILE_LIFE;
	MissileBlock.preprocess_func = plasma_held_preprocess;
	MissileBlock.blast_offs = MISSILE_OFFSET;
	PlasmaArray[0] = initialize_missile (&MissileBlock);

	if (PlasmaArray[0])
	{
		ELEMENTPTR PlasmaPtr;

		LockElement (PlasmaArray[0], &PlasmaPtr);
		PlasmaPtr->collision_func = plasma_collision;
		PlasmaPtr->turn_wait = TRACK_WAIT + 2;
		UnlockElement (PlasmaArray[0]);
	}

	return (1);
}

#define REGEN_WAIT 15
#define REGEN_ENERGY_COST 1

static void
mycon_postprocess (PELEMENT ElementPtr)
{
	HELEMENT hShip, hNextShip;
	ELEMENTPTR Enemy;
	STARSHIPPTR StarShipPtr, EnemyStarShip;

	GetElementStarShip (ElementPtr, &StarShipPtr);

	for (hShip = GetHeadElement (); hShip != 0; hShip = hNextShip)
	{
		LockElement (hShip, &Enemy);
		hNextShip = GetSuccElement (Enemy);
		if (Enemy->state_flags & PLAYER_SHIP
			&& (Enemy->state_flags & (GOOD_GUY | BAD_GUY)) !=
			(ElementPtr->state_flags & (GOOD_GUY | BAD_GUY)))
		{
			GetElementStarShip (Enemy, &EnemyStarShip);
			if(EnemyStarShip->RaceResIndex == StarShipPtr->RaceResIndex)
			{
				/*ElementPtr->state_flags |= NONSOLID;
				ElementPtr->life_span = 0;*/
				do_damage(ElementPtr, 50);
				ElementPtr->life_span = 0;
				ElementPtr->crew_level = 0;
				ElementPtr->state_flags |= COLLISION | NONSOLID;
				//if you're in a mirror match, DIE!
			}
		}
		UnlockElement(hShip);
	}

	if ((StarShipPtr->cur_status_flags & SPECIAL) && ElementPtr->turn_wait < TURN_WAIT)
	{
		COUNT cost;
		
		cost = (ElementPtr->turn_wait + 4) / 5;

		if(DeltaEnergy (ElementPtr, -cost))
		{
			ElementPtr->turn_wait = 0;
		}
	}


	if (/*(StarShipPtr->cur_status_flags & SPECIAL)
			&&*/ StarShipPtr->special_counter == 0
			&& ElementPtr->crew_level != StarShipPtr->RaceDescPtr->ship_info.max_crew
			&& DeltaEnergy (ElementPtr, -REGEN_ENERGY_COST))
	{
#define REGENERATION_AMOUNT 2
		SIZE add_crew;

		ProcessSound (SetAbsSoundIndex (
						/* GROW_NEW_CREW */
				StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1), ElementPtr);
		if ((add_crew = REGENERATION_AMOUNT) >
				StarShipPtr->RaceDescPtr->ship_info.max_crew - ElementPtr->crew_level)
			add_crew = StarShipPtr->RaceDescPtr->ship_info.max_crew - ElementPtr->crew_level;
		DeltaCrew (ElementPtr, add_crew);
		
		StarShipPtr->special_counter = REGEN_WAIT;
				//StarShipPtr->RaceDescPtr->characteristics.special_wait;
	}
}

static void
mycon_preprocess (PELEMENT ElementPtr)
{
	STARSHIPPTR StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	if (StarShipPtr->weapon_counter == 0
			&& (StarShipPtr->cur_status_flags
			& StarShipPtr->old_status_flags & WEAPON))
		++StarShipPtr->weapon_counter;

	ZeroVelocityComponents(&ElementPtr->velocity);
}

RACE_DESCPTR
init_mycon (void)
{
	RACE_DESCPTR RaceDescPtr;

	mycon_desc.preprocess_func = mycon_preprocess;
	mycon_desc.postprocess_func = mycon_postprocess;
	mycon_desc.init_weapon_func = initialize_plasma;
	mycon_desc.cyborg_control.intelligence_func =
			(void (*) (PVOID ShipPtr, PVOID ObjectsOfConcern, COUNT
					ConcernCounter)) mycon_intelligence;

	RaceDescPtr = &mycon_desc;

	return (RaceDescPtr);
}

