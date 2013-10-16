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
#include "ships/pkunk/resinst.h"

#include "globdata.h"
#include "libs/mathlib.h"


#define MAX_CREW 12 //8
#define MAX_ENERGY 12
#define ENERGY_REGENERATION 0
#define WEAPON_ENERGY_COST 1
#define SPECIAL_ENERGY_COST 8 //2
#define ENERGY_WAIT 0
#define MAX_THRUST 128 //64
#define THRUST_INCREMENT 24 //32 //16
#define TURN_WAIT 0
#define THRUST_WAIT 0
#define WEAPON_WAIT 0
#define SPECIAL_WAIT 5 //16

#define SHIP_MASS 1
#define MISSILE_SPEED DISPLAY_TO_WORLD (24)
#define MISSILE_LIFE 5

static RACE_DESC pkunk_desc =
{
	{
		FIRES_FORE | FIRES_LEFT | FIRES_RIGHT,
		36, /* Super Melee cost */
		666 / SPHERE_RADIUS_INCREMENT, /* Initial sphere of influence radius */
		MAX_CREW, MAX_CREW,
		MAX_ENERGY, MAX_ENERGY,
		{
			502, 401,
		},
		(STRING)PKUNK_RACE_STRINGS,
		(FRAME)PKUNK_ICON_MASK_PMAP_ANIM,
		(FRAME)PKUNK_MICON_MASK_PMAP_ANIM,
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
		0, /* SPECIAL_WAIT */
		SHIP_MASS,
	},
	{
		{
			(FRAME)PKUNK_BIG_MASK_PMAP_ANIM,
			(FRAME)PKUNK_MED_MASK_PMAP_ANIM,
			(FRAME)PKUNK_SML_MASK_PMAP_ANIM,
		},
		{
			(FRAME)BUG_BIG_MASK_PMAP_ANIM,
			(FRAME)BUG_MED_MASK_PMAP_ANIM,
			(FRAME)BUG_SML_MASK_PMAP_ANIM,
		},
		{
			(FRAME)0,
			(FRAME)0,
			(FRAME)0,
		},
		{
			(FRAME)PKUNK_CAPTAIN_MASK_PMAP_ANIM,
			(FRAME)0,
			(FRAME)0,
			(FRAME)0,
			(FRAME)0,
		},
		(SOUND)PKUNK_VICTORY_SONG,
		(SOUND)PKUNK_SHIP_SOUNDS,
	},
	{
		0,
		CLOSE_RANGE_WEAPON + 1,
		NULL_PTR,
	},
	(UNINIT_FUNC *) NULL,
	(PREPROCESS_FUNC *) NULL,
	(POSTPROCESS_FUNC *) NULL,
	(INIT_WEAPON_FUNC *) NULL,
	0,
};

static void
animate (PELEMENT ElementPtr)
{
	if (ElementPtr->turn_wait > 0)
		--ElementPtr->turn_wait;
	else
	{
		ElementPtr->next.image.frame =
				IncFrameIndex (ElementPtr->current.image.frame);
		ElementPtr->state_flags |= CHANGING;

		ElementPtr->turn_wait = ElementPtr->next_turn;
	}
}

