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
#include "arilou.h"
#include "resinst.h"

#include "libs/mathlib.h"

// Core characteristics
#define MAX_CREW 16
#define MAX_ENERGY 20
#define ENERGY_REGENERATION 1
#define ENERGY_WAIT 6
#define MAX_THRUST 50
#define THRUST_INCREMENT 10
#define THRUST_WAIT 0 
#define TURN_WAIT 1
#define SHIP_MASS 1

// Speed bolts
#define WEAPON_ENERGY_COST 1
#define WEAPON_WAIT 1
#define ARILOU_OFFSET 9
#define MISSILE_SPEED DISPLAY_TO_WORLD (70)
#define MISSILE_HITS 1
#define MISSILE_LIFE 7
#define MISSILE_DAMAGE 0
#define MISSILE_OFFSET 0

// Teleporter
#define SPECIAL_ENERGY_COST 3
#define SPECIAL_WAIT 1 /* This now seems to be thrown out so special_counter
                        * can be abused to control the alternating speedbolt
                        * things.
                        */
#define HYPER_LIFE 5

static RACE_DESC arilou_desc =
{
	{ /* SHIP_INFO */
		"skiff",
		/* FIRES_FORE | */ IMMEDIATE_WEAPON,
		28, /* Super Melee cost */
		MAX_CREW, MAX_CREW,
		MAX_ENERGY, MAX_ENERGY,
		ARILOU_RACE_STRINGS,
		ARILOU_ICON_MASK_PMAP_ANIM,
		ARILOU_MICON_MASK_PMAP_ANIM,
		NULL, NULL, NULL
	},
	{ /* FLEET_STUFF */
		250 / SPHERE_RADIUS_INCREMENT * 2, /* Initial SoI radius */
		{ /* Known location (center of SoI) */
			438, 6372,
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
			ARILOU_BIG_MASK_PMAP_ANIM,
			ARILOU_MED_MASK_PMAP_ANIM,
			ARILOU_SML_MASK_PMAP_ANIM,
		},
		{
			NULL_RESOURCE,
			NULL_RESOURCE,
			NULL_RESOURCE,
		},
		{
			WARP_BIG_MASK_PMAP_ANIM,
			WARP_MED_MASK_PMAP_ANIM,
			WARP_SML_MASK_PMAP_ANIM,
		},
		{
			ARILOU_CAPTAIN_MASK_PMAP_ANIM,
			NULL, NULL, NULL, NULL, NULL
		},
		ARILOU_VICTORY_SONG,
		ARILOU_SHIP_SOUNDS,
		{ NULL, NULL, NULL },
		{ NULL, NULL, NULL },
		{ NULL, NULL, NULL },
		NULL, NULL
	},
	{
		0,
		100,
		NULL,
	},
	(UNINIT_FUNC *) NULL,
	(PREPROCESS_FUNC *) NULL,
	(POSTPROCESS_FUNC *) NULL,
	(INIT_WEAPON_FUNC *) NULL,
	0,
	0, /* CodeRef */
};

static void
speedlaser_collision (ELEMENT *ElementPtr0, POINT *pPt0, ELEMENT *
		ElementPtr1, POINT *pPt1)
{
	if (ElementPtr1->state_flags & PLAYER_SHIP)
	{
		STARSHIP *StarShipPtr;
		RACE_DESC *RDPtr;
		COUNT num_thrusts;
		SIZE dx, dy;

		GetElementStarShip (ElementPtr1, &StarShipPtr);
		RDPtr = StarShipPtr->RaceDescPtr;

		if(RDPtr->characteristics.thrust_increment > 0)
		{
			num_thrusts = RDPtr->characteristics.max_thrust /
						RDPtr->characteristics.thrust_increment;

			RDPtr->characteristics.thrust_increment += (RDPtr->characteristics.thrust_increment / 3) + 1;

			RDPtr->characteristics.max_thrust =
					RDPtr->characteristics.thrust_increment * num_thrusts;
		}

		GetCurrentVelocityComponents (&ElementPtr0->velocity, &dx, &dy);
		DeltaVelocityComponents (&ElementPtr1->velocity, dx / 20, dy / 20);

		ElementPtr0->mass_points = 0;
	}

	ElementPtr0->hit_points = 0;
	weapon_collision(ElementPtr0, pPt0, ElementPtr1, pPt1);

	(void) pPt0;  /* Satisfying compiler (unused parameter) */
	(void) pPt1;  /* Satisfying compiler (unused parameter) */
}

static COUNT initialize_speedlaser (ELEMENT *ShipPtr, HELEMENT
		LaserArray[]);

static void
speedlaser_death (ELEMENT *ElementPtr)
{
	if(ElementPtr->hit_points && ElementPtr->turn_wait && !(ElementPtr->state_flags & COLLISION))
	{
		HELEMENT Laser;

		initialize_speedlaser (ElementPtr, &Laser);
		if (Laser)
			PutElement (Laser);
	}
}

/* TODO: this function is quite hairy, it also seems to abuse special_counter.
 * Rework it sometime.
 */
