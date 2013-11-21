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
#include "yehat.h"
#include "resinst.h"

#include "libs/mathlib.h"
#include "uqm/globdata.h"

// Core characteristics
#define MAX_CREW 20
#define MAX_ENERGY 10
#define ENERGY_REGENERATION 2
#define ENERGY_WAIT 4
#define MAX_THRUST 30
#define THRUST_INCREMENT 6
#define THRUST_WAIT 2
#define TURN_WAIT 1
#define SHIP_MASS 3

// Twin Pulse Cannon
#define WEAPON_ENERGY_COST 1
#define WEAPON_WAIT 0
#define YEHAT_OFFSET 16
#define LAUNCH_OFFS DISPLAY_TO_WORLD (8)
#define MISSILE_SPEED DISPLAY_TO_WORLD (20)
#define MISSILE_LIFE 10
#define MISSILE_HITS 1
#define MISSILE_DAMAGE 1
#define MISSILE_OFFSET 1

// Reverse tractor field
#define SPECIAL_ENERGY_COST 0
#define NUM_SHADOWS 5

// Intermittent Force Shield
#define SHIELD_LIFE 10
#define SHIELD_WAIT 17 /* Confusingly, this is stored in special_wait - the code
                        * for the tractor beam therefore ignores special_wait
                        * entirely.
                        */

