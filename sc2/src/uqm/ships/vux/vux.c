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
#include "vux.h"
#include "resinst.h"

#include "uqm/globdata.h"
#include "libs/mathlib.h"

// Core characteristics
#define MAX_CREW 20
#define MAX_ENERGY 40
#define ENERGY_REGENERATION 1
#define ENERGY_WAIT 2
#define MAX_THRUST /* DISPLAY_TO_WORLD (5) */ 21
#define THRUST_INCREMENT /* DISPLAY_TO_WORLD (2) */ 7
#define THRUST_WAIT 3
#define TURN_WAIT 4
#define SHIP_MASS 6

// Laser
#define WEAPON_ENERGY_COST 1
#define WEAPON_WAIT 0
#define VUX_OFFSET 12
#define LASER_RANGE 1023 /* Nigh-infinite in practice */

// Repulsion
#define SPECIAL_ENERGY_COST 5
#define SPECIAL_WAIT 12
#define REPULSION_LEVEL 50000L

// Distant Entry
#define MIN_ARRIVAL_DISTANCE DISPLAY_TO_WORLD (700)

static RACE_DESC vux_desc =
{
	{ /* SHIP_INFO */
		"intruder",
		FIRES_FORE | SEEKING_SPECIAL | IMMEDIATE_WEAPON,
		22, /* Super Melee cost */
		MAX_CREW, MAX_CREW,
		MAX_ENERGY, MAX_ENERGY,
		VUX_RACE_STRINGS,
		VUX_ICON_MASK_PMAP_ANIM,
		VUX_MICON_MASK_PMAP_ANIM,
		NULL, NULL, NULL
	},
	{ /* FLEET_STUFF */
		900 / SPHERE_RADIUS_INCREMENT * 2, /* Initial SoI radius */
		{ /* Known location (center of SoI) */
			4412, 1558,
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
			VUX_BIG_MASK_PMAP_ANIM,
			VUX_MED_MASK_PMAP_ANIM,
			VUX_SML_MASK_PMAP_ANIM,
		},
		{
			SLIME_MASK_PMAP_ANIM,
			NULL_RESOURCE,
			NULL_RESOURCE,
		},
		{
			LIMPETS_BIG_MASK_PMAP_ANIM,
			LIMPETS_MED_MASK_PMAP_ANIM,
			LIMPETS_SML_MASK_PMAP_ANIM,
		},
		{
			VUX_CAPTAIN_MASK_PMAP_ANIM,
			NULL, NULL, NULL, NULL, NULL
		},
		VUX_VICTORY_SONG,
		VUX_SHIP_SOUNDS,
		{ NULL, NULL, NULL },
		{ NULL, NULL, NULL },
		{ NULL, NULL, NULL },
		NULL, NULL
	},
	{
		0,
		CLOSE_RANGE_WEAPON,
		NULL,
	},
	(UNINIT_FUNC *) NULL,
	(PREPROCESS_FUNC *) NULL,
	(POSTPROCESS_FUNC *) NULL,
	(INIT_WEAPON_FUNC *) NULL,
	0,
	0, /* CodeRef */
};

static COUNT initialize_horrific_laser (ELEMENT *ElementPtr, HELEMENT
		LaserArray[]);

static void
horrific_laser_postprocess (ELEMENT *ElementPtr)
{
	if (ElementPtr->turn_wait
			&& !(ElementPtr->state_flags & COLLISION))
	{
		HELEMENT Laser;

		initialize_horrific_laser (ElementPtr, &Laser);
		if (Laser)
			PutElement (Laser);
	}
}

static void
horrific_laser_collision (ELEMENT *ElementPtr0, POINT *pPt0, ELEMENT *ElementPtr1, POINT *pPt1)
{
	ElementPtr0->turn_wait = 0;
	weapon_collision (ElementPtr0, pPt0, ElementPtr1, pPt1);
}

