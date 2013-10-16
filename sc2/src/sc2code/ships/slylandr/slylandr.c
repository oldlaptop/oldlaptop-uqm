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


#define MAX_CREW 12
#define MAX_ENERGY 30
#define ENERGY_REGENERATION 1
#define WEAPON_ENERGY_COST 7
#define SPECIAL_ENERGY_COST 2
#define ENERGY_WAIT 2
#define MAX_THRUST 75
#define THRUST_INCREMENT 12
#define TURN_WAIT 1
#define THRUST_WAIT 1
#define WEAPON_WAIT 6
#define SPECIAL_WAIT 2

#define SHIP_MASS 3
#define SLYLANDRO_OFFSET 9
#define MISSILE_SPEED DISPLAY_TO_WORLD (20) //(43)
#define MISSILE_LIFE 18 //10

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

	StarShipPtr->ship_input_state |= WEAPON;

	StarShipPtr->ship_input_state &= ~SPECIAL;
}




/*static COUNT initialize_lightning (PELEMENT ElementPtr, HELEMENT
		LaserArray[]);

static void
lightning_postprocess (PELEMENT ElementPtr)
{
	if (ElementPtr->turn_wait
			&& !(ElementPtr->state_flags & COLLISION))
	{
		HELEMENT Lightning;

		initialize_lightning (ElementPtr, &Lightning);
		if (Lightning)
			PutElement (Lightning);
	}
}

static void
lightning_collision (PELEMENT ElementPtr0, PPOINT pPt0, PELEMENT ElementPtr1, PPOINT pPt1)
{
	STARSHIPPTR StarShipPtr;

	GetElementStarShip (ElementPtr0, &StarShipPtr);
	if (StarShipPtr->special_counter > SPECIAL_WAIT >> 1)
		StarShipPtr->special_counter =
				SPECIAL_WAIT - StarShipPtr->special_counter;
	StarShipPtr->special_counter -= ElementPtr0->turn_wait;
	ElementPtr0->turn_wait = 0;

	weapon_collision (ElementPtr0, pPt0, ElementPtr1, pPt1);
}

static COUNT
initialize_lightning (PELEMENT ElementPtr, HELEMENT LaserArray[])
{
	LASER_BLOCK LaserBlock;

	ELEMENTPTR AsteroidElementPtr;
	ELEMENTPTR ClosestAsteroidPtr = 0;
	long square_distance = 0;
	HELEMENT hElement, hNextElement;
	SIZE delta;
	COUNT angle, facing;
	SIZE delta_x, delta_y, delta_facing;
	STARSHIPPTR StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);

	for (hElement = GetHeadElement (); hElement != 0; hElement = hNextElement)
	{
		LockElement (hElement, &AsteroidElementPtr);
		hNextElement = GetSuccElement (AsteroidElementPtr);
		if (!(AsteroidElementPtr->state_flags
				& (APPEARING | GOOD_GUY | BAD_GUY
				| PLAYER_SHIP | FINITE_LIFE))
				&& !GRAVITY_MASS (AsteroidElementPtr->mass_points)
				&& CollisionPossible (AsteroidElementPtr, ElementPtr))
		{
			long dist = 
				(long)WRAP_DELTA_X(AsteroidElementPtr->next.location.x - ElementPtr->next.location.x) * WRAP_DELTA_X(AsteroidElementPtr->next.location.x - ElementPtr->next.location.x) 
				+ (long)WRAP_DELTA_Y(AsteroidElementPtr->next.location.y - ElementPtr->next.location.y) * WRAP_DELTA_Y(AsteroidElementPtr->next.location.y - ElementPtr->next.location.y);
			if(square_distance == 0 || dist < square_distance)
			{
				square_distance = dist;
				ClosestAsteroidPtr = AsteroidElementPtr;
			}
		}
		UnlockElement (hElement);
	}
	if (!(ElementPtr->state_flags & PLAYER_SHIP))
	{
		angle = GetVelocityTravelAngle (&ElementPtr->velocity);
		facing = NORMALIZE_FACING (ANGLE_TO_FACING (angle));
	}
	else
	{
		facing = StarShipPtr->ShipFacing;
	}
	if(ClosestAsteroidPtr)
	{
	delta_x = ClosestAsteroidPtr->next.location.x
			- ElementPtr->next.location.x;
	delta_y = ClosestAsteroidPtr->next.location.y
			- ElementPtr->next.location.y;
	delta_x = WRAP_DELTA_X (delta_x);
	delta_y = WRAP_DELTA_Y (delta_y);
	delta_facing = NORMALIZE_FACING (
						ANGLE_TO_FACING (ARCTAN (delta_x, delta_y)) - facing
						);
	delta = delta_facing;
	}
	else delta = -1;

	LaserBlock.cx = ElementPtr->next.location.x;
	LaserBlock.cy = ElementPtr->next.location.y;
	LaserBlock.ex = 0;
	LaserBlock.ey = 0;

	LaserBlock.sender = (ElementPtr->state_flags & (GOOD_GUY | BAD_GUY))
			| IGNORE_SIMILAR;
	LaserBlock.face = 0;
	LaserBlock.pixoffs = 0;
	LaserArray[0] = initialize_laser (&LaserBlock);

	if (LaserArray[0])
	{
		//SIZE delta;
		//COUNT angle, facing;
		DWORD rand_val;
		ELEMENTPTR LaserPtr;

		LockElement (LaserArray[0], &LaserPtr);
		LaserPtr->postprocess_func = lightning_postprocess;
		LaserPtr->collision_func = lightning_collision;

		rand_val = TFB_Random ();

		if (!(ElementPtr->state_flags & PLAYER_SHIP))
		{
			//angle = GetVelocityTravelAngle (&ElementPtr->velocity);
			//facing = NORMALIZE_FACING (ANGLE_TO_FACING (angle));
			//delta = TrackShip (ElementPtr, &facing);

			LaserPtr->turn_wait = ElementPtr->turn_wait - 1;

			SetPrimColor (&(GLOBAL (DisplayArray))[LaserPtr->PrimIndex],
					GetPrimColor (&(GLOBAL (DisplayArray))[ElementPtr->PrimIndex]));
		}
		else
		{
			//facing = StarShipPtr->ShipFacing;
			//ElementPtr->hTarget = 0;
			//delta = TrackShip (ElementPtr, &facing);
			//ElementPtr->hTarget = 0;
			angle = FACING_TO_ANGLE (facing);

			if ((LaserPtr->turn_wait = StarShipPtr->special_counter) == 0)
				LaserPtr->turn_wait = SPECIAL_WAIT;

			if (LaserPtr->turn_wait > SPECIAL_WAIT >> 1)
				LaserPtr->turn_wait = SPECIAL_WAIT - LaserPtr->turn_wait;

			switch (HIBYTE (LOWORD (rand_val)) & 3)
			{
				case 0:
					SetPrimColor (
							&(GLOBAL (DisplayArray))[LaserPtr->PrimIndex],
							BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x1F), 0x0F)
							);
					break;
				case 1:
					SetPrimColor (
							&(GLOBAL (DisplayArray))[LaserPtr->PrimIndex],
							BUILD_COLOR (MAKE_RGB15 (0x16, 0x17, 0x1F), 0x42)
							);
					break;
				case 2:
					SetPrimColor (
							&(GLOBAL (DisplayArray))[LaserPtr->PrimIndex],
							BUILD_COLOR (MAKE_RGB15 (0x06, 0x07, 0x1F), 0x4A)
							);
					break;
				case 3:
					SetPrimColor (
							&(GLOBAL (DisplayArray))[LaserPtr->PrimIndex],
							BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x18), 0x50)
							);
					break;
			}
		}

		if (delta == -1 || delta == ANGLE_TO_FACING (HALF_CIRCLE))
			angle += LOWORD (rand_val);
		else if (delta == 0)
			angle += LOWORD (rand_val) & 1 ? -1 : 1;
		else if (delta < ANGLE_TO_FACING (HALF_CIRCLE))
			angle += LOWORD (rand_val) & (QUADRANT - 1);
		else
			angle -= LOWORD (rand_val) & (QUADRANT - 1);
#define LASER_RANGE 32
		delta = WORLD_TO_VELOCITY (
				DISPLAY_TO_WORLD ((HIWORD (rand_val) & (LASER_RANGE - 1)) + 4)
				);
		SetVelocityComponents (&LaserPtr->velocity,
				COSINE (angle, delta), SINE (angle, delta));

		SetElementStarShip (LaserPtr, StarShipPtr);
		UnlockElement (LaserArray[0]);
	}

	return (1);
}*/


