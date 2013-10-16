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
#include "ships/mmrnmhrm/resinst.h"

#include "init.h"
#include "colors.h"
#include "globdata.h"


#define MAX_CREW 20
#define MAX_ENERGY 10
#define ENERGY_REGENERATION 2
#define WEAPON_ENERGY_COST 0 //1
#define SPECIAL_ENERGY_COST MAX_ENERGY
#define ENERGY_WAIT 6
#define MAX_THRUST 20
#define THRUST_INCREMENT 5
#define TURN_WAIT 1
#define THRUST_WAIT 1
#define WEAPON_WAIT 0
#define SPECIAL_WAIT 0

#define YWING_MAX_THRUST 80 //50
#define YWING_THRUST_INCREMENT 15 //10
#define YWING_THRUST_WAIT 0
#define YWING_TURN_WAIT 1

#define SHIP_MASS 7 //3
#define MMRNMHRM_OFFSET 16
#define LASER_RANGE DISPLAY_TO_WORLD (125 + MMRNMHRM_OFFSET)

static RACE_DESC mmrnmhrm_desc =
{
	{
		FIRES_FORE | IMMEDIATE_WEAPON,
		19, /* Super Melee cost */
		0 / SPHERE_RADIUS_INCREMENT, /* Initial sphere of influence radius */
		MAX_CREW, MAX_CREW,
		MAX_ENERGY, MAX_ENERGY,
		{
			0, 0,
		},
		(STRING)MMRNMHRM_RACE_STRINGS,
		(FRAME)MMRNMHRM_ICON_MASK_PMAP_ANIM,
		(FRAME)MMRNMHRM_MICON_MASK_PMAP_ANIM,
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
			(FRAME)MMRNMHRM_BIG_MASK_PMAP_ANIM,
			(FRAME)MMRNMHRM_MED_MASK_PMAP_ANIM,
			(FRAME)MMRNMHRM_SML_MASK_PMAP_ANIM,
		},
		{
			(FRAME)TORP_BIG_MASK_PMAP_ANIM,
			(FRAME)TORP_MED_MASK_PMAP_ANIM,
			(FRAME)TORP_SML_MASK_PMAP_ANIM,
		},
		{
			(FRAME)YWING_BIG_MASK_PMAP_ANIM,
			(FRAME)YWING_MED_MASK_PMAP_ANIM,
			(FRAME)YWING_SML_MASK_PMAP_ANIM,
		},
		{
			(FRAME)MMRNMHRM_CAPTAIN_MASK_PMAP_ANIM,
			(FRAME)0,
			(FRAME)0,
			(FRAME)0,
			(FRAME)0,
		},
		(SOUND)MMRNMHRM_VICTORY_SONG,
		(SOUND)MMRNMHRM_SHIP_SOUNDS,
	},
	{
		0,
		CLOSE_RANGE_WEAPON,
		NULL_PTR,
	},
	(UNINIT_FUNC *) NULL,
	(PREPROCESS_FUNC *) NULL,
	(POSTPROCESS_FUNC *) NULL,
	(INIT_WEAPON_FUNC *) NULL,
	0,
};

/*#define MISSILE_SPEED DISPLAY_TO_WORLD (20)
#define TRACK_WAIT 5

static void
missile_preprocess (PELEMENT ElementPtr)
{
	if (ElementPtr->turn_wait > 0)
		--ElementPtr->turn_wait;
	else
	{
		COUNT facing;

		facing = GetFrameIndex (ElementPtr->next.image.frame);
		if (TrackShip (ElementPtr, &facing) > 0)
		{
			ElementPtr->next.image.frame =
					SetAbsFrameIndex (ElementPtr->next.image.frame,
					facing);
			ElementPtr->state_flags |= CHANGING;

			SetVelocityVector (&ElementPtr->velocity,
					MISSILE_SPEED, facing);
		}

		ElementPtr->turn_wait = TRACK_WAIT;
	}
}*/