static COUNT
initialize_horrific_laser (ELEMENT *ShipPtr, HELEMENT LaserArray[])
{
	BOOLEAN isFirstSegment;
	STARSHIP *StarShipPtr;
	LASER_BLOCK LaserBlock;

	GetElementStarShip (ShipPtr, &StarShipPtr);
	isFirstSegment = (ShipPtr->state_flags & PLAYER_SHIP);

	LaserBlock.face = StarShipPtr->ShipFacing;
	LaserBlock.cx = ShipPtr->next.location.x;
	LaserBlock.cy = ShipPtr->next.location.y;
	LaserBlock.ex = COSINE (FACING_TO_ANGLE (LaserBlock.face), LASER_RANGE);
	LaserBlock.ey = SINE (FACING_TO_ANGLE (LaserBlock.face), LASER_RANGE);
	LaserBlock.sender = ShipPtr->playerNr;
	LaserBlock.flags = IGNORE_SIMILAR;
	LaserBlock.pixoffs = isFirstSegment ? VUX_OFFSET : 0;
	LaserBlock.color = BUILD_COLOR (MAKE_RGB15 (0x0A, 0x1F, 0x0A), 0x0A);
	LaserArray[0] = initialize_laser (&LaserBlock);

	if(LaserArray[0])
	{
		ELEMENT *LaserPtr;
		
		LockElement(LaserArray[0], &LaserPtr);

		SetElementStarShip (LaserPtr, StarShipPtr);
		LaserPtr->postprocess_func = horrific_laser_postprocess;
		LaserPtr->collision_func = horrific_laser_collision;
		if(isFirstSegment)
		{
			if ((StarShipPtr->ShipFacing / 4) * 4 == StarShipPtr->ShipFacing)
				LaserPtr->turn_wait = 8;
			else LaserPtr->turn_wait = 200;
		}
		else
		{
			LaserPtr->turn_wait = ShipPtr->turn_wait - 1;
		}

		UnlockElement(LaserArray[0]);
	}

	return (1);
}

static void
vux_intelligence (ELEMENT *ShipPtr, EVALUATE_DESC *ObjectsOfConcern,
		COUNT ConcernCounter)
{
	EVALUATE_DESC *lpEvalDesc;
	STARSHIP *StarShipPtr;

	lpEvalDesc = &ObjectsOfConcern[ENEMY_SHIP_INDEX];
	 lpEvalDesc->MoveState = PURSUE;
	if (ObjectsOfConcern[ENEMY_WEAPON_INDEX].ObjectPtr != 0
			&& ObjectsOfConcern[ENEMY_WEAPON_INDEX].MoveState == ENTICE)
	{
		if ((ObjectsOfConcern[ENEMY_WEAPON_INDEX].ObjectPtr->state_flags
				& FINITE_LIFE)
				&& !(ObjectsOfConcern[ENEMY_WEAPON_INDEX].ObjectPtr->state_flags
				& CREW_OBJECT))
			ObjectsOfConcern[ENEMY_WEAPON_INDEX].MoveState = AVOID;
		else
			ObjectsOfConcern[ENEMY_WEAPON_INDEX].MoveState = PURSUE;
	}

	ship_intelligence (ShipPtr,
			ObjectsOfConcern, ConcernCounter);

	GetElementStarShip (ShipPtr, &StarShipPtr);
	if (StarShipPtr->special_counter == 0
			&& lpEvalDesc->ObjectPtr != 0
			&& lpEvalDesc->which_turn <= 12
			&& (StarShipPtr->ship_input_state & (LEFT | RIGHT))
			&& StarShipPtr->RaceDescPtr->ship_info.energy_level >=
			(BYTE)(StarShipPtr->RaceDescPtr->ship_info.max_energy >> 1))
		StarShipPtr->ship_input_state |= SPECIAL;
	else
		StarShipPtr->ship_input_state &= ~SPECIAL;
}

//because everyone knows VUX are REPULSIVE! HA HA HA HA HA! - EP
static void
repel (ELEMENT *ElementPtr)
{
	if (ElementPtr->state_flags & PLAYER_SHIP)
	{
		HELEMENT hRepulsion;
		hRepulsion = AllocElement ();
		if (hRepulsion)
		{
			ELEMENT *RePtr;
			STARSHIP *StarShipPtr;
			
			LockElement (hRepulsion, &RePtr);
			RePtr->playerNr = ElementPtr->playerNr;
			RePtr->state_flags =
					(FINITE_LIFE | NONSOLID | IGNORE_SIMILAR 
					| APPEARING);
			RePtr->life_span = 2;
			RePtr->preprocess_func = repel;
			
			GetElementStarShip (ElementPtr, &StarShipPtr);
			SetElementStarShip (RePtr, StarShipPtr);
			
			SetPrimType (&(GLOBAL (DisplayArray))[
					RePtr->PrimIndex
					], NO_PRIM);
					
			UnlockElement (hRepulsion);
			PutElement (hRepulsion);
		}
	}
	else
	{
		ELEMENT *ShipElementPtr;
		STARSHIP *StarShipPtr;
		ELEMENT *EnemyElementPtr;
		HELEMENT hShip, hNextShip;

		GetElementStarShip (ElementPtr, &StarShipPtr);
		LockElement (StarShipPtr->hShip, &ShipElementPtr);
	
		for (hShip = GetHeadElement (); hShip != 0; hShip = hNextShip)
		{
			LockElement (hShip, &EnemyElementPtr);
			hNextShip = GetSuccElement (EnemyElementPtr);
			if (
				!GRAVITY_MASS(EnemyElementPtr->mass_points)
				&&
				CollidingElement(EnemyElementPtr)
				&&
				!(EnemyElementPtr->state_flags & PLAYER_SHIP)
			)
			{
				SIZE delta_x, delta_y;
				SIZE magnitude;
				delta_x = EnemyElementPtr->current.location.x -
						ShipElementPtr->current.location.x;
				delta_y = EnemyElementPtr->current.location.y -
						ShipElementPtr->current.location.y;
				delta_x = WRAP_DELTA_X (delta_x);
				delta_y = WRAP_DELTA_Y (delta_y);

				magnitude = square_root (((long)delta_x * delta_x) + ((long)delta_y * delta_y));

				EnemyElementPtr->next.location.x += delta_x * REPULSION_LEVEL
						/ magnitude / (magnitude + 300);
				EnemyElementPtr->next.location.y += delta_y * REPULSION_LEVEL
						/ magnitude / (magnitude + 300);
			}
			UnlockElement (hShip);
		}
		UnlockElement (StarShipPtr->hShip);
	}
}