//***********CODE COPIED FROM spawn_asteroid (misc.c)***********

extern FRAME asteroid[];

extern void asteroid_preprocess (PELEMENT ElementPtr);

static void
spawn_slylandro_rubble (PELEMENT AsteroidElementPtr)
{
	HELEMENT hRubbleElement;
	SIZE dx, dy;

	GetCurrentVelocityComponents(&AsteroidElementPtr->velocity, &dx, &dy);

	hRubbleElement = AllocElement ();
	if (hRubbleElement)
	{
		ELEMENTPTR RubbleElementPtr;

		PutElement (hRubbleElement);
		LockElement (hRubbleElement, &RubbleElementPtr);
		RubbleElementPtr->state_flags = APPEARING | FINITE_LIFE | NONSOLID;
		RubbleElementPtr->life_span = 5;
		RubbleElementPtr->turn_wait = RubbleElementPtr->next_turn = 0;
		SetPrimType (&DisplayArray[RubbleElementPtr->PrimIndex], STAMP_PRIM);
		RubbleElementPtr->current.image.farray = asteroid;
		RubbleElementPtr->current.image.frame =
				SetAbsFrameIndex (asteroid[0], ANGLE_TO_FACING (FULL_CIRCLE));
		RubbleElementPtr->current.location = AsteroidElementPtr->current.location;
		RubbleElementPtr->preprocess_func = animation_preprocess;

		SetVelocityComponents(&RubbleElementPtr->velocity, dx, dy);

		UnlockElement (hRubbleElement);
	}
}