static void
mmrnmhrm_intelligence (PELEMENT ShipPtr, PEVALUATE_DESC ObjectsOfConcern, COUNT ConcernCounter)
{
	//BOOLEAN CanTransform;
	PEVALUATE_DESC lpEvalDesc;
	STARSHIPPTR StarShipPtr, EnemyStarShipPtr;

	GetElementStarShip (ShipPtr, &StarShipPtr);
	/*CanTransform = (BOOLEAN)(StarShipPtr->special_counter == 0
			&& StarShipPtr->RaceDescPtr->ship_info.energy_level >=
			StarShipPtr->RaceDescPtr->characteristics.special_energy_cost);*/

	StarShipPtr->ship_input_state |= WEAPON;
	StarShipPtr->ship_input_state &= ~(SPECIAL | THRUST);

	lpEvalDesc = &ObjectsOfConcern[ENEMY_SHIP_INDEX];
	if (lpEvalDesc->ObjectPtr)
	{
		GetElementStarShip (lpEvalDesc->ObjectPtr, &EnemyStarShipPtr);
		if(EnemyStarShipPtr && EnemyStarShipPtr->hShip)
		{
			SIZE dx, dy;
			COUNT angle;
			ELEMENTPTR EnemyShipPtr;
			LockElement(EnemyStarShipPtr->hShip, &EnemyShipPtr);
	
			dx = EnemyShipPtr->next.location.x - ShipPtr->next.location.x;
			dy = EnemyShipPtr->next.location.y - ShipPtr->next.location.y;
			dx = WRAP_DELTA_X(dx);
			dy = WRAP_DELTA_Y(dy);
			angle = ARCTAN (dx, dy);
			angle = NORMALIZE_ANGLE (angle - StarShipPtr->ShipFacing);
			if(angle == 0)
			{
				StarShipPtr->ship_input_state &= ~(LEFT | RIGHT);
			}
			else if(angle < HALF_CIRCLE)
			{
				StarShipPtr->ship_input_state |= RIGHT;
				StarShipPtr->ship_input_state &= ~LEFT;
			}
			else
			{
				StarShipPtr->ship_input_state |= LEFT;
				StarShipPtr->ship_input_state &= ~RIGHT;
			}
			UnlockElement(EnemyStarShipPtr->hShip);
		}
	}

	//ship_intelligence (ShipPtr, ObjectsOfConcern, ConcernCounter);
	//
	/*if (CanTransform
			&& lpEvalDesc->ObjectPtr
			&& !(StarShipPtr->ship_input_state & WEAPON))
	{
		SIZE delta_x, delta_y;
		COUNT travel_angle, direction_angle;

		GetCurrentVelocityComponents (&lpEvalDesc->ObjectPtr->velocity,
				&delta_x, &delta_y);
		if (delta_x == 0 && delta_y == 0)
			direction_angle = travel_angle = 0;
		else
		{
			delta_x = lpEvalDesc->ObjectPtr->current.location.x
					- ShipPtr->current.location.x;
			delta_y = lpEvalDesc->ObjectPtr->current.location.y
					- ShipPtr->current.location.y;
			direction_angle = ARCTAN (-delta_x, -delta_y);
			travel_angle = GetVelocityTravelAngle (
					&lpEvalDesc->ObjectPtr->velocity
					);
		}

		if (ShipPtr->next.image.farray == StarShipPtr->RaceDescPtr->ship_data.ship)
		{
			if (lpEvalDesc->which_turn > 8)
			{
				if (MANEUVERABILITY (&EnemyStarShipPtr->RaceDescPtr->cyborg_control) <= SLOW_SHIP
						|| NORMALIZE_ANGLE (
								direction_angle - travel_angle + QUADRANT
								) > HALF_CIRCLE)
					StarShipPtr->ship_input_state |= SPECIAL;
			}
		}
		else
		{
			SIZE ship_delta_x, ship_delta_y;

			GetCurrentVelocityComponents (&ShipPtr->velocity,
					&ship_delta_x, &ship_delta_y);
			delta_x -= ship_delta_x;
			delta_y -= ship_delta_y;
			travel_angle = ARCTAN (delta_x, delta_y);
			if (lpEvalDesc->which_turn < 16)
			{
				if (lpEvalDesc->which_turn <= 8
						|| NORMALIZE_ANGLE (
								direction_angle - travel_angle + OCTANT
								) <= QUADRANT)
					StarShipPtr->ship_input_state |= SPECIAL;
			}
			else if (lpEvalDesc->which_turn > 32
					&& NORMALIZE_ANGLE (
							direction_angle - travel_angle + QUADRANT
							) > HALF_CIRCLE)
				StarShipPtr->ship_input_state |= SPECIAL;
		}
	}*/

	/*if (ShipPtr->current.image.farray == StarShipPtr->RaceDescPtr->ship_data.special)
	{
		if (!(StarShipPtr->ship_input_state & SPECIAL)
				&& lpEvalDesc->ObjectPtr)
			StarShipPtr->ship_input_state |= WEAPON;
		else
			StarShipPtr->ship_input_state &= ~WEAPON;
	}*/
}

