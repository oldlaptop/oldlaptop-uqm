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
#include "melnorme.h"
#include "resinst.h"

#include "uqm/globdata.h"
#include "libs/mathlib.h"

// Core characteristics
#define MAX_CREW 20
#define MAX_ENERGY MAX_ENERGY_SIZE
#define ENERGY_REGENERATION 1
#define ENERGY_WAIT 5
#define MAX_THRUST 56
#define THRUST_INCREMENT 24
#define THRUST_WAIT 4
#define TURN_WAIT 5
#define SHIP_MASS 10

// Starspray
#define WEAPON_ENERGY_COST 2
#define WEAPON_WAIT 0
#define MELNORME_OFFSET 24
#define LEVEL_COUNTER 72
#define REVERSE_DIR (BYTE)(1 << 7)

/* These are now used only for the AI */
#define PUMPUP_SPEED DISPLAY_TO_WORLD (45)
#define PUMPUP_LIFE 10

// Confusion Pulse
#define SPECIAL_ENERGY_COST 10
#define SPECIAL_WAIT 10
#define CMISSILE_SPEED DISPLAY_TO_WORLD (30)
#define CMISSILE_LIFE 20
#define CMISSILE_HITS 200
#define CMISSILE_DAMAGE 0
#define CMISSILE_OFFSET 4

