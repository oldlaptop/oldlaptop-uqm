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
#include "ships/shofixti/resinst.h"

#include "globdata.h"
#include "libs/mathlib.h"


#define MAX_CREW MAX_CREW_SIZE //6
#define MAX_ENERGY MAX_ENERGY_SIZE //4
#define ENERGY_REGENERATION 1
#define WEAPON_ENERGY_COST 1
#define SPECIAL_ENERGY_COST 20 //0
#define ENERGY_WAIT 2 //9
#define MAX_THRUST 28 //35
#define SCOUT_MAX_THRUST 35
#define THRUST_INCREMENT 5
#define SCOUT_THRUST_INCREMENT 5
#define TURN_WAIT 4 //1
#define THRUST_WAIT 2 //0
#define WEAPON_WAIT 0 //3
#define SPECIAL_WAIT 18 //0

#define SHIP_MASS 10 //1
#define MISSILE_SPEED DISPLAY_TO_WORLD (18) //35 //DISPLAY_TO_WORLD (40) //(24)
#define MISSILE_LIFE 64 /* actually, it's as long as you hold the button down. */

static FRAME carrierGraphicsHack[3];

static RACE_DESC shofixti_desc =
{
	{
		FIRES_FORE,
		25, /* Super Melee cost */
		0 / SPHERE_RADIUS_INCREMENT, /* Initial sphere of influence radius */
		MAX_CREW, MAX_CREW,
		MAX_ENERGY, MAX_ENERGY,
		{
			0, 0,
		},
		(STRING)SHOFIXTI_RACE_STRINGS,
		(FRAME)SHOFIXTI_ICON_MASK_PMAP_ANIM,
		(FRAME)SHOFIXTI_MICON_MASK_PMAP_ANIM,
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
			(FRAME)SHOFIXTI_BIG_MASK_PMAP_ANIM,
			(FRAME)SHOFIXTI_MED_MASK_PMAP_ANIM,
			(FRAME)SHOFIXTI_SML_MASK_PMAP_ANIM,
		},
		{
			(FRAME)DART_BIG_MASK_PMAP_ANIM,
			(FRAME)DART_MED_MASK_PMAP_ANIM,
			(FRAME)DART_SML_MASK_PMAP_ANIM,
		},
		{
			(FRAME)DESTRUCT_BIG_MASK_ANIM,
			(FRAME)DESTRUCT_MED_MASK_ANIM,
			(FRAME)DESTRUCT_SML_MASK_ANIM,
		},
		{
			(FRAME)SHOFIXTI_CAPTAIN_MASK_PMAP_ANIM,
			(FRAME)0,
			(FRAME)0,
			(FRAME)0,
			(FRAME)0,
		},
		(SOUND)SHOFIXTI_VICTORY_SONG,
		(SOUND)SHOFIXTI_SHIP_SOUNDS,
	},
	{
		0,
		MISSILE_SPEED * MISSILE_LIFE,
		NULL_PTR,
	},
	(UNINIT_FUNC *) NULL,
	(PREPROCESS_FUNC *) NULL,
	(POSTPROCESS_FUNC *) NULL,
	(INIT_WEAPON_FUNC *) NULL,
	0,
};

static void self_destruct (PELEMENT ElementPtr);

static void
shofixti_self_destruct_death (PELEMENT ElementPtr)
{
	ElementPtr->life_span = NUM_EXPLOSION_FRAMES * 3;
	ElementPtr->state_flags &= ~DISAPPEARING;
	ElementPtr->postprocess_func = ElementPtr->death_func = NULL_PTR;
}