static void
spawn_indicator_laser (PELEMENT ShipPtr)
{
	HELEMENT Laser;
	STARSHIPPTR StarShipPtr;
	LASER_BLOCK LaserBlock;

	GetElementStarShip (ShipPtr, &StarShipPtr);
	LaserBlock.face = StarShipPtr->ShipFacing / 4;
	LaserBlock.cx = ShipPtr->next.location.x;
	LaserBlock.cy = ShipPtr->next.location.y;
	LaserBlock.ex = COSINE (StarShipPtr->ShipFacing, 360);
	LaserBlock.ey = SINE (StarShipPtr->ShipFacing, 360);
	LaserBlock.sender = (ShipPtr->state_flags & (GOOD_GUY | BAD_GUY))
			| IGNORE_SIMILAR | NONSOLID;
	LaserBlock.pixoffs = 0;
	LaserBlock.color = BUILD_COLOR (MAKE_RGB15 (0x18, 0x18, 0x1F), 0x0A);
	Laser = initialize_laser (&LaserBlock);

	if(Laser)
	{
		ELEMENTPTR LaserPtr;
		LockElement(Laser, &LaserPtr);
		LaserPtr->mass_points = 0;
		SetElementStarShip(LaserPtr, StarShipPtr);
		UnlockElement(Laser);
		PutElement(Laser);
	}
}

static void
xform_postprocess (PELEMENT ElementPtr)
{
	STARSHIPPTR StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);

	
	spawn_indicator_laser(ElementPtr);


	if (true //(StarShipPtr->cur_status_flags & WEAPON)
			/*&& DeltaEnergy (ElementPtr, -WEAPON_ENERGY_COST)*/)
	{
		ELEMENTPTR AsteroidElementPtr;
		HELEMENT hElement, hNextElement;
		for (hElement = GetHeadElement (); hElement != 0; hElement = hNextElement)
		{
			LockElement (hElement, &AsteroidElementPtr);
			hNextElement = GetSuccElement (AsteroidElementPtr);
		
#define NUM_SHADOWS 1
			if (!(AsteroidElementPtr->state_flags
				& (APPEARING | GOOD_GUY | BAD_GUY
				| PLAYER_SHIP | FINITE_LIFE))
				&& !GRAVITY_MASS (AsteroidElementPtr->mass_points)
				&& CollisionPossible (AsteroidElementPtr, ElementPtr))
			{
				SIZE i, dx, dy;
				COUNT angle, magnitude;
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

				// calculate tractor beam effect
				angle = FACING_TO_ANGLE (StarShipPtr->ShipFacing);
				dx = ElementPtr->next.location.x
						- AsteroidElementPtr->next.location.x;
				dy = ElementPtr->next.location.y
						- AsteroidElementPtr->next.location.y;
				angle = ARCTAN (dx, dy);
				magnitude = WORLD_TO_VELOCITY (48) /*(12)*/ / AsteroidElementPtr->mass_points;
				DeltaVelocityComponents (&AsteroidElementPtr->velocity,
						COSINE (angle, magnitude), SINE (angle, magnitude));

			GetCurrentVelocityComponents(&AsteroidElementPtr->velocity, &dx, &dy);
			dx = (dx * 9) / 10;
			dy = (dy * 9) / 10;
			SetVelocityComponents(&AsteroidElementPtr->velocity, dx, dy);

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
								| (AsteroidElementPtr->state_flags & (GOOD_GUY | BAD_GUY));
						ShadowElementPtr->life_span = 1;

						ShadowElementPtr->current = AsteroidElementPtr->next;
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
			UnlockElement (hElement);
		}
	}
}