static void
vux_preprocess (ELEMENT *ElementPtr)
{
	PRIMITIVE * lpPrim;
	STARSHIP *StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);

	if ((StarShipPtr->cur_status_flags & SPECIAL)
			&& StarShipPtr->special_counter == 0
			&& DeltaEnergy (ElementPtr, -SPECIAL_ENERGY_COST))
	{
		StarShipPtr->special_counter = 
				StarShipPtr->RaceDescPtr->characteristics.special_wait;
	}

	lpPrim = &(GLOBAL (DisplayArray))[ElementPtr->PrimIndex];
	if(StarShipPtr->special_counter)
	{	
		repel(ElementPtr);
		SetPrimColor (lpPrim, BUILD_COLOR (MAKE_RGB15 (TFB_Random()&7, TFB_Random()&31, TFB_Random()&7), 0));
		SetPrimType (lpPrim, STAMPFILL_PRIM);
	}
	else
	{
		SetPrimType (lpPrim, STAMP_PRIM);
	}
}

static void
vux_arrival_preprocess (ELEMENT *ElementPtr)
{
	if (ElementPtr->state_flags & APPEARING)
	{
		COUNT facing;
		STARSHIP *StarShipPtr;

		GetElementStarShip (ElementPtr, &StarShipPtr);
		facing = StarShipPtr->ShipFacing;
		if (LOBYTE (GLOBAL (CurrentActivity)) != IN_ENCOUNTER
				&& TrackShip (ElementPtr, &facing) >= 0)
		{
			SIZE distance;
			ELEMENT *OtherShipPtr;

			LockElement (ElementPtr->hTarget, &OtherShipPtr);

			do
			{
				SIZE dx, dy;

				ElementPtr->current.location.x =
						WRAP_X (DISPLAY_ALIGN_X (TFB_Random ()));
				ElementPtr->current.location.y =
						WRAP_Y (DISPLAY_ALIGN_Y (TFB_Random ()));

				dx = WRAP_DELTA_X(OtherShipPtr->current.location.x -
						ElementPtr->current.location.x);
				dy = WRAP_DELTA_Y(OtherShipPtr->current.location.y -
						ElementPtr->current.location.y);
				distance = square_root (dx*dx + dy*dy);

				facing = NORMALIZE_FACING (
						ANGLE_TO_FACING (ARCTAN (-dx, -dy))
						);
				ElementPtr->current.image.frame =
						SetAbsFrameIndex (ElementPtr->current.image.frame,
						facing);
			} while (CalculateGravity (ElementPtr)
					|| TimeSpaceMatterConflict (ElementPtr)
					|| (distance < MIN_ARRIVAL_DISTANCE));

			UnlockElement (ElementPtr->hTarget);
			ElementPtr->hTarget = 0;

			ElementPtr->next = ElementPtr->current;
			InitIntersectStartPoint (ElementPtr);
			InitIntersectEndPoint (ElementPtr);
			InitIntersectFrame (ElementPtr);

			StarShipPtr->ShipFacing = facing;
		}

		StarShipPtr->RaceDescPtr->preprocess_func = vux_preprocess;
	}
}

RACE_DESC*
init_vux (void)
{
	RACE_DESC *RaceDescPtr;

	vux_desc.preprocess_func = vux_arrival_preprocess;
	vux_desc.postprocess_func = NULL;
	vux_desc.init_weapon_func = initialize_horrific_laser;
	vux_desc.cyborg_control.intelligence_func = vux_intelligence;

	RaceDescPtr = &vux_desc;

	return (RaceDescPtr);
}

