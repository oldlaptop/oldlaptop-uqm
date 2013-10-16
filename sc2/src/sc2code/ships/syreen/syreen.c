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
#include "ships/syreen/resinst.h"

#include "globdata.h"
#include "libs/mathlib.h"


#define SYREEN_MAX_CREW_SIZE MAX_CREW_SIZE
#define MAX_CREW MAX_CREW_SIZE //12
#define MAX_ENERGY 32 //16
#define ENERGY_REGENERATION 1
#define WEAPON_ENERGY_COST 1
#define SPECIAL_ENERGY_COST 1 //5
#define ENERGY_WAIT 5 //6
#define MAX_THRUST /* DISPLAY_TO_WORLD (8) */ 36
#define THRUST_INCREMENT /* DISPLAY_TO_WORLD (2) */ 9
#define TURN_WAIT 1
#define THRUST_WAIT 1
#define WEAPON_WAIT 0 //8
#define SPECIAL_WAIT 0 //20

#define SHIP_MASS 2
#define MISSILE_SPEED DISPLAY_TO_WORLD (30)
#define MISSILE_LIFE 10

static FRAME zapSatGraphicsHack[3];

static RACE_DESC syreen_desc =
{
	{
		FIRES_FORE,
		42, /* Super Melee cost */
		0 / SPHERE_RADIUS_INCREMENT, /* Initial sphere of influence radius */
		MAX_CREW, SYREEN_MAX_CREW_SIZE,
		MAX_ENERGY, MAX_ENERGY,
		{
			0, 0,
		},
		(STRING)SYREEN_RACE_STRINGS,
		(FRAME)SYREEN_ICON_MASK_PMAP_ANIM,
		(FRAME)SYREEN_MICON_MASK_PMAP_ANIM,
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
			(FRAME)SYREEN_BIG_MASK_PMAP_ANIM,
			(FRAME)SYREEN_MED_MASK_PMAP_ANIM,
			(FRAME)SYREEN_SML_MASK_PMAP_ANIM,
		},
		{
			(FRAME)DAGGER_BIG_MASK_PMAP_ANIM,
			(FRAME)DAGGER_MED_MASK_PMAP_ANIM,
			(FRAME)DAGGER_SML_MASK_PMAP_ANIM,
		},
		{
			(FRAME)0,
			(FRAME)0,
			(FRAME)0,
		},
		{
			(FRAME)SYREEN_CAPTAIN_MASK_PMAP_ANIM,
			(FRAME)0,
			(FRAME)0,
			(FRAME)0,
			(FRAME)0,
		},
		(SOUND)SYREEN_VICTORY_SONG,
		(SOUND)SYREEN_SHIP_SOUNDS,
	},
	{
		0,
		(MISSILE_SPEED * MISSILE_LIFE * 2 / 3),
		NULL_PTR,
	},
	(UNINIT_FUNC *) NULL,
	(PREPROCESS_FUNC *) NULL,
	(POSTPROCESS_FUNC *) NULL,
	(INIT_WEAPON_FUNC *) NULL,
	0,
};

static COUNT
initialize_dagger (PELEMENT ShipPtr, HELEMENT DaggerArray[])
{
#define SYREEN_OFFSET 30
#define MISSILE_HITS 1
#define MISSILE_DAMAGE 2
#define MISSILE_OFFSET 3
	STARSHIPPTR StarShipPtr;
	MISSILE_BLOCK MissileBlock;

	GetElementStarShip (ShipPtr, &StarShipPtr);
	MissileBlock.cx = ShipPtr->next.location.x;
	MissileBlock.cy = ShipPtr->next.location.y;
	MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.weapon;
	MissileBlock.face = MissileBlock.index = StarShipPtr->ShipFacing;
	MissileBlock.sender = (ShipPtr->state_flags & (GOOD_GUY | BAD_GUY))
			| IGNORE_SIMILAR;
	MissileBlock.pixoffs = SYREEN_OFFSET;
	MissileBlock.speed = MISSILE_SPEED;
	MissileBlock.hit_points = MISSILE_HITS;
	MissileBlock.damage = MISSILE_DAMAGE;
	MissileBlock.life = MISSILE_LIFE;
	MissileBlock.preprocess_func = NULL_PTR;
	MissileBlock.blast_offs = MISSILE_OFFSET;
	DaggerArray[0] = initialize_missile (&MissileBlock);

	return (1);
}

