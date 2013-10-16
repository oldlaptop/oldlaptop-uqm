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
#include "ships/slylandr/resinst.h"

#include "globdata.h"
#include "libs/mathlib.h"


#define MAX_CREW 25
#define MAX_ENERGY 30
#define ENERGY_REGENERATION 1
#define WEAPON_ENERGY_COST 7
#define SPECIAL_ENERGY_COST 0
#define ENERGY_WAIT 2
#define MAX_THRUST 75
#define THRUST_INCREMENT 12
#define TURN_WAIT 1
#define THRUST_WAIT 1
#define WEAPON_WAIT 6
#define SPECIAL_WAIT 0

#define SHIP_MASS 3
#define SLYLANDRO_OFFSET 9
#define MISSILE_SPEED DISPLAY_TO_WORLD(43)
#define MISSILE_LIFE 10

static RACE_DESC slylandro_desc =
{
	{
		SEEKING_WEAPON | CREW_IMMUNE,
		27, /* Super Melee cost */
		~0, /* Initial sphere of influence radius */
		MAX_CREW, MAX_CREW,
		MAX_ENERGY, MAX_ENERGY,
		{
			333, 9812,
		},
		(STRING)SLYLANDRO_RACE_STRINGS,
		(FRAME)SLYLANDRO_ICON_MASK_PMAP_ANIM,
		(FRAME)SLYLANDRO_MICON_MASK_PMAP_ANIM,
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
			(FRAME)SLYLANDRO_BIG_MASK_PMAP_ANIM,
			(FRAME)SLYLANDRO_MED_MASK_PMAP_ANIM,
			(FRAME)SLYLANDRO_SML_MASK_PMAP_ANIM,
		},
		{
			(FRAME)0,
			(FRAME)0,
			(FRAME)0,
		},
		{
			(FRAME)0,
			(FRAME)0,
			(FRAME)0,
		},
		{
			(FRAME)SLYLANDRO_CAPTAIN_MASK_PMAP_ANIM,
			(FRAME)0,
			(FRAME)0,
			(FRAME)0,
			(FRAME)0,
		},
		(SOUND)SLYLANDRO_VICTORY_SONG,
		(SOUND)SLYLANDRO_SHIP_SOUNDS,
	},
	{
		0,
		CLOSE_RANGE_WEAPON << 1,
		NULL_PTR,
	},
	(UNINIT_FUNC *) NULL,
	(PREPROCESS_FUNC *) NULL,
	(POSTPROCESS_FUNC *) NULL,
	(INIT_WEAPON_FUNC *) NULL,
	0,
};


static void
slylandro_intelligence (PELEMENT ShipPtr, PEVALUATE_DESC ObjectsOfConcern, COUNT ConcernCounter)
{
	PEVALUATE_DESC lpEvalDesc;
	STARSHIPPTR StarShipPtr;

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

	/*if (lpEvalDesc->ObjectPtr && lpEvalDesc->which_turn <= 14)*/
		StarShipPtr->ship_input_state |= WEAPON;
	/*else
		StarShipPtr->ship_input_state &= ~WEAPON;*/

	StarShipPtr->ship_input_state &= ~SPECIAL;
	/*if (StarShipPtr->RaceDescPtr->ship_info.energy_level <
			StarShipPtr->RaceDescPtr->ship_info.max_energy)
	{
		lpEvalDesc = &ObjectsOfConcern[FIRST_EMPTY_INDEX];
		if (lpEvalDesc->ObjectPtr && lpEvalDesc->which_turn <= 14)
			StarShipPtr->ship_input_state |= SPECIAL;
	}*/
}

static COUNT
initialize_nukes (PELEMENT ShipPtr, HELEMENT MissileArray[])
{
#define MISSILE_DAMAGE 4
#define MISSILE_HITS 8
#define NUKE_OFFSET 8
	COUNT i;
	STARSHIPPTR StarShipPtr;
	MISSILE_BLOCK MissileBlock;

	GetElementStarShip (ShipPtr, &StarShipPtr);
	MissileBlock.cx = ShipPtr->next.location.x;
	MissileBlock.cy = ShipPtr->next.location.y;
	MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.weapon;
	MissileBlock.face = MissileBlock.index = StarShipPtr->ShipFacing;
	MissileBlock.sender = (ShipPtr->state_flags & (GOOD_GUY | BAD_GUY))
			| IGNORE_SIMILAR;
	MissileBlock.pixoffs = 42;
	MissileBlock.speed = MISSILE_SPEED;
	MissileBlock.hit_points = MISSILE_HITS;
	MissileBlock.damage = MISSILE_DAMAGE;
	MissileBlock.life = MISSILE_LIFE;
	MissileBlock.preprocess_func = NULL_PTR;
	MissileBlock.blast_offs = NUKE_OFFSET;

	MissileArray[0] = initialize_missile (&MissileBlock);
	MissileBlock.cx = ShipPtr->next.location.x + COSINE(FACING_TO_ANGLE(MissileBlock.face + 4), 30);
	MissileBlock.cy = ShipPtr->next.location.y + SINE(FACING_TO_ANGLE(MissileBlock.face + 4), 30);
	MissileArray[1] = initialize_missile (&MissileBlock);
	MissileBlock.cx = ShipPtr->next.location.x - COSINE(FACING_TO_ANGLE(MissileBlock.face + 4), 30);
	MissileBlock.cy = ShipPtr->next.location.y - SINE(FACING_TO_ANGLE(MissileBlock.face + 4), 30);
	MissileArray[2] = initialize_missile (&MissileBlock);

	return (3);
}

static void
slylandro_postprocess (PELEMENT ElementPtr)
{
	STARSHIPPTR StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);	
}


static COUNT slylandroes_present = 0;

static void
slylandro_dispose_graphics (RACE_DESCPTR RaceDescPtr)
{
	--slylandroes_present;

	if(!slylandroes_present)
	{
		extern void clearGraphicsHack(FRAME farray[]);
		clearGraphicsHack(RaceDescPtr->ship_data.weapon);
	}
}

static void
slylandro_preprocess (PELEMENT ElementPtr)
{
	STARSHIPPTR StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);

	if (ElementPtr->state_flags & APPEARING)
	{
		if(StarShipPtr->RaceDescPtr->ship_data.weapon[0] == (FRAME)0)
		{
			StarShipPtr->RaceDescPtr->ship_data.weapon[0] = CaptureDrawable(LoadCelFile("human/saturn.big"));
			StarShipPtr->RaceDescPtr->ship_data.weapon[1] = CaptureDrawable(LoadCelFile("human/saturn.med"));
			StarShipPtr->RaceDescPtr->ship_data.weapon[2] = CaptureDrawable(LoadCelFile("human/saturn.sml"));
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

RACE_DESCPTR
init_slylandro (void)
{
	RACE_DESCPTR RaceDescPtr;

	slylandro_desc.preprocess_func = slylandro_preprocess;
	slylandro_desc.postprocess_func = slylandro_postprocess;
	slylandro_desc.init_weapon_func = initialize_nukes;
	slylandro_desc.uninit_func = slylandro_dispose_graphics;
	slylandro_desc.cyborg_control.intelligence_func =
			(void (*) (PVOID ShipPtr, PVOID ObjectsOfConcern, COUNT
					ConcernCounter)) slylandro_intelligence;

	RaceDescPtr = &slylandro_desc;

	return (RaceDescPtr);
}
