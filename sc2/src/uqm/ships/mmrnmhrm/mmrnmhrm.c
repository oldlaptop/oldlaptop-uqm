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
#include "mmrnmhrm.h"
#include "resinst.h"

#include "uqm/colors.h"
#include "uqm/globdata.h"

// Core characteristics
#define MAX_CREW 20
#define MAX_ENERGY 10
#define SHIP_MASS 3

// X-Wing characteristics
#define ENERGY_REGENERATION 2
#define ENERGY_WAIT 6
#define MAX_THRUST 20
#define THRUST_INCREMENT 5
#define THRUST_WAIT 1
#define TURN_WAIT 1 /* Actually turns quite slowly (64-angle turning!) */

// Y-Wing characteristics
#define YWING_ENERGY_REGENERATION 1
#define YWING_SPECIAL_ENERGY_COST MAX_ENERGY
#define YWING_ENERGY_WAIT 6
#define YWING_MAX_THRUST 80
#define YWING_THRUST_INCREMENT 15
#define YWING_THRUST_WAIT 0
#define YWING_TURN_WAIT 1

// X-Wing Lasers
#define WEAPON_ENERGY_COST 1
#define WEAPON_WAIT 0

// Asteroid tossing
#define ASTEROID_FIRING_SPEED 130 //200
#define MIN_DANGEROUS_SPEED 50

// Transform
#define SPECIAL_ENERGY_COST MAX_ENERGY
#define SPECIAL_WAIT 0
#define YWING_SPECIAL_WAIT 0

// Tractor asteroids
#define NUM_SHADOWS 1

static RACE_DESC mmrnmhrm_desc =
{
	{ /* SHIP_INFO */
		"xform",
		FIRES_FORE | IMMEDIATE_WEAPON,
		19, /* Super Melee cost */
		MAX_CREW, MAX_CREW,
		MAX_ENERGY, MAX_ENERGY,
		MMRNMHRM_RACE_STRINGS,
		MMRNMHRM_ICON_MASK_PMAP_ANIM,
		MMRNMHRM_MICON_MASK_PMAP_ANIM,
		NULL, NULL, NULL
	},
	{ /* FLEET_STUFF */
		0, /* Initial sphere of influence radius */
		{ /* Known location (center of SoI) */
			0, 0,
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
			MMRNMHRM_BIG_MASK_PMAP_ANIM,
			MMRNMHRM_MED_MASK_PMAP_ANIM,
			MMRNMHRM_SML_MASK_PMAP_ANIM,
		},
		{
			TORP_BIG_MASK_PMAP_ANIM,
			TORP_MED_MASK_PMAP_ANIM,
			TORP_SML_MASK_PMAP_ANIM,
		},
		{
			YWING_BIG_MASK_PMAP_ANIM,
			YWING_MED_MASK_PMAP_ANIM,
			YWING_SML_MASK_PMAP_ANIM,
		},
		{
			MMRNMHRM_CAPTAIN_MASK_PMAP_ANIM,
			NULL, NULL, NULL, NULL, NULL
		},
		MMRNMHRM_VICTORY_SONG,
		MMRNMHRM_SHIP_SOUNDS,
		{ NULL, NULL, NULL },
		{ NULL, NULL, NULL },
		{ NULL, NULL, NULL },
		NULL, NULL
	},
	{
		0,
		CLOSE_RANGE_WEAPON,
		NULL,
	},
	(UNINIT_FUNC *) NULL,
	(PREPROCESS_FUNC *) NULL,
	(POSTPROCESS_FUNC *) NULL,
	(INIT_WEAPON_FUNC *) NULL,
	0,
	0, /* CodeRef */
};

// Private per-instance ship data
typedef CHARACTERISTIC_STUFF MMRNMHRM_DATA;

// Local typedef
typedef MMRNMHRM_DATA CustomShipData_t;

