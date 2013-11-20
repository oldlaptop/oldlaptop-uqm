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

#include "../ship.h"
#include "slylandr.h"
#include "resinst.h"
#include "../human/resinst.h"

#include "uqm/globdata.h"
#include "uqm/element.h"
#include "libs/mathlib.h"

// Core characteristics
#define MAX_CREW 12
#define MAX_ENERGY 30
#define ENERGY_REGENERATION 1
#define ENERGY_WAIT 2
#define MAX_THRUST 75
#define THRUST_INCREMENT 12
#define THRUST_WAIT 1
#define TURN_WAIT 1
#define SHIP_MASS 3

// Missiles
#define WEAPON_ENERGY_COST 7
#define WEAPON_WAIT 17
#define MISSILE_DAMAGE 4
#define MISSILE_HITS 1
#define MISSILE_SPEED DISPLAY_TO_WORLD (20)
#define MISSILE_LIFE 18
#define NUM_MISSILES 8
#define MISSILE_SPACING 42
#define NUKE_OFFSET 8
#define SLYLANDRO_OFFSET 42

// Asteroid spitter
#define ASTEROID_VELOCITY WORLD_TO_VELOCITY (DISPLAY_TO_WORLD (4))
#define MAX_ASTEROID_VELOCITY WORLD_TO_VELOCITY (DISPLAY_TO_WORLD (28))
#define SPECIAL_ENERGY_COST 2
#define SPECIAL_WAIT 2

static RACE_DESC slylandro_desc =
{
	{ /* SHIP_INFO */
		"probe",
		SEEKING_WEAPON | CREW_IMMUNE,
		27, /* Super Melee cost */
		MAX_CREW, MAX_CREW,
		MAX_ENERGY, MAX_ENERGY,
		SLYLANDRO_RACE_STRINGS,
		SLYLANDRO_ICON_MASK_PMAP_ANIM,
		SLYLANDRO_MICON_MASK_PMAP_ANIM,
		NULL, NULL, NULL
	},
	{ /* FLEET_STUFF */
		INFINITE_RADIUS, /* Initial sphere of influence radius */
		{ /* Known location (center of SoI) */
			333, 9812,
		},
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
			SLYLANDRO_BIG_MASK_PMAP_ANIM,
			SLYLANDRO_MED_MASK_PMAP_ANIM,
			SLYLANDRO_SML_MASK_PMAP_ANIM,
		},
		{
			NULL_RESOURCE,
			NULL_RESOURCE,
			NULL_RESOURCE,
		},
		{
			NULL_RESOURCE,
			NULL_RESOURCE,
			NULL_RESOURCE,
		},
		{
			SLYLANDRO_CAPTAIN_MASK_PMAP_ANIM,
			NULL, NULL, NULL, NULL, NULL
		},
		SLYLANDRO_VICTORY_SONG,
		SLYLANDRO_SHIP_SOUNDS,
		{ NULL, NULL, NULL },
		{ NULL, NULL, NULL },
		{ NULL, NULL, NULL },
		NULL, NULL
	},
	{
		0,
		CLOSE_RANGE_WEAPON << 1,
		NULL,
	},
	(UNINIT_FUNC *) NULL,
	(PREPROCESS_FUNC *) NULL,
	(POSTPROCESS_FUNC *) NULL,
	(INIT_WEAPON_FUNC *) NULL,
	0,
	0, /* CodeRef */
};

