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
#include "ships/arilou/resinst.h"

#include "libs/mathlib.h"
#include "colors.h"
#include "globdata.h"


#define MAX_CREW 16 //6
#define MAX_ENERGY 20
#define ENERGY_REGENERATION 1
#define WEAPON_ENERGY_COST (MAX_ENERGY + 1) //make sure firing never does anything
#define SPECIAL_ENERGY_COST 3
#define ENERGY_WAIT 7 //6
#define MAX_THRUST /* DISPLAY_TO_WORLD (10) */ 50 //40
#define THRUST_INCREMENT 10 //MAX_THRUST
#define TURN_WAIT 1 //0
#define THRUST_WAIT 0
#define WEAPON_WAIT 255
#define SPECIAL_WAIT 2

#define SHIP_MASS 1
#define ARILOU_OFFSET 9

static RACE_DESC arilou_desc =
{
	{
		/* FIRES_FORE | */ IMMEDIATE_WEAPON,
		28, /* Super Melee cost */
		250 / SPHERE_RADIUS_INCREMENT, /* Initial sphere of influence radius */
		MAX_CREW, MAX_CREW,
		MAX_ENERGY, MAX_ENERGY,
		{
			438, 6372,
		},
		(STRING)ARILOU_RACE_STRINGS,
		(FRAME)ARILOU_ICON_MASK_PMAP_ANIM,
		(FRAME)ARILOU_MICON_MASK_PMAP_ANIM,
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
			(FRAME)ARILOU_BIG_MASK_PMAP_ANIM,
			(FRAME)ARILOU_MED_MASK_PMAP_ANIM,
			(FRAME)ARILOU_SML_MASK_PMAP_ANIM,
		},
		{
			(FRAME)0,
			(FRAME)0,
			(FRAME)0,
		},
		{
			(FRAME)WARP_BIG_MASK_PMAP_ANIM,
			(FRAME)WARP_MED_MASK_PMAP_ANIM,
			(FRAME)WARP_SML_MASK_PMAP_ANIM,
		},
		{
			(FRAME)ARILOU_CAPTAIN_MASK_PMAP_ANIM,
			(FRAME)0,
			(FRAME)0,
			(FRAME)0,
			(FRAME)0,
		},
		(SOUND)ARILOU_VICTORY_SONG,
		(SOUND)ARILOU_SHIP_SOUNDS,
	},
	{
		0,
		100,
		NULL_PTR,
	},
	(UNINIT_FUNC *) NULL,
	(PREPROCESS_FUNC *) NULL,
	(POSTPROCESS_FUNC *) NULL,
	(INIT_WEAPON_FUNC *) NULL,
	0,
};

static COUNT
initialize_nothing (PELEMENT ShipPtr, HELEMENT NothingArray[])
{	
	return (0);
}

//#define INVISIBILITY_ENERGY_WAIT (ENERGY_WAIT * 2)
#define CONFUSE_ENERGY_COST 7

#define SPELL_WAIT 5

FRAME demonGraphicsHack[3];

static COUNT arilou_present = 0;

static void
arilou_dispose_graphics (RACE_DESCPTR RaceDescPtr)
{
	--arilou_present;

	if(!arilou_present)
	{
		extern void clearGraphicsHack(FRAME farray[]);
		clearGraphicsHack(demonGraphicsHack);
	}
}

static void
arilou_intelligence (PELEMENT ShipPtr, PEVALUATE_DESC ObjectsOfConcern,
		COUNT ConcernCounter)
{
	STARSHIPPTR StarShipPtr;

	GetElementStarShip (ShipPtr, &StarShipPtr);

	ship_intelligence (ShipPtr, ObjectsOfConcern, ConcernCounter);
}

static void
demon_collision (PELEMENT ElementPtr0, PPOINT pPt0, PELEMENT ElementPtr1, PPOINT pPt1)
{
	ElementPtr0->hit_points = 10000;
	
	if(!(ElementPtr1->state_flags & FINITE_LIFE) && GRAVITY_MASS(ElementPtr1->mass_points))
	{

	}
	else
	{
		ZeroVelocityComponents (&ElementPtr1->velocity);
		if((ElementPtr1->state_flags & PLAYER_SHIP) && !ElementPtr1->thrust_wait)++ElementPtr1->thrust_wait;
		if((ElementPtr1->state_flags & PLAYER_SHIP) && !ElementPtr1->turn_wait)++ElementPtr1->turn_wait;
	}

}