void
thrust_hack (PELEMENT ElementPtr)
{
	STARSHIPPTR StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);

	extern void spawn_ion_trail (PELEMENT ElementPtr);
	//major hack!
	ElementPtr->state_flags |= PLAYER_SHIP;
	spawn_ion_trail (ElementPtr);
	ElementPtr->state_flags &= ~PLAYER_SHIP;

	//major hack!
	{
		COUNT orig_facing;
		DWORD current_speed, max_speed;
		ELEMENT_FLAGS orig_flags;
		SIZE dx, dy;

		orig_facing = StarShipPtr->ShipFacing;
		orig_flags = StarShipPtr->cur_status_flags;

		GetCurrentVelocityComponents (&ElementPtr->velocity, &dx, &dy);
		max_speed = VelocitySquared (WORLD_TO_VELOCITY (SCOUT_MAX_THRUST), 0);
		current_speed = VelocitySquared (dx, dy);

		if(current_speed == max_speed)StarShipPtr->cur_status_flags |= SHIP_AT_MAX_SPEED;
		else StarShipPtr->cur_status_flags &= ~SHIP_AT_MAX_SPEED;
		if(current_speed > max_speed)StarShipPtr->cur_status_flags |= SHIP_BEYOND_MAX_SPEED;
		else StarShipPtr->cur_status_flags &= ~SHIP_BEYOND_MAX_SPEED;
		if(CalculateGravity (ElementPtr))StarShipPtr->cur_status_flags |= SHIP_IN_GRAVITY_WELL;
		else StarShipPtr->cur_status_flags &= ~SHIP_IN_GRAVITY_WELL;
		StarShipPtr->ShipFacing = GetFrameIndex(ElementPtr->current.image.frame);
		StarShipPtr->RaceDescPtr->characteristics.max_thrust = SCOUT_MAX_THRUST;
		StarShipPtr->RaceDescPtr->characteristics.thrust_increment = SCOUT_THRUST_INCREMENT;

		inertial_thrust(ElementPtr);

		StarShipPtr->ShipFacing = orig_facing;
		StarShipPtr->cur_status_flags = orig_flags;
		StarShipPtr->RaceDescPtr->characteristics.max_thrust = MAX_THRUST;
		StarShipPtr->RaceDescPtr->characteristics.thrust_increment = THRUST_INCREMENT;
	}
}

static void
shofixti_fighter_preprocess (PELEMENT ElementPtr)
{
	thrust_hack(ElementPtr);
}

static void
shofixti_fighter_postprocess (PELEMENT ElementPtr)
{
	STARSHIPPTR StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);

	if(ElementPtr->turn_wait)--ElementPtr->turn_wait;

	if (StarShipPtr->cur_status_flags & SPECIAL)
	{
		++ElementPtr->life_span;
	}
	else if(!ElementPtr->turn_wait)
	{
		ZeroVelocityComponents (&ElementPtr->velocity);
		PlaySound (SetAbsSoundIndex (
				StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1),
				CalcSoundPosition (ElementPtr), ElementPtr,
				GAME_SOUND_PRIORITY + 1);
		ElementPtr->death_func = shofixti_self_destruct_death;

		self_destruct (ElementPtr);
	}
}

static void
shofixti_fighter_collision (PELEMENT ElementPtr0, PPOINT pPt0,
		PELEMENT ElementPtr1, PPOINT pPt1)
{
	if (!(ElementPtr1->state_flags & FINITE_LIFE))
	{
		ElementPtr0->state_flags |= COLLISION;
		if (GRAVITY_MASS (ElementPtr1->mass_points))
		{
			//blow up the planet!!!
			do_damage (ElementPtr1, 1000);

			ElementPtr0->state_flags |= DISAPPEARING;
			ElementPtr0->hit_points = 0;
			ElementPtr0->life_span = 0;
			ElementPtr0->state_flags |= COLLISION | NONSOLID;
		}
	}
}

static COUNT
initialize_standard_missile (PELEMENT ShipPtr, HELEMENT MissileArray[])
{
	STARSHIPPTR StarShipPtr;
	MISSILE_BLOCK MissileBlock;

	GetElementStarShip (ShipPtr, &StarShipPtr);
	MissileBlock.cx = ShipPtr->next.location.x;
	MissileBlock.cy = ShipPtr->next.location.y;
	MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.weapon;
	MissileBlock.face = MissileBlock.index = StarShipPtr->ShipFacing;
	MissileBlock.sender = (ShipPtr->state_flags & (GOOD_GUY | BAD_GUY))
			| IGNORE_SIMILAR;
	MissileBlock.pixoffs = 15;
	MissileBlock.speed = DISPLAY_TO_WORLD(24);
	MissileBlock.hit_points = 1;
	MissileBlock.damage = 2;
	MissileBlock.life = 10;
	MissileBlock.preprocess_func = NULL_PTR;
	MissileBlock.blast_offs = 1;

	MissileArray[0] = initialize_missile (&MissileBlock);
	MissileBlock.face = MissileBlock.index = StarShipPtr->ShipFacing + (((TFB_Random() & 1) << 1) - 1);
	MissileArray[1] = initialize_missile (&MissileBlock);

	return (2);
}