static COUNT
initialize_nukes (ELEMENT *ShipPtr, HELEMENT MissileArray[])
{
	COUNT i;
	STARSHIP *StarShipPtr;
	MISSILE_BLOCK MissileBlock;

	GetElementStarShip (ShipPtr, &StarShipPtr);
	MissileBlock.cx = ShipPtr->next.location.x;
	MissileBlock.cy = ShipPtr->next.location.y;
	MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.weapon;
	MissileBlock.face = MissileBlock.index = StarShipPtr->ShipFacing;
	MissileBlock.sender = ShipPtr->playerNr;
	MissileBlock.flags = IGNORE_SIMILAR;
	MissileBlock.pixoffs = SLYLANDRO_OFFSET;
	MissileBlock.speed = MISSILE_SPEED;
	MissileBlock.hit_points = MISSILE_HITS;
	MissileBlock.damage = MISSILE_DAMAGE;
	MissileBlock.life = MISSILE_LIFE;
	MissileBlock.preprocess_func = NULL;
	MissileBlock.blast_offs = NUKE_OFFSET;

	for(i = 0; i < NUM_MISSILES; ++i)
	{
		MissileBlock.cx =
				ShipPtr->next.location.x +
				COSINE (FACING_TO_ANGLE (MissileBlock.face + 4),
				(i * 2 + 1 - NUM_MISSILES) * MISSILE_SPACING / 2);
		MissileBlock.cy =
				ShipPtr->next.location.y +
				SINE (FACING_TO_ANGLE (MissileBlock.face + 4),
				(i * 2 + 1 - NUM_MISSILES) * MISSILE_SPACING / 2);
		MissileArray[i] = initialize_missile (&MissileBlock);
	}

	return (NUM_MISSILES);
}

//BEGIN copied code from misc.c

extern FRAME asteroid[];

/* spawn_slylandro_rubble is spawn_rubble from misc.c, minus setting the
 * death_func to spawn_asteroid.
 */
static void
spawn_slylandro_rubble (ELEMENT *AsteroidElementPtr)
{
	HELEMENT hRubbleElement;

	SIZE dx, dy;
	GetCurrentVelocityComponents(&AsteroidElementPtr->velocity, &dx, &dy);

	hRubbleElement = AllocElement ();
	if (hRubbleElement)
	{
		ELEMENT *RubbleElementPtr;

		PutElement (hRubbleElement);
		LockElement (hRubbleElement, &RubbleElementPtr);
		RubbleElementPtr->playerNr = AsteroidElementPtr->playerNr;
		RubbleElementPtr->state_flags = APPEARING | FINITE_LIFE | NONSOLID;
		RubbleElementPtr->life_span = 5;
		RubbleElementPtr->turn_wait = RubbleElementPtr->next_turn = 0;
		SetPrimType (&DisplayArray[RubbleElementPtr->PrimIndex], STAMP_PRIM);
		RubbleElementPtr->current.image.farray = asteroid;
		RubbleElementPtr->current.image.frame =
				SetAbsFrameIndex (asteroid[0], ANGLE_TO_FACING (FULL_CIRCLE));
		RubbleElementPtr->current.location = AsteroidElementPtr->current.location;
		RubbleElementPtr->preprocess_func = animation_preprocess;

		SetVelocityComponents(&RubbleElementPtr->velocity, dx, dy);

		UnlockElement (hRubbleElement);
	}
}

/* spawn_slylandro_asteroid is spawn_asteroid from misc.c, with a few changes
 * to position the new asteroid at the Slylando ship rather than a random
 * location.
 */