// Retrieve race-specific ship data from a race desc
static CustomShipData_t *
GetCustomShipData (RACE_DESC *pRaceDesc)
{
	return pRaceDesc->data;
}

// Set the race-specific data in a race desc
// (Re)Allocates its own storage for the data.
static void
SetCustomShipData (RACE_DESC *pRaceDesc, const CustomShipData_t *data)
{
	if (pRaceDesc->data == data) 
		return;  // no-op

	if (pRaceDesc->data) // Out with the old
	{
		HFree (pRaceDesc->data);
		pRaceDesc->data = NULL;
	}

	if (data) // In with the new
	{
		CustomShipData_t* newData = HMalloc (sizeof (*data));
		*newData = *data;
		pRaceDesc->data = newData;
	}
}

static void
mmrnmhrm_intelligence (ELEMENT *ShipPtr, EVALUATE_DESC *ObjectsOfConcern,
		COUNT ConcernCounter)
{
	BOOLEAN CanTransform;
	EVALUATE_DESC *lpEvalDesc;
	STARSHIP *StarShipPtr;
	STARSHIP *EnemyStarShipPtr = NULL;

	GetElementStarShip (ShipPtr, &StarShipPtr);
	CanTransform = (BOOLEAN)(StarShipPtr->special_counter == 0
			&& StarShipPtr->RaceDescPtr->ship_info.energy_level >=
			StarShipPtr->RaceDescPtr->characteristics.special_energy_cost);

	lpEvalDesc = &ObjectsOfConcern[ENEMY_SHIP_INDEX];
	if (lpEvalDesc->ObjectPtr)
	{
		GetElementStarShip (lpEvalDesc->ObjectPtr, &EnemyStarShipPtr);
	}

	ship_intelligence (ShipPtr, ObjectsOfConcern, ConcernCounter);

	StarShipPtr->ship_input_state &= ~SPECIAL;
	if (CanTransform
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
	}

	if (ShipPtr->current.image.farray == StarShipPtr->RaceDescPtr->ship_data.special)
	{
		if (!(StarShipPtr->ship_input_state & SPECIAL)
				&& lpEvalDesc->ObjectPtr)
			StarShipPtr->ship_input_state |= WEAPON;
		else
			StarShipPtr->ship_input_state &= ~WEAPON;
	}
}

static void
mmrnmhrm_asteroid_collision (ELEMENT *ElementPtr0, POINT *pPt0, ELEMENT *ElementPtr1, POINT *pPt1)
{
	//if it's an asteroid
	if ((!(ElementPtr1->state_flags
			& (APPEARING | PLAYER_SHIP | FINITE_LIFE))
			&& !GRAVITY_MASS (ElementPtr1->mass_points)
			&& ElementPtr1->playerNr == NEUTRAL_PLAYER_NUM
			&& CollisionPossible (ElementPtr1, ElementPtr0)))
	{
		//pass right through it
	}
	else
		weapon_collision (ElementPtr0, pPt0, ElementPtr1, pPt1);

	if (ElementPtr0->state_flags & DISAPPEARING)
	{
		ElementPtr0->state_flags &= ~DISAPPEARING;
	}
	if (GRAVITY_MASS (ElementPtr1->mass_points))
	{
		ElementPtr0->crew_level = 0;
	}
}

static void
mmrnmhrm_asteroid_preprocess (ELEMENT *ElementPtr)
{
	SIZE dx, dy;
	COUNT old_turn_wait;

	old_turn_wait = ElementPtr->turn_wait;
	ElementPtr->turn_wait = 0;
	spin_asteroid (ElementPtr);
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
		ElementPtr->state_flags &= ~(IGNORE_SIMILAR | PERSISTENT);
	}
}