static void
mmrnmhrm_asteroid_collision (PELEMENT ElementPtr0, PPOINT pPt0, PELEMENT ElementPtr1, PPOINT pPt1)
{
	//if it's an asteroid
	if((!(ElementPtr1->state_flags
			& (APPEARING | GOOD_GUY | BAD_GUY
			| PLAYER_SHIP | FINITE_LIFE))
			&& !GRAVITY_MASS (ElementPtr1->mass_points)
			&& CollisionPossible (ElementPtr1, ElementPtr0)))
	{
		//pass right through it
	}
	else weapon_collision (ElementPtr0, pPt0, ElementPtr1, pPt1);
	if(ElementPtr0->state_flags & DISAPPEARING)
	{
		ElementPtr0->state_flags &= ~DISAPPEARING;
	}
	if(GRAVITY_MASS(ElementPtr1->mass_points))
	{
		ElementPtr0->crew_level = 0;
	}
}

extern void spin_asteroid (PELEMENT ElementPtr);
extern void asteroid_preprocess (PELEMENT ElementPtr);

#define ASTEROID_FIRING_SPEED 130 //200
#define MIN_DANGEROUS_SPEED 50

static void
mmrnmhrm_asteroid_preprocess (PELEMENT ElementPtr)
{
	SIZE dx, dy;
	COUNT old_turn_wait;

	old_turn_wait = ElementPtr->turn_wait;
	ElementPtr->turn_wait = 0;
	spin_asteroid(ElementPtr);
	ElementPtr->turn_wait = old_turn_wait;
	
	--ElementPtr->turn_wait;

	GetCurrentVelocityComponents(&ElementPtr->velocity, &dx, &dy);
	if((long)dx*(long)dx + (long)dy*(long)dy < (long)MIN_DANGEROUS_SPEED*(long)MIN_DANGEROUS_SPEED
		|| ElementPtr->turn_wait == 0)
	{
		ElementPtr->mass_points = 3;
		ElementPtr->crew_level = 1;
		ElementPtr->pParent = NULL;
		ElementPtr->collision_func = collision;
		ElementPtr->preprocess_func = asteroid_preprocess;
		ElementPtr->state_flags &= ~(GOOD_GUY | BAD_GUY | IGNORE_SIMILAR | PERSISTENT);
	}
}

static void
mmrnmhrm_asteroid_init (PELEMENT ElementPtr)
{
	STARSHIPPTR StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);

	//SetVelocityVector(&ElementPtr->velocity, ASTEROID_FIRING_SPEED, StarShipPtr->ShipFacing);
	SetVelocityComponents(&ElementPtr->velocity,
			COSINE(StarShipPtr->ShipFacing, WORLD_TO_VELOCITY(ASTEROID_FIRING_SPEED)),
			SINE(StarShipPtr->ShipFacing, WORLD_TO_VELOCITY(ASTEROID_FIRING_SPEED)));
	ElementPtr->mass_points = 6;
	ElementPtr->crew_level = 2;
	ElementPtr->turn_wait = 48; //frames until it stops being dangerous
	//ElementPtr->thrust_wait = 0;
	ElementPtr->collision_func = mmrnmhrm_asteroid_collision;
	ElementPtr->preprocess_func = mmrnmhrm_asteroid_preprocess;
}

static void
xform_collision (PELEMENT ElementPtr0, PPOINT pPt0, PELEMENT ElementPtr1, PPOINT pPt1)
{
	STARSHIPPTR StarShipPtr;

	GetElementStarShip (ElementPtr0, &StarShipPtr);
	//if it's an asteroid
	if ((!(ElementPtr1->state_flags
			& (APPEARING | GOOD_GUY | BAD_GUY
			| PLAYER_SHIP | FINITE_LIFE))
			&& !GRAVITY_MASS (ElementPtr1->mass_points)
			&& CollisionPossible (ElementPtr1, ElementPtr0)))
	{
		if (StarShipPtr->cur_status_flags & WEAPON)
		{
			//bit of recoil. no sitting still and sniping for you!
			DeltaVelocityComponents(&ElementPtr0->velocity,
					COSINE(StarShipPtr->ShipFacing, -100),
					SINE(StarShipPtr->ShipFacing, -100));
			
			//the real effect:
			SetElementStarShip(ElementPtr1, StarShipPtr);
			ElementPtr1->pParent = ElementPtr0->pParent;
			ElementPtr1->state_flags |= (ElementPtr0->state_flags & (GOOD_GUY | BAD_GUY))
				| IGNORE_SIMILAR | PERSISTENT;
			ElementPtr1->preprocess_func = mmrnmhrm_asteroid_init;

			ProcessSound (SetAbsSoundIndex (
					StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 0), ElementPtr0);
		}
	}
	else collision (ElementPtr0, pPt0, ElementPtr1, pPt1);
}