static COUNT initialize_homing_laser (PELEMENT ElementPtr, HELEMENT
		LaserArray[]);

static void
homing_laser_postprocess (PELEMENT ElementPtr)
{
	if (ElementPtr->turn_wait
			&& !(ElementPtr->state_flags & COLLISION))
	{
		HELEMENT Laser;

		initialize_homing_laser (ElementPtr, &Laser);
		if (Laser)
			PutElement (Laser);
	}
}

static void
homing_laser_collision (PELEMENT ElementPtr0, PPOINT pPt0, PELEMENT ElementPtr1, PPOINT pPt1)
{
	ElementPtr0->turn_wait = 0;
	weapon_collision (ElementPtr0, pPt0, ElementPtr1, pPt1);
}

static COUNT
initialize_homing_laser (PELEMENT ElementPtr, HELEMENT LaserArray[])
{
	BOOLEAN isFirstSegment;
	STARSHIPPTR StarShipPtr;
	LASER_BLOCK LaserBlock;
	COUNT delta_facing, not_facing;
	SIZE angle, rotation;

	isFirstSegment = (ElementPtr->state_flags & PLAYER_SHIP);
	GetElementStarShip (ElementPtr, &StarShipPtr);
	angle = GetVelocityTravelAngle (&ElementPtr->velocity);
	not_facing = ANGLE_TO_FACING (angle);
	rotation = 0;
	if((delta_facing = TrackShip(ElementPtr, &not_facing)) && ElementPtr->hTarget)
	{
		if(delta_facing > 0 && delta_facing < 8)rotation = 1;
		if(delta_facing > 8)rotation = -1;
		if(delta_facing == 8)rotation = (TFB_Random() & 1) * 2 + 1;
	}
	LaserBlock.face = StarShipPtr->ShipFacing;
	LaserBlock.cx = ElementPtr->next.location.x;
	LaserBlock.cy = ElementPtr->next.location.y;
	LaserBlock.ex = COSINE (isFirstSegment ? (FACING_TO_ANGLE (LaserBlock.face)) : NORMALIZE_ANGLE (angle + rotation), DISPLAY_TO_WORLD(8));
	LaserBlock.ey = SINE (isFirstSegment ? (FACING_TO_ANGLE (LaserBlock.face)) : NORMALIZE_ANGLE (angle + rotation), DISPLAY_TO_WORLD(8));
	LaserBlock.sender = (ElementPtr->state_flags & (GOOD_GUY | BAD_GUY))
			| IGNORE_SIMILAR;
	LaserBlock.pixoffs = 0;
	LaserBlock.color = BUILD_COLOR (MAKE_RGB15 (0x1F, 0x07, 0x03), 0);
	LaserArray[0] = initialize_laser (&LaserBlock);

	if(LaserArray[0])
	{
		ELEMENTPTR LaserPtr;
		
		LockElement(LaserArray[0], &LaserPtr);

		SetElementStarShip (LaserPtr, StarShipPtr);
		LaserPtr->postprocess_func = homing_laser_postprocess;
		LaserPtr->collision_func = homing_laser_collision;
		LaserPtr->turn_wait = isFirstSegment ? 20 : ElementPtr->turn_wait - 1;
		LaserPtr->hTarget = 0;

		UnlockElement(LaserArray[0]);
	}

	return (1);
}

