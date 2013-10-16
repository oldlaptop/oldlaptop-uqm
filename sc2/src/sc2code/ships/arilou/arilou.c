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

#define MISSILE_ENERGY_COST 2 //right primary
#define FIREBALL_ENERGY_COST 10 //left primary
#define DEMON_ENERGY_COST 15 //left secondary
#define TELEPORT_ENERGY_COST 7 //thrust secondary
#define INVISIBILITY_ENERGY_WAIT (ENERGY_WAIT * 2) //right secondary
#define CONFUSE_ENERGY_COST 4 //thrust primary

#define SPELL_WAIT 5

FRAME missileGraphicsHack[3];
FRAME fireballGraphicsHack[3];
//FRAME confuseGraphicsHack[3];
FRAME demonGraphicsHack[3];

static COUNT arilou_present = 0;

static void
arilou_dispose_graphics (RACE_DESCPTR RaceDescPtr)
{
	--arilou_present;

	if(!arilou_present)
	{
		extern void clearGraphicsHack(FRAME farray[]);
		clearGraphicsHack(missileGraphicsHack);
		clearGraphicsHack(fireballGraphicsHack);
		//clearGraphicsHack(confuseGraphicsHack);
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

	if(StarShipPtr->RaceDescPtr->ship_info.energy_level >= DEMON_ENERGY_COST)
	{
		StarShipPtr->ship_input_state &= ~(SPECIAL | RIGHT | THRUST);
		StarShipPtr->ship_input_state |= WEAPON | LEFT;
	}
}

static void
spawn_magic_missile (PELEMENT ElementPtr)
{
#define MAGIC_MISSILE_SPEED DISPLAY_TO_WORLD(40)
	COUNT orig_facing;
	SIZE delta_facing;
	STARSHIPPTR StarShipPtr;
	MISSILE_BLOCK MissileBlock;
	HELEMENT hMissile;
	COUNT real_angle;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	MissileBlock.farray = missileGraphicsHack;
	MissileBlock.face = orig_facing = StarShipPtr->ShipFacing;
	if ((delta_facing = TrackShip (ElementPtr, &MissileBlock.face)) > 0)
	{
		if(ElementPtr->hTarget)
		{
			ELEMENTPTR EnemyPtr;
			COUNT num_frames;
			COUNT e;
			SIZE delta_x, delta_y;

			LockElement (ElementPtr->hTarget, &EnemyPtr);
			delta_x = EnemyPtr->next.location.x
					- ElementPtr->next.location.x;
			delta_x = WRAP_DELTA_X (delta_x);
			delta_y = EnemyPtr->next.location.y
					- ElementPtr->next.location.y;
			delta_y = WRAP_DELTA_Y (delta_y);
	
			num_frames = (square_root ((long)delta_x * delta_x
					+ (long)delta_y * delta_y)) / MAGIC_MISSILE_SPEED;
			if (num_frames == 0)
				num_frames = 1;
	
			//GetNextVelocityComponents (&EnemyPtr->velocity,
			//		&delta_x, &delta_y, num_frames);
	e = (COUNT)((COUNT)EnemyPtr->velocity.error.width +
			((COUNT)EnemyPtr->velocity.fract.width * num_frames));
	delta_x = (EnemyPtr->velocity.vector.width * num_frames)
			+ ((SIZE)((SBYTE)LOBYTE (EnemyPtr->velocity.incr.width))
			* (e >> VELOCITY_SHIFT));

	e = (COUNT)((COUNT)EnemyPtr->velocity.error.height +
			((COUNT)EnemyPtr->velocity.fract.height * num_frames));
	delta_y = (EnemyPtr->velocity.vector.height * num_frames)
			+ ((SIZE)((SBYTE)LOBYTE (EnemyPtr->velocity.incr.height))
			* (e >> VELOCITY_SHIFT));

	
			delta_x = (EnemyPtr->next.location.x + (delta_x / 2))
					- ElementPtr->next.location.x;
			delta_y = (EnemyPtr->next.location.y + (delta_y / 2))
					- ElementPtr->next.location.y;
	
			real_angle = NORMALIZE_ANGLE(ARCTAN (delta_x, delta_y) + (((TFB_Random() & 3) + 1) / 2) - 1);

			MissileBlock.face = MissileBlock.index = ANGLE_TO_FACING (real_angle);

			UnlockElement(ElementPtr->hTarget);
		}
		else MissileBlock.face = MissileBlock.index = NORMALIZE_FACING (orig_facing + delta_facing);
	}
	else MissileBlock.face = MissileBlock.index = StarShipPtr->ShipFacing;

	MissileBlock.cx = ElementPtr->next.location.x;
	MissileBlock.cy = ElementPtr->next.location.y;
	MissileBlock.sender = (ElementPtr->state_flags & (GOOD_GUY | BAD_GUY))
			| IGNORE_SIMILAR;
	MissileBlock.pixoffs = ARILOU_OFFSET;
	MissileBlock.preprocess_func = NULL_PTR;
	MissileBlock.blast_offs = 4;
	MissileBlock.speed = MAGIC_MISSILE_SPEED;
	MissileBlock.hit_points = 2;
	MissileBlock.damage = 2;
	MissileBlock.life = 12;

	hMissile = initialize_missile (&MissileBlock);

	if (hMissile)
	{
		ELEMENTPTR MissilePtr;
		LockElement(hMissile, &MissilePtr);
		SetElementStarShip (MissilePtr, StarShipPtr);
		if(ElementPtr->hTarget)
		{
			SetVelocityComponents(&MissilePtr->velocity,
					COSINE(real_angle, WORLD_TO_VELOCITY(MAGIC_MISSILE_SPEED)),
					SINE(real_angle, WORLD_TO_VELOCITY(MAGIC_MISSILE_SPEED)));
		}
		UnlockElement(hMissile);
		PutElement(hMissile);
	}
	ElementPtr->hTarget = 0;
}

static void
fireball_particle_preprocess (PELEMENT ElementPtr)
{
	SetPrimColor (&DisplayArray[ElementPtr->PrimIndex],
			BUILD_COLOR (MAKE_RGB15 ((0x1F * ElementPtr->life_span / 15), (0x1F * ElementPtr->life_span / 31), (0x1F * ElementPtr->life_span / 31)), 0));
	ElementPtr->mass_points = 3;
}

static void
fireball_explode (PELEMENT ElementPtr)
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
			ParticleElementPtr->mass_points = 1;
			ParticleElementPtr->state_flags =
					APPEARING | FINITE_LIFE | (ElementPtr->state_flags & (GOOD_GUY | BAD_GUY)) | IGNORE_SIMILAR;
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
fireball_explode_collision (PELEMENT ElementPtr0, PPOINT pPt0, PELEMENT ElementPtr1, PPOINT pPt1)
{
	fireball_explode(ElementPtr0);
	weapon_collision(ElementPtr0, pPt0, ElementPtr1, pPt1);
}

static void
fireball_preprocess (PELEMENT ElementPtr)
{
	if(ElementPtr->life_span == 1)
	{
		ElementPtr->postprocess_func = fireball_explode;
	}
}

static void
spawn_fireball (PELEMENT ElementPtr)
{
	HELEMENT hFireball;
	STARSHIPPTR StarShipPtr;
	MISSILE_BLOCK MissileBlock;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	MissileBlock.cx = ElementPtr->next.location.x;
	MissileBlock.cy = ElementPtr->next.location.y;
	MissileBlock.farray = fireballGraphicsHack;
	MissileBlock.face = MissileBlock.index = StarShipPtr->ShipFacing;
	MissileBlock.sender = (ElementPtr->state_flags & (GOOD_GUY | BAD_GUY))
			| IGNORE_SIMILAR;
	MissileBlock.pixoffs = ARILOU_OFFSET;
	MissileBlock.speed = DISPLAY_TO_WORLD(40);
	MissileBlock.hit_points = 1;
	MissileBlock.damage = 1;
	MissileBlock.life = 12;
	MissileBlock.preprocess_func = fireball_preprocess;
	MissileBlock.blast_offs = 8;
	hFireball = initialize_missile (&MissileBlock);

	if (hFireball)
	{
		ELEMENTPTR FireballPtr;

		LockElement (hFireball, &FireballPtr);
		FireballPtr->collision_func = fireball_explode_collision;
		SetElementStarShip (FireballPtr, StarShipPtr);
		UnlockElement (hFireball);
		PutElement (hFireball);
	}
}

static void
demon_collision (PELEMENT ElementPtr0, PPOINT pPt0, PELEMENT ElementPtr1, PPOINT pPt1)
{
	ElementPtr0->hit_points = 10000;
	/*if((ElementPtr1->state_flags & PLAYER_SHIP) || (!(ElementPtr1->state_flags & FINITE_LIFE) && GRAVITY_MASS(ElementPtr1->mass_points)))
	{*/
	/*if(GetFrameIndex (ElementPtr0->next.image.frame) == 3
			|| GetFrameIndex (ElementPtr0->next.image.frame) == 5
			|| GetFrameIndex (ElementPtr0->next.image.frame) == 7)*/
	/*{
		COUNT damage;
		damage = ElementPtr0->mass_points;
		do_damage(ElementPtr1, damage);
		damage = TARGET_DAMAGED_FOR_1_PT + (damage >> 1);
		if (damage > TARGET_DAMAGED_FOR_6_PLUS_PT)
			damage = TARGET_DAMAGED_FOR_6_PLUS_PT;
		ProcessSound (SetAbsSoundIndex (GameSounds, damage),
				ElementPtr1);
		}*/
	/*}
	else
	{
		weapon_collision(ElementPtr0, pPt0, ElementPtr1, pPt1);
	}*/

	if(!(ElementPtr1->state_flags & FINITE_LIFE) && GRAVITY_MASS(ElementPtr1->mass_points))
	{

	}
	else
	{
		/*SetVelocityComponents (&ElementPtr1->velocity,
				COSINE (FACING_TO_ANGLE(ElementPtr0->thrust_wait), WORLD_TO_VELOCITY(DISPLAY_TO_WORLD(16))),
				SINE (FACING_TO_ANGLE(ElementPtr0->thrust_wait), WORLD_TO_VELOCITY(DISPLAY_TO_WORLD(16))));*/
		ZeroVelocityComponents (&ElementPtr1->velocity);
		if(!ElementPtr1->thrust_wait)++ElementPtr1->thrust_wait;
		if(!ElementPtr1->turn_wait)++ElementPtr1->turn_wait;
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
}

static void
arilou_postprocess (PELEMENT ElementPtr)
{
	STARSHIPPTR StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);

	if (!(ElementPtr->state_flags & NONSOLID))
	{
		COUNT num_keys;
		PPRIMITIVE lpPrim;
		lpPrim = &(GLOBAL (DisplayArray))[ElementPtr->PrimIndex];
		if (GetPrimType (lpPrim) == STAMPFILL_PRIM)
		{
			spawn_indicator_laser(ElementPtr);
		}
		
		num_keys = 0;
		if(StarShipPtr->cur_status_flags & WEAPON)++num_keys;
		if(StarShipPtr->cur_status_flags & SPECIAL)++num_keys;
		if(StarShipPtr->cur_status_flags & THRUST)++num_keys;
		if(StarShipPtr->cur_status_flags & LEFT)++num_keys;
		if(StarShipPtr->cur_status_flags & RIGHT)++num_keys;
		if(num_keys > 2)goto dont_cast_anything;

		if ((StarShipPtr->cur_status_flags & WEAPON)
				&& (StarShipPtr->cur_status_flags & RIGHT)
				&& StarShipPtr->special_counter == 0
				&& CleanDeltaEnergy (ElementPtr, -MISSILE_ENERGY_COST))
		{
			spawn_magic_missile(ElementPtr);
			StarShipPtr->special_counter = SPELL_WAIT;
		}
	
		if ((StarShipPtr->cur_status_flags & WEAPON)
				&& (StarShipPtr->cur_status_flags & LEFT)
				&& StarShipPtr->special_counter == 0
				&& CleanDeltaEnergy (ElementPtr, -FIREBALL_ENERGY_COST))
		{
			spawn_fireball(ElementPtr);
			StarShipPtr->special_counter = SPELL_WAIT;
		}
	
		if ((StarShipPtr->cur_status_flags & SPECIAL)
				&& (StarShipPtr->cur_status_flags & LEFT)
				&& StarShipPtr->special_counter == 0
				&& !detect_demons()
				&& CleanDeltaEnergy (ElementPtr, -DEMON_ENERGY_COST))
		{
			spawn_demon(ElementPtr);
			StarShipPtr->special_counter = SPELL_WAIT;
		}
	
		if ((StarShipPtr->cur_status_flags & WEAPON)
				&& (StarShipPtr->cur_status_flags & THRUST)
				&& StarShipPtr->special_counter == 0
				&& CleanDeltaEnergy (ElementPtr, -CONFUSE_ENERGY_COST))
		{
			extern BOOLEAN spawn_confusion(PELEMENT ShipPtr/*, FRAME farray[]*/);
			if(spawn_confusion(ElementPtr/*, confuseGraphicsHack*/));
			{
				StarShipPtr->special_counter = SPELL_WAIT;
			}
		}
		dont_cast_anything:
		;
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
			missileGraphicsHack[0] = CaptureDrawable(LoadCelFile("spathi/discrim.big"));
			missileGraphicsHack[1] = CaptureDrawable(LoadCelFile("spathi/discrim.med"));
			missileGraphicsHack[2] = CaptureDrawable(LoadCelFile("spathi/discrim.sml"));
			fireballGraphicsHack[0] = CaptureDrawable(LoadCelFile("urquan/fusion.big"));
			fireballGraphicsHack[1] = CaptureDrawable(LoadCelFile("urquan/fusion.med"));
			fireballGraphicsHack[2] = CaptureDrawable(LoadCelFile("urquan/fusion.sml"));
			/*confuseGraphicsHack[0] = CaptureDrawable(LoadCelFile("melnorme/confubig.ani"));
			confuseGraphicsHack[1] = CaptureDrawable(LoadCelFile("melnorme/confumed.ani"));
			confuseGraphicsHack[2] = CaptureDrawable(LoadCelFile("melnorme/confusml.ani"));*/
			demonGraphicsHack[0] = CaptureDrawable(LoadCelFile("ipanims/lavaspot.ani"));
			demonGraphicsHack[1] = CaptureDrawable(LoadCelFile("ipanims/lavaspot.ani"));
			demonGraphicsHack[2] = CaptureDrawable(LoadCelFile("ipanims/lavaspot.ani"));
		}
		++arilou_present;
	}

	++StarShipPtr->weapon_counter;

	if (StarShipPtr->cur_status_flags & (WEAPON | SPECIAL))
	{
		++ElementPtr->thrust_wait;
		++ElementPtr->turn_wait;
		/*ZeroVelocityComponents (&ElementPtr->velocity);
		StarShipPtr->cur_status_flags &= ~SHIP_AT_MAX_SPEED;*/
	}

	if (!(ElementPtr->state_flags & NONSOLID))
	{
		if (
			(StarShipPtr->cur_status_flags & SPECIAL)
			&&
			(StarShipPtr->cur_status_flags & RIGHT)
			&&
			(
				!(StarShipPtr->old_status_flags & SPECIAL)
				||
				!(StarShipPtr->old_status_flags & RIGHT)
			)
		)
		{
			PPRIMITIVE lpPrim;
			lpPrim = &(GLOBAL (DisplayArray))[ElementPtr->PrimIndex];
			if (GetPrimType (lpPrim) == STAMPFILL_PRIM)
			{
				SetPrimType (lpPrim, STAMP_PRIM);
				StarShipPtr->RaceDescPtr->characteristics.energy_wait = ENERGY_WAIT;
				ElementPtr->state_flags |= CHANGING;
			}
			else
			{
				SetPrimType (lpPrim, STAMPFILL_PRIM);
				SetPrimColor (lpPrim, BLACK_COLOR);
				StarShipPtr->RaceDescPtr->characteristics.energy_wait = INVISIBILITY_ENERGY_WAIT;
				ElementPtr->state_flags |= CHANGING;
			}
		}

		if ((StarShipPtr->cur_status_flags & SPECIAL)
				&& (StarShipPtr->cur_status_flags & THRUST)
				&& StarShipPtr->special_counter == 0
				&& CleanDeltaEnergy (ElementPtr, -TELEPORT_ENERGY_COST))
		{
#define HYPER_LIFE 5
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
			StarShipPtr->special_counter = SPELL_WAIT;
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
			ElementPtr->state_flags &= ~(NONSOLID | FINITE_LIFE);
			//ElementPtr->state_flags |= APPEARING;
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