static COUNT
initialize_speedlaser (ELEMENT *ShipPtr, HELEMENT LaserArray[])
{
	BOOLEAN isFirstSegment;
	STARSHIP *StarShipPtr;
	LASER_BLOCK MissileBlock;
	SIZE sideways_offs;

	isFirstSegment = (ShipPtr->state_flags & PLAYER_SHIP);
	GetElementStarShip (ShipPtr, &StarShipPtr);
	MissileBlock.face = isFirstSegment ? StarShipPtr->ShipFacing : ANGLE_TO_FACING (GetVelocityTravelAngle(&ShipPtr->velocity));
	sideways_offs = isFirstSegment ? (StarShipPtr->special_counter ? -50 : 50) : 0;
	MissileBlock.cx = ShipPtr->next.location.x + COSINE(FACING_TO_ANGLE (MissileBlock.face + 4), sideways_offs);
	MissileBlock.cy = ShipPtr->next.location.y + SINE(FACING_TO_ANGLE (MissileBlock.face + 4), sideways_offs);
	MissileBlock.ex = COSINE(FACING_TO_ANGLE (MissileBlock.face), MISSILE_SPEED);
	MissileBlock.ey = SINE(FACING_TO_ANGLE (MissileBlock.face), MISSILE_SPEED);
	MissileBlock.sender = ShipPtr->playerNr;
	MissileBlock.flags = IGNORE_SIMILAR;
	MissileBlock.pixoffs = isFirstSegment ? ARILOU_OFFSET : 0;
	MissileBlock.color = BUILD_COLOR (MAKE_RGB15 (0x08, 0x1F, 0x06), 0x0A);
	LaserArray[0] = initialize_laser (&MissileBlock);

	if (LaserArray[0])
	{
		ELEMENT *LaserPtr;

		LockElement (LaserArray[0], &LaserPtr);
		LaserPtr->collision_func = speedlaser_collision;
		LaserPtr->death_func = speedlaser_death;
		LaserPtr->turn_wait = (ShipPtr->state_flags & PLAYER_SHIP) ? 8 : ShipPtr->turn_wait - 1;

		UnlockElement (LaserArray[0]);
	}
	return (1);
}

static void
arilou_intelligence (ELEMENT *ShipPtr, EVALUATE_DESC *ObjectsOfConcern,
		COUNT ConcernCounter)
{
	STARSHIP *StarShipPtr;

	GetElementStarShip (ShipPtr, &StarShipPtr);

	ship_intelligence (ShipPtr, ObjectsOfConcern, ConcernCounter);
}

static void
arilou_preprocess (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	
	if(StarShipPtr->weapon_counter)
	{
		if(StarShipPtr->special_counter)
			StarShipPtr->special_counter = 0;
		else StarShipPtr->special_counter = WEAPON_WAIT + ENERGY_WAIT + 1;
	}

	if (!(ElementPtr->state_flags & NONSOLID))
	{
		if ((StarShipPtr->cur_status_flags & SPECIAL)
				&& CleanDeltaEnergy (ElementPtr, -SPECIAL_ENERGY_COST))
		{
			/* Special key is pressed; start teleport */
			ZeroVelocityComponents (&ElementPtr->velocity);
			StarShipPtr->cur_status_flags &=
					~(SHIP_AT_MAX_SPEED | LEFT | RIGHT | THRUST | WEAPON);

			ElementPtr->state_flags |= NONSOLID | FINITE_LIFE | CHANGING;
			ElementPtr->life_span = HYPER_LIFE;

			ElementPtr->next.image.farray =
					StarShipPtr->RaceDescPtr->ship_data.special;
			ElementPtr->next.image.frame =
					StarShipPtr->RaceDescPtr->ship_data.special[0];

			ProcessSound (SetAbsSoundIndex (
							/* HYPERJUMP */
					StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1), ElementPtr);
		}
	}
	else if (ElementPtr->next.image.farray == StarShipPtr->RaceDescPtr->ship_data.special)
	{
		COUNT life_span;

		ZeroVelocityComponents(&ElementPtr->velocity); //just in case

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
			/* Ending teleport */
			ElementPtr->state_flags &= ~(NONSOLID | FINITE_LIFE);
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
			/* Teleporting in progress */
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
				POINT real_current_location;
				real_current_location = ElementPtr->current.location;
				do
				{
					ElementPtr->next.location.x = ElementPtr->current.location.x =
							WRAP_X (DISPLAY_ALIGN_X (TFB_Random ()));
					ElementPtr->next.location.y = ElementPtr->current.location.y =
							WRAP_Y (DISPLAY_ALIGN_Y (TFB_Random ()));
				}
				while(TimeSpaceMatterConflict(ElementPtr));
				ElementPtr->current.location = real_current_location;
			}
		}

		ElementPtr->state_flags |= CHANGING;
	}
}

RACE_DESC*
init_arilou (void)
{
	RACE_DESC *RaceDescPtr;

	arilou_desc.preprocess_func = arilou_preprocess;
	arilou_desc.init_weapon_func = initialize_speedlaser;
	arilou_desc.cyborg_control.intelligence_func = arilou_intelligence;

	RaceDescPtr = &arilou_desc;

	return (RaceDescPtr);
}