static RACE_DESC yehat_desc =
{
	{ /* SHIP_INFO */
		"terminator",
		FIRES_FORE | SHIELD_DEFENSE,
		38, /* Super Melee cost */
		MAX_CREW, MAX_CREW,
		MAX_ENERGY, MAX_ENERGY,
		YEHAT_RACE_STRINGS,
		YEHAT_ICON_MASK_PMAP_ANIM,
		YEHAT_MICON_MASK_PMAP_ANIM,
		NULL, NULL, NULL
	},
	{ /* FLEET_STUFF */
		750 / SPHERE_RADIUS_INCREMENT * 2, /* Initial SoI radius */
		{ /* Known location (center of SoI) */
			4970, 40,
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
		SHIELD_WAIT,
		SHIP_MASS,
	},
	{
		{
			YEHAT_BIG_MASK_PMAP_ANIM,
			YEHAT_MED_MASK_PMAP_ANIM,
			YEHAT_SML_MASK_PMAP_ANIM,
		},
		{
			YEHAT_CANNON_BIG_MASK_PMAP_ANIM,
			YEHAT_CANNON_MED_MASK_PMAP_ANIM,
			YEHAT_CANNON_SML_MASK_PMAP_ANIM,
		},
		{
			SHIELD_BIG_MASK_ANIM,
			SHIELD_MED_MASK_ANIM,
			SHIELD_SML_MASK_ANIM,
		},
		{
			YEHAT_CAPTAIN_MASK_PMAP_ANIM,
			NULL, NULL, NULL, NULL, NULL
		},
		YEHAT_VICTORY_SONG,
		YEHAT_SHIP_SOUNDS,
		{ NULL, NULL, NULL },
		{ NULL, NULL, NULL },
		{ NULL, NULL, NULL },
		NULL, NULL
	},
	{
		0,
		MISSILE_SPEED * MISSILE_LIFE / 3,
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
initialize_standard_missiles (ELEMENT *ShipPtr, HELEMENT MissileArray[])
{
	SIZE offs_x, offs_y;
	STARSHIP *StarShipPtr;
	MISSILE_BLOCK MissileBlock;

	GetElementStarShip (ShipPtr, &StarShipPtr);
	MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.weapon;
	MissileBlock.face = MissileBlock.index = StarShipPtr->ShipFacing;
	MissileBlock.sender = ShipPtr->playerNr;
	MissileBlock.flags = IGNORE_SIMILAR;
	MissileBlock.pixoffs = YEHAT_OFFSET;
	MissileBlock.speed = MISSILE_SPEED;
	MissileBlock.hit_points = MISSILE_HITS;
	MissileBlock.damage = MISSILE_DAMAGE;
	MissileBlock.life = MISSILE_LIFE;
	MissileBlock.preprocess_func = NULL;
	MissileBlock.blast_offs = MISSILE_OFFSET;

	offs_x = -SINE (FACING_TO_ANGLE (MissileBlock.face), LAUNCH_OFFS);
	offs_y = COSINE (FACING_TO_ANGLE (MissileBlock.face), LAUNCH_OFFS);

	MissileBlock.cx = ShipPtr->next.location.x + offs_x;
	MissileBlock.cy = ShipPtr->next.location.y + offs_y;
	MissileArray[0] = initialize_missile (&MissileBlock);

	MissileBlock.cx = ShipPtr->next.location.x - offs_x;
	MissileBlock.cy = ShipPtr->next.location.y - offs_y;
	MissileArray[1] = initialize_missile (&MissileBlock);

	return (2);
}

static void
yehat_intelligence (ELEMENT *ShipPtr, EVALUATE_DESC *ObjectsOfConcern,
		COUNT ConcernCounter)
{
	SIZE ShieldStatus;
	STARSHIP *StarShipPtr;
	EVALUATE_DESC *lpEvalDesc;

	ShieldStatus = -1;
	lpEvalDesc = &ObjectsOfConcern[ENEMY_WEAPON_INDEX];
	if (lpEvalDesc->ObjectPtr && lpEvalDesc->MoveState == ENTICE)
	{
		ShieldStatus = 0;
		if (!(lpEvalDesc->ObjectPtr->state_flags & (FINITE_LIFE | CREW_OBJECT)))
			lpEvalDesc->MoveState = PURSUE;
		else if (lpEvalDesc->ObjectPtr->mass_points
				|| (lpEvalDesc->ObjectPtr->state_flags & CREW_OBJECT))
		{
			if (!(lpEvalDesc->ObjectPtr->state_flags & FINITE_LIFE))
				lpEvalDesc->which_turn <<= 1;
			else
			{
				if ((lpEvalDesc->which_turn >>= 1) == 0)
					lpEvalDesc->which_turn = 1;

				if (lpEvalDesc->ObjectPtr->mass_points)
					lpEvalDesc->ObjectPtr = 0;
				else
					lpEvalDesc->MoveState = PURSUE;
			}
			ShieldStatus = 1;
		}
	}

	GetElementStarShip (ShipPtr, &StarShipPtr);
	if (StarShipPtr->special_counter == 0)
	{
		StarShipPtr->ship_input_state &= ~SPECIAL;
		if (ShieldStatus)
		{
			if (ShipPtr->life_span <= NORMAL_LIFE + 1
					&& (ShieldStatus > 0 || lpEvalDesc->ObjectPtr)
					&& lpEvalDesc->which_turn <= 2
					&& (ShieldStatus > 0
					|| (lpEvalDesc->ObjectPtr->state_flags
					& PLAYER_SHIP) /* means IMMEDIATE WEAPON */
					|| PlotIntercept (lpEvalDesc->ObjectPtr,
					ShipPtr, 2, 0))
					&& (TFB_Random () & 3))
				StarShipPtr->ship_input_state |= SPECIAL;

			if (lpEvalDesc->ObjectPtr
					&& !(lpEvalDesc->ObjectPtr->state_flags & CREW_OBJECT))
				lpEvalDesc->ObjectPtr = 0;
		}
	}

	if ((lpEvalDesc = &ObjectsOfConcern[ENEMY_SHIP_INDEX])->ObjectPtr)
	{
		STARSHIP *EnemyStarShipPtr;

		GetElementStarShip (lpEvalDesc->ObjectPtr, &EnemyStarShipPtr);
		if (!(EnemyStarShipPtr->RaceDescPtr->ship_info.ship_flags
				& IMMEDIATE_WEAPON))
			lpEvalDesc->MoveState = PURSUE;
	}
	ship_intelligence (ShipPtr, ObjectsOfConcern, ConcernCounter);
/*
	if (StarShipPtr->RaceDescPtr->ship_info.energy_level <= SPECIAL_ENERGY_COST)
		StarShipPtr->ship_input_state &= ~WEAPON;
*/
}

static void
yehat_postprocess (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);

	/* Code copied from Chmmr */

	//BEGIN copied code	
	if ((StarShipPtr->cur_status_flags & SPECIAL)
			&& DeltaEnergy (ElementPtr, -SPECIAL_ENERGY_COST))
	{
		COUNT facing;
		ELEMENT *EnemyElementPtr;

		LockElement (ElementPtr->hTarget, &EnemyElementPtr);
		
		ProcessSound (SetAbsSoundIndex (
				StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1),
				 EnemyElementPtr);

		UnlockElement (ElementPtr->hTarget);

		facing = 0;
		if (TrackShip (ElementPtr, &facing) >= 0)
		{
			ELEMENT *EnemyElementPtr;

			LockElement (ElementPtr->hTarget, &EnemyElementPtr);

			/* TODO: verify this condition is actually necessary */
			if (!GRAVITY_MASS (EnemyElementPtr->mass_points + 1))
			{
				SIZE i, dx, dy;
				COUNT angle, magnitude;
				STARSHIP *EnemyStarShipPtr;
				static const SIZE shadow_offs[] =
				{
					DISPLAY_TO_WORLD (8),
					DISPLAY_TO_WORLD (8 + 9),
					DISPLAY_TO_WORLD (8 + 9 + 11),
					DISPLAY_TO_WORLD (8 + 9 + 11 + 14),
					DISPLAY_TO_WORLD (8 + 9 + 11 + 14 + 18),
				};
				static const Color color_tab[] =
				{
					BUILD_COLOR (MAKE_RGB15_INIT (0x00, 0x00, 0x10), 0x53),
					BUILD_COLOR (MAKE_RGB15_INIT (0x00, 0x00, 0x0E), 0x54),
					BUILD_COLOR (MAKE_RGB15_INIT (0x00, 0x00, 0x0C), 0x55),
					BUILD_COLOR (MAKE_RGB15_INIT (0x00, 0x00, 0x09), 0x56),
					BUILD_COLOR (MAKE_RGB15_INIT (0x00, 0x00, 0x07), 0x57),
				};
				DWORD current_speed, max_speed;

				GetElementStarShip (EnemyElementPtr, &EnemyStarShipPtr);

				// calculate tractor beam effect
				angle = FACING_TO_ANGLE (StarShipPtr->ShipFacing);

				/* These magic numbers were originally
				 * (LASER_RANGE / 3) and CHMMR_OFFSET - they
				 * still match those constants in chmmr.c
				 */
				dx = (EnemyElementPtr->next.location.x
						+ COSINE (angle, (DISPLAY_TO_WORLD (150) / 3)
						+ DISPLAY_TO_WORLD (18)))
						- ElementPtr->next.location.x;
				dy = (EnemyElementPtr->next.location.y
						+ SINE (angle, (DISPLAY_TO_WORLD (150) / 3)
						+ DISPLAY_TO_WORLD (18)))
						- ElementPtr->next.location.y;
				angle = ARCTAN (dx, dy);
				magnitude = WORLD_TO_VELOCITY (12) /
						ElementPtr->mass_points;
				DeltaVelocityComponents (&ElementPtr->velocity,
						COSINE (angle, magnitude), SINE (angle, magnitude));

				GetCurrentVelocityComponents (&ElementPtr->velocity,
						&dx, &dy);

				// set the effected ship's speed flags
				current_speed = VelocitySquared (dx, dy);
				max_speed = VelocitySquared (WORLD_TO_VELOCITY (
						StarShipPtr->RaceDescPtr->characteristics.max_thrust),
						0);
				StarShipPtr->cur_status_flags &= ~(SHIP_AT_MAX_SPEED
						| SHIP_BEYOND_MAX_SPEED);
				if (current_speed > max_speed)
					StarShipPtr->cur_status_flags |= (SHIP_AT_MAX_SPEED
							| SHIP_BEYOND_MAX_SPEED);
				else if (current_speed == max_speed)
					StarShipPtr->cur_status_flags |= SHIP_AT_MAX_SPEED;

				// add tractor beam graphical effects
				for (i = 0; i < NUM_SHADOWS; ++i)
				{
					HELEMENT hShadow;

					hShadow = AllocElement ();
					if (hShadow)
					{
						ELEMENT *ShadowElementPtr;

						LockElement (hShadow, &ShadowElementPtr);
						ShadowElementPtr->playerNr = ElementPtr->playerNr;
						ShadowElementPtr->state_flags = FINITE_LIFE | NONSOLID
								| IGNORE_SIMILAR | POST_PROCESS;
						ShadowElementPtr->life_span = 1;

						ShadowElementPtr->current = ElementPtr->next;
						ShadowElementPtr->current.location.x +=
								COSINE (angle, shadow_offs[i]);
						ShadowElementPtr->current.location.y +=
								SINE (angle, shadow_offs[i]);
						ShadowElementPtr->next = ShadowElementPtr->current;

						SetElementStarShip (ShadowElementPtr, StarShipPtr);
						SetVelocityComponents (&ShadowElementPtr->velocity,
								dx, dy);

						SetPrimType (&(GLOBAL (DisplayArray))[
								ShadowElementPtr->PrimIndex
								], STAMPFILL_PRIM);
						SetPrimColor (&(GLOBAL (DisplayArray))[
								ShadowElementPtr->PrimIndex
								], color_tab[i]);

						UnlockElement (hShadow);
						InsertElement (hShadow, GetHeadElement ());
					}
				}
			}
			UnlockElement (ElementPtr->hTarget);
		}
	}
	//END copied code
}

static void
yehat_collision (ELEMENT *ElementPtr0, POINT *pPt0,
		ELEMENT *ElementPtr1, POINT *pPt1)
{
	//shield also blocks planet damage
	if (
		!(ElementPtr1->state_flags & FINITE_LIFE)
		&& GRAVITY_MASS (ElementPtr1->mass_points) //it is a planet
		&& ElementPtr0->life_span > NORMAL_LIFE //and I have a shield
		)
	{
		ElementPtr0->state_flags |= COLLISION; //collision without damage
	}
	else
	{
		collision(ElementPtr0, pPt0, ElementPtr1, pPt1); //normal collision
	}
}

static void
yehat_preprocess (ELEMENT *ElementPtr)
{
	if (ElementPtr->state_flags & APPEARING)
	{
		ElementPtr->collision_func = yehat_collision;
	}
	else
	{
		STARSHIP *StarShipPtr;

		GetElementStarShip (ElementPtr, &StarShipPtr);
		if ((ElementPtr->life_span > NORMAL_LIFE
				/* take care of shield effect */
				&& --ElementPtr->life_span == NORMAL_LIFE)
				|| (ElementPtr->life_span == NORMAL_LIFE
				&& ElementPtr->next.image.farray
						== StarShipPtr->RaceDescPtr->ship_data.special))
		{
#ifdef NEVER
			SetPrimType (
					&(GLOBAL (DisplayArray))[ElementPtr->PrimIndex],
					STAMP_PRIM
					);
#endif /* NEVER */

			ElementPtr->next.image.farray = StarShipPtr->RaceDescPtr->ship_data.ship;
			ElementPtr->next.image.frame =
					SetEquFrameIndex (StarShipPtr->RaceDescPtr->ship_data.ship[0],
					ElementPtr->next.image.frame);
			ElementPtr->state_flags |= CHANGING;
		}

		if (StarShipPtr->special_counter == 0)
		{
			ElementPtr->life_span = SHIELD_LIFE + NORMAL_LIFE;

			ElementPtr->next.image.farray =
					StarShipPtr->RaceDescPtr->ship_data.special;
			ElementPtr->next.image.frame =
					SetEquFrameIndex (StarShipPtr->RaceDescPtr->ship_data.
					special[0], ElementPtr->next.image.frame);
			ElementPtr->state_flags |= CHANGING;

			StarShipPtr->special_counter =
					StarShipPtr->RaceDescPtr->characteristics.special_wait;
		}
	}
}

RACE_DESC*
init_yehat (void)
{
	RACE_DESC *RaceDescPtr;

	yehat_desc.preprocess_func = yehat_preprocess;
	yehat_desc.postprocess_func = yehat_postprocess;
	yehat_desc.init_weapon_func = initialize_standard_missiles;
	yehat_desc.cyborg_control.intelligence_func = yehat_intelligence;

	RaceDescPtr = &yehat_desc;

	return (RaceDescPtr);
}