static void
spawn_shofixti_fighter (PELEMENT ShipPtr)
{
#define SHOFIXTI_OFFSET 15
#define FIGHTER_HITS 6
	SIZE i;
	HELEMENT Missile;
	STARSHIPPTR StarShipPtr;
	MISSILE_BLOCK MissileBlock;

	GetElementStarShip (ShipPtr, &StarShipPtr);
	MissileBlock.cx = ShipPtr->next.location.x;
	MissileBlock.cy = ShipPtr->next.location.y;
	MissileBlock.farray = /*carrierGraphicsHack;*/ StarShipPtr->RaceDescPtr->ship_data.ship;//weapon;
	MissileBlock.face = MissileBlock.index = StarShipPtr->ShipFacing;
	MissileBlock.sender = (ShipPtr->state_flags & (GOOD_GUY | BAD_GUY))
			| IGNORE_SIMILAR;
	MissileBlock.pixoffs = SHOFIXTI_OFFSET;
	MissileBlock.speed = 0;
	i = ShipPtr->crew_level > FIGHTER_HITS ? FIGHTER_HITS : (ShipPtr->crew_level - 2);
	DeltaCrew(ShipPtr, -i);
	MissileBlock.hit_points = i; //FIGHTER_HITS;
	MissileBlock.damage = 0;
	MissileBlock.life = MISSILE_LIFE;
	MissileBlock.preprocess_func = shofixti_fighter_preprocess;
	MissileBlock.blast_offs = 0;

	Missile = initialize_missile (&MissileBlock);
	if (Missile)
	{
		ELEMENTPTR MissilePtr;

		LockElement (Missile, &MissilePtr);
		SetElementStarShip(MissilePtr, StarShipPtr);
		MissilePtr->postprocess_func = shofixti_fighter_postprocess;
		MissilePtr->collision_func = shofixti_fighter_collision;
		MissilePtr->turn_wait = SPECIAL_WAIT;
		UnlockElement (Missile);
		PutElement (Missile);
	}
}

static void
destruct_preprocess (PELEMENT ElementPtr)
{
#define DESTRUCT_SWITCH ((NUM_EXPLOSION_FRAMES * 3) - 3)
	PPRIMITIVE lpPrim;

	lpPrim = &(GLOBAL (DisplayArray))[ElementPtr->PrimIndex];
	ElementPtr->state_flags |= CHANGING;
	if (ElementPtr->life_span > DESTRUCT_SWITCH)
	{
		SetPrimType (lpPrim, STAMPFILL_PRIM);
		if (ElementPtr->life_span == DESTRUCT_SWITCH + 2)
			SetPrimColor (lpPrim, BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x0A), 0x0E));
		else
			SetPrimColor (lpPrim, BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x1F), 0x0F));
	}
	else if (ElementPtr->life_span < DESTRUCT_SWITCH)
	{
		ElementPtr->next.image.frame =
				IncFrameIndex (ElementPtr->current.image.frame);
		if (GetPrimColor (lpPrim) == BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x1F), 0x0F))
			SetPrimColor (lpPrim, BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x0A), 0x0E));
		else if (GetPrimColor (lpPrim) == BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x0A), 0x0E))
			SetPrimColor (lpPrim, BUILD_COLOR (MAKE_RGB15 (0x1F, 0x0A, 0x0A), 0x0C));
		else if (GetPrimColor (lpPrim) == BUILD_COLOR (MAKE_RGB15 (0x1F, 0x0A, 0x0A), 0x0C))
			SetPrimColor (lpPrim, BUILD_COLOR (MAKE_RGB15 (0x14, 0x0A, 0x00), 0x06));
		else if (GetPrimColor (lpPrim) == BUILD_COLOR (MAKE_RGB15 (0x14, 0x0A, 0x00), 0x06))
			SetPrimColor (lpPrim, BUILD_COLOR (MAKE_RGB15 (0x14, 0x00, 0x00), 0x04));
	}
	else
	{
		HELEMENT hDestruct;

		SetPrimType (lpPrim, NO_PRIM);
		ElementPtr->preprocess_func = NULL_PTR;

		hDestruct = AllocElement ();
		if (hDestruct)
		{
			ELEMENTPTR DestructPtr;
			STARSHIPPTR StarShipPtr;

			GetElementStarShip (ElementPtr, &StarShipPtr);

			PutElement (hDestruct);
			LockElement (hDestruct, &DestructPtr);
			SetElementStarShip (DestructPtr, StarShipPtr);
			DestructPtr->hit_points = DestructPtr->mass_points = 0;
			DestructPtr->state_flags = APPEARING | FINITE_LIFE | NONSOLID;
			DestructPtr->life_span = (NUM_EXPLOSION_FRAMES - 3) - 1;
			SetPrimType (
					&(GLOBAL (DisplayArray))[DestructPtr->PrimIndex],
					STAMPFILL_PRIM
					);
			SetPrimColor (
					&(GLOBAL (DisplayArray))[DestructPtr->PrimIndex],
					BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x1F), 0x0F)
					);
			DestructPtr->current.image.farray = StarShipPtr->RaceDescPtr->ship_data.special;
			DestructPtr->current.image.frame = StarShipPtr->RaceDescPtr->ship_data.special[0];
			DestructPtr->current.location = ElementPtr->current.location;
			{
				DestructPtr->preprocess_func = destruct_preprocess;
			}
			DestructPtr->postprocess_func =
					DestructPtr->death_func = NULL_PTR;
			ZeroVelocityComponents (&DestructPtr->velocity);
			UnlockElement (hDestruct);
		}
	}
}