static COUNT
initialize_bug_missile (PELEMENT ShipPtr, HELEMENT MissileArray[])
{
#define PKUNK_OFFSET 15
#define MISSILE_HITS 1
#define MISSILE_DAMAGE 1
#define MISSILE_OFFSET 1
	COUNT i,j;
	STARSHIPPTR StarShipPtr;
	MISSILE_BLOCK MissileBlock;

	GetElementStarShip (ShipPtr, &StarShipPtr);
	MissileBlock.cx = ShipPtr->next.location.x;
	MissileBlock.cy = ShipPtr->next.location.y;
	MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.weapon;
	MissileBlock.index = 0;
	MissileBlock.sender = (ShipPtr->state_flags & (GOOD_GUY | BAD_GUY))
			| IGNORE_SIMILAR;
	MissileBlock.pixoffs = PKUNK_OFFSET;
	MissileBlock.speed = MISSILE_SPEED;
	MissileBlock.hit_points = MISSILE_HITS;
	MissileBlock.damage = MISSILE_DAMAGE;
	MissileBlock.life = MISSILE_LIFE;
	MissileBlock.preprocess_func = NULL_PTR;
	MissileBlock.blast_offs = MISSILE_OFFSET;

	for (i = 0; i < 6; ++i)
	{
		MissileBlock.face =
				StarShipPtr->ShipFacing
				+ (ANGLE_TO_FACING (QUADRANT) * (i%3));
		if ((i%3) == 2)
			MissileBlock.face += ANGLE_TO_FACING (QUADRANT);
		MissileBlock.face = NORMALIZE_FACING (MissileBlock.face);

		if ((MissileArray[i] = initialize_missile (&MissileBlock)))
		{
			ELEMENTPTR MissilePtr;

			LockElement (MissileArray[i], &MissilePtr);
			if(i < 3)
			{
				SIZE dx, dy;
				GetCurrentVelocityComponents (&ShipPtr->velocity, &dx, &dy);
				DeltaVelocityComponents (&MissilePtr->velocity, dx, dy);
				MissilePtr->current.location.x -= VELOCITY_TO_WORLD (dx);
				MissilePtr->current.location.y -= VELOCITY_TO_WORLD (dy);
			}

			MissilePtr->preprocess_func = animate;
			UnlockElement (MissileArray[i]);
		}
	}

	return (6);
}

static HELEMENT hPhoenix = 0;

void
charge_intelligence (PELEMENT ShipPtr, PEVALUATE_DESC ObjectsOfConcern, COUNT ConcernCounter)
{
	SIZE delta_facing;
	COUNT facing;

	STARSHIPPTR StarShipPtr;

	GetElementStarShip (ShipPtr, &StarShipPtr);
	/*if (hPhoenix && StarShipPtr->special_counter)
	{
		RemoveElement (hPhoenix);
		FreeElement (hPhoenix);
		hPhoenix = 0;
	}

	if (StarShipPtr->RaceDescPtr->ship_info.energy_level <
			StarShipPtr->RaceDescPtr->ship_info.max_energy
			&& (StarShipPtr->special_counter == 0
			|| (BYTE)TFB_Random () < 20))
		StarShipPtr->ship_input_state |= SPECIAL;
	else
		StarShipPtr->ship_input_state &= ~SPECIAL;
	ship_intelligence (ShipPtr, ObjectsOfConcern, ConcernCounter);*/

	facing = StarShipPtr->ShipFacing;
	delta_facing = TrackShip(ShipPtr, &facing);
	
	StarShipPtr->ship_input_state |= THRUST | WEAPON;
	if(delta_facing == -1)
	{
		StarShipPtr->ship_input_state &= ~WEAPON;
		delta_facing = NORMALIZE_FACING(ANGLE_TO_FACING(GetVelocityTravelAngle(&ShipPtr->velocity)) + 8 - StarShipPtr->ShipFacing);
		if(delta_facing == 0)
		{
			StarShipPtr->ship_input_state |= THRUST;
			StarShipPtr->ship_input_state &= ~(LEFT | RIGHT);
		}
		else if(delta_facing < 8)
		{
			StarShipPtr->ship_input_state &= ~THRUST;
			StarShipPtr->ship_input_state |= RIGHT;
		}
		else //(delta_facing >= 8)
		{
			StarShipPtr->ship_input_state &= ~THRUST;
			StarShipPtr->ship_input_state |= LEFT;
		}
	}
	else if(delta_facing > 0 && delta_facing < 8)
		StarShipPtr->ship_input_state |= RIGHT;
	else if(delta_facing > 8)
		StarShipPtr->ship_input_state |= LEFT;
	else StarShipPtr->ship_input_state &= ~(LEFT | RIGHT);
}

static void pkunk_preprocess (PELEMENT ElementPtr);
static void pkunk_postprocess (PELEMENT ElementPtr);

