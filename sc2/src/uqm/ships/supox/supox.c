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
#include "supox.h"
#include "resinst.h"

#include "libs/mathlib.h"
#include "uqm/colors.h"

// Core characteristics
#define MAX_CREW 18
#define MAX_ENERGY 16
#define ENERGY_REGENERATION 1
#define ENERGY_WAIT 4
#define MAX_THRUST 40
#define THRUST_INCREMENT 8
#define THRUST_WAIT 0
#define TURN_WAIT 1
#define SHIP_MASS 2

// Gob launcher
#define WEAPON_ENERGY_COST 1
#define WEAPON_WAIT 2
#define SUPOX_OFFSET 23
#define MISSILE_OFFSET 2
#define MISSILE_SPEED DISPLAY_TO_WORLD (30)
#define MISSILE_LIFE 10
#define MISSILE_HITS 10000
#define MISSILE_DAMAGE 1
#define SHOTS_ON_EACH_SIDE 1
#define SHOT_OFFSET 30

// Enhanced thrust
#define SPECIAL_ENERGY_COST 1
#define SPECIAL_WAIT 2
#define SPECIAL_SHIP_MASS 10
#define SPECIAL_MAX_THRUST 120
#define SPECIAL_THRUST_INCREMENT 24
#define SPECIAL_TURN_WAIT 0
#define START_ION_COLOR BUILD_COLOR (MAKE_RGB15 (0x00, 0x10, 0x00), 0x00)
#define ION_LIFE 1