static void
spawn_slylandro_asteroid (ELEMENT *ElementPtr)
{
	HELEMENT hAsteroidElement;
	STARSHIP * StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);

	if ((hAsteroidElement = AllocElement ()) == 0)
	{
		if (ElementPtr != 0)
		{
			ElementPtr->state_flags &= ~DISAPPEARING;
			SetPrimType (&DisplayArray[ElementPtr->PrimIndex], NO_PRIM);
			ElementPtr->life_span = 1;
		}
	}
	else
	{
		ELEMENT *AsteroidElementPtr;
		COUNT offset;

		LockElement (hAsteroidElement, &AsteroidElementPtr);
		AsteroidElementPtr->playerNr = NEUTRAL_PLAYER_NUM;
		AsteroidElementPtr->hit_points = 1;
		AsteroidElementPtr->mass_points = 3;
		AsteroidElementPtr->state_flags = APPEARING;
		AsteroidElementPtr->life_span = NORMAL_LIFE;
		SetPrimType (&DisplayArray[AsteroidElementPtr->PrimIndex], STAMP_PRIM);
		{
			// Using these temporary variables because the execution order
			// of function arguments may vary per system, which may break
			// synchronisation on network games.
			SIZE magnitude =
					DISPLAY_TO_WORLD (((SIZE)TFB_Random () & 7) + 4);
			COUNT facing = (COUNT)TFB_Random ();
			SetVelocityVector (&AsteroidElementPtr->velocity, magnitude,
					facing);
		}
		AsteroidElementPtr->current.image.farray = asteroid;
		AsteroidElementPtr->current.image.frame =
				SetAbsFrameIndex (asteroid[0],
				NORMALIZE_FACING (TFB_Random ()));
		AsteroidElementPtr->turn_wait =
				AsteroidElementPtr->thrust_wait =
				(BYTE)TFB_Random () & (BYTE)((1 << 2) - 1);
		AsteroidElementPtr->thrust_wait |=
				(BYTE)TFB_Random () & (BYTE)(1 << 7);
		AsteroidElementPtr->preprocess_func = asteroid_preprocess;
		AsteroidElementPtr->death_func = spawn_slylandro_rubble;
		AsteroidElementPtr->collision_func = collision;

		offset = 0;
		do
		{
			COUNT angle;
			angle = TFB_Random() & (FULL_CIRCLE - 1);
			AsteroidElementPtr->current.location.x =
					ElementPtr->current.location.x
					+ COSINE (FACING_TO_ANGLE (StarShipPtr->ShipFacing), 180)
					+ COSINE (angle, offset);
			AsteroidElementPtr->current.location.y =
					ElementPtr->current.location.y
					+ SINE (FACING_TO_ANGLE (StarShipPtr->ShipFacing), 180)
					+ SINE (angle, offset);
			offset += 10;
		}
		while(TimeSpaceMatterConflict(AsteroidElementPtr));

		UnlockElement (hAsteroidElement);

		PutElement (hAsteroidElement);
	}
}
//END copied code from misc.c

static void
slylandro_intelligence (ELEMENT *ShipPtr, EVALUATE_DESC *ObjectsOfConcern,
		COUNT ConcernCounter)
{
	EVALUATE_DESC *lpEvalDesc;
	STARSHIP *StarShipPtr;

	if (LOBYTE (GLOBAL (CurrentActivity)) == IN_ENCOUNTER)
			/* no dodging in role playing game */
		ObjectsOfConcern[ENEMY_WEAPON_INDEX].ObjectPtr = 0;

	lpEvalDesc = &ObjectsOfConcern[ENEMY_SHIP_INDEX];

	GetElementStarShip (ShipPtr, &StarShipPtr);
	if (StarShipPtr->special_counter == 0
			&& StarShipPtr->RaceDescPtr->ship_info.energy_level == 0
			&& ObjectsOfConcern[GRAVITY_MASS_INDEX].ObjectPtr == 0)
		ConcernCounter = FIRST_EMPTY_INDEX + 1;
	if (lpEvalDesc->ObjectPtr && lpEvalDesc->MoveState == PURSUE
			&& lpEvalDesc->which_turn <= 6)
		lpEvalDesc->MoveState = ENTICE;

	++ShipPtr->thrust_wait;
	ship_intelligence (ShipPtr, ObjectsOfConcern, ConcernCounter);
	--ShipPtr->thrust_wait;

	if (lpEvalDesc->ObjectPtr && lpEvalDesc->which_turn <= 14)
		StarShipPtr->ship_input_state |= WEAPON;
	else
		StarShipPtr->ship_input_state &= ~WEAPON;

	StarShipPtr->ship_input_state &= ~SPECIAL;
	if (StarShipPtr->RaceDescPtr->ship_info.energy_level <
			StarShipPtr->RaceDescPtr->ship_info.max_energy)
	{
		lpEvalDesc = &ObjectsOfConcern[FIRST_EMPTY_INDEX];
		if (lpEvalDesc->ObjectPtr && lpEvalDesc->which_turn <= 14)
			StarShipPtr->ship_input_state |= SPECIAL;
	}
}