static void
new_pkunk (PELEMENT ElementPtr)
{
	STARSHIPPTR StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	if (!(ElementPtr->state_flags & PLAYER_SHIP))
	{
		ELEMENTPTR ShipPtr;

		LockElement (StarShipPtr->hShip, &ShipPtr);
		ShipPtr->death_func = new_pkunk;
		UnlockElement (StarShipPtr->hShip);
	}
	else
	{
		ElementPtr->state_flags = APPEARING | PLAYER_SHIP | IGNORE_SIMILAR
				| (ElementPtr->state_flags & (GOOD_GUY | BAD_GUY));

		ElementPtr->mass_points = SHIP_MASS;
		ElementPtr->preprocess_func = StarShipPtr->RaceDescPtr->preprocess_func;
		ElementPtr->postprocess_func = StarShipPtr->RaceDescPtr->postprocess_func;
		ElementPtr->death_func =
				(void (*) (PELEMENT ElementPtr))
						StarShipPtr->RaceDescPtr->init_weapon_func;
		StarShipPtr->RaceDescPtr->preprocess_func = pkunk_preprocess;
		StarShipPtr->RaceDescPtr->postprocess_func = pkunk_postprocess;
		StarShipPtr->RaceDescPtr->init_weapon_func = initialize_bug_missile;
		StarShipPtr->RaceDescPtr->ship_info.crew_level = MAX_CREW;
		StarShipPtr->RaceDescPtr->ship_info.energy_level = MAX_ENERGY;
					/* fix vux impairment */
		StarShipPtr->RaceDescPtr->characteristics.max_thrust = MAX_THRUST;
		StarShipPtr->RaceDescPtr->characteristics.thrust_increment = THRUST_INCREMENT;
		StarShipPtr->RaceDescPtr->characteristics.turn_wait = TURN_WAIT;
		StarShipPtr->RaceDescPtr->characteristics.thrust_wait = THRUST_WAIT;
		StarShipPtr->RaceDescPtr->characteristics.special_wait = 0;

		StarShipPtr->ship_input_state = 0;
		StarShipPtr->cur_status_flags =
				StarShipPtr->old_status_flags = 0;
		StarShipPtr->energy_counter =
				StarShipPtr->weapon_counter =
				StarShipPtr->special_counter = 0;
		ElementPtr->crew_level =
				ElementPtr->turn_wait = ElementPtr->thrust_wait = 0;
		ElementPtr->life_span = NORMAL_LIFE;

		StarShipPtr->ShipFacing = NORMALIZE_FACING (TFB_Random ());
		ElementPtr->current.image.farray = StarShipPtr->RaceDescPtr->ship_data.ship;
		ElementPtr->current.image.frame =
				SetAbsFrameIndex (StarShipPtr->RaceDescPtr->ship_data.ship[0],
				StarShipPtr->ShipFacing);
		SetPrimType (&(GLOBAL (DisplayArray))[
				ElementPtr->PrimIndex
				], STAMP_PRIM);

		do
		{
			ElementPtr->current.location.x =
					WRAP_X (DISPLAY_ALIGN_X (TFB_Random ()));
			ElementPtr->current.location.y =
					WRAP_Y (DISPLAY_ALIGN_Y (TFB_Random ()));
		} while (CalculateGravity (ElementPtr)
				|| TimeSpaceMatterConflict (ElementPtr));

		ElementPtr->hTarget = StarShipPtr->hShip;
	}
}

static void
intercept_pkunk_death (PELEMENT ElementPtr)
{
	STARSHIPPTR StarShipPtr;

	ElementPtr->state_flags &= ~DISAPPEARING;
	ElementPtr->life_span = 1;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	if (StarShipPtr->RaceDescPtr->ship_info.crew_level == 0)
	{
		ELEMENTPTR ShipPtr;

		LockElement (StarShipPtr->hShip, &ShipPtr);
		if (GRAVITY_MASS (ShipPtr->mass_points + 1))
		{
			ElementPtr->state_flags |= DISAPPEARING;
			ElementPtr->life_span = 0;
		}
		else
		{
			ShipPtr->mass_points = MAX_SHIP_MASS + 1;
			StarShipPtr->RaceDescPtr->preprocess_func = ShipPtr->preprocess_func;
			StarShipPtr->RaceDescPtr->postprocess_func = ShipPtr->postprocess_func;
			StarShipPtr->RaceDescPtr->init_weapon_func =
					(COUNT (*) (PELEMENT ElementPtr,
							HELEMENT Weapon[]))
							ShipPtr->death_func;

			ElementPtr->death_func = new_pkunk;
		}
		UnlockElement (StarShipPtr->hShip);
	}
}

