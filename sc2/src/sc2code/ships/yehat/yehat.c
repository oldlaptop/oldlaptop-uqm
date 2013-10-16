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
#include "ships/yehat/resinst.h"

#include "colors.h"
#include "globdata.h"
#include "libs/mathlib.h"


#define MAX_CREW 20
#define MAX_ENERGY 10
#define ENERGY_REGENERATION 2
#define WEAPON_ENERGY_COST 1
#define SPECIAL_ENERGY_COST 0 //1 //3
#define ENERGY_WAIT 4 //6
#define MAX_THRUST 30
#define THRUST_INCREMENT 6
#define TURN_WAIT 2
#define THRUST_WAIT 2
#define WEAPON_WAIT 0
#define SPECIAL_WAIT 17 //15 //2

#define SHIP_MASS 3
#define MISSILE_SPEED DISPLAY_TO_WORLD (20)
#define MISSILE_LIFE 10

static RACE_DESC yehat_desc =
{
	{
		FIRES_FORE | SHIELD_DEFENSE,
		38, /* Super Melee cost */
		750 / SPHERE_RADIUS_INCREMENT, /* Initial sphere of influence radius */
		MAX_CREW, MAX_CREW,
		MAX_ENERGY, MAX_ENERGY,
		{
			4970, 40,
		},
		(STRING)YEHAT_RACE_STRINGS,
		(FRAME)YEHAT_ICON_MASK_PMAP_ANIM,
		(FRAME)YEHAT_MICON_MASK_PMAP_ANIM,
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
			(FRAME)YEHAT_BIG_MASK_PMAP_ANIM,
			(FRAME)YEHAT_MED_MASK_PMAP_ANIM,
			(FRAME)YEHAT_SML_MASK_PMAP_ANIM,
		},
		{
			(FRAME)YEHAT_CANNON_BIG_MASK_PMAP_ANIM,
			(FRAME)YEHAT_CANNON_MED_MASK_PMAP_ANIM,
			(FRAME)YEHAT_CANNON_SML_MASK_PMAP_ANIM,
		},
		{
			(FRAME)SHIELD_BIG_MASK_ANIM,
			(FRAME)SHIELD_MED_MASK_ANIM,
			(FRAME)SHIELD_SML_MASK_ANIM,
		},
		{
			(FRAME)YEHAT_CAPTAIN_MASK_PMAP_ANIM,
			(FRAME)0,
			(FRAME)0,
			(FRAME)0,
			(FRAME)0,
		},
		(SOUND)YEHAT_VICTORY_SONG,
		(SOUND)YEHAT_SHIP_SOUNDS,
	},
	{
		0,
		MISSILE_SPEED * MISSILE_LIFE / 3,
		NULL_PTR,
	},
	(UNINIT_FUNC *) NULL,
	(PREPROCESS_FUNC *) NULL,
	(POSTPROCESS_FUNC *) NULL,
	(INIT_WEAPON_FUNC *) NULL,
	0,
};