static COUNT
detect_demons ()
{
	COUNT result;
	ELEMENTPTR ElementPtr;
	HELEMENT hElement, hNextElement;

	result = 0;
	for (hElement = GetHeadElement (); hElement != 0; hElement = hNextElement)
	{
		LockElement (hElement, &ElementPtr);
		hNextElement = GetSuccElement (ElementPtr);
		if(ElementPtr->current.image.farray == demonGraphicsHack)++result;
		UnlockElement (hElement);
	}
	return result;
}

static void spawn_demon (PELEMENT ElementPtr);

static void
demon_preprocess (PELEMENT ElementPtr)
{
	COUNT crawl_index, old_crawl_index, old_turn_wait;
	ElementPtr->next.image.frame = IncFrameIndex (ElementPtr->next.image.frame);
	ElementPtr->state_flags |= CHANGING;

	old_crawl_index = 2;
	if(ElementPtr->turn_wait > 100)
	{
		old_crawl_index += (ElementPtr->turn_wait - 100) / 10;
	}

	old_turn_wait = ElementPtr->turn_wait;
	if(!(TFB_Random() & 3))++ElementPtr->turn_wait;

	crawl_index = 2;
	if(ElementPtr->turn_wait > 100)
	{
		crawl_index += (ElementPtr->turn_wait - 100) / 10;
	}

	if((GetFrameIndex (ElementPtr->next.image.frame) == crawl_index) && !(old_turn_wait != ElementPtr->turn_wait && old_crawl_index != crawl_index))
	{
		spawn_demon(ElementPtr);
		if(!(TFB_Random() & 63) && detect_demons() < 64)spawn_demon(ElementPtr);
	}
}

static void
spawn_demon (PELEMENT ElementPtr)
{
	STARSHIPPTR StarShipPtr;
	MISSILE_BLOCK MissileBlock;
	HELEMENT hMissile;
	BOOLEAN isFirstSegment;

	isFirstSegment = ElementPtr->state_flags & PLAYER_SHIP;
	GetElementStarShip (ElementPtr, &StarShipPtr);
	MissileBlock.farray = demonGraphicsHack;
	MissileBlock.index = 0;
	if(isFirstSegment)
	{
		MissileBlock.face = StarShipPtr->ShipFacing;
	}
	else
	{	
		COUNT original_facing;
		original_facing = MissileBlock.face = ElementPtr->thrust_wait;
		TrackShip(ElementPtr, &MissileBlock.face);
		if(MissileBlock.face == original_facing)
		{
			MissileBlock.face = NORMALIZE_FACING(MissileBlock.face + (TFB_Random() % 3) - 1);
		}
		else if(TFB_Random() & 1)
		{
			if(TFB_Random() & 1)
			{
				MissileBlock.face = original_facing;
			}
			else MissileBlock.face = NORMALIZE_FACING(original_facing + original_facing - MissileBlock.face);
		}
	}
	
	MissileBlock.cx = ElementPtr->next.location.x
			+ COSINE(FACING_TO_ANGLE(MissileBlock.face), DISPLAY_TO_WORLD(18));
	MissileBlock.cy = ElementPtr->next.location.y
			+ SINE(FACING_TO_ANGLE(MissileBlock.face), DISPLAY_TO_WORLD(18));
	MissileBlock.sender = (ElementPtr->state_flags & (GOOD_GUY | BAD_GUY))
			| IGNORE_SIMILAR;
	MissileBlock.pixoffs = 0;
	MissileBlock.preprocess_func = demon_preprocess;
	MissileBlock.blast_offs = 0;
	MissileBlock.speed = 0;
	MissileBlock.hit_points = 10000;
	MissileBlock.damage = 1;
	MissileBlock.life = GetFrameCount(demonGraphicsHack[0]);

	hMissile = initialize_missile (&MissileBlock);

	if (hMissile)
	{
		ELEMENTPTR MissilePtr;
		LockElement(hMissile, &MissilePtr);
		SetElementStarShip (MissilePtr, StarShipPtr);
		MissilePtr->thrust_wait = MissileBlock.face;
		MissilePtr->turn_wait = isFirstSegment ? 0 : ElementPtr->turn_wait;
		MissilePtr->collision_func = demon_collision;
		UnlockElement(hMissile);
		PutElement(hMissile);
	}
	ElementPtr->hTarget = 0;
}