static void
spawn_slylandro_asteroid (PELEMENT ElementPtr)
{
	HELEMENT hAsteroidElement;
	STARSHIPPTR StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);

	if ((hAsteroidElement = AllocElement ()))
	{
		ELEMENTPTR AsteroidElementPtr;
		COUNT offset;

		LockElement (hAsteroidElement, &AsteroidElementPtr);
		AsteroidElementPtr->hit_points = 1;
		AsteroidElementPtr->mass_points = 3;
		AsteroidElementPtr->state_flags = APPEARING;
		AsteroidElementPtr->life_span = NORMAL_LIFE;
		SetPrimType (&DisplayArray[AsteroidElementPtr->PrimIndex], STAMP_PRIM);
		{
			// Using these temporary variables because the execution order
			// of function arguments may vary per system, which may break
			// synchronisation on network games.
			SIZE magnitude =
					DISPLAY_TO_WORLD (((SIZE)TFB_Random () & 7) + 4);
			COUNT facing = (COUNT)TFB_Random ();
			SetVelocityVector (&AsteroidElementPtr->velocity, magnitude,
					facing);
		}
		AsteroidElementPtr->current.image.farray = asteroid;
		AsteroidElementPtr->current.image.frame =
				SetAbsFrameIndex (asteroid[0],
				NORMALIZE_FACING (TFB_Random ()));
		AsteroidElementPtr->turn_wait =
				AsteroidElementPtr->thrust_wait =
				(BYTE)TFB_Random () & (BYTE)((1 << 2) - 1);
		AsteroidElementPtr->thrust_wait |=
				(BYTE)TFB_Random () & (BYTE)(1 << 7);
		AsteroidElementPtr->preprocess_func = asteroid_preprocess;
		AsteroidElementPtr->death_func = spawn_slylandro_rubble;
		AsteroidElementPtr->collision_func = collision;


		offset = 0;
		do
		{
			COUNT angle;
			angle = TFB_Random() & (FULL_CIRCLE - 1);
			AsteroidElementPtr->current.location.x =
					ElementPtr->current.location.x
					+ COSINE (FACING_TO_ANGLE (StarShipPtr->ShipFacing), 180)
					+ COSINE (angle, offset);
			AsteroidElementPtr->current.location.y =
					ElementPtr->current.location.y
					+ SINE (FACING_TO_ANGLE (StarShipPtr->ShipFacing), 180)
					+ SINE (angle, offset);
			offset += 10;
		}
		while(TimeSpaceMatterConflict(AsteroidElementPtr));

		UnlockElement (hAsteroidElement);

		PutElement (hAsteroidElement);
	}
}