/* In order to detect any Orz Marines that have boarded the ship
   when it self-destructs, we'll need to see these Orz functions */
void intruder_preprocess (PELEMENT);
void marine_collision (PELEMENT, PPOINT, PELEMENT, PPOINT);
#define ORZ_MARINE(ptr) (ptr->preprocess_func == intruder_preprocess && \
		ptr->collision_func == marine_collision)

static void
self_destruct (PELEMENT ElementPtr)
{
	STARSHIPPTR StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	if (ElementPtr->state_flags & PLAYER_SHIP || ElementPtr->postprocess_func == shofixti_fighter_postprocess)
	{
		HELEMENT hDestruct;
		
		hDestruct = AllocElement ();
		if (hDestruct)
		{
			ELEMENTPTR DestructPtr;

			LockElement (hDestruct, &DestructPtr);
			DestructPtr->state_flags = APPEARING | NONSOLID | FINITE_LIFE |
					(ElementPtr->state_flags & (GOOD_GUY | BAD_GUY));
			DestructPtr->next.location = ElementPtr->next.location;
			DestructPtr->life_span = 0;
			DestructPtr->pParent = ElementPtr->pParent;
			DestructPtr->hTarget = 0;

			DestructPtr->death_func = self_destruct;

			UnlockElement (hDestruct);

			PutElement (hDestruct);
		}

		ElementPtr->state_flags |= NONSOLID;
		ElementPtr->life_span = 0;

		ElementPtr->preprocess_func = destruct_preprocess;
	}
	else
	{
		HELEMENT hElement, hNextElement;

		for (hElement = GetHeadElement ();
				hElement != 0; hElement = hNextElement)
		{
			ELEMENTPTR ObjPtr;

			LockElement (hElement, &ObjPtr);
			hNextElement = GetSuccElement (ObjPtr);

			//if((ObjPtr->state_flags & (GOOD_GUY | BAD_GUY)) != (ElementPtr->state_flags & (GOOD_GUY | BAD_GUY)))

			if (CollidingElement (ObjPtr) || ORZ_MARINE (ObjPtr))
			{
#define DESTRUCT_RANGE 180
				SIZE delta_x, delta_y;
				DWORD dist;

				if ((delta_x = ObjPtr->next.location.x
						- ElementPtr->next.location.x) < 0)
					delta_x = -delta_x;
				if ((delta_y = ObjPtr->next.location.y
						- ElementPtr->next.location.y) < 0)
					delta_y = -delta_y;
				delta_x = WORLD_TO_DISPLAY (delta_x);
				delta_y = WORLD_TO_DISPLAY (delta_y);
				if (delta_x <= DESTRUCT_RANGE && delta_y <= DESTRUCT_RANGE
						&& (dist = (DWORD)(delta_x * delta_x)
						+ (DWORD)(delta_y * delta_y)) <=
						(DWORD)(DESTRUCT_RANGE * DESTRUCT_RANGE))
				{
#define MAX_DESTRUCTION (DESTRUCT_RANGE / 10)
					SIZE destruction;

					destruction = ((MAX_DESTRUCTION
							* (DESTRUCT_RANGE - square_root (dist)))
							/ DESTRUCT_RANGE) + 1;

					if (ObjPtr->state_flags & PLAYER_SHIP)
					{
						if (!DeltaCrew (ObjPtr, -destruction))
							ObjPtr->life_span = 0;
					}
					else// if (!GRAVITY_MASS (ObjPtr->mass_points))
					{
						if ((BYTE)destruction < ObjPtr->hit_points || ORZ_MARINE (ObjPtr))
							ObjPtr->hit_points -= (BYTE)destruction;
						else
						{
							ObjPtr->hit_points = 0;
							ObjPtr->life_span = 0;
						}
					}
				}
			}

			UnlockElement (hElement);
		}
	}
}