static void
asteroid_danger_collision (PELEMENT ElementPtr0, PPOINT pPt0, PELEMENT ElementPtr1, PPOINT pPt1)
{
	if(ElementPtr1->state_flags & (GOOD_GUY | BAD_GUY))weapon_collision(ElementPtr0, pPt0, ElementPtr1, pPt1);
}

static void
spawn_asteroid_dangers (PELEMENT ElementPtr)
{
	COUNT i;
	SIZE angle;
	STARSHIPPTR StarShipPtr;
	LASER_BLOCK LaserBlock;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	LaserBlock.face = 0;
	LaserBlock.sender = IGNORE_SIMILAR;
	LaserBlock.pixoffs = 0;
	LaserBlock.color = BUILD_COLOR (MAKE_RGB15 (0x1F, 0x0A, 0x0A), 0);
	LaserBlock.cx = ElementPtr->next.location.x;
	LaserBlock.cy = ElementPtr->next.location.y;

	angle = TFB_Random() & 63;

	for(i = 0; i < 3; ++i)
	{
		HELEMENT hLaser;
		LaserBlock.ex = COSINE(angle, 60);
		LaserBlock.ey = SINE(angle, 60);

		hLaser = initialize_laser (&LaserBlock);
		if(hLaser)
		{
			ELEMENTPTR LaserPtr;

			LockElement(hLaser, &LaserPtr);
			SetElementStarShip(LaserPtr, StarShipPtr);
			LaserPtr->collision_func = asteroid_danger_collision;
			UnlockElement(hLaser);
			PutElement(hLaser);
		}

		LaserBlock.cx += LaserBlock.ex;
		LaserBlock.cy += LaserBlock.ey;
		angle = NORMALIZE_ANGLE(angle + (TFB_Random() & 15) - (TFB_Random() & 15));
	}
}

/*static void
spawn_indicator_laser (PELEMENT ElementPtr)
{
	COUNT i;
	STARSHIPPTR StarShipPtr;
	LASER_BLOCK LaserBlock;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	LaserBlock.face = StarShipPtr->ShipFacing;
	LaserBlock.sender = (ElementPtr->state_flags & (GOOD_GUY | BAD_GUY))
			| IGNORE_SIMILAR;
	LaserBlock.pixoffs = 0;
	LaserBlock.color = BUILD_COLOR (MAKE_RGB15 (0x0F, 0x0F, 0x1F), 0);

	for(i = 0; i < 2; ++i)
	{
		HELEMENT hLaser;

		LaserBlock.cx = ElementPtr->next.location.x + COSINE(FACING_TO_ANGLE (LaserBlock.face + 4), 34 * (i * 2 - 1));
		LaserBlock.cy = ElementPtr->next.location.y + SINE(FACING_TO_ANGLE (LaserBlock.face + 4), 34 * (i * 2 - 1));
		LaserBlock.ex = COSINE(FACING_TO_ANGLE(NORMALIZE_FACING(LaserBlock.face - (i * 4 - 2))), square_root(34 * 34 * 2));
		LaserBlock.ey = SINE(FACING_TO_ANGLE(NORMALIZE_FACING(LaserBlock.face - (i * 4 - 2))), square_root(34 * 34 * 2));

		hLaser = initialize_laser (&LaserBlock);
		if(hLaser)
		{
			ELEMENTPTR LaserPtr;

			LockElement(hLaser, &LaserPtr);
			SetElementStarShip(LaserPtr, StarShipPtr);
			LaserPtr->mass_points = 0;
			UnlockElement(hLaser);
			PutElement(hLaser);
		}
	}
}*/