static void
compel_enemy (PELEMENT ElementPtr)
{
	if(ElementPtr->state_flags & PLAYER_SHIP)
	{
		HELEMENT hCompulsion;
		hCompulsion = AllocElement ();
		if(hCompulsion)
		{
			ELEMENTPTR ComPtr;
			STARSHIPPTR StarShipPtr;
			
			LockElement (hCompulsion, &ComPtr);
			ComPtr->state_flags =
					FINITE_LIFE | NONSOLID | IGNORE_SIMILAR | APPEARING
					| (ElementPtr->state_flags & (GOOD_GUY | BAD_GUY));
			ComPtr->life_span = 2;
			ComPtr->preprocess_func = compel_enemy;
			
			GetElementStarShip (ElementPtr, &StarShipPtr);
			SetElementStarShip (ComPtr, StarShipPtr);
			
			SetPrimType (&(GLOBAL (DisplayArray))[
					ComPtr->PrimIndex
					], NO_PRIM);
					
			UnlockElement (hCompulsion);
			InsertElement (hCompulsion, GetHeadElement ());
		}
	}
	else
	{
		HELEMENT original_target;
		ELEMENTPTR EnemyElementPtr;
		STARSHIPPTR EnemyStarShipPtr;
		COUNT delta_facing, facing;
	
		HELEMENT hShip, hNextShip;
		BOOLEAN found_an_enemy = false;
	
		for (hShip = GetHeadElement (); hShip != 0; hShip = hNextShip)
		{
			LockElement (hShip, &EnemyElementPtr);
			hNextShip = GetSuccElement (EnemyElementPtr);
			GetElementStarShip (EnemyElementPtr, &EnemyStarShipPtr);
			if ((EnemyElementPtr->state_flags & PLAYER_SHIP)
				&& !(EnemyStarShipPtr->RaceDescPtr->ship_info.ship_flags & CREW_IMMUNE)
				&& ((EnemyElementPtr->state_flags & (GOOD_GUY | BAD_GUY)) !=
				(ElementPtr->state_flags & (GOOD_GUY | BAD_GUY))))
			{
				found_an_enemy = true;
				break;
			}
			UnlockElement(hShip);
		}
		
		if(!found_an_enemy)return;
			
		EnemyStarShipPtr->ship_input_state |= THRUST;
		EnemyStarShipPtr->ship_input_state &= ~(LEFT | RIGHT);
	
		original_target = EnemyElementPtr->hTarget;
		
		facing = EnemyStarShipPtr->ShipFacing;
		delta_facing = TrackShip (EnemyElementPtr, &facing);
		if(delta_facing > 0 && delta_facing < 8)EnemyStarShipPtr->ship_input_state |= RIGHT;
		else if(delta_facing > 8)EnemyStarShipPtr->ship_input_state |= LEFT;
		else if(delta_facing == 8)EnemyStarShipPtr->ship_input_state |= (TFB_Random() & 1) ? LEFT : RIGHT;
	
		EnemyElementPtr->hTarget = original_target;
	
		UnlockElement(hShip);
	}
}	

static void
syreen_intelligence (PELEMENT ShipPtr, PEVALUATE_DESC ObjectsOfConcern, COUNT ConcernCounter)
{
	PEVALUATE_DESC lpEvalDesc;

	ship_intelligence (ShipPtr,
			ObjectsOfConcern, ConcernCounter);

	lpEvalDesc = &ObjectsOfConcern[ENEMY_SHIP_INDEX];
	if (lpEvalDesc->ObjectPtr != NULL_PTR)
	{
		STARSHIPPTR StarShipPtr, EnemyStarShipPtr;

		GetElementStarShip (ShipPtr, &StarShipPtr);
		GetElementStarShip (lpEvalDesc->ObjectPtr, &EnemyStarShipPtr);
		if (!(EnemyStarShipPtr->RaceDescPtr->ship_info.ship_flags & CREW_IMMUNE)
				&& StarShipPtr->special_counter == 0
				&& lpEvalDesc->ObjectPtr->crew_level > 1
				&& lpEvalDesc->which_turn <= 14)
			StarShipPtr->ship_input_state |= SPECIAL;
		else
			StarShipPtr->ship_input_state &= ~SPECIAL;
	}
}