static void
shofixti_intelligence (PELEMENT ShipPtr, PEVALUATE_DESC ObjectsOfConcern, COUNT ConcernCounter)
{
	STARSHIPPTR StarShipPtr;

	ship_intelligence (ShipPtr,
			ObjectsOfConcern, ConcernCounter);

	GetElementStarShip (ShipPtr, &StarShipPtr);
	if (StarShipPtr->special_counter == 0)
	{
		if (StarShipPtr->ship_input_state & SPECIAL)
			StarShipPtr->ship_input_state &= ~SPECIAL;
		else
		{
			PEVALUATE_DESC lpWeaponEvalDesc, lpShipEvalDesc;

			lpWeaponEvalDesc = &ObjectsOfConcern[ENEMY_WEAPON_INDEX];
			lpShipEvalDesc = &ObjectsOfConcern[ENEMY_SHIP_INDEX];
			if (StarShipPtr->RaceDescPtr->ship_data.special[0]
					&& (GetFrameCount (StarShipPtr->RaceDescPtr->ship_data.captain_control.special)
					- GetFrameIndex (StarShipPtr->RaceDescPtr->ship_data.captain_control.special) > 5
					|| (lpShipEvalDesc->ObjectPtr != NULL_PTR
					&& lpShipEvalDesc->which_turn <= 4)
					|| (lpWeaponEvalDesc->ObjectPtr != NULL_PTR
								/* means IMMEDIATE WEAPON */
					&& (((lpWeaponEvalDesc->ObjectPtr->state_flags & PLAYER_SHIP)
					&& ShipPtr->crew_level == 1)
					|| (PlotIntercept (lpWeaponEvalDesc->ObjectPtr, ShipPtr, 2, 0)
					&& lpWeaponEvalDesc->ObjectPtr->mass_points >= ShipPtr->crew_level
					&& (TFB_Random () & 1))))))
				StarShipPtr->ship_input_state |= SPECIAL;
		}
	}
}

static void
shofixti_postprocess (PELEMENT ElementPtr)
{
	STARSHIPPTR StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	/*if ((StarShipPtr->cur_status_flags
			^ StarShipPtr->old_status_flags) & SPECIAL)
	{
		StarShipPtr->RaceDescPtr->ship_data.captain_control.special =
				IncFrameIndex (StarShipPtr->RaceDescPtr->ship_data.captain_control.special);
		if (GetFrameCount (StarShipPtr->RaceDescPtr->ship_data.captain_control.special)
				- GetFrameIndex (StarShipPtr->RaceDescPtr->ship_data.captain_control.special) == 3)
			self_destruct (ElementPtr);
	}*/

	if ((StarShipPtr->cur_status_flags & SPECIAL)
			&& ElementPtr->crew_level > 2
			&& StarShipPtr->special_counter == 0
			&& DeltaEnergy (ElementPtr, -SPECIAL_ENERGY_COST))
	{
		/*ProcessSound (SetAbsSoundIndex (
						// LAUNCH_FIGHTERS
				StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1), ElementPtr);*/
		spawn_shofixti_fighter (ElementPtr);

		StarShipPtr->special_counter = SPECIAL_WAIT;
	}
	
	//shofixti reproduce fast ;)
	{
		/*COUNT i;
		for(i = 0; i < (ElementPtr->crew_level / 2); ++i)
		{
			if(!(TFB_Random() & 127))
			{
				DeltaCrew(ElementPtr, 1);
			}
		}*/
		if(!(TFB_Random() & 31))DeltaCrew(ElementPtr, 1);
	}
}

static COUNT shofixties_present = 0;

void
clearGraphicsHack (FRAME farray[])
{
	DestroyDrawable(ReleaseDrawable(farray[0]));
	farray[0] = (FRAME)0;
	DestroyDrawable(ReleaseDrawable(farray[1]));
	farray[1] = (FRAME)0;
	DestroyDrawable(ReleaseDrawable(farray[2]));
	farray[2] = (FRAME)0;
}

