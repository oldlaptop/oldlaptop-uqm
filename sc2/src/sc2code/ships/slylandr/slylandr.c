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
#define MAX_ENERGY MAX_ENERGY_SIZE //20
#define ASTEROID_ENERGY_RECHARGE 28
#define INITIAL_ENERGY ASTEROID_ENERGY_RECHARGE
#define ENERGY_REGENERATION 0
#define WEAPON_ENERGY_COST 1 //2
#define SPECIAL_ENERGY_COST MAX_ENERGY //0
#define ENERGY_WAIT 10
#define MAX_THRUST 50
#define THRUST_INCREMENT MAX_THRUST
#define TURN_WAIT 0
#define THRUST_WAIT 0
#define WEAPON_WAIT 1 //1 //17
#define SPECIAL_WAIT 6 //20

#define SHIP_MASS 1
#define SLYLANDRO_OFFSET 9

static RACE_DESC slylandro_desc =
{
	{
		SEEKING_WEAPON | CREW_IMMUNE,
		33, /* Super Melee cost */
		~0, /* Initial sphere of influence radius */
		MAX_CREW, MAX_CREW,
		INITIAL_ENERGY, MAX_ENERGY,
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

#define LIGHTNING_SEGMENTS 100

static COUNT initialize_lightning (PELEMENT ElementPtr, HELEMENT
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
	/*if (StarShipPtr->weapon_counter > WEAPON_WAIT >> 1)
		StarShipPtr->weapon_counter =
				WEAPON_WAIT - StarShipPtr->weapon_counter;
	StarShipPtr->weapon_counter -= ElementPtr0->turn_wait;*/
	if(((COUNT)TFB_Random() % 3) != 1)
	{
		ElementPtr0->mass_points = 0;
	}
	ElementPtr0->turn_wait = 0;
	weapon_collision (ElementPtr0, pPt0, ElementPtr1, pPt1);
}

static COUNT
initialize_lightning (PELEMENT ElementPtr, HELEMENT LaserArray[])
{
	LASER_BLOCK LaserBlock;

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
		SIZE delta;
		COUNT angle, facing;
		DWORD rand_val;
		ELEMENTPTR LaserPtr;
		STARSHIPPTR StarShipPtr;

		GetElementStarShip (ElementPtr, &StarShipPtr);

		LockElement (LaserArray[0], &LaserPtr);
		LaserPtr->postprocess_func = lightning_postprocess;
		LaserPtr->collision_func = lightning_collision;

		rand_val = TFB_Random ();

		if (!(ElementPtr->state_flags & PLAYER_SHIP))
		{
			angle = GetVelocityTravelAngle (&ElementPtr->velocity);
			facing = NORMALIZE_FACING (ANGLE_TO_FACING (angle));
			delta = TrackShip (ElementPtr, &facing);

			LaserPtr->turn_wait = ElementPtr->turn_wait - 1;
			if(delta == -1)LaserPtr->turn_wait /= 2;

			SetPrimColor (&(GLOBAL (DisplayArray))[LaserPtr->PrimIndex],
					GetPrimColor (&(GLOBAL (DisplayArray))[ElementPtr->PrimIndex]));
		}
		else
		{
			facing = StarShipPtr->ShipFacing;
			ElementPtr->hTarget = 0;
			delta = TrackShip (ElementPtr, &facing);
			ElementPtr->hTarget = 0;
			angle = FACING_TO_ANGLE (facing);

			/*if ((LaserPtr->turn_wait = StarShipPtr->weapon_counter) == 0)
				LaserPtr->turn_wait = WEAPON_WAIT;

			if (LaserPtr->turn_wait > WEAPON_WAIT >> 1)
				LaserPtr->turn_wait = WEAPON_WAIT - LaserPtr->turn_wait;*/

			LaserPtr->turn_wait = LIGHTNING_SEGMENTS;

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
			angle += (LOWORD (rand_val) & 1 ? -1 : 1) * (1 + HIBYTE ( HIWORD (rand_val)));
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
}

#define HEALMISSILE_SPEED 90
#define HEALMISSILE_TURN_WAIT 0

static void
healing_thingy_preprocess (PELEMENT ElementPtr)
{
	if (ElementPtr->turn_wait > 0)
		--ElementPtr->turn_wait;
	else
	{
		COUNT facing;
		STARSHIPPTR StarShipPtr;

		GetElementStarShip (ElementPtr, &StarShipPtr);

		facing = NORMALIZE_FACING (ANGLE_TO_FACING (
				GetVelocityTravelAngle (&ElementPtr->velocity)
				));

		ElementPtr->hTarget = StarShipPtr->hShip;
		TrackShip (ElementPtr, &facing);

		SetVelocityVector (&ElementPtr->velocity,
				HEALMISSILE_SPEED, facing);

		ElementPtr->turn_wait = HEALMISSILE_TURN_WAIT;
	}
}


static void
healing_thingy_collision (PELEMENT ElementPtr0, PPOINT pPt0, PELEMENT ElementPtr1, PPOINT pPt1)
{
	if ((ElementPtr1->state_flags & PLAYER_SHIP)
		&& ((ElementPtr0->state_flags & (GOOD_GUY | BAD_GUY)) == (ElementPtr1->state_flags & (GOOD_GUY | BAD_GUY))))
	{
		DeltaCrew(ElementPtr1, 1);
	}

	if(ElementPtr0->collision_func != ElementPtr1->collision_func) weapon_collision (ElementPtr0, pPt0, ElementPtr1, pPt1);
}

#define HEALMISSILE_OFFSET 20
#define HEALMISSILE_START (HEALMISSILE_OFFSET + HEALMISSILE_SPEED)

static void
spawn_healing_thingies (PELEMENT ElementPtr)
{
	COUNT i;
	STARSHIPPTR StarShipPtr;
	LASER_BLOCK LaserBlock;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	//LaserBlock.face = StarShipPtr->ShipFacing;
	LaserBlock.cx = ElementPtr->next.location.x;
	LaserBlock.cy = ElementPtr->next.location.y;
	//LaserBlock.ex = COSINE (FACING_TO_ANGLE (LaserBlock.face), HEALMISSILE_SPEED);
	//LaserBlock.ey = SINE (FACING_TO_ANGLE (LaserBlock.face), HEALMISSILE_SPEED);
	LaserBlock.sender = (ElementPtr->state_flags & (GOOD_GUY | BAD_GUY));
	LaserBlock.pixoffs = HEALMISSILE_OFFSET;
	LaserBlock.color = BUILD_COLOR (MAKE_RGB15 (0x03, 0x1F, 0x00), 0);

	for(i = 0; i < 4; ++i)
	{
		HELEMENT hLaser;
		
		//LaserBlock.face = StarShipPtr->ShipFacing + 3 + (3 * i) + ((i < 2) ? 0 : 1);
		//LaserBlock.face = NORMALIZE_FACING (LaserBlock.face);
		if(i==0)LaserBlock.face = NORMALIZE_FACING (StarShipPtr->ShipFacing + 3);
		if(i==1)LaserBlock.face = NORMALIZE_FACING (StarShipPtr->ShipFacing - 3);
		if(i==2)LaserBlock.face = NORMALIZE_FACING (StarShipPtr->ShipFacing + 6);
		if(i==3)LaserBlock.face = NORMALIZE_FACING (StarShipPtr->ShipFacing - 6);

		LaserBlock.ex = COSINE (FACING_TO_ANGLE (LaserBlock.face), HEALMISSILE_START);
		LaserBlock.ey = SINE (FACING_TO_ANGLE (LaserBlock.face), HEALMISSILE_START);

		hLaser = initialize_laser(&LaserBlock);

		if(hLaser)
		{
			ELEMENTPTR LaserPtr;
	
			LockElement (hLaser, &LaserPtr);
	
			SetElementStarShip (LaserPtr, StarShipPtr);
			LaserPtr->life_span = 60;
			LaserPtr->mass_points = 0;
			LaserPtr->turn_wait = HEALMISSILE_TURN_WAIT;
			LaserPtr->collision_func = healing_thingy_collision;
			LaserPtr->preprocess_func = healing_thingy_preprocess;
			SetVelocityVector (&LaserPtr->velocity,
				HEALMISSILE_SPEED, LaserBlock.face);
	
			UnlockElement (hLaser);
			PutElement (hLaser);
		}
	}
}

#define MISSILE_LIFE 12
#define MISSILE_INITIAL_SPEED 50
#define MISSILE_MAX_SPEED 150

static void
slylandro_missile_preprocess (PELEMENT ElementPtr)
{
	SIZE speed;
	COUNT facing;

	facing = GetFrameIndex (ElementPtr->next.image.frame);

#define MISSILE_THRUST 15
	if ((speed = MISSILE_INITIAL_SPEED +
			((MISSILE_LIFE - ElementPtr->life_span) *
			MISSILE_THRUST)) > MISSILE_MAX_SPEED)
		speed = MISSILE_MAX_SPEED;
	SetVelocityVector (&ElementPtr->velocity,
			speed, facing);
}

static void
spawn_slylandro_missile (PELEMENT ShipPtr)
{
#define MISSILE_DAMAGE 4
#define MISSILE_HITS 1
#define NUKE_OFFSET 8
	COUNT i;
	HELEMENT Missile;
	STARSHIPPTR StarShipPtr;
	MISSILE_BLOCK MissileBlock;

	GetElementStarShip (ShipPtr, &StarShipPtr);
	//MissileBlock.cx = ShipPtr->next.location.x;
	//MissileBlock.cy = ShipPtr->next.location.y;
	MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.weapon;
	MissileBlock.face = MissileBlock.index = StarShipPtr->ShipFacing;
	MissileBlock.sender = (ShipPtr->state_flags & (GOOD_GUY | BAD_GUY))
			| IGNORE_SIMILAR;
	MissileBlock.pixoffs = 21; //42;
	MissileBlock.speed = MISSILE_INITIAL_SPEED;
	MissileBlock.hit_points = MISSILE_HITS;
	MissileBlock.damage = MISSILE_DAMAGE;
	MissileBlock.life = MISSILE_LIFE;
	MissileBlock.preprocess_func = slylandro_missile_preprocess;
	MissileBlock.blast_offs = NUKE_OFFSET;

	for(i = 0; i < 2; ++i)
	{
		MissileBlock.cx = ShipPtr->next.location.x + COSINE(FACING_TO_ANGLE(MissileBlock.face + 4), DISPLAY_TO_WORLD(27) /*42*/ * (i*2 - 1));
		MissileBlock.cy = ShipPtr->next.location.y + SINE(FACING_TO_ANGLE(MissileBlock.face + 4), DISPLAY_TO_WORLD(27)  /*42*/ * (i*2 - 1));

		Missile = initialize_missile (&MissileBlock);
		if (Missile)
		{
			ELEMENTPTR MissilePtr;
	
			LockElement (Missile, &MissilePtr);
			UnlockElement (Missile);
			PutElement (Missile);
		}
	}
}

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

static BOOLEAN
harvest_space_junk (PELEMENT ElementPtr)
{
	BOOLEAN retval;
	HELEMENT hElement, hNextElement;

	retval = FALSE;
	for (hElement = GetHeadElement ();
			hElement; hElement = hNextElement)
	{
		ELEMENTPTR ObjPtr;

		LockElement (hElement, &ObjPtr);
		hNextElement = GetSuccElement (ObjPtr);

		if (!(ObjPtr->state_flags
				& (APPEARING | GOOD_GUY | BAD_GUY
				| PLAYER_SHIP | FINITE_LIFE))
				&& !GRAVITY_MASS (ObjPtr->mass_points)
				&& CollisionPossible (ObjPtr, ElementPtr))
		{
//HARVEST_RANGE was originally (SPACE_HEIGHT * 3 / 8)
#define HARVEST_RANGE (208 * 3 / 8)
			SIZE dx, dy;

			if ((dx = ObjPtr->next.location.x
					- ElementPtr->next.location.x) < 0)
				dx = -dx;
			if ((dy = ObjPtr->next.location.y
					- ElementPtr->next.location.y) < 0)
				dy = -dy;
			dx = WORLD_TO_DISPLAY (dx);
			dy = WORLD_TO_DISPLAY (dy);
			if (dx <= HARVEST_RANGE && dy <= HARVEST_RANGE
					&& dx * dx + dy * dy <= HARVEST_RANGE * HARVEST_RANGE)
			{
				ObjPtr->life_span = 0;
				ObjPtr->state_flags |= NONSOLID;

				if (!retval)
				{
					STARSHIPPTR StarShipPtr;

					retval = TRUE;

					GetElementStarShip (ElementPtr, &StarShipPtr);
					ProcessSound (SetAbsSoundIndex (
							StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1), ElementPtr);
					DeltaEnergy (ElementPtr, ASTEROID_ENERGY_RECHARGE /*MAX_ENERGY*/);
				}
			}
		}

		UnlockElement (hElement);
	}

	return (retval);
}

static void
slylandro_postprocess (PELEMENT ElementPtr)
{
	STARSHIPPTR StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	/*if (StarShipPtr->weapon_counter
			&& StarShipPtr->weapon_counter < WEAPON_WAIT)
	{
		HELEMENT Lightning;

		initialize_lightning (ElementPtr, &Lightning);
		if (Lightning)
			PutElement (Lightning);
	}*/

	/*if (StarShipPtr->special_counter == 0
			&& (StarShipPtr->cur_status_flags & SPECIAL)
			&& */harvest_space_junk (ElementPtr)/*)
	{
		StarShipPtr->special_counter =
				StarShipPtr->RaceDescPtr->characteristics.special_wait;
	}*/;

	if ((StarShipPtr->cur_status_flags & THRUST) && StarShipPtr->special_counter == 0)
	{
		spawn_slylandro_missile(ElementPtr);
		StarShipPtr->special_counter = SPECIAL_WAIT;
	}

	if (StarShipPtr->special_counter == 0
			&& (StarShipPtr->cur_status_flags & SPECIAL)
			&& DeltaEnergy(ElementPtr, -SPECIAL_ENERGY_COST))
	{
		spawn_healing_thingies(ElementPtr);
		
		StarShipPtr->special_counter = SPECIAL_WAIT;
	}
}

static void
slylandro_collision (PELEMENT ElementPtr0, PPOINT pPt0,
		PELEMENT ElementPtr1, PPOINT pPt1)
{
	STARSHIPPTR StarShipPtr;

	GetElementStarShip (ElementPtr0, &StarShipPtr);

	//if hitting a planet, reverse in a hurry
	if (!(ElementPtr1->state_flags & FINITE_LIFE) && GRAVITY_MASS (ElementPtr1->mass_points))
	{
		StarShipPtr->ShipFacing += ANGLE_TO_FACING (HALF_CIRCLE);
	}

	collision(ElementPtr0, pPt0, ElementPtr1, pPt1);
}

static COUNT slylandroes_present;

static void
slylandro_dispose_graphics (RACE_DESCPTR RaceDescPtr)
{
	--slylandroes_present;

	if(!slylandroes_present)
	{
		DestroyDrawable(ReleaseDrawable(RaceDescPtr->ship_data.weapon[0]));
		RaceDescPtr->ship_data.weapon[0] = (FRAME)0;
		DestroyDrawable(ReleaseDrawable(RaceDescPtr->ship_data.weapon[1]));
		RaceDescPtr->ship_data.weapon[1] = (FRAME)0;
		DestroyDrawable(ReleaseDrawable(RaceDescPtr->ship_data.weapon[2]));
		RaceDescPtr->ship_data.weapon[2] = (FRAME)0;
	}
}

static void
slylandro_preprocess (PELEMENT ElementPtr)
{
	STARSHIPPTR StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);

	if (ElementPtr->state_flags & APPEARING)
	{
		ElementPtr->collision_func = slylandro_collision;

		if(StarShipPtr->RaceDescPtr->ship_data.weapon[0] == (FRAME)0)
		{
			StarShipPtr->RaceDescPtr->ship_data.weapon[0] = CaptureDrawable(LoadCelFile("human/saturn.big"));
			StarShipPtr->RaceDescPtr->ship_data.weapon[1] = CaptureDrawable(LoadCelFile("human/saturn.med"));
			StarShipPtr->RaceDescPtr->ship_data.weapon[2] = CaptureDrawable(LoadCelFile("human/saturn.sml"));
		}
		++slylandroes_present;
	}

	if (!(ElementPtr->state_flags & (APPEARING | NONSOLID)))
	{
		/*if ((StarShipPtr->cur_status_flags & THRUST)
				&& !(StarShipPtr->old_status_flags & THRUST))
			StarShipPtr->ShipFacing += ANGLE_TO_FACING (HALF_CIRCLE);*/
		if(StarShipPtr->weapon_counter == 0 && StarShipPtr->special_counter > 0)
		{
			++StarShipPtr->weapon_counter; //can't lightning while firing missiles
		}

		if (ElementPtr->turn_wait == 0)
		{
			ElementPtr->turn_wait +=
					StarShipPtr->RaceDescPtr->characteristics.turn_wait + 1;
			if (StarShipPtr->cur_status_flags & LEFT)
				--StarShipPtr->ShipFacing;
			else if (StarShipPtr->cur_status_flags & RIGHT)
				++StarShipPtr->ShipFacing;
		}

		StarShipPtr->ShipFacing = NORMALIZE_FACING (StarShipPtr->ShipFacing);

		if (ElementPtr->thrust_wait == 0)
		{
			ElementPtr->thrust_wait +=
					StarShipPtr->RaceDescPtr->characteristics.thrust_wait + 1;

			extern void spawn_ion_trail (PELEMENT ElementPtr);

			spawn_ion_trail (ElementPtr);

			SetVelocityVector (&ElementPtr->velocity,
					StarShipPtr->RaceDescPtr->characteristics.max_thrust,
					StarShipPtr->ShipFacing);
			StarShipPtr->cur_status_flags |= SHIP_AT_MAX_SPEED;
			StarShipPtr->cur_status_flags &= ~SHIP_IN_GRAVITY_WELL;
		}

		ElementPtr->next.image.frame = IncFrameIndex (ElementPtr->next.image.frame);
		ElementPtr->next.image.frame = IncFrameIndex (ElementPtr->next.image.frame);
		ElementPtr->next.image.frame = IncFrameIndex (ElementPtr->next.image.frame);
		ElementPtr->state_flags |= CHANGING;
	}
}

RACE_DESCPTR
init_slylandro (void)
{
	RACE_DESCPTR RaceDescPtr;

	slylandro_desc.preprocess_func = slylandro_preprocess;
	slylandro_desc.postprocess_func = slylandro_postprocess;
	slylandro_desc.init_weapon_func = initialize_lightning;
	slylandro_desc.uninit_func = slylandro_dispose_graphics;
	slylandro_desc.cyborg_control.intelligence_func =
			(void (*) (PVOID ShipPtr, PVOID ObjectsOfConcern, COUNT
					ConcernCounter)) slylandro_intelligence;

	RaceDescPtr = &slylandro_desc;

	return (RaceDescPtr);
}