static void
mmrnmhrm_asteroid_init (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);

	SetVelocityComponents(&ElementPtr->velocity,
			COSINE(StarShipPtr->ShipFacing, WORLD_TO_VELOCITY(ASTEROID_FIRING_SPEED)),
			SINE(StarShipPtr->ShipFacing, WORLD_TO_VELOCITY(ASTEROID_FIRING_SPEED)));
	ElementPtr->mass_points = 6;
	ElementPtr->crew_level = 2;
	ElementPtr->turn_wait = 48; //frames until it stops being dangerous
	ElementPtr->collision_func = mmrnmhrm_asteroid_collision;
	ElementPtr->preprocess_func = mmrnmhrm_asteroid_preprocess;
}

/* This is the entry point for crazy Mmrnmhrm's sole weapon - when xform
 * collides with an asteroid while WEAPON is pressed, the asteroid is fired */
static void
xform_collision (ELEMENT *ElementPtr0, POINT *pPt0, ELEMENT *ElementPtr1, POINT *pPt1)
{
	STARSHIP *StarShipPtr;

	GetElementStarShip (ElementPtr0, &StarShipPtr);
	//if it's an asteroid
	if ((!(ElementPtr1->state_flags
			& (APPEARING | PLAYER_SHIP | FINITE_LIFE))
			&& ElementPtr1->playerNr == NEUTRAL_PLAYER_NUM
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
			ElementPtr1->playerNr = ElementPtr0->playerNr;
			ElementPtr1->state_flags |= IGNORE_SIMILAR | PERSISTENT;
			ElementPtr1->preprocess_func = mmrnmhrm_asteroid_init;

			ProcessSound (SetAbsSoundIndex (
					StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 0), ElementPtr0);
		}
	}
	else
	{
		collision (ElementPtr0, pPt0, ElementPtr1, pPt1);
	}
}

static void
spawn_indicator_laser (ELEMENT *ShipPtr)
{
	HELEMENT Laser;
	STARSHIP *StarShipPtr;
	LASER_BLOCK LaserBlock;

	GetElementStarShip (ShipPtr, &StarShipPtr);
	LaserBlock.face = StarShipPtr->ShipFacing / 4;
	LaserBlock.cx = ShipPtr->next.location.x;
	LaserBlock.cy = ShipPtr->next.location.y;
	LaserBlock.ex = COSINE (StarShipPtr->ShipFacing, 360);
	LaserBlock.ey = SINE (StarShipPtr->ShipFacing, 360);
	LaserBlock.sender = ShipPtr->playerNr;
	LaserBlock.flags = IGNORE_SIMILAR | NONSOLID;
	LaserBlock.pixoffs = 0;
	LaserBlock.color = BUILD_COLOR (MAKE_RGB15 (0x18, 0x18, 0x1F), 0x0A);
	Laser = initialize_laser (&LaserBlock);

	if(Laser)
	{
		ELEMENT *LaserPtr;
		LockElement(Laser, &LaserPtr);
		LaserPtr->mass_points = 0;
		SetElementStarShip(LaserPtr, StarShipPtr);
		UnlockElement(Laser);
		PutElement(Laser);
	}
}