#define START_PHOENIX_COLOR BUILD_COLOR (MAKE_RGB15 (0x1F, 0x15, 0x00), 0x7A)
#define TRANSITION_LIFE 1

void
spawn_phoenix_trail (PELEMENT ElementPtr)
{
	static const COLOR color_tab[] =
	{
		BUILD_COLOR (MAKE_RGB15 (0x1F, 0x00, 0x00), 0x2a),
		BUILD_COLOR (MAKE_RGB15 (0x1B, 0x00, 0x00), 0x2b),
		BUILD_COLOR (MAKE_RGB15 (0x17, 0x00, 0x00), 0x2c),
		BUILD_COLOR (MAKE_RGB15 (0x13, 0x00, 0x00), 0x2d),
		BUILD_COLOR (MAKE_RGB15 (0x0F, 0x00, 0x00), 0x2e),
		BUILD_COLOR (MAKE_RGB15 (0x0B, 0x00, 0x00), 0x2f),
		BUILD_COLOR (MAKE_RGB15 (0x1F, 0x15, 0x00), 0x7a),
		BUILD_COLOR (MAKE_RGB15 (0x1F, 0x11, 0x00), 0x7b),
		BUILD_COLOR (MAKE_RGB15 (0x1F, 0x0E, 0x00), 0x7c),
		BUILD_COLOR (MAKE_RGB15 (0x1F, 0x0A, 0x00), 0x7d),
		BUILD_COLOR (MAKE_RGB15 (0x1F, 0x07, 0x00), 0x7e),
		BUILD_COLOR (MAKE_RGB15 (0x1F, 0x03, 0x00), 0x7f),
	};
	
#define NUM_TAB_COLORS (sizeof (color_tab) / sizeof (color_tab[0]))

	COUNT color_index = 0;
	COLOR Color;

	Color = COLOR_256 (
			GetPrimColor (&(GLOBAL (DisplayArray))[ElementPtr->PrimIndex])
			);
	if (Color != 0x2F)
	{
		ElementPtr->life_span = TRANSITION_LIFE;

		++Color;
		if (Color > 0x7F)
			Color = 0x2A;
		if (Color <= 0x2f && Color >= 0x2a)
				color_index = (COUNT)Color - 0x2a;
		else /* color is between 0x7a and 0x7f */
				color_index = (COUNT)(Color - 0x7a) + (NUM_TAB_COLORS >> 1);
		SetPrimColor (
				&(GLOBAL (DisplayArray))[ElementPtr->PrimIndex],
				color_tab[color_index]
				);

		ElementPtr->state_flags &= ~DISAPPEARING;
		ElementPtr->state_flags |= CHANGING;
	}
}

#define PHOENIX_LIFE 12

void
phoenix_transition (PELEMENT ElementPtr)
{
	HELEMENT hShipImage;
	ELEMENTPTR ShipImagePtr;
	STARSHIPPTR StarShipPtr;
	
	GetElementStarShip (ElementPtr, &StarShipPtr);
	LockElement (StarShipPtr->hShip, &ShipImagePtr);

	if (!(ShipImagePtr->state_flags & NONSOLID))
	{
		ElementPtr->preprocess_func = NULL_PTR;
	}
	else if ((hShipImage = AllocElement ()))
	{
#define TRANSITION_SPEED DISPLAY_TO_WORLD (20)
		COUNT angle;

		PutElement (hShipImage);

		LockElement (hShipImage, &ShipImagePtr);
		ShipImagePtr->state_flags = APPEARING | FINITE_LIFE | NONSOLID;
		ShipImagePtr->life_span = TRANSITION_LIFE;
		SetPrimType (&(GLOBAL (DisplayArray))[ShipImagePtr->PrimIndex],
				STAMPFILL_PRIM);
		SetPrimColor (
				&(GLOBAL (DisplayArray))[ShipImagePtr->PrimIndex],
				START_PHOENIX_COLOR
				);
		ShipImagePtr->current.image = ElementPtr->current.image;
		ShipImagePtr->current.location = ElementPtr->current.location;
		if (!(ElementPtr->state_flags & PLAYER_SHIP))
		{
			angle = ElementPtr->mass_points;

			ShipImagePtr->current.location.x +=
					COSINE (angle, TRANSITION_SPEED);
			ShipImagePtr->current.location.y +=
					SINE (angle, TRANSITION_SPEED);
			ElementPtr->preprocess_func = NULL_PTR;
		}
		else
		{
			angle = FACING_TO_ANGLE (StarShipPtr->ShipFacing);

			ShipImagePtr->current.location.x -=
					COSINE (angle, TRANSITION_SPEED)
					* (ElementPtr->life_span - 1);
			ShipImagePtr->current.location.y -=
					SINE (angle, TRANSITION_SPEED)
					* (ElementPtr->life_span - 1);

			ShipImagePtr->current.location.x =
					WRAP_X (ShipImagePtr->current.location.x);
			ShipImagePtr->current.location.y =
					WRAP_Y (ShipImagePtr->current.location.y);
		}

		ShipImagePtr->mass_points = (BYTE)angle;
		ShipImagePtr->preprocess_func = phoenix_transition;
		ShipImagePtr->death_func = spawn_phoenix_trail;
		SetElementStarShip (ShipImagePtr, StarShipPtr);

		UnlockElement (hShipImage);
	}

	UnlockElement (StarShipPtr->hShip);
}