static RACE_DESC supox_desc =
{
	{ /* SHIP_INFO */
		"blade",
		FIRES_FORE,
		26, /* Super Melee cost */
		MAX_CREW, MAX_CREW,
		MAX_ENERGY, MAX_ENERGY,
		SUPOX_RACE_STRINGS,
		SUPOX_ICON_MASK_PMAP_ANIM,
		SUPOX_MICON_MASK_PMAP_ANIM,
		NULL, NULL, NULL
	},
	{ /* FLEET_STUFF */
		333 / SPHERE_RADIUS_INCREMENT * 2, /* Initial SoI radius */
		{ /* Known location (center of SoI) */
			7468, 9246,
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
			SUPOX_BIG_MASK_PMAP_ANIM,
			SUPOX_MED_MASK_PMAP_ANIM,
			SUPOX_SML_MASK_PMAP_ANIM,
		},
		{
			GOB_BIG_MASK_PMAP_ANIM,
			GOB_MED_MASK_PMAP_ANIM,
			GOB_SML_MASK_PMAP_ANIM,
		},
		{
			NULL_RESOURCE,
			NULL_RESOURCE,
			NULL_RESOURCE,
		},
		{
			SUPOX_CAPTAIN_MASK_PMAP_ANIM,
			NULL, NULL, NULL, NULL, NULL
		},
		SUPOX_VICTORY_SONG,
		SUPOX_SHIP_SOUNDS,
		{ NULL, NULL, NULL },
		{ NULL, NULL, NULL },
		{ NULL, NULL, NULL },
		NULL, NULL
	},
	{
		0,
		(MISSILE_SPEED * MISSILE_LIFE) >> 1,
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
supox_intelligence (ELEMENT *ShipPtr, EVALUATE_DESC *ObjectsOfConcern,
		COUNT ConcernCounter)
{
	STARSHIP *StarShipPtr;
	EVALUATE_DESC *lpEvalDesc;

	GetElementStarShip (ShipPtr, &StarShipPtr);

	lpEvalDesc = &ObjectsOfConcern[ENEMY_SHIP_INDEX];
	if (StarShipPtr->special_counter || lpEvalDesc->ObjectPtr == 0)
		StarShipPtr->ship_input_state &= ~SPECIAL;
	else
	{
		BOOLEAN LinedUp;
		COUNT direction_angle;
		SIZE delta_x, delta_y;

		delta_x = lpEvalDesc->ObjectPtr->next.location.x
				- ShipPtr->next.location.x;
		delta_y = lpEvalDesc->ObjectPtr->next.location.y
				- ShipPtr->next.location.y;
		direction_angle = ARCTAN (delta_x, delta_y);

		LinedUp = (BOOLEAN)(NORMALIZE_ANGLE (NORMALIZE_ANGLE (direction_angle
				- FACING_TO_ANGLE (StarShipPtr->ShipFacing))
				+ QUADRANT) <= HALF_CIRCLE);

		if (!LinedUp
				|| lpEvalDesc->which_turn > 20
				|| NORMALIZE_ANGLE (
				lpEvalDesc->facing
				- (FACING_TO_ANGLE (StarShipPtr->ShipFacing)
				+ HALF_CIRCLE) + OCTANT
				) > QUADRANT)
			StarShipPtr->ship_input_state &= ~SPECIAL;
		else if (LinedUp && lpEvalDesc->which_turn <= 12)
			StarShipPtr->ship_input_state |= SPECIAL;

		if (StarShipPtr->ship_input_state & SPECIAL)
			lpEvalDesc->MoveState = PURSUE;
	}

	ship_intelligence (ShipPtr,
			ObjectsOfConcern, ConcernCounter);

	if (StarShipPtr->ship_input_state & SPECIAL)
		StarShipPtr->ship_input_state |= THRUST | WEAPON;

	lpEvalDesc = &ObjectsOfConcern[ENEMY_WEAPON_INDEX];
	if (StarShipPtr->special_counter == 0
			&& lpEvalDesc->ObjectPtr
			&& lpEvalDesc->MoveState == AVOID
			&& ShipPtr->turn_wait == 0)
	{
		StarShipPtr->ship_input_state &= ~THRUST;
		StarShipPtr->ship_input_state |= SPECIAL;
		if (!(StarShipPtr->cur_status_flags & (LEFT | RIGHT)))
			StarShipPtr->ship_input_state |= 1 << ((BYTE)TFB_Random () & 1);
		else
			StarShipPtr->ship_input_state |=
					StarShipPtr->cur_status_flags & (LEFT | RIGHT);
	}
}

/* HEY YOU - Following two funcs copied from tactrans.c and modified (
 * cycle_ion_trail, spawn_ion_trail)
 */
//BEGIN copied code
static void
cycle_supox_trail (ELEMENT *ElementPtr)
{
	static const Color colorTab[] =
	{
			BUILD_COLOR (MAKE_RGB15_INIT (0x00, 0x10, 0x00), 0),
			BUILD_COLOR (MAKE_RGB15_INIT (0x00, 0x0E, 0x00), 0),
			BUILD_COLOR (MAKE_RGB15_INIT (0x00, 0x0C, 0x00), 0),
			BUILD_COLOR (MAKE_RGB15_INIT (0x00, 0x0A, 0x00), 0),
			BUILD_COLOR (MAKE_RGB15_INIT (0x00, 0x08, 0x00), 0),
			BUILD_COLOR (MAKE_RGB15_INIT (0x00, 0x06, 0x00), 0),
			BUILD_COLOR (MAKE_RGB15_INIT (0x00, 0x04, 0x00), 0),
			BUILD_COLOR (MAKE_RGB15_INIT (0x00, 0x02, 0x00), 0),
	};
	const size_t colorTabCount = sizeof colorTab / sizeof colorTab[0];
			
	assert (!(ElementPtr->state_flags & PLAYER_SHIP));

	ElementPtr->colorCycleIndex++;
	if (ElementPtr->colorCycleIndex != colorTabCount)
	{
		ElementPtr->life_span = ElementPtr->thrust_wait;
				// Reset the life span.
		
		SetPrimColor (&DisplayArray[ElementPtr->PrimIndex],
				colorTab[ElementPtr->colorCycleIndex]);

		ElementPtr->state_flags &= ~DISAPPEARING;
		ElementPtr->state_flags |= CHANGING;
	} // else, the element disappears.
}

static void
spawn_supox_trail (ELEMENT *ElementPtr)
{
	HELEMENT hIonElement;

	assert (ElementPtr->state_flags & PLAYER_SHIP);

	hIonElement = AllocElement ();
	if (hIonElement)
	{
		COUNT angle;
		RECT r;
		ELEMENT *IonElementPtr;
		STARSHIP *StarShipPtr;

		GetElementStarShip (ElementPtr, &StarShipPtr);
		angle = FACING_TO_ANGLE (StarShipPtr->ShipFacing) + HALF_CIRCLE;
		GetFrameRect (StarShipPtr->RaceDescPtr->ship_data.ship[0], &r);
		r.extent.height = DISPLAY_TO_WORLD (r.extent.height + r.corner.y);

		InsertElement (hIonElement, GetHeadElement ());
		LockElement (hIonElement, &IonElementPtr);
		IonElementPtr->playerNr = NEUTRAL_PLAYER_NUM;
		IonElementPtr->state_flags = APPEARING | FINITE_LIFE | NONSOLID;
		IonElementPtr->thrust_wait = ION_LIFE;
		IonElementPtr->life_span = IonElementPtr->thrust_wait;
				// When the element "dies", in the death_func
				// 'cycle_ion_trail', it is given new life a number of
				// times, by setting life_span to thrust_wait.
		SetPrimType (&DisplayArray[IonElementPtr->PrimIndex], STAMPFILL_PRIM);
		SetPrimColor (&DisplayArray[IonElementPtr->PrimIndex],
				START_ION_COLOR);
		IonElementPtr->colorCycleIndex = 0;
		IonElementPtr->current.image.frame =
				ElementPtr->current.image.frame;
		IonElementPtr->current.image.farray = 
				ElementPtr->current.image.farray;
		IonElementPtr->current.location = ElementPtr->current.location;
		IonElementPtr->death_func = cycle_supox_trail;

		SetElementStarShip (IonElementPtr, StarShipPtr);

		{
			/* normally done during preprocess, but because
			 * object is being inserted at head rather than
			 * appended after tail it may never get preprocessed.
			 */
			IonElementPtr->next = IonElementPtr->current;
			--IonElementPtr->life_span;
			IonElementPtr->state_flags |= PRE_PROCESS;
		}

		UnlockElement (hIonElement);
	}
}
//END copied code

//this is much like yehat_collision, also - EP
static void
supox_special_collision (ELEMENT *ElementPtr0, POINT *pPt0,
		ELEMENT *ElementPtr1, POINT *pPt1)
{
	if (!(ElementPtr1->state_flags & FINITE_LIFE)
		&& GRAVITY_MASS (ElementPtr1->mass_points))
	{
		ElementPtr0->state_flags |= COLLISION; //collision without damage
	}
	else
	{
		collision(ElementPtr0, pPt0, ElementPtr1, pPt1); //normal collision
	}
}

static void
horn_collision (ELEMENT *ElementPtr0, POINT *pPt0, ELEMENT *ElementPtr1, POINT *pPt1)
{
	ElementPtr0->turn_wait = 1;
	//if it's an asteroid
	if((!(ElementPtr1->state_flags
			& (APPEARING | PLAYER_SHIP | FINITE_LIFE))
			&& !GRAVITY_MASS (ElementPtr1->mass_points))
			&& ElementPtr0->playerNr == NEUTRAL_PLAYER_NUM)
	{
		//no getting killed by asteroids!
		//just kill them.
		do_damage (ElementPtr1, 1);
	}
	else if(ElementPtr1->collision_func == horn_collision)
	{
		//don't even COLLIDE with fellow Supox shots.
	}
	else
	{
		ElementPtr0->hit_points = MISSILE_HITS; //let's keep our HP up
		weapon_collision (ElementPtr0, pPt0, ElementPtr1, pPt1);
	}
}

static void
horn_preprocess (ELEMENT *ElementPtr)
{
	if(ElementPtr->turn_wait)
	{
		++ElementPtr->life_span;
		ElementPtr->turn_wait = 0;
	}
}

static COUNT
initialize_horn (ELEMENT *ShipPtr, HELEMENT HornArray[])
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
	MissileBlock.pixoffs = SUPOX_OFFSET;
	MissileBlock.speed = MISSILE_SPEED;
	MissileBlock.hit_points = MISSILE_HITS;
	MissileBlock.damage = MISSILE_DAMAGE;
	MissileBlock.life = MISSILE_LIFE;
	MissileBlock.preprocess_func = horn_preprocess;
	MissileBlock.blast_offs = MISSILE_OFFSET;

	for (i = 0; i <= SHOTS_ON_EACH_SIDE * 2; ++i)
	{
		MissileBlock.cx = ShipPtr->next.location.x + COSINE (FACING_TO_ANGLE (MissileBlock.face + 4), SHOT_OFFSET * (i - SHOTS_ON_EACH_SIDE));
		MissileBlock.cy = ShipPtr->next.location.y +  SINE (FACING_TO_ANGLE (MissileBlock.face + 4), SHOT_OFFSET * (i - SHOTS_ON_EACH_SIDE));
		HornArray[i] = initialize_missile (&MissileBlock);

		if(HornArray[i])
		{
			ELEMENT *HornPtr;

			LockElement (HornArray[i], &HornPtr);
			HornPtr->collision_func = horn_collision;
			HornPtr->turn_wait = 0;
			UnlockElement (HornArray[i]);
		}
	}

	return (1 + (SHOTS_ON_EACH_SIDE * 2));
}

static void
supox_preprocess (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);

	if (ElementPtr->mass_points == SPECIAL_SHIP_MASS)
	{
		spawn_supox_trail (ElementPtr);

		if (StarShipPtr->special_counter != 0
				|| DeltaEnergy (ElementPtr, -SPECIAL_ENERGY_COST))
		{
			if (StarShipPtr->special_counter == 0)
				StarShipPtr->special_counter = SPECIAL_WAIT;
		}
		else
		{
			ElementPtr->collision_func = collision;
			ElementPtr->mass_points = SHIP_MASS;
			StarShipPtr->RaceDescPtr->characteristics.max_thrust =
					MAX_THRUST;
			StarShipPtr->RaceDescPtr->characteristics.thrust_increment =
					THRUST_INCREMENT;
			StarShipPtr->RaceDescPtr->characteristics.turn_wait = TURN_WAIT;
		}
	}
	else if (ElementPtr->mass_points != SPECIAL_SHIP_MASS
			&& (StarShipPtr->cur_status_flags & SPECIAL)
			&& !(StarShipPtr->old_status_flags & SPECIAL)
			&& StarShipPtr->RaceDescPtr->ship_info.energy_level > 2)
	{
		ElementPtr->collision_func = supox_special_collision;
		ElementPtr->mass_points = SPECIAL_SHIP_MASS;
		StarShipPtr->RaceDescPtr->characteristics.max_thrust =
				SPECIAL_MAX_THRUST;
		StarShipPtr->RaceDescPtr->characteristics.thrust_increment =
				SPECIAL_THRUST_INCREMENT;
		StarShipPtr->RaceDescPtr->characteristics.turn_wait =
				SPECIAL_TURN_WAIT;

		StarShipPtr->special_counter = SPECIAL_WAIT;
	}
	
}

RACE_DESC*
init_supox (void)
{
	RACE_DESC *RaceDescPtr;

	supox_desc.preprocess_func = supox_preprocess;
	supox_desc.init_weapon_func = initialize_horn;
	supox_desc.cyborg_control.intelligence_func = supox_intelligence;

	RaceDescPtr = &supox_desc;

	return (RaceDescPtr);
}