#define MAX_MACE_DISTANCE DISPLAY_TO_WORLD (200)
#define MAX_MACE_VELOCITY WORLD_TO_VELOCITY (DISPLAY_TO_WORLD (40))
#define MACE_ACCELERATION DISPLAY_TO_WORLD (5)

static void
mace_preprocess (PELEMENT ElementPtr)
{
	STARSHIPPTR StarShipPtr;

	++ElementPtr->life_span;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	if (StarShipPtr->hShip)
	{
		SIZE dx, dy;
		COUNT angle, distance, velocity;
		COUNT percent;
		ELEMENTPTR ShipPtr;

		LockElement (StarShipPtr->hShip, &ShipPtr);

		dx = ShipPtr->next.location.x
				- ElementPtr->current.location.x;
		dy = ShipPtr->next.location.y
				- ElementPtr->current.location.y;
		dx = WRAP_DELTA_X (dx);
		dy = WRAP_DELTA_Y (dy);
		angle = ARCTAN (dx, dy);

		distance = square_root((long)dx * dx + (long)dy * dy);
		percent = 100 * distance / MAX_MACE_DISTANCE;
		DeltaVelocityComponents(&ElementPtr->velocity,
				COSINE (angle, WORLD_TO_VELOCITY (MACE_ACCELERATION * percent / 100)),
				SINE (angle, WORLD_TO_VELOCITY (MACE_ACCELERATION * percent / 100))
				);

		GetCurrentVelocityComponents(&ElementPtr->velocity, &dx, &dy);
		dx = (dx * 29) / 30;
		dy = (dy * 29) / 30;

		velocity = square_root((long)dx * dx + (long)dy * dy);
		percent = 100 * velocity / MAX_MACE_VELOCITY;
		if(percent > 100)
		{
			dx = (dx * 100) / percent;
			dy = (dy * 100) / percent;
		}
		SetVelocityComponents(&ElementPtr->velocity, dx, dy);

		angle = ARCTAN (dx, dy);
		if(GetFrameIndex (ElementPtr->current.image.frame) != ANGLE_TO_FACING(angle))
		{
			ElementPtr->next.image.frame =
				SetAbsFrameIndex (ElementPtr->current.image.frame, ANGLE_TO_FACING(angle));
				ElementPtr->state_flags |= CHANGING;
		}

		UnlockElement (StarShipPtr->hShip);
	}
}

#define MACE_LASER_COLOR BUILD_COLOR (MAKE_RGB15 (0x18, 0x07, 0x1D), 0)

static void
mace_laser_postprocess (PELEMENT ElementPtr)
{
	STARSHIPPTR StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	if (!(ElementPtr->state_flags & COLLISION))
	{
		SIZE dx, dy;
		LASER_BLOCK LaserBlock;
		HELEMENT hLaser;

		GetCurrentVelocityComponents(&ElementPtr->velocity, &dx, &dy);
		LaserBlock.cx = ElementPtr->next.location.x;
		LaserBlock.cy = ElementPtr->next.location.y;
		LaserBlock.face = 0;
		LaserBlock.ex = VELOCITY_TO_WORLD (dx);
		LaserBlock.ey = VELOCITY_TO_WORLD (dy);
		LaserBlock.sender =
				(ElementPtr->state_flags & (GOOD_GUY | BAD_GUY))
				| IGNORE_SIMILAR;
		LaserBlock.pixoffs = 0;
		LaserBlock.color = MACE_LASER_COLOR;
	
		hLaser = initialize_laser (&LaserBlock);
		if (hLaser)
		{
			ELEMENTPTR LaserPtr;
		
			LockElement (hLaser, &LaserPtr);
			SetElementStarShip (LaserPtr, StarShipPtr);
			LaserPtr->hTarget = 0;
			LaserPtr->mass_points = ElementPtr->mass_points;

			UnlockElement (hLaser);
	
			PutElement (hLaser);
		}
	}
}