static void
yform_preprocess (PELEMENT ElementPtr);

static void
xform_preprocess (PELEMENT ElementPtr)
{
	STARSHIPPTR StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);

	if(ElementPtr->next.image.farray != StarShipPtr->RaceDescPtr->ship_data.ship)
	{
		ElementPtr->next.image.farray = StarShipPtr->RaceDescPtr->ship_data.ship;
		ElementPtr->next.image.frame = SetAbsFrameIndex(StarShipPtr->RaceDescPtr->ship_data.ship[0], NORMALIZE_FACING((StarShipPtr->ShipFacing + 2) / 4));
		ElementPtr->state_flags |= CHANGING;
	}

	ElementPtr->collision_func = xform_collision;

	if(ElementPtr->turn_wait)
		--ElementPtr->turn_wait;
	else
	{
		if(StarShipPtr->cur_status_flags & LEFT)
		{
			--StarShipPtr->ShipFacing;	
			ElementPtr->turn_wait = StarShipPtr->RaceDescPtr->characteristics.turn_wait;
		}
		if(StarShipPtr->cur_status_flags & RIGHT)
		{
			++StarShipPtr->ShipFacing;
			ElementPtr->turn_wait = StarShipPtr->RaceDescPtr->characteristics.turn_wait;
		}
	}

	StarShipPtr->ShipFacing = NORMALIZE_ANGLE (StarShipPtr->ShipFacing);
	ElementPtr->next.image.frame = SetAbsFrameIndex(ElementPtr->next.image.frame, NORMALIZE_FACING((StarShipPtr->ShipFacing + 2) / 4));
	ElementPtr->state_flags |= CHANGING;

	if(ElementPtr->thrust_wait)
		--ElementPtr->thrust_wait;
	else if(StarShipPtr->cur_status_flags & THRUST)
	{
		COUNT real_facing = StarShipPtr->ShipFacing;
		StarShipPtr->ShipFacing = NORMALIZE_FACING((StarShipPtr->ShipFacing + 2) / 4);

		//stolen from ship.c:
		UWORD thrust_status;

		thrust_status = inertial_thrust ((ELEMENTPTR)ElementPtr);
		StarShipPtr->cur_status_flags &=
				~(SHIP_AT_MAX_SPEED
				| SHIP_BEYOND_MAX_SPEED
				| SHIP_IN_GRAVITY_WELL);
		StarShipPtr->cur_status_flags |= thrust_status;

		ElementPtr->thrust_wait = StarShipPtr->RaceDescPtr->characteristics.thrust_wait;

		extern void spawn_ion_trail (PELEMENT ElementPtr);

		spawn_ion_trail (ElementPtr);
		//end copied code
		
		StarShipPtr->ShipFacing = real_facing;
	}
	++ElementPtr->turn_wait;
	++ElementPtr->thrust_wait;
	++StarShipPtr->weapon_counter;

	if ((StarShipPtr->cur_status_flags & SPECIAL)
			&& DeltaEnergy (ElementPtr, -SPECIAL_ENERGY_COST))
	{
		ProcessSound (SetAbsSoundIndex (
					StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1), ElementPtr);

		StarShipPtr->ShipFacing = NORMALIZE_FACING((StarShipPtr->ShipFacing + 2) / 4);
		ElementPtr->next.image.farray = StarShipPtr->RaceDescPtr->ship_data.special;
		ElementPtr->next.image.frame = SetAbsFrameIndex(StarShipPtr->RaceDescPtr->ship_data.special[0], StarShipPtr->ShipFacing);
		StarShipPtr->RaceDescPtr->preprocess_func = yform_preprocess;
		StarShipPtr->RaceDescPtr->postprocess_func = NULL_PTR;
		ElementPtr->collision_func = collision;

		StarShipPtr->RaceDescPtr->characteristics.max_thrust = YWING_MAX_THRUST;
		StarShipPtr->RaceDescPtr->characteristics.thrust_increment = YWING_THRUST_INCREMENT;
		StarShipPtr->RaceDescPtr->characteristics.thrust_wait = YWING_THRUST_WAIT;
		StarShipPtr->RaceDescPtr->characteristics.turn_wait = YWING_TURN_WAIT;
	}
}