/*static void
pkunk_collision (PELEMENT ElementPtr0, PPOINT pPt0,
		PELEMENT ElementPtr1, PPOINT pPt1)
{
	//Take minimal damage from hitting planets
	if (!(ElementPtr1->state_flags & FINITE_LIFE))
	{
		ElementPtr0->state_flags |= COLLISION;
		if (GRAVITY_MASS (ElementPtr1->mass_points))
		{

			do_damage ((ELEMENTPTR)ElementPtr0, 1);

			ProcessSound (SetAbsSoundIndex (GameSounds, TARGET_DAMAGED_FOR_1_PT), ElementPtr0);
		}
	}
}*/

static void spawn_tongue (PELEMENT ElementPtr);

static void
tongue_postprocess (PELEMENT ElementPtr)
{
	if (ElementPtr->turn_wait)
		spawn_tongue (ElementPtr);
}

static void
tongue_collision (PELEMENT ElementPtr0, PPOINT pPt0, PELEMENT ElementPtr1, PPOINT pPt1)
{
	STARSHIPPTR StarShipPtr;

	GetElementStarShip (ElementPtr0, &StarShipPtr);
	if (StarShipPtr->special_counter == 6)
		weapon_collision (ElementPtr0, pPt0, ElementPtr1, pPt1);

	StarShipPtr->special_counter -= ElementPtr0->turn_wait;
	ElementPtr0->turn_wait = 0;
	ElementPtr0->state_flags |= NONSOLID;
}