static void
tractor_asteroids (ELEMENT * ElementPtr)
{
	STARSHIP *StarShipPtr;
	ELEMENT *AsteroidElementPtr;
	HELEMENT hElement, hNextElement;

	GetElementStarShip (ElementPtr, &StarShipPtr);

	for (hElement = GetHeadElement (); hElement != 0;
			hElement = hNextElement)
	{
		LockElement (hElement, &AsteroidElementPtr);
		hNextElement = GetSuccElement (AsteroidElementPtr);

		if (!(AsteroidElementPtr->state_flags & (APPEARING | PLAYER_SHIP | FINITE_LIFE))
				&& !GRAVITY_MASS (AsteroidElementPtr->mass_points)
				&& AsteroidElementPtr->playerNr == NEUTRAL_PLAYER_NUM
				&& CollisionPossible (AsteroidElementPtr, ElementPtr))
		{
			SIZE i, dx, dy;
			COUNT angle, magnitude;

			static const SIZE shadow_offs[] = {
				DISPLAY_TO_WORLD (8),
				DISPLAY_TO_WORLD (8 + 9),
				DISPLAY_TO_WORLD (8 + 9 + 11),
				DISPLAY_TO_WORLD (8 + 9 + 11 + 14),
				DISPLAY_TO_WORLD (8 + 9 + 11 + 14 + 18),
			};
			static const Color color_tab[] = {
				BUILD_COLOR (MAKE_RGB15_INIT (0x00, 0x00, 0x10), 0x53),
				BUILD_COLOR (MAKE_RGB15_INIT (0x00, 0x00, 0x0E), 0x54),
				BUILD_COLOR (MAKE_RGB15_INIT (0x00, 0x00, 0x0C), 0x55),
				BUILD_COLOR (MAKE_RGB15_INIT (0x00, 0x00, 0x09), 0x56),
				BUILD_COLOR (MAKE_RGB15_INIT (0x00, 0x00, 0x07), 0x57),
			};

			// calculate tractor beam effect
			angle = FACING_TO_ANGLE (StarShipPtr->ShipFacing);
			dx = ElementPtr->next.location.x -
					AsteroidElementPtr->next.location.x;
			dy = ElementPtr->next.location.y -
					AsteroidElementPtr->next.location.y;
			angle = ARCTAN (dx, dy);
			magnitude = WORLD_TO_VELOCITY (48) /*(12) */  /
					AsteroidElementPtr->mass_points;
			DeltaVelocityComponents (&AsteroidElementPtr->velocity,
					COSINE (angle, magnitude), SINE (angle, magnitude));

			GetCurrentVelocityComponents (&AsteroidElementPtr->velocity,
					&dx, &dy);
			dx = (dx * 9) / 10;
			dy = (dy * 9) / 10;
			SetVelocityComponents (&AsteroidElementPtr->velocity, dx, dy);

			// add tractor beam graphical effects
			for (i = 0; i < NUM_SHADOWS; ++i)
			{
				HELEMENT hShadow;

				hShadow = AllocElement ();
				if (hShadow)
				{
					ELEMENT *ShadowElementPtr;

					LockElement (hShadow, &ShadowElementPtr);

					ShadowElementPtr->state_flags =
							FINITE_LIFE | NONSOLID | IGNORE_SIMILAR |
							POST_PROCESS;
					ShadowElementPtr->playerNr = AsteroidElementPtr->playerNr;
					ShadowElementPtr->life_span = 1;

					ShadowElementPtr->current = AsteroidElementPtr->next;
					ShadowElementPtr->current.location.x +=
							COSINE (angle, shadow_offs[i]);
					ShadowElementPtr->current.location.y +=
							SINE (angle, shadow_offs[i]);
					ShadowElementPtr->next = ShadowElementPtr->current;

					SetElementStarShip (ShadowElementPtr, StarShipPtr);
					SetVelocityComponents (&ShadowElementPtr->velocity, dx,
							dy);

					SetPrimType (&(GLOBAL (DisplayArray))
							[ShadowElementPtr->PrimIndex], STAMPFILL_PRIM);
					SetPrimColor (&(GLOBAL (DisplayArray))
							[ShadowElementPtr->PrimIndex], color_tab[i]);

					UnlockElement (hShadow);
					InsertElement (hShadow, GetHeadElement ());
				}
			}
			UnlockElement (hElement);
		}
	}
}

static inline void
xform_postprocess (ELEMENT *ElementPtr)
{
	spawn_indicator_laser (ElementPtr);
	tractor_asteroids (ElementPtr);
}

static inline void
yform_postprocess (ELEMENT *ElementPtr)
{
	/* We don't actually need to do anything here yet, kept for future
	 * expansion */
	(void) (ElementPtr);
}