/*static SIZE mmrnquake_xoffs;
static SIZE mmrnquake_yoffs;*/

static void
yform_preprocess (PELEMENT ElementPtr)
{
	STARSHIPPTR StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);

	if(ElementPtr->next.image.farray != StarShipPtr->RaceDescPtr->ship_data.special)
	{
		ElementPtr->next.image.farray = StarShipPtr->RaceDescPtr->ship_data.special;
		ElementPtr->next.image.frame = SetAbsFrameIndex(StarShipPtr->RaceDescPtr->ship_data.special[0], StarShipPtr->ShipFacing);
		ElementPtr->state_flags |= CHANGING;
	}

	/*{
		SIZE old_xoffs, old_yoffs;
		ELEMENTPTR OtherElementPtr;
		HELEMENT hElement, hNextElement;

		old_xoffs = mmrnquake_xoffs; old_yoffs = mmrnquake_yoffs;
		mmrnquake_xoffs += TFB_Random()&255 - 127 - (mmrnquake_xoffs / 10);
		mmrnquake_yoffs = TFB_Random()&255 - 127 - (mmrnquake_yoffs / 10);

		for (hElement = GetHeadElement (); hElement != 0; hElement = hNextElement)
		{
			LockElement (hElement, &OtherElementPtr);
			hNextElement = GetSuccElement (OtherElementPtr);
			
			if((OtherElementPtr->state_flags & PLAYER_SHIP))
			{
				OtherElementPtr->next.location.x += mmrnquake_xoffs - old_xoffs;
				OtherElementPtr->next.location.y += mmrnquake_yoffs - old_yoffs;
				OtherElementPtr->state_flags |= CHANGING;
			}

			UnlockElement (hElement);
		}
	}*/

	++StarShipPtr->weapon_counter;

	if ((StarShipPtr->cur_status_flags & SPECIAL)
			&& DeltaEnergy (ElementPtr, -SPECIAL_ENERGY_COST))
	{
		//SIZE dx, dy;

		ProcessSound (SetAbsSoundIndex (
					StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1), ElementPtr);

		ElementPtr->next.image.frame = SetAbsFrameIndex(StarShipPtr->RaceDescPtr->ship_data.special[0], StarShipPtr->ShipFacing);
		StarShipPtr->RaceDescPtr->preprocess_func = xform_preprocess;
		StarShipPtr->RaceDescPtr->postprocess_func = xform_postprocess;
		StarShipPtr->ShipFacing = StarShipPtr->ShipFacing * 4;
		ElementPtr->next.image.farray = StarShipPtr->RaceDescPtr->ship_data.ship;
		ElementPtr->collision_func = xform_collision;

		StarShipPtr->RaceDescPtr->characteristics.max_thrust = MAX_THRUST;
		StarShipPtr->RaceDescPtr->characteristics.thrust_increment = THRUST_INCREMENT;
		StarShipPtr->RaceDescPtr->characteristics.thrust_wait = THRUST_WAIT;
		StarShipPtr->RaceDescPtr->characteristics.turn_wait = TURN_WAIT;

		/*GetCurrentVelocityComponents(&ElementPtr->velocity, &dx, &dy);
		dx = dx * MAX_THRUST / YWING_MAX_THRUST;
		dy = dy * MAX_THRUST / YWING_MAX_THRUST;
		SetVelocityComponents(&ElementPtr->velocity, dx, dy);*/
	}
}

static COUNT
initialize_nothing (PELEMENT ShipPtr, HELEMENT NothingArray[])
{	
	return (0);
}

RACE_DESCPTR
init_mmrnmhrm (void)
{
	RACE_DESCPTR RaceDescPtr;

	mmrnmhrm_desc.preprocess_func = xform_preprocess;
	mmrnmhrm_desc.postprocess_func = xform_postprocess;
	mmrnmhrm_desc.init_weapon_func = initialize_nothing;
	mmrnmhrm_desc.cyborg_control.intelligence_func =
			(void (*) (PVOID ShipPtr, PVOID ObjectsOfConcern, COUNT
					ConcernCounter)) mmrnmhrm_intelligence;

	RaceDescPtr = &mmrnmhrm_desc;

	return (RaceDescPtr);
}