static void
shofixti_dispose_graphics (RACE_DESCPTR RaceDescPtr)
{
	--shofixties_present;

	if(!shofixties_present)
	{
		clearGraphicsHack(carrierGraphicsHack);
	}
}

static void
shofixti_preprocess (PELEMENT ElementPtr)
{
	STARSHIPPTR StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);

	if (ElementPtr->state_flags & APPEARING)
	{
		if(!shofixties_present)
		{
			carrierGraphicsHack[0] = CaptureDrawable(LoadCelFile("sis_ship/sis.big"));
			carrierGraphicsHack[1] = CaptureDrawable(LoadCelFile("sis_ship/sis.med"));
			carrierGraphicsHack[2] = CaptureDrawable(LoadCelFile("sis_ship/sis.sml"));
		}
		++shofixties_present;
		ElementPtr->next.image.farray = carrierGraphicsHack;
		ElementPtr->next.image.frame =
						SetEquFrameIndex (carrierGraphicsHack[0],
						ElementPtr->next.image.frame);
		ElementPtr->state_flags |= CHANGING;
	}

	if (StarShipPtr->special_counter == 1
			&& (StarShipPtr->cur_status_flags
			& StarShipPtr->old_status_flags & SPECIAL))
		++StarShipPtr->special_counter;
}

RACE_DESCPTR
init_shofixti (void)
{
	RACE_DESCPTR RaceDescPtr;

	static RACE_DESC new_shofixti_desc;

	shofixti_desc.preprocess_func = shofixti_preprocess;
	shofixti_desc.postprocess_func = shofixti_postprocess;
	shofixti_desc.init_weapon_func = initialize_standard_missile;
	shofixti_desc.uninit_func = shofixti_dispose_graphics;
	shofixti_desc.cyborg_control.intelligence_func =
			(void (*) (PVOID ShipPtr, PVOID ObjectsOfConcern, COUNT
					ConcernCounter)) shofixti_intelligence;

	new_shofixti_desc = shofixti_desc;
	if (LOBYTE (GLOBAL (CurrentActivity)) == IN_ENCOUNTER
			&& !GET_GAME_STATE (SHOFIXTI_RECRUITED))
	{
#define NUM_LIMPETS 3
		COUNT i;

		new_shofixti_desc.ship_data.ship[0] = (FRAME)OLDSHOF_BIG_MASK_PMAP_ANIM;
		new_shofixti_desc.ship_data.ship[1] = (FRAME)OLDSHOF_MED_MASK_PMAP_ANIM;
		new_shofixti_desc.ship_data.ship[2] = (FRAME)OLDSHOF_SML_MASK_PMAP_ANIM;
		new_shofixti_desc.ship_data.special[0] =
				new_shofixti_desc.ship_data.special[1] =
				new_shofixti_desc.ship_data.special[2] = (FRAME)0;
		new_shofixti_desc.ship_data.captain_control.background =
				(FRAME)OLDSHOF_CAPTAIN_MASK_PMAP_ANIM;

				/* weapon doesn't work as well */
		new_shofixti_desc.characteristics.weapon_wait = 10;
				/* simulate VUX limpets */
		for (i = 0; i < NUM_LIMPETS; ++i)
		{
			if (++new_shofixti_desc.characteristics.turn_wait == 0)
				--new_shofixti_desc.characteristics.turn_wait;
			if (++new_shofixti_desc.characteristics.thrust_wait == 0)
				--new_shofixti_desc.characteristics.thrust_wait;
#define MIN_THRUST_INCREMENT DISPLAY_TO_WORLD (1)
			if (new_shofixti_desc.characteristics.thrust_increment <= MIN_THRUST_INCREMENT)
			{
				new_shofixti_desc.characteristics.max_thrust =
						new_shofixti_desc.characteristics.thrust_increment << 1;
			}
			else
			{
				COUNT num_thrusts;

				num_thrusts = new_shofixti_desc.characteristics.max_thrust /
						new_shofixti_desc.characteristics.thrust_increment;
				--new_shofixti_desc.characteristics.thrust_increment;
				new_shofixti_desc.characteristics.max_thrust =
						new_shofixti_desc.characteristics.thrust_increment * num_thrusts;
			}
		}
	}

	RaceDescPtr = &new_shofixti_desc;

	return (RaceDescPtr);
}