static void
spawn_tongue (PELEMENT ElementPtr)
{
#define TONGUE_SPEED 0
#define TONGUE_HITS 1
#define TONGUE_DAMAGE 100
#define TONGUE_OFFSET 4
	STARSHIPPTR StarShipPtr;
	MISSILE_BLOCK TongueBlock;
	HELEMENT Tongue;
	COUNT real_facing;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	
	TongueBlock.cx = ElementPtr->next.location.x;
	TongueBlock.cy = ElementPtr->next.location.y;
	TongueBlock.farray = StarShipPtr->RaceDescPtr->ship_data.special;
	TongueBlock.face = TongueBlock.index = (ElementPtr->state_flags & PLAYER_SHIP) ? (NORMALIZE_FACING (StarShipPtr->ShipFacing + 8)) : StarShipPtr->ShipFacing;
	TongueBlock.sender = (ElementPtr->state_flags & (GOOD_GUY | BAD_GUY))
			| IGNORE_SIMILAR;
	TongueBlock.pixoffs = (ElementPtr->state_flags & PLAYER_SHIP) ? 20 : 0;
	TongueBlock.speed = TONGUE_SPEED;
	TongueBlock.hit_points = TONGUE_HITS;
	TongueBlock.damage = TONGUE_DAMAGE;
	TongueBlock.life = 1;
	TongueBlock.preprocess_func = 0;
	TongueBlock.blast_offs = TONGUE_OFFSET;
	Tongue = initialize_missile (&TongueBlock);
	if (Tongue)
	{
		ELEMENTPTR TonguePtr;

		LockElement (Tongue, &TonguePtr);
		TonguePtr->postprocess_func = tongue_postprocess;
		TonguePtr->collision_func = tongue_collision;
		if (ElementPtr->state_flags & PLAYER_SHIP)
			TonguePtr->turn_wait = StarShipPtr->special_counter;
		else
		{
			COUNT angle;
			RECT r;
			SIZE x_offs, y_offs;

			TonguePtr->turn_wait = ElementPtr->turn_wait - 1;

			GetFrameRect (TonguePtr->current.image.frame, &r);
			x_offs = DISPLAY_TO_WORLD (r.extent.width >> 1);
			y_offs = DISPLAY_TO_WORLD (r.extent.height >> 1);

			angle = FACING_TO_ANGLE (NORMALIZE_FACING (StarShipPtr->ShipFacing + 8));
			if (angle > HALF_CIRCLE && angle < FULL_CIRCLE)
				TonguePtr->current.location.x -= x_offs;
			else if (angle > 0 && angle < HALF_CIRCLE)
				TonguePtr->current.location.x += x_offs;
			if (angle < QUADRANT || angle > FULL_CIRCLE - QUADRANT)
				TonguePtr->current.location.y -= y_offs;
			else if (angle > QUADRANT && angle < FULL_CIRCLE - QUADRANT)
				TonguePtr->current.location.y += y_offs;
		}

		SetElementStarShip (TonguePtr, StarShipPtr);
		UnlockElement (Tongue);
		PutElement (Tongue);
	}
}

static COUNT pkunk_present = 0;

static void
pkunk_dispose_graphics (RACE_DESCPTR RaceDescPtr)
{
	--pkunk_present;

	if(!pkunk_present)
	{
		extern void clearGraphicsHack(FRAME farray[]);
		clearGraphicsHack(RaceDescPtr->ship_data.special);
	}
}

static void
pkunk_preprocess (PELEMENT ElementPtr)
{
	STARSHIPPTR StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	
	if (ElementPtr->state_flags & APPEARING)
	{
		if(StarShipPtr->RaceDescPtr->ship_data.special[0] == (FRAME)0)
		{
			StarShipPtr->RaceDescPtr->ship_data.special[0] = CaptureDrawable(LoadCelFile("zoqfot/stibig.ani"));
			StarShipPtr->RaceDescPtr->ship_data.special[1] = CaptureDrawable(LoadCelFile("zoqfot/stimed.ani"));
			StarShipPtr->RaceDescPtr->ship_data.special[2] = CaptureDrawable(LoadCelFile("zoqfot/stisml.ani"));
		}
		++pkunk_present;

		extern void less_planet_damage_collision
				(PELEMENT ElementPtr0, PPOINT pPt0,
				PELEMENT ElementPtr1, PPOINT pPt1);
		ElementPtr->collision_func = less_planet_damage_collision;
	}

	if (ElementPtr->state_flags & APPEARING)
	{
		ELEMENTPTR PhoenixPtr;

		if ((/*(BYTE)TFB_Random () & 1*/ StarShipPtr->RaceDescPtr->characteristics.energy_wait == 0)
				&& (hPhoenix = AllocElement ()))
		{

			LockElement (hPhoenix, &PhoenixPtr);
			PhoenixPtr->state_flags =
					FINITE_LIFE | NONSOLID | IGNORE_SIMILAR
					| (ElementPtr->state_flags & (GOOD_GUY | BAD_GUY));
			PhoenixPtr->life_span = 1;

			PhoenixPtr->death_func = intercept_pkunk_death;

			SetElementStarShip (PhoenixPtr, StarShipPtr);

			UnlockElement (hPhoenix);
			InsertElement (hPhoenix, GetHeadElement ());

			StarShipPtr->RaceDescPtr->characteristics.energy_wait = 1;
		}

		if (ElementPtr->hTarget == 0)
			StarShipPtr->RaceDescPtr->preprocess_func = 0;
		else
		{
			COUNT angle, facing;

			ProcessSound (SetAbsSoundIndex (
					StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1
					), ElementPtr);

			ElementPtr->life_span = PHOENIX_LIFE;
			SetPrimType (&(GLOBAL (DisplayArray))[ElementPtr->PrimIndex],
					NO_PRIM);
			ElementPtr->state_flags |= NONSOLID | FINITE_LIFE | CHANGING;

			facing = StarShipPtr->ShipFacing;
			for (angle = OCTANT; angle < FULL_CIRCLE; angle += QUADRANT)
			{
				StarShipPtr->ShipFacing = NORMALIZE_FACING (
						facing + ANGLE_TO_FACING (angle)
						);
				phoenix_transition (ElementPtr);
			}
			StarShipPtr->ShipFacing = facing;
		}
	}

	if (StarShipPtr->RaceDescPtr->preprocess_func)
	{
		ZeroVelocityComponents (&((ELEMENTPTR)ElementPtr)->velocity);
		StarShipPtr->cur_status_flags &=
				~(LEFT | RIGHT | THRUST | WEAPON | SPECIAL);

		if (ElementPtr->life_span == NORMAL_LIFE)
		{
			ElementPtr->current.image.frame =
					ElementPtr->next.image.frame =
					SetEquFrameIndex (
					ElementPtr->current.image.farray[0],
					ElementPtr->current.image.frame);
			SetPrimType (&(GLOBAL (DisplayArray))[ElementPtr->PrimIndex],
					STAMP_PRIM);
			InitIntersectStartPoint (ElementPtr);
			InitIntersectEndPoint (ElementPtr);
			InitIntersectFrame (ElementPtr);
			ZeroVelocityComponents (&((ELEMENTPTR)ElementPtr)->velocity);
			ElementPtr->state_flags &= ~(NONSOLID | FINITE_LIFE);
			ElementPtr->state_flags |= CHANGING;

			StarShipPtr->RaceDescPtr->preprocess_func = 0;
		}
	}
}