static void
fireball_particle_preprocess (PELEMENT ElementPtr)
{
	SetPrimColor (&DisplayArray[ElementPtr->PrimIndex],
			BUILD_COLOR (MAKE_RGB15 ((0x1F * ElementPtr->life_span / 15), (0x1F * ElementPtr->life_span / 31), (0x1F * ElementPtr->life_span / 31)), 0));
}

static void
planet_explosion (PELEMENT ElementPtr)
{
#define NUM_PARTICLES 45
	COUNT i;
	STARSHIPPTR StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);

	ElementPtr->collision_func =
			(void (*) (struct element *ElementPtr0, PPOINT pPt0,
			struct element *ElementPtr1, PPOINT pPt1)) weapon_collision;
	ElementPtr->preprocess_func = ElementPtr->postprocess_func = NULL_PTR;

	for(i = 0; i < NUM_PARTICLES; ++i)
	{
		HELEMENT hParticleElement;
		hParticleElement = AllocElement ();
		if(!hParticleElement)
		{
			break; //no sense failing more times
		}
		else
		{
			COUNT angle, magnitude;
			ELEMENTPTR ParticleElementPtr;
			LockElement (hParticleElement, &ParticleElementPtr);

			ParticleElementPtr->hit_points = 1;
			ParticleElementPtr->mass_points = 3;
			ParticleElementPtr->state_flags =
					APPEARING | FINITE_LIFE | (ElementPtr->state_flags & (GOOD_GUY | BAD_GUY))/* | IGNORE_SIMILAR*/;
			ParticleElementPtr->life_span = TFB_Random() & 31;
			SetPrimType (&DisplayArray[ParticleElementPtr->PrimIndex], LINE_PRIM);
			SetPrimColor (&DisplayArray[ParticleElementPtr->PrimIndex],
					BUILD_COLOR (MAKE_RGB15 (0x1F, 0x0F, 0x0F), 0));
			extern FRAME stars_in_space;
			ParticleElementPtr->current.image.frame = DecFrameIndex (stars_in_space);
			ParticleElementPtr->current.image.farray = &stars_in_space;
			ParticleElementPtr->preprocess_func = fireball_particle_preprocess;
			ParticleElementPtr->collision_func =
					(void (*) (struct element *ElementPtr0, PPOINT pPt0,
					struct element *ElementPtr1, PPOINT pPt1)) weapon_collision;
			ParticleElementPtr->blast_offset = 0;

			angle = TFB_Random() & (FULL_CIRCLE - 1);
			magnitude = WORLD_TO_VELOCITY ((TFB_Random() & 127) + 32);
			ParticleElementPtr->current.location.x = ElementPtr->current.location.x;
			ParticleElementPtr->current.location.y = ElementPtr->current.location.y;
			SetVelocityComponents (&ParticleElementPtr->velocity,
					COSINE (angle, magnitude),
					SINE (angle, magnitude));

			SetElementStarShip (ParticleElementPtr, StarShipPtr);

			UnlockElement(hParticleElement);
			PutElement(hParticleElement);
		}
	}
}

static void
arilou_death (PELEMENT ElementPtr)
{
	ELEMENTPTR PlanetElementPtr;
	HELEMENT hElement, hNextElement;
	for (hElement = GetHeadElement (); hElement != 0; hElement = hNextElement)
	{
		LockElement (hElement, &PlanetElementPtr);
		hNextElement = GetSuccElement (PlanetElementPtr);
		if(GRAVITY_MASS(PlanetElementPtr->mass_points))
		{
			do_damage(PlanetElementPtr, 1000);
		}
		UnlockElement (hElement);
	}

	ship_death(ElementPtr);
}