static void
slylandro_postprocess (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);

	if((StarShipPtr->cur_status_flags & SPECIAL)
		&& StarShipPtr->special_counter == 0
		&& DeltaEnergy(ElementPtr, -SPECIAL_ENERGY_COST))
	{
		COUNT angle;
		SIZE cur_delta_x, cur_delta_y;


		StarShipPtr->special_counter = SPECIAL_WAIT;
		spawn_slylandro_asteroid(ElementPtr);

		//*********  CODE COPIED FROM druuge_postprocess  ***********
		StarShipPtr->cur_status_flags &= ~SHIP_AT_MAX_SPEED;

		angle = FACING_TO_ANGLE (StarShipPtr->ShipFacing) + HALF_CIRCLE;
		DeltaVelocityComponents (&ElementPtr->velocity,
				COSINE (angle, ASTEROID_VELOCITY),
				SINE (angle, ASTEROID_VELOCITY));
		GetCurrentVelocityComponents (&ElementPtr->velocity,
				&cur_delta_x, &cur_delta_y);
		if ((long)cur_delta_x * (long)cur_delta_x
				+ (long)cur_delta_y * (long)cur_delta_y
				> (long)MAX_ASTEROID_VELOCITY * (long)MAX_ASTEROID_VELOCITY)
		{
			angle = ARCTAN (cur_delta_x, cur_delta_y);
			SetVelocityComponents (&ElementPtr->velocity,
					COSINE (angle, MAX_ASTEROID_VELOCITY),
					SINE (angle, MAX_ASTEROID_VELOCITY));
		}
		//******************    END COPIED CODE   *******************
	}
}

static COUNT slylandroes_present;

static void
slylandro_preprocess (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);

	if (ElementPtr->state_flags & APPEARING)
	{
		if(StarShipPtr->RaceDescPtr->ship_data.weapon[0] == (FRAME)0)
		{
			load_animation (&StarShipPtr->RaceDescPtr->ship_data.weapon,
					SATURN_BIG_MASK_PMAP_ANIM,
					SATURN_MED_MASK_PMAP_ANIM,
					SATURN_SML_MASK_PMAP_ANIM);
		}
		++slylandroes_present;
	}

	if (ElementPtr->turn_wait == (StarShipPtr->RaceDescPtr->characteristics.turn_wait + 1) / 2)
	{
		ElementPtr->next.image.frame = SetAbsFrameIndex (StarShipPtr->RaceDescPtr->ship_data.ship[0],
				StarShipPtr->ShipFacing * 2);
		ElementPtr->state_flags |= CHANGING;
	}
	if (ElementPtr->turn_wait == 0)
	{
		ElementPtr->turn_wait += StarShipPtr->RaceDescPtr->characteristics.turn_wait + 1;
		if (StarShipPtr->cur_status_flags & LEFT)
		{
			--StarShipPtr->ShipFacing;
			ElementPtr->next.image.frame = DecFrameIndex (ElementPtr->next.image.frame);
			ElementPtr->state_flags |= CHANGING;
		}
		else if (StarShipPtr->cur_status_flags & RIGHT)
		{
			++StarShipPtr->ShipFacing;
			ElementPtr->next.image.frame = IncFrameIndex (ElementPtr->next.image.frame);
			ElementPtr->state_flags |= CHANGING;
		}


		StarShipPtr->ShipFacing = NORMALIZE_FACING (StarShipPtr->ShipFacing);
	}
}

static void
slylandro_dispose_graphicshack (RACE_DESC *RaceDescPtr)
{
	--slylandroes_present;

	if(!slylandroes_present)
	{
		clearGraphicsHack(RaceDescPtr->ship_data.weapon);
	}
}

RACE_DESC*
init_slylandro (void)
{
	RACE_DESC *RaceDescPtr;

	slylandro_desc.preprocess_func = slylandro_preprocess;
	slylandro_desc.postprocess_func = slylandro_postprocess;
	slylandro_desc.init_weapon_func = initialize_nukes;
	slylandro_desc.cyborg_control.intelligence_func = slylandro_intelligence;
	slylandro_desc.uninit_func = slylandro_dispose_graphicshack;

	RaceDescPtr = &slylandro_desc;

	return (RaceDescPtr);
}