static void
pkunk_postprocess (PELEMENT ElementPtr)
{
	STARSHIPPTR StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);

	if ((StarShipPtr->cur_status_flags & SPECIAL)
			&& StarShipPtr->special_counter == 0
			&& DeltaEnergy (ElementPtr, -SPECIAL_ENERGY_COST))
	{
		StarShipPtr->special_counter = 6;
	}
	if (StarShipPtr->special_counter)
		spawn_tongue (ElementPtr);


	if (StarShipPtr->RaceDescPtr->characteristics.special_wait)
		--StarShipPtr->RaceDescPtr->characteristics.special_wait;
	else if (/*(StarShipPtr->cur_status_flags & SPECIAL)
			&& */StarShipPtr->RaceDescPtr->ship_info.energy_level <
			StarShipPtr->RaceDescPtr->ship_info.max_energy)
	{
		COUNT CurSound;
		static COUNT LastSound = 0;

		do
		{
			CurSound =
					2 + ((COUNT)TFB_Random ()
					% (GetSoundCount (StarShipPtr->RaceDescPtr->ship_data.ship_sounds) - 2));
		} while (CurSound == LastSound);
		ProcessSound (SetAbsSoundIndex (
				StarShipPtr->RaceDescPtr->ship_data.ship_sounds, CurSound
				), ElementPtr);
		LastSound = CurSound;

		DeltaEnergy (ElementPtr, 2);

		StarShipPtr->RaceDescPtr->characteristics.special_wait = SPECIAL_WAIT;
	}
}

RACE_DESCPTR
init_pkunk (void)
{
	RACE_DESCPTR RaceDescPtr;

	pkunk_desc.preprocess_func = pkunk_preprocess;
	pkunk_desc.postprocess_func = pkunk_postprocess;
	pkunk_desc.init_weapon_func = initialize_bug_missile;
	pkunk_desc.uninit_func = pkunk_dispose_graphics;
	pkunk_desc.cyborg_control.intelligence_func =
			(void (*) (PVOID ShipPtr, PVOID ObjectsOfConcern, COUNT
					ConcernCounter)) charge_intelligence;

	RaceDescPtr = &pkunk_desc;

	return (RaceDescPtr);
}