static void
mace_postprocess (PELEMENT ElementPtr)
{
	STARSHIPPTR StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	if (StarShipPtr->hShip)
	{
		SIZE dx, dy;
		ELEMENTPTR ShipPtr;

		LockElement (StarShipPtr->hShip, &ShipPtr);

		dx = ShipPtr->next.location.x
				- ElementPtr->next.location.x;
		dy = ShipPtr->next.location.y
				- ElementPtr->next.location.y;
		dx = WRAP_DELTA_X (dx);
		dy = WRAP_DELTA_Y (dy);

		{
			LASER_BLOCK LaserBlock;
			HELEMENT hLaser;
			
			LaserBlock.cx = ElementPtr->next.location.x;
			LaserBlock.cy = ElementPtr->next.location.y;
			LaserBlock.face = 0;
			LaserBlock.ex = dx / 2;
			LaserBlock.ey = dy / 2;
			LaserBlock.sender =
				(ElementPtr->state_flags & (GOOD_GUY | BAD_GUY))
				| IGNORE_SIMILAR;
			LaserBlock.pixoffs = 0;
			LaserBlock.color = MACE_LASER_COLOR;

			hLaser = initialize_laser (&LaserBlock);
			if (hLaser)
			{
				ELEMENTPTR LaserPtr;
	
				LockElement (hLaser, &LaserPtr);
				SetElementStarShip (LaserPtr, StarShipPtr);
				LaserPtr->hTarget = 0;
				LaserPtr->mass_points = ElementPtr->mass_points;
				LaserPtr->postprocess_func = mace_laser_postprocess;
				UnlockElement (hLaser);
	
				PutElement (hLaser);
			}
		}

		UnlockElement (StarShipPtr->hShip);
	}
}



static void
mace_collision (PELEMENT ElementPtr0, PPOINT pPt0,
		PELEMENT ElementPtr1, PPOINT pPt1)
{
	if(!GRAVITY_MASS(ElementPtr1->mass_points))
	{
		SIZE damage;
		damage = ElementPtr0->mass_points;
		ElementPtr0->hit_points = 10000;
		do_damage (ElementPtr1, damage);
		if (damage > TARGET_DAMAGED_FOR_6_PLUS_PT)
		damage = TARGET_DAMAGED_FOR_6_PLUS_PT;
				ProcessSound (SetAbsSoundIndex (GameSounds, damage), ElementPtr1);
	}
}

static void
spawn_mace (PELEMENT ElementPtr)
{
	STARSHIPPTR StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);

	HELEMENT hMace;

	hMace = AllocElement ();
	if(hMace)
	{
		ELEMENTPTR MacePtr;

		LockElement(hMace, &MacePtr);
		MacePtr->state_flags =
				IGNORE_SIMILAR | APPEARING | FINITE_LIFE
				| (ElementPtr->state_flags & (GOOD_GUY | BAD_GUY));
		MacePtr->life_span = NORMAL_LIFE + 2;
		MacePtr->hit_points = 10000; //actually, it's invincible!
		MacePtr->mass_points = 2;

		MacePtr->current.location.x = ElementPtr->next.location.x;
		MacePtr->current.location.y = ElementPtr->next.location.y;

		MacePtr->current.image.farray =
				StarShipPtr->RaceDescPtr->ship_data.special;
		MacePtr->current.image.frame = SetAbsFrameIndex (
				StarShipPtr->RaceDescPtr->ship_data.special[0],
				StarShipPtr->ShipFacing
				);

		MacePtr->preprocess_func = mace_preprocess;
		MacePtr->postprocess_func = mace_postprocess;
		MacePtr->collision_func = mace_collision;
		SetElementStarShip (MacePtr, StarShipPtr);
		
		SetPrimType (&(GLOBAL (DisplayArray))[
				MacePtr->PrimIndex
				], STAMP_PRIM);

		UnlockElement (hMace);
		PutElement (hMace);
	}
}