//******************END COPIED CODE**************



static COUNT
initialize_nukes (PELEMENT ShipPtr, HELEMENT MissileArray[])
{
#define MISSILE_DAMAGE 4
#define MISSILE_HITS 1
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

#define NUM_MISSILES 8
#define MISSILE_SPACING 42

	for(i = 0; i < NUM_MISSILES; ++i)
	{
		MissileBlock.cx = ShipPtr->next.location.x + COSINE(FACING_TO_ANGLE(MissileBlock.face + 4), (i*2 + 1 - NUM_MISSILES) * MISSILE_SPACING / 2);
		MissileBlock.cy = ShipPtr->next.location.y + SINE(FACING_TO_ANGLE(MissileBlock.face + 4), (i*2 + 1 - NUM_MISSILES) * MISSILE_SPACING / 2);
		MissileArray[i] = initialize_missile (&MissileBlock);
	}

	return (NUM_MISSILES);
}

static void
slylandro_postprocess (PELEMENT ElementPtr)
{
	STARSHIPPTR StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);


	if((StarShipPtr->cur_status_flags & SPECIAL)
		&& StarShipPtr->special_counter == 0
		&& DeltaEnergy(ElementPtr, -SPECIAL_ENERGY_COST))
	{
		COUNT angle;
		SIZE cur_delta_x, cur_delta_y;


		StarShipPtr->special_counter = SPECIAL_WAIT;
		spawn_slylandro_asteroid(ElementPtr);


//*********  CODE COPIED FROM druuge_postprocess  ***********

#define ASTEROID_VELOCITY WORLD_TO_VELOCITY (DISPLAY_TO_WORLD (4))
#define MAX_ASTEROID_VELOCITY WORLD_TO_VELOCITY (DISPLAY_TO_WORLD (28))

		StarShipPtr->cur_status_flags &= ~SHIP_AT_MAX_SPEED;

		angle = FACING_TO_ANGLE (StarShipPtr->ShipFacing) + HALF_CIRCLE;
		DeltaVelocityComponents (&ElementPtr->velocity,
				COSINE (angle, ASTEROID_VELOCITY),
				SINE (angle, ASTEROID_VELOCITY));
		GetCurrentVelocityComponents (&ElementPtr->velocity,
				&cur_delta_x, &cur_delta_y);
		if ((long)cur_delta_x * (long)cur_delta_x
				+ (long)cur_delta_y * (long)cur_delta_y
				> (long)MAX_ASTEROID_VELOCITY * (long)MAX_ASTEROID_VELOCITY)
		{
			angle = ARCTAN (cur_delta_x, cur_delta_y);
			SetVelocityComponents (&ElementPtr->velocity,
					COSINE (angle, MAX_ASTEROID_VELOCITY),
					SINE (angle, MAX_ASTEROID_VELOCITY));
		}
//******************    END COPIED CODE   *******************
	}


	/*if((StarShipPtr->cur_status_flags & SPECIAL)
		&& StarShipPtr->special_counter == 0
		&& DeltaEnergy(ElementPtr, -SPECIAL_ENERGY_COST))
	{
		StarShipPtr->special_counter = SPECIAL_WAIT;
		ProcessSound (StarShipPtr->RaceDescPtr->ship_data.ship_sounds, ElementPtr);
	}
	if(StarShipPtr->special_counter)
	{
		HELEMENT Lightning;

		initialize_lightning (ElementPtr, &Lightning);
		if (Lightning)
			PutElement (Lightning);
	}*/
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