static void
mmrnmhrm_postprocess (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);

	{
		BOOLEAN is_xform;

		is_xform = (ElementPtr->current.image.farray ==
				StarShipPtr->RaceDescPtr->ship_data.ship);

		switch (is_xform)
		{
			/* In X form */
			case (TRUE):
			{
				xform_postprocess (ElementPtr);
				break;
			}
			/* In Y form */
			case (FALSE):
			{
				yform_postprocess (ElementPtr);
				break;
			}
			default:
			{
				/* this should never ever happen */
				assert (false);
			}
		}
	}

			/* take care of transform effect */
	if (ElementPtr->next.image.farray != ElementPtr->current.image.farray)
	{
		MMRNMHRM_DATA tempShipData;
		MMRNMHRM_DATA *otherwing_desc;

		ProcessSound (SetAbsSoundIndex (
						/* TRANSFORM */
				StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1), ElementPtr);

		StarShipPtr->weapon_counter = 0;

		/* Swap characteristics descriptors around */
		otherwing_desc = GetCustomShipData (StarShipPtr->RaceDescPtr);
		if (!otherwing_desc)
			return;  // No ship data (?!)

		tempShipData = *otherwing_desc;
		SetCustomShipData (StarShipPtr->RaceDescPtr, &StarShipPtr->RaceDescPtr->characteristics);
		StarShipPtr->RaceDescPtr->characteristics = tempShipData;
		StarShipPtr->RaceDescPtr->cyborg_control.ManeuverabilityIndex = 0;

		if (ElementPtr->next.image.farray == StarShipPtr->RaceDescPtr->ship_data.special)
		{
			StarShipPtr->RaceDescPtr->cyborg_control.WeaponRange = LONG_RANGE_WEAPON - 1;
			StarShipPtr->RaceDescPtr->ship_info.ship_flags &= ~IMMEDIATE_WEAPON;
			StarShipPtr->RaceDescPtr->ship_info.ship_flags |= SEEKING_WEAPON;
			StarShipPtr->RaceDescPtr->ship_data.ship_sounds =
					SetAbsSoundIndex (StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 2);

			StarShipPtr->cur_status_flags &=
					~(SHIP_AT_MAX_SPEED | SHIP_BEYOND_MAX_SPEED);
		}
		else
		{
			StarShipPtr->RaceDescPtr->cyborg_control.WeaponRange = CLOSE_RANGE_WEAPON;
			StarShipPtr->RaceDescPtr->ship_info.ship_flags &= ~SEEKING_WEAPON;
			StarShipPtr->RaceDescPtr->ship_info.ship_flags |= IMMEDIATE_WEAPON;
			StarShipPtr->RaceDescPtr->ship_data.ship_sounds =
					SetAbsSoundIndex (StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 0);

			if (StarShipPtr->cur_status_flags
					& (SHIP_AT_MAX_SPEED | SHIP_BEYOND_MAX_SPEED))
				StarShipPtr->cur_status_flags |=
						SHIP_AT_MAX_SPEED | SHIP_BEYOND_MAX_SPEED;
		}
	}
}

static void
xform_preprocess (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);

	if (ElementPtr->collision_func != xform_collision)
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

		thrust_status = inertial_thrust (ElementPtr);
		StarShipPtr->cur_status_flags &=
				~(SHIP_AT_MAX_SPEED
				| SHIP_BEYOND_MAX_SPEED
				| SHIP_IN_GRAVITY_WELL);
		StarShipPtr->cur_status_flags |= thrust_status;

		ElementPtr->thrust_wait = StarShipPtr->RaceDescPtr->characteristics.thrust_wait;

		extern void spawn_ion_trail (ELEMENT *ElementPtr);

		spawn_ion_trail (ElementPtr);
		//end copied code
		
		StarShipPtr->ShipFacing = real_facing;
	}
	++ElementPtr->turn_wait;
	++ElementPtr->thrust_wait;
	++StarShipPtr->weapon_counter;
}

static inline void
yform_preprocess (ELEMENT *ElementPtr)
{
	if (ElementPtr->collision_func != collision)
		ElementPtr->collision_func = collision;
}