static void
syreen_postprocess (PELEMENT ElementPtr)
{
	STARSHIPPTR StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);

	if ((StarShipPtr->cur_status_flags & SPECIAL)
			&& DeltaEnergy (ElementPtr, -SPECIAL_ENERGY_COST))
	{
#define SONG_WAIT 22
		if(StarShipPtr->special_counter == 0)
		{
			ProcessSound (SetAbsSoundIndex (
							// SYREEN_SONG
					StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1), ElementPtr);
			StarShipPtr->special_counter = SONG_WAIT;
		}

		compel_enemy(ElementPtr);
	}
}

static COUNT syreen_present = 0;

static void
syreen_dispose_graphics (RACE_DESCPTR RaceDescPtr)
{
	--syreen_present;

	if(!syreen_present)
	{
		DestroyDrawable(ReleaseDrawable(RaceDescPtr->ship_data.special[0]));
		RaceDescPtr->ship_data.special[0] = (FRAME)0;
		DestroyDrawable(ReleaseDrawable(RaceDescPtr->ship_data.special[1]));
		RaceDescPtr->ship_data.special[1] = (FRAME)0;
		DestroyDrawable(ReleaseDrawable(RaceDescPtr->ship_data.special[2]));
		RaceDescPtr->ship_data.special[2] = (FRAME)0;

		DestroyDrawable(ReleaseDrawable(zapSatGraphicsHack[0]));
		zapSatGraphicsHack[0] = (FRAME)0;
		DestroyDrawable(ReleaseDrawable(zapSatGraphicsHack[1]));
		zapSatGraphicsHack[1] = (FRAME)0;
		DestroyDrawable(ReleaseDrawable(zapSatGraphicsHack[2]));
		zapSatGraphicsHack[2] = (FRAME)0;
	}
}

static void
syreen_preprocess (PELEMENT ElementPtr)
{
	STARSHIPPTR StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);

	if (ElementPtr->state_flags & APPEARING)
	{
		if(StarShipPtr->RaceDescPtr->ship_data.special[0] == (FRAME)0)
		{
			StarShipPtr->RaceDescPtr->ship_data.special[0] = CaptureDrawable(LoadCelFile("androsyn/blazer.big"));
			StarShipPtr->RaceDescPtr->ship_data.special[1] = CaptureDrawable(LoadCelFile("androsyn/blazer.med"));
			StarShipPtr->RaceDescPtr->ship_data.special[2] = CaptureDrawable(LoadCelFile("androsyn/blazer.sml"));
			zapSatGraphicsHack[0] = CaptureDrawable(LoadCelFile("chmmr/satbig.ani"));
			zapSatGraphicsHack[1] = CaptureDrawable(LoadCelFile("chmmr/satmed.ani"));
			zapSatGraphicsHack[2] = CaptureDrawable(LoadCelFile("chmmr/satsml.ani"));
		}
		++syreen_present;

		spawn_mace(ElementPtr);

		extern void spawn_satellite (PELEMENT ElementPtr, COUNT hit_points, COUNT angle, BYTE offset, FRAME farray[]);

		spawn_satellite(ElementPtr, 10000, 0, 64, zapSatGraphicsHack);
	}
}

RACE_DESCPTR
init_syreen (void)
{
	RACE_DESCPTR RaceDescPtr;

	syreen_desc.preprocess_func = syreen_preprocess;
	syreen_desc.postprocess_func = syreen_postprocess;
	syreen_desc.init_weapon_func = initialize_homing_laser;//dagger;
	syreen_desc.uninit_func = syreen_dispose_graphics;
	syreen_desc.cyborg_control.intelligence_func =
			(void (*) (PVOID ShipPtr, PVOID ObjectsOfConcern, COUNT
					ConcernCounter)) syreen_intelligence;

	RaceDescPtr = &syreen_desc;

	return (RaceDescPtr);
}