static void
arilou_postprocess (PELEMENT ElementPtr)
{
	STARSHIPPTR StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);

	if (!(ElementPtr->state_flags & NONSOLID))
	{
		//PPRIMITIVE lpPrim;
		ELEMENTPTR PlanetElementPtr;
		HELEMENT hElement, hNextElement;

		/*lpPrim = &(GLOBAL (DisplayArray))[ElementPtr->PrimIndex];
		if (GetPrimType (lpPrim) == STAMPFILL_PRIM)
		{
			spawn_indicator_laser(ElementPtr);
		}*/
	
		if (!detect_demons())
		{
			spawn_demon(ElementPtr);
		}
	
		for (hElement = GetHeadElement (); hElement != 0; hElement = hNextElement)
		{
			SIZE angle;
			angle = TFB_Random() & 63;
			LockElement (hElement, &PlanetElementPtr);
			hNextElement = GetSuccElement (PlanetElementPtr);
			DeltaVelocityComponents(&PlanetElementPtr->velocity, COSINE(angle, 5), SINE(angle, 5));
			if(GRAVITY_MASS(PlanetElementPtr->mass_points))
			{
				if(!(TFB_Random() & 1023))
				{
					planet_explosion(PlanetElementPtr);
					do_damage(PlanetElementPtr, 1000);
				}
			}
			else if(!(PlanetElementPtr->state_flags
					& (APPEARING | GOOD_GUY | BAD_GUY
					| PLAYER_SHIP | FINITE_LIFE))
					&& CollisionPossible (PlanetElementPtr, ElementPtr))
			{
				spawn_asteroid_dangers(PlanetElementPtr);
			}
			UnlockElement (hElement);
		}
	
		if ((StarShipPtr->cur_status_flags & WEAPON)
				&& StarShipPtr->special_counter == 0
				&& CleanDeltaEnergy (ElementPtr, -CONFUSE_ENERGY_COST))
		{
			extern BOOLEAN spawn_confusion(PELEMENT ShipPtr);
			if(spawn_confusion(ElementPtr));
			{
				StarShipPtr->special_counter = SPELL_WAIT;
			}
		}
	}
}

static void
arilou_preprocess (PELEMENT ElementPtr)
{
	STARSHIPPTR StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);

	if (ElementPtr->state_flags & APPEARING)
	{
		if(!arilou_present)
		{
			demonGraphicsHack[0] = CaptureDrawable(LoadCelFile("ipanims/lavaspot.ani"));
			demonGraphicsHack[1] = CaptureDrawable(LoadCelFile("ipanims/lavaspot.ani"));
			demonGraphicsHack[2] = CaptureDrawable(LoadCelFile("ipanims/lavaspot.ani"));
		}
		++arilou_present;

		ElementPtr->death_func = arilou_death;
	}

	++StarShipPtr->weapon_counter;

	if (!(ElementPtr->state_flags & NONSOLID))
	{
		if ((StarShipPtr->cur_status_flags & SPECIAL)
				&& !(StarShipPtr->old_status_flags & SPECIAL))
		{
			PPRIMITIVE lpPrim;
			lpPrim = &(GLOBAL (DisplayArray))[ElementPtr->PrimIndex];
			if (GetPrimType (lpPrim) == STAMPFILL_PRIM)
			{
				SetPrimType (lpPrim, STAMP_PRIM);
				//StarShipPtr->RaceDescPtr->characteristics.energy_wait = ENERGY_WAIT;
				ElementPtr->state_flags |= CHANGING;
			}
			else
			{
				SetPrimType (lpPrim, STAMPFILL_PRIM);
				SetPrimColor (lpPrim, BLACK_COLOR);
				//StarShipPtr->RaceDescPtr->characteristics.energy_wait = INVISIBILITY_ENERGY_WAIT;
				ElementPtr->state_flags |= CHANGING;
			}
		}
	}
}

RACE_DESCPTR
init_arilou (void)
{
	RACE_DESCPTR RaceDescPtr;

	arilou_desc.preprocess_func = arilou_preprocess;
	arilou_desc.postprocess_func = arilou_postprocess;
	arilou_desc.init_weapon_func = initialize_nothing;
	arilou_desc.cyborg_control.intelligence_func =
			(void (*) (PVOID ShipPtr, PVOID ObjectsOfConcern, COUNT
					ConcernCounter)) arilou_intelligence;

	RaceDescPtr = &arilou_desc;

	return (RaceDescPtr);
}