static void
mmrnmhrm_preprocess (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);

	{
		BOOLEAN is_xform;

		is_xform = (ElementPtr->current.image.farray ==
				StarShipPtr->RaceDescPtr->ship_data.ship);

		switch (is_xform)
		{
			/* In X form */
			case (TRUE):
			{
				xform_preprocess (ElementPtr);
				break;
			}
			/* In Y form */
			case (FALSE):
			{
				yform_preprocess (ElementPtr);
				break;
			}
			default:
			{
				/* this should never ever happen */
				assert (false);
			}
		}
	}

	if (!(ElementPtr->state_flags & APPEARING))
	{
		if ((StarShipPtr->cur_status_flags & SPECIAL)
				&& StarShipPtr->special_counter == 0)
		{
			/* Either we transform or text will flash */
			if (DeltaEnergy (ElementPtr,
					-StarShipPtr->RaceDescPtr->characteristics.special_energy_cost))
			{
				if (ElementPtr->next.image.farray == StarShipPtr->RaceDescPtr->ship_data.ship)
					ElementPtr->next.image.farray =
							StarShipPtr->RaceDescPtr->ship_data.special;
				else
					ElementPtr->next.image.farray =
							StarShipPtr->RaceDescPtr->ship_data.ship;
				ElementPtr->next.image.frame =
						SetEquFrameIndex (ElementPtr->next.image.farray[0],
						ElementPtr->next.image.frame);
				ElementPtr->state_flags |= CHANGING;
	
				StarShipPtr->special_counter =
						StarShipPtr->RaceDescPtr->characteristics.special_wait;
			}
		}
	}
}

static void
uninit_mmrnmhrm (RACE_DESC *pRaceDesc)
{
	SetCustomShipData (pRaceDesc, NULL);
}

/* Neither Mmrnmhrm form actually has a weapon now, we need a dummy function to
 * make this happen. */
static COUNT
initialize_nothing (ELEMENT *ShipPtr, HELEMENT NothingArray[])
{
	(void) ShipPtr;
	(void) NothingArray;
	return 0;
}

RACE_DESC*
init_mmrnmhrm (void)
{
	RACE_DESC *RaceDescPtr;
	// The caller of this func will copy the struct
	static RACE_DESC new_mmrnmhrm_desc;
	MMRNMHRM_DATA otherwing_desc;

	mmrnmhrm_desc.uninit_func = uninit_mmrnmhrm;
	mmrnmhrm_desc.preprocess_func = mmrnmhrm_preprocess;
	mmrnmhrm_desc.postprocess_func = mmrnmhrm_postprocess;
	mmrnmhrm_desc.init_weapon_func = initialize_nothing;
	mmrnmhrm_desc.cyborg_control.intelligence_func = mmrnmhrm_intelligence;

	new_mmrnmhrm_desc = mmrnmhrm_desc;

	otherwing_desc.max_thrust = YWING_MAX_THRUST;
	otherwing_desc.thrust_increment = YWING_THRUST_INCREMENT;
	otherwing_desc.energy_regeneration = YWING_ENERGY_REGENERATION;
	otherwing_desc.weapon_energy_cost = 0; // no weapon
	otherwing_desc.special_energy_cost = YWING_SPECIAL_ENERGY_COST;
	otherwing_desc.energy_wait = YWING_ENERGY_WAIT;
	otherwing_desc.turn_wait = YWING_TURN_WAIT;
	otherwing_desc.thrust_wait = YWING_THRUST_WAIT;
	otherwing_desc.weapon_wait = 0; // no weapon
	otherwing_desc.special_wait = YWING_SPECIAL_WAIT;
	otherwing_desc.ship_mass = SHIP_MASS;

	SetCustomShipData (&new_mmrnmhrm_desc, &otherwing_desc);

	RaceDescPtr = &new_mmrnmhrm_desc;

	return (RaceDescPtr);
}