static RACE_DESC melnorme_desc =
{
	{ /* SHIP_INFO */
		"trader",
		FIRES_FORE,
		21, /* Super Melee cost */
		MAX_CREW, MAX_CREW,
		MAX_ENERGY, MAX_ENERGY,
		MELNORME_RACE_STRINGS,
		MELNORME_ICON_MASK_PMAP_ANIM,
		MELNORME_MICON_MASK_PMAP_ANIM,
		NULL, NULL, NULL
	},
	{ /* FLEET_STUFF */
		INFINITE_RADIUS, /* Initial sphere of influence radius */
		{ /* Known location (center of SoI) */
			MAX_X_UNIVERSE >> 1, MAX_Y_UNIVERSE >> 1,
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
			MELNORME_BIG_MASK_PMAP_ANIM,
			MELNORME_MED_MASK_PMAP_ANIM,
			MELNORME_SML_MASK_PMAP_ANIM,
		},
		{
			PUMPUP_BIG_MASK_PMAP_ANIM,
			PUMPUP_MED_MASK_PMAP_ANIM,
			PUMPUP_SML_MASK_PMAP_ANIM,
		},
		{
			CONFUSE_BIG_MASK_PMAP_ANIM,
			CONFUSE_MED_MASK_PMAP_ANIM,
			CONFUSE_SML_MASK_PMAP_ANIM,
		},
		{
			MELNORME_CAPTAIN_MASK_PMAP_ANIM,
			NULL, NULL, NULL, NULL, NULL
		},
		MELNORME_VICTORY_SONG,
		MELNORME_SHIP_SOUNDS,
		{ NULL, NULL, NULL },
		{ NULL, NULL, NULL },
		{ NULL, NULL, NULL },
		NULL, NULL
	},
	{
		0,
		PUMPUP_SPEED * PUMPUP_LIFE,
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
starspray_preprocess (ELEMENT *ElementPtr)
{
	BYTE max_turn_wait, turn_wait;

	max_turn_wait = HINIBBLE (ElementPtr->turn_wait);
	turn_wait = LONIBBLE (ElementPtr->turn_wait);

	if (turn_wait > 0)
		--turn_wait;
	else
	{
		COUNT facing;

		facing = NORMALIZE_FACING (ANGLE_TO_FACING (
				GetVelocityTravelAngle (&ElementPtr->velocity)
				));

		SIZE speed;

		if (max_turn_wait == 1)
			speed = DISPLAY_TO_WORLD (20);
		else if (max_turn_wait == 2)
			speed = DISPLAY_TO_WORLD (40);
		else
			speed = DISPLAY_TO_WORLD (30);

		SetVelocityVector (&ElementPtr->velocity,
				speed, facing);

		turn_wait = max_turn_wait;
	}

	ElementPtr->turn_wait = MAKE_BYTE (turn_wait, max_turn_wait);
}


static COUNT
initialize_starspray (ELEMENT *ShipPtr, HELEMENT StarSprayArray[])
{
#define STARSPRAY_OFFSET 8
	COUNT which_type;
	STARSHIP *StarShipPtr;
	MISSILE_BLOCK MissileBlock;

	GetElementStarShip (ShipPtr, &StarShipPtr);
	MissileBlock.cx = ShipPtr->next.location.x;
	MissileBlock.cy = ShipPtr->next.location.y;
	MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.weapon;
	MissileBlock.face = StarShipPtr->ShipFacing;
	MissileBlock.sender = ShipPtr->playerNr;
	MissileBlock.flags = IGNORE_SIMILAR;
	MissileBlock.pixoffs = MELNORME_OFFSET;
	MissileBlock.preprocess_func = starspray_preprocess;
	MissileBlock.blast_offs = STARSPRAY_OFFSET;

	which_type = (COUNT)TFB_Random () % 4;

	switch (which_type)
	{
		case 0:
		{
			MissileBlock.speed = DISPLAY_TO_WORLD (45);
			MissileBlock.life = 17;
			MissileBlock.hit_points = 1;
			MissileBlock.damage = 1;
			MissileBlock.index = 1;
			break;
		}
		case 1:
		{
			MissileBlock.speed = DISPLAY_TO_WORLD (25);
			MissileBlock.life = 15;
			MissileBlock.hit_points = 2;
			MissileBlock.damage = 2;
			MissileBlock.index = 6;
			break;
		}
		case 2:
		{
			MissileBlock.speed = DISPLAY_TO_WORLD (15);
			MissileBlock.life = 12;
			MissileBlock.hit_points = 2;
			MissileBlock.damage = 2;
			MissileBlock.index = 11;
			break;
		}
		case 3:
		{
			MissileBlock.speed = DISPLAY_TO_WORLD (35);
			MissileBlock.life = 9;
			MissileBlock.hit_points = 3;
			MissileBlock.damage = 3;
			MissileBlock.index = 16;
			break;
		}
	}

	StarSprayArray[0] = initialize_missile (&MissileBlock);

	if (StarSprayArray[0])
	{
		ELEMENT *SprayPtr;

		LockElement (StarSprayArray[0], &SprayPtr);
		switch (which_type)
		{
			case 0:
			{
				SprayPtr->turn_wait = MAKE_BYTE (2,2);
				break;
			}
			case 1:
			{
				SprayPtr->turn_wait = MAKE_BYTE (4,4);
				break;
			}
			case 2:
			{
				SprayPtr->turn_wait = MAKE_BYTE (1,1);
				break;
			}
			case 3:
			{
				SprayPtr->turn_wait = MAKE_BYTE (3,3);
				break;
			}
		}

		UnlockElement (StarSprayArray[0]);
	}

	return (1);
}

static void
confuse_preprocess (ELEMENT *ElementPtr)
{
	if (!(ElementPtr->state_flags & NONSOLID))
	{
		ElementPtr->next.image.frame = SetAbsFrameIndex (
				ElementPtr->current.image.frame,
				(GetFrameIndex (ElementPtr->current.image.frame) + 1) & 7);
		ElementPtr->state_flags |= CHANGING;
	}
	else if (ElementPtr->hTarget == 0)
	{
		ElementPtr->life_span = 0;
		ElementPtr->state_flags |= DISAPPEARING;
	}
	else
	{
		ELEMENT *eptr;

		LockElement (ElementPtr->hTarget, &eptr);

		ElementPtr->next.location = eptr->next.location;

		if (ElementPtr->turn_wait)
		{
			HELEMENT hEffect;
			STARSHIP *StarShipPtr;

			if (GetFrameIndex (ElementPtr->next.image.frame =
					IncFrameIndex (ElementPtr->current.image.frame)) == 0)
				ElementPtr->next.image.frame =
						SetRelFrameIndex (ElementPtr->next.image.frame, -8);

			GetElementStarShip (eptr, &StarShipPtr);
			StarShipPtr->ship_input_state =
					(StarShipPtr->ship_input_state
					& ~(LEFT | RIGHT | SPECIAL))
					| ElementPtr->turn_wait;

			hEffect = AllocElement ();
			if (hEffect)
			{
				LockElement (hEffect, &eptr);
				eptr->playerNr = ElementPtr->playerNr;
				eptr->state_flags = FINITE_LIFE | NONSOLID | CHANGING;
				eptr->life_span = 1;
				eptr->current = eptr->next = ElementPtr->next;
				eptr->preprocess_func = confuse_preprocess;
				SetPrimType (&(GLOBAL (DisplayArray))[eptr->PrimIndex],
						STAMP_PRIM);

				GetElementStarShip (ElementPtr, &StarShipPtr);
				SetElementStarShip (eptr, StarShipPtr);
				eptr->hTarget = ElementPtr->hTarget;

				UnlockElement (hEffect);
				PutElement (hEffect);
			}
		}

		UnlockElement (ElementPtr->hTarget);
	}
}

static void
confusion_collision (ELEMENT *ElementPtr0, POINT *pPt0,
		ELEMENT *ElementPtr1, POINT *pPt1)
{
	STARSHIP *EnemyStarShipPtr;
	GetElementStarShip (ElementPtr1, &EnemyStarShipPtr);

	if (ElementPtr1->state_flags & PLAYER_SHIP
		&& !(EnemyStarShipPtr->RaceDescPtr->ship_info.ship_flags & CREW_IMMUNE))
	{
		HELEMENT hConfusionElement, hNextElement;
		ELEMENT *ConfusionPtr;
		STARSHIP *StarShipPtr;

		GetElementStarShip (ElementPtr0, &StarShipPtr);
		for (hConfusionElement = GetHeadElement ();
				hConfusionElement; hConfusionElement = hNextElement)
		{
			LockElement (hConfusionElement, &ConfusionPtr);
			if (elementsOfSamePlayer (ConfusionPtr, ElementPtr0)
					&& ConfusionPtr->current.image.farray ==
					StarShipPtr->RaceDescPtr->ship_data.special
					&& (ConfusionPtr->state_flags & NONSOLID))
			{
				UnlockElement (hConfusionElement);
				break;
			}
			hNextElement = GetSuccElement (ConfusionPtr);
			UnlockElement (hConfusionElement);
		}

		if (hConfusionElement || (hConfusionElement = AllocElement ()))
		{
			LockElement (hConfusionElement, &ConfusionPtr);

			if (ConfusionPtr->state_flags == 0) /* not allocated before */
			{
				InsertElement (hConfusionElement, GetHeadElement ());

				ConfusionPtr->current = ElementPtr0->next;
				ConfusionPtr->current.image.frame = SetAbsFrameIndex (
						ConfusionPtr->current.image.frame, 8
						);
				ConfusionPtr->next = ConfusionPtr->current;
				ConfusionPtr->playerNr = ElementPtr0->playerNr;
				ConfusionPtr->state_flags = FINITE_LIFE | NONSOLID | CHANGING;
				ConfusionPtr->preprocess_func = confuse_preprocess;
				SetPrimType (
						&(GLOBAL (DisplayArray))[ConfusionPtr->PrimIndex],
						NO_PRIM
						);

				SetElementStarShip (ConfusionPtr, StarShipPtr);
				GetElementStarShip (ElementPtr1, &StarShipPtr);
				ConfusionPtr->hTarget = StarShipPtr->hShip;
			}

			ConfusionPtr->life_span = 400;
			ConfusionPtr->turn_wait =
					(BYTE)(1 << ((BYTE)TFB_Random () & 1)); /* LEFT or RIGHT */

			UnlockElement (hConfusionElement);
		}

		ElementPtr0->hit_points = 0;
		ElementPtr0->life_span = 0;
		ElementPtr0->state_flags |= DISAPPEARING | COLLISION | NONSOLID;
	}
	(void) pPt0;  /* Satisfying compiler (unused parameter) */
	(void) pPt1;  /* Satisfying compiler (unused parameter) */
}

static COUNT
initialize_confusion (ELEMENT *ShipPtr, HELEMENT ConfusionArray[])
{
	STARSHIP *StarShipPtr;
	MISSILE_BLOCK ConfusionBlock;

	GetElementStarShip (ShipPtr, &StarShipPtr);
	ConfusionBlock.cx = ShipPtr->next.location.x;
	ConfusionBlock.cy = ShipPtr->next.location.y;
	ConfusionBlock.farray = StarShipPtr->RaceDescPtr->ship_data.special;
	ConfusionBlock.index = 0;
	ConfusionBlock.face = StarShipPtr->ShipFacing;
	ConfusionBlock.sender = ShipPtr->playerNr;
	ConfusionBlock.flags = IGNORE_SIMILAR;
	ConfusionBlock.pixoffs = MELNORME_OFFSET;
	ConfusionBlock.speed = CMISSILE_SPEED;
	ConfusionBlock.hit_points = CMISSILE_HITS;
	ConfusionBlock.damage = CMISSILE_DAMAGE;
	ConfusionBlock.life = CMISSILE_LIFE;
	ConfusionBlock.preprocess_func = confuse_preprocess;
	ConfusionBlock.blast_offs = CMISSILE_OFFSET;
	ConfusionArray[0] = initialize_missile (&ConfusionBlock);

	if (ConfusionArray[0])
	{
		ELEMENT *CMissilePtr;

		LockElement (ConfusionArray[0], &CMissilePtr);
		CMissilePtr->collision_func = confusion_collision;
		SetElementStarShip (CMissilePtr, StarShipPtr);
		UnlockElement (ConfusionArray[0]);
	}
	return (1);
}

static void
melnorme_intelligence (ELEMENT *ShipPtr, EVALUATE_DESC *ObjectsOfConcern,
		COUNT ConcernCounter)
{
	BYTE old_count;
	STARSHIP *StarShipPtr;
	EVALUATE_DESC *lpEvalDesc;

	GetElementStarShip (ShipPtr, &StarShipPtr);

	StarShipPtr->RaceDescPtr->init_weapon_func = initialize_starspray;
	old_count = StarShipPtr->weapon_counter;

	if (StarShipPtr->weapon_counter == WEAPON_WAIT)
		StarShipPtr->weapon_counter = 0;

	lpEvalDesc = &ObjectsOfConcern[ENEMY_SHIP_INDEX];
	if (lpEvalDesc->ObjectPtr)
	{
		if (StarShipPtr->RaceDescPtr->ship_info.energy_level < SPECIAL_ENERGY_COST
				+ WEAPON_ENERGY_COST
				&& !(StarShipPtr->old_status_flags & WEAPON))
			lpEvalDesc->MoveState = ENTICE;
		else
		{
			STARSHIP *EnemyStarShipPtr;

			GetElementStarShip (lpEvalDesc->ObjectPtr, &EnemyStarShipPtr);
			if (!(EnemyStarShipPtr->RaceDescPtr->ship_info.ship_flags
					& IMMEDIATE_WEAPON))
				lpEvalDesc->MoveState = PURSUE;
		}
	}
	ship_intelligence (ShipPtr, ObjectsOfConcern, ConcernCounter);

	if (StarShipPtr->weapon_counter == 0
			&& (old_count != 0
			|| ((StarShipPtr->special_counter
			|| StarShipPtr->RaceDescPtr->ship_info.energy_level >= SPECIAL_ENERGY_COST
			+ WEAPON_ENERGY_COST)
			&& !(StarShipPtr->ship_input_state & WEAPON))))
		StarShipPtr->ship_input_state ^= WEAPON;

	StarShipPtr->ship_input_state &= ~SPECIAL;
	if (StarShipPtr->special_counter == 0
			&& StarShipPtr->RaceDescPtr->ship_info.energy_level >= SPECIAL_ENERGY_COST)
	{
		BYTE old_input_state;

		old_input_state = StarShipPtr->ship_input_state;

		StarShipPtr->RaceDescPtr->init_weapon_func = initialize_confusion;

		++ShipPtr->turn_wait;
		++ShipPtr->thrust_wait;
		ship_intelligence (ShipPtr, ObjectsOfConcern, ENEMY_SHIP_INDEX + 1);
		--ShipPtr->thrust_wait;
		--ShipPtr->turn_wait;

		if (StarShipPtr->ship_input_state & WEAPON)
		{
			StarShipPtr->ship_input_state &= ~WEAPON;
			StarShipPtr->ship_input_state |= SPECIAL;
		}

		StarShipPtr->ship_input_state = (unsigned char)(old_input_state
				| (StarShipPtr->ship_input_state & SPECIAL));
	}

	StarShipPtr->weapon_counter = old_count;

	StarShipPtr->RaceDescPtr->init_weapon_func = initialize_starspray;
}

static void
melnorme_postprocess (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	if ((StarShipPtr->cur_status_flags & SPECIAL)
			&& StarShipPtr->special_counter == 0
			&& DeltaEnergy (ElementPtr, -SPECIAL_ENERGY_COST))
	{
		HELEMENT Confusion;

		initialize_confusion (ElementPtr, &Confusion);
		if (Confusion)
		{
			ELEMENT *CMissilePtr;
			LockElement (Confusion, &CMissilePtr);
			
			ProcessSound (SetAbsSoundIndex (
					StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1), CMissilePtr);
			
			UnlockElement (Confusion);
			PutElement (Confusion);
			StarShipPtr->special_counter =
					StarShipPtr->RaceDescPtr->characteristics.special_wait;
		}
	}
}

RACE_DESC*
init_melnorme (void)
{
	RACE_DESC *RaceDescPtr;

	melnorme_desc.postprocess_func = melnorme_postprocess;
	melnorme_desc.init_weapon_func = initialize_starspray;
	melnorme_desc.cyborg_control.intelligence_func = melnorme_intelligence;

	RaceDescPtr = &melnorme_desc;

	return (RaceDescPtr);
}