static COUNT
initialize_standard_missiles (PELEMENT ShipPtr, HELEMENT MissileArray[])
{
#define YEHAT_OFFSET 16
#define LAUNCH_OFFS DISPLAY_TO_WORLD (8)
#define MISSILE_HITS 1
#define MISSILE_DAMAGE 1
#define MISSILE_OFFSET 1
	SIZE offs_x, offs_y;
	STARSHIPPTR StarShipPtr;
	MISSILE_BLOCK MissileBlock;

	GetElementStarShip (ShipPtr, &StarShipPtr);
	MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.weapon;
	MissileBlock.face = MissileBlock.index = StarShipPtr->ShipFacing;
	MissileBlock.sender = (ShipPtr->state_flags & (GOOD_GUY | BAD_GUY))
			| IGNORE_SIMILAR;
	MissileBlock.pixoffs = YEHAT_OFFSET;
	MissileBlock.speed = MISSILE_SPEED;
	MissileBlock.hit_points = MISSILE_HITS;
	MissileBlock.damage = MISSILE_DAMAGE;
	MissileBlock.life = MISSILE_LIFE;
	MissileBlock.preprocess_func = NULL_PTR;
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
yehat_intelligence (PELEMENT ShipPtr, PEVALUATE_DESC ObjectsOfConcern, COUNT ConcernCounter)
{
	SIZE ShieldStatus;
	STARSHIPPTR StarShipPtr;
	PEVALUATE_DESC lpEvalDesc;

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
		STARSHIPPTR EnemyStarShipPtr;

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
yehat_postprocess (PELEMENT ElementPtr)
{
	STARSHIPPTR StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
				/* take care of shield effect */
	/*if (StarShipPtr->special_counter == SPECIAL_ENERGY_COST - 1)
	{
		if (ElementPtr->life_span == NORMAL_LIFE)
			StarShipPtr->special_counter = 0;
		else
		{
			ProcessSound (SetAbsSoundIndex (
							// YEHAT_SHIELD_ON
					StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1), ElementPtr);
			DeltaEnergy (ElementPtr, -SPECIAL_ENERGY_COST);
		}
	}*/


	//CODE COPIED FROM CHMMR
	
	
	if ((StarShipPtr->cur_status_flags & SPECIAL)
			&& DeltaEnergy (ElementPtr, -SPECIAL_ENERGY_COST))
	{
		COUNT facing;
		ELEMENTPTR EnemyElementPtr;

		LockElement (ElementPtr->hTarget, &EnemyElementPtr);
		
		ProcessSound (SetAbsSoundIndex (
				StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1),
				EnemyElementPtr);

		UnlockElement (ElementPtr->hTarget);

		facing = 0;
		if (TrackShip (ElementPtr, &facing) >= 0)
		{
#define NUM_SHADOWS 5
					
			ELEMENTPTR EnemyElementPtr;

			LockElement (ElementPtr->hTarget, &EnemyElementPtr);
			if (!GRAVITY_MASS (EnemyElementPtr->mass_points + 1))
			{
				SIZE i, dx, dy;
				COUNT angle, magnitude;
				STARSHIPPTR EnemyStarShipPtr;
				static const SIZE shadow_offs[] =
				{
					DISPLAY_TO_WORLD (8),
					DISPLAY_TO_WORLD (8 + 9),
					DISPLAY_TO_WORLD (8 + 9 + 11),
					DISPLAY_TO_WORLD (8 + 9 + 11 + 14),
					DISPLAY_TO_WORLD (8 + 9 + 11 + 14 + 18),
				};
				static const COLOR color_tab[] =
				{
					BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x10), 0x53),
					BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x0E), 0x54),
					BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x0C), 0x55),
					BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x09), 0x56),
					BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x07), 0x57),
				};
				DWORD current_speed, max_speed;

				GetElementStarShip (EnemyElementPtr, &EnemyStarShipPtr);

				// calculate tractor beam effect
				angle = FACING_TO_ANGLE (EnemyStarShipPtr->ShipFacing);
				dx = (EnemyElementPtr->next.location.x
						+ COSINE (angle, (DISPLAY_TO_WORLD (150) / 3)
						+ DISPLAY_TO_WORLD (18)))
						- ElementPtr->next.location.x;
				dy = (EnemyElementPtr->next.location.y
						+ SINE (angle, (DISPLAY_TO_WORLD (150) / 3)
						+ DISPLAY_TO_WORLD (18)))
						- ElementPtr->next.location.y;
				angle = ARCTAN (dx, dy);
				magnitude = WORLD_TO_VELOCITY (12) / ElementPtr->mass_points;
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
						ELEMENTPTR ShadowElementPtr;

						LockElement (hShadow, &ShadowElementPtr);

						ShadowElementPtr->state_flags =
								FINITE_LIFE | NONSOLID | IGNORE_SIMILAR | POST_PROCESS
								| (ElementPtr->state_flags & (GOOD_GUY | BAD_GUY));
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
	//END OF COPIED CODE


}

static void
yehat_collision (PELEMENT ElementPtr0, PPOINT pPt0,
		PELEMENT ElementPtr1, PPOINT pPt1)
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
yehat_preprocess (PELEMENT ElementPtr)
{
	if (ElementPtr->state_flags & APPEARING)
	{
		ElementPtr->collision_func = yehat_collision;
	}

	if (!(ElementPtr->state_flags & APPEARING))
	{
		STARSHIPPTR StarShipPtr;

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

		if (/*(StarShipPtr->cur_status_flags & SPECIAL)
				&& */StarShipPtr->special_counter == 0)
		{
			/*if (StarShipPtr->RaceDescPtr->ship_info.energy_level < SPECIAL_ENERGY_COST)
				DeltaEnergy (ElementPtr, -SPECIAL_ENERGY_COST);*/ /* so text will flash */
			/*else
			{*/
#define SHIELD_LIFE 10
				ElementPtr->life_span = SHIELD_LIFE + NORMAL_LIFE;

				ElementPtr->next.image.farray = StarShipPtr->RaceDescPtr->ship_data.special;
				ElementPtr->next.image.frame =
						SetEquFrameIndex (StarShipPtr->RaceDescPtr->ship_data.special[0],
						ElementPtr->next.image.frame);
				ElementPtr->state_flags |= CHANGING;

				StarShipPtr->special_counter =
						StarShipPtr->RaceDescPtr->characteristics.special_wait;
			/*}*/
		}
	}
}

RACE_DESCPTR
init_yehat (void)
{
	RACE_DESCPTR RaceDescPtr;

	yehat_desc.preprocess_func = yehat_preprocess;
	yehat_desc.postprocess_func = yehat_postprocess;
	yehat_desc.init_weapon_func = initialize_standard_missiles;
	yehat_desc.cyborg_control.intelligence_func =
			(void (*) (PVOID ShipPtr, PVOID ObjectsOfConcern, COUNT
					ConcernCounter)) yehat_intelligence;

	RaceDescPtr = &yehat_desc;

	return (RaceDescPtr);
}
