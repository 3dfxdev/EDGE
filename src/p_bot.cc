//----------------------------------------------------------------------------
//  EDGE Play Simulation Action routines: 'DeathBots'
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2004  The EDGE Team.
// 
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//----------------------------------------------------------------------------
//
//  Based on the DOOM source code, released by Id Software under the
//  following copyright:
//
//    Copyright (C) 1993-1996 by id Software, Inc.
//
//----------------------------------------------------------------------------

#include "i_defs.h"
#include "p_bot.h"

#include "dm_defs.h"
#include "dm_state.h"
#include "g_game.h"
#include "m_random.h"
#include "p_local.h"
#include "p_weapon.h"
#include "r_state.h"
#include "s_sound.h"
#include "w_wad.h"
#include "z_zone.h"
#include "p_action.h"
#include "rad_trig.h"

// Linked list of all bots.
static bot_t *bots = NULL;

static float bot_atkrange;
static mobj_t *bot_shooter = NULL;

static void NewBotChaseDir(bot_t * bot)
{
	// FIXME: This is not very intelligent...
	if (bot->target && (M_Random() & 1))
	{
		bot->angle = R_PointToAngle(bot->pl->mo->x, bot->pl->mo->y,
			bot->target->x, bot->target->y);
	}
	else
		bot->angle = M_Random() << 24;
}

static void Confidence(bot_t * bot)
{
	const player_t *p = bot->pl;
	const mobj_t *mo = p->mo;

	if (p->powers[PW_Invulnerable])
		bot->confidence = 1;
	else if (mo->health < mo->info->spawnhealth / 3)
		bot->confidence = -1;
	else if (mo->health > 3 * mo->info->spawnhealth / 4 && p->ready_wp >= 0 &&
		p->weapons[p->ready_wp].info->ammo != AM_NoAmmo)
	{
	        ammotype_e ammo = p->weapons[p->ready_wp].info->ammo;

		if (p->ammo[ammo].num > p->ammo[ammo].max / 2)
			bot->confidence = 1;
	}
	bot->confidence = 0;
}

static int EvaluateWeapon(player_t *p, int w_num)
{
	playerweapon_t *wp = p->weapons + w_num;
	weaponinfo_t *weapon;
	attacktype_t *attack;
	float value;

	// Don't have this weapon
	if (! wp->owned)
		return INT_MIN;

	weapon = wp->info;
	DEV_ASSERT2(weapon);

	attack = weapon->attack;

	if (!attack)
		return INT_MIN;

	// Don't have enough ammo
	if (weapon->ammo != AM_NoAmmo)
	{
		if (p->ammo[weapon->ammo].num < weapon->ammopershot)
			return INT_MIN;
	}

	value = 64 * attack->damage.nominal;

	switch (attack->attackstyle)
	{
	case ATK_CLOSECOMBAT:
		if (p->powers[PW_Berserk])
			value /= 10;
		else
			value /= 20;
		break;
	case ATK_SMARTPROJECTILE:
		value *= 2;
	case ATK_PROJECTILE:
		value += 256 * attack->atk_mobj->damage.nominal;
		value *= attack->atk_mobj->speed / 20;
		break;
	case ATK_SPAWNER:
		value *= 2;
		break;
	case ATK_TRIPLESPAWNER:
		value *= 6;
		break;
	case ATK_SHOT:
		value *= attack->count;
		break;

	default:  // do nothing ??
		break;
	}

	//  value -= (attack->accuracy_angle + attack->x_accuracy_slope + 1) / 2;
	value -= weapon->ammopershot * 8;

	if (w_num == p->ready_wp)
		value += 2048;

	value += (M_Random() - 128) * 16;
	return (int)value;
}

//
// P_RemoveBots
//
// Done at level shutdown, right after all mobjs have been removed.
// Erases anything level specific from the bot structs.
//
void P_RemoveBots(void)
{
}

static bot_t *looking_bot;

static bool PTR_BotLook(intercept_t * in)
{
	line_t *li;
	mobj_t *th;

	if (in->type == INCPT_Line)
	{
		li = in->d.line;

		if (!(li->flags & ML_TwoSided))
			return false;  // stop

		// Crosses a two sided line.
		// A two sided line will restrict
		// the possible target ranges.
		// -AJA- 1999/07/19: Gaps are now stored in line_t.

		if (li->gap_num == 0)
			return false;  // stop

		if (li->frontsector->f_h != li->backsector->f_h)
		{
			if (fabs(li->frontsector->f_h - li->backsector->f_h) > 24.0f)
				return false;
		}

		return true;  // shot continues

	}

	DEV_ASSERT2(in->type == INCPT_Thing);

	// shoot a thing
	th = in->d.thing;

	if (th == bot_shooter)
		return true;  // can't shoot self

	if (!(th->flags & MF_SPECIAL))
		return true;  // has to be able to be shot

#if 0 //!!!! FIXME
	int ammotype;
	ammotype = th->info->benefitammo;
	switch (th->info->benefittype)
	{
		// -KM- 1998/11/25 New weapon handling
	case WEAPON:  // WEAPON

		if (!bot_shooter->player->weapons[th->info->benefitweapon].owned)
			break;
		ammotype = bot_shooter->player->weapons[th->info->benefitweapon].info->ammo;
	case AMMO_TYPE:
		if (bot_shooter->player->ammo[ammotype].num ==
			bot_shooter->player->ammo[ammotype].max)
			return true;
		break;
	case KEY_BLUECARD:
		if (bot_shooter->player->cards[KEY_BlueCard])
			return true;
		break;
	case KEY_REDCARD:
		if (bot_shooter->player->cards[KEY_RedCard])
			return true;
		break;
	case KEY_YELLOWCARD:
		if (bot_shooter->player->cards[KEY_YellowCard])
			return true;
		break;
	case KEY_BLUESKULL:
		if (bot_shooter->player->cards[KEY_BlueSkull])
			return true;
		break;
	case KEY_REDSKULL:
		if (bot_shooter->player->cards[KEY_RedSkull])
			return true;
		break;
	case KEY_YELLOWSKULL:
		if (bot_shooter->player->cards[KEY_YellowSkull])
			return true;
		break;
	case POWERUP_ACIDSUIT:
		if (bot_shooter->player->powers[PW_AcidSuit] >= th->info->benefitamount * TICRATE / 3)
			return true;
		break;
	case POWERUP_ARMOUR:
		if (bot_shooter->player->armourpoints >= th->info->limit)
			return true;
		break;
	case POWERUP_AUTOMAP:
		return true;
	case POWERUP_BACKPACK:
		break;
	case POWERUP_BERSERK:
		if (bot_shooter->player->powers[PW_Berserk])
			return true;
		break;

	case POWERUP_HEALTH:
		if (bot_shooter->health >= th->info->limit)
			return true;
		break;

	case POWERUP_HEALTHARMOUR:
		if ((bot_shooter->health >= th->info->limit) &&
			(bot_shooter->player->armourpoints >= th->info->limit))
			return true;
		break;

	case POWERUP_INVULNERABLE:
		break;

	case POWERUP_JETPACK:
		if (bot_shooter->player->powers[PW_Jetpack] >= th->info->benefitamount * TICRATE / 3)
			return true;
		break;

	case POWERUP_LIGHTGOGGLES:
		if (bot_shooter->player->powers[PW_Infrared] >= th->info->benefitamount * TICRATE / 3)
			return true;
		break;

	case POWERUP_NIGHTVISION:
		if (bot_shooter->player->powers[PW_NightVision] >= th->info->benefitamount * TICRATE / 3)
			return true;
		break;

	case POWERUP_PARTINVIS:
		if (bot_shooter->player->powers[PW_PartInvis] >= th->info->benefitamount * TICRATE / 3)
			return true;
		break;

	}
#endif

	linetarget = th;

	return false;  // don't go any farther
}

// Finds items for the bot to get.
static mobj_t *LookForStuff(bot_t *bot, angle_t angle)
{
	float x2;
	float y2;

	bot_shooter = bot->pl->mo;
	looking_bot = bot;

	x2 = bot->pl->mo->x + 1024 * M_Cos(angle);
	y2 = bot->pl->mo->y + 1024 * M_Sin(angle);

	linetarget = NULL;
	bot_atkrange = 1024.0f;

	P_PathTraverse(bot->pl->mo->x, bot->pl->mo->y, x2, y2, 
		PT_ADDLINES | PT_ADDTHINGS, PTR_BotLook);

	looking_bot = NULL;

	return linetarget;
}

static void MoveBot(bot_t *bot, angle_t angle)
{
	bot->cmd.followtype = BOTCMD_FOLLOW_DIR;
	bot->cmd.followobj.dir.angle = angle;
	bot->cmd.followobj.dir.distance = 1024;
}

// Based on P_LookForTargets from p_enemy.c
static bool LookForBotTargets(bot_t *bot)
{
	mobj_t *object = bot->pl->mo;
	mobj_t *currmobj;

	for (currmobj = mobjlisthead; currmobj; currmobj = currmobj->next)
	{
		if ((currmobj->side & object->side) && !object->supportobj)
		{
			if (currmobj->supportobj != object && P_CheckSight(object, currmobj))
			{
				bot->supportobj = currmobj;
				return true;
			}
		}
		// The following must be true to justify that you attack a target:
		// 1. The target may not be yourself or your support obj.
		// 2. The target must either want to attack you, or be on a different side
		// 3. The target may not have the same supportobj as you.
		// 4. The target's type must be different from your, if you aren't disloyal.
		// 5. You must be able to see and shoot the target.
		if ((((currmobj->target == object->supportobj || currmobj->target == object)
			&& currmobj->target)
			|| (object->side && !(currmobj->side & object->side)))
			&& ((currmobj != object) &&
			(currmobj != object->supportobj) &&
			(object->info != currmobj->info || (object->extendedflags & EF_DISLOYALTYPE)) &&
			((object->supportobj == NULL) || object->supportobj != currmobj->supportobj)))
		{
			if ((currmobj->flags & MF_SHOOTABLE) && P_CheckSight(object, currmobj))
			{
				bot->target = currmobj;
				return true;
			}
		}
	}

	return false;
}

static void BotThink(bot_t * bot)
{
	bool move_ok;
	bool seetarget = false;
	int best;
	int best_val = INT_MIN;
	unsigned int i, j;

	DEV_ASSERT2(bot->pl);
	DEV_ASSERT2(bot->pl->mo);

	bot->target = bot->pl->mo->target;

	best = bot->pl->ready_wp;

	move_ok = P_ApproxDistance(bot->pl->mo->mom.x, bot->pl->mo->mom.y) > STOPSPEED;
	if (bot->pl->health <= 0)
	{
		// Dead. Respawn.
		bot->cmd.use = true;
		return;
	}

	// Check if we can see the target
	if (bot->target)
		seetarget = P_CheckSight(bot->pl->mo, bot->target);

	bot->threshold--;
	bot->movecount--;

	// Select a suitable weapon
	if (bot->confidence <= 0 && (bot->threshold < 0 || bot->movecount < 0))
	{
		for (i=0; i < MAXWEAPONS; i++)
		{
			j = EvaluateWeapon((player_t*)bot->pl, i);

			if (j > (unsigned int)best_val)
			{
				best_val = (int)j;
				best = i;
			}
		}

		if (best != bot->pl->ready_wp)
			bot->cmd.new_weapon = best;
	}

	// Look for enemies
	// If we aren't confident gather more than hunt.
	if (!seetarget && bot->confidence >= 0)
	{
		if (bot->threshold < 0)
		{
			LookForBotTargets(bot);
			if (bot->target)
				bot->threshold = M_Random() & 31;
		}
	}

	// Can't see a target || don't have a suitable weapon to take it out with?
	if (!seetarget)
	{
		mobj_t *newtarget = NULL;

		if (bot->threshold < 0)
		{
			angle_t search = ANG180;

			// If we are confident, hunt more than gather.
			if (bot->confidence == 0)
				search = ANG90;
			else if (bot->confidence > 0)
				search = ANG45 / 2;

			// Find some stuff!
			for (i = (search / (5 * TICRATE)) * (leveltime % TICRATE); i < search; i += search / 5)
			{
				newtarget = LookForStuff(bot, bot->angle + i);

				if (!newtarget)
					newtarget = LookForStuff(bot, bot->angle - i);

				if (newtarget)
				{
					bot->target = newtarget;
					P_MobjSetTarget(bot->pl->mo, newtarget); // Fixme
					bot->threshold = M_Random() & 31;
					seetarget = true;
					break;
				}
			}
		}
	}

	Confidence(bot);

	if (bot->target)
	{
		// Got a special (item) target?
		if (bot->target->flags & MF_SPECIAL)
		{
			if ((bot->pl->mo->flags & MF_JUSTPICKEDUP))
			{
				// Just got the item.
				bot->target = NULL;
			}
			else if (bot->movecount < 0)
			{
				// Move in the direction of the item.
				bot->angle = R_PointToAngle(bot->pl->mo->x, bot->pl->mo->y,
					bot->target->x, bot->target->y);

				bot->movecount = (M_Random() & 15) * 16;
			}

			// If there is a wall or something in the way, pick a new direction.
			if (!move_ok)
				NewBotChaseDir(bot); // fixme: check out

			// Move the bot.
			MoveBot(bot, bot->angle);
			return;
		}

		// Target can be killed.
		if (bot->target->flags & MF_SHOOTABLE)
		{
			// Can see a target,
			if (seetarget)
			{
				// So face it,
				bot->angle = R_PointToAngle(bot->pl->mo->x, bot->pl->mo->y,
					bot->target->x, bot->target->y);
				bot->cmd.facetype = BOTCMD_FACE_MOBJ;
				bot->cmd.faceobj.mo = bot->target;
				// Shoot it,
				bot->cmd.attack = true;
				// strafe it.
				MoveBot(bot, bot->angle + (bot->strafedir ? ANG90 : -ANG90));
				if (bot->movecount < 0)
				{
					bot->movecount = M_Random() & 63;
					bot->strafedir = M_Random() & 1;
				}
			}
			else
				MoveBot(bot, bot->angle);

			// chase towards player
			if (bot->movecount < 0 || !move_ok)
			{
				NewBotChaseDir(bot);
				if (!move_ok)
					bot->strafedir = M_Random() & 1;
			}
			return;
		}
	}

	// Wander around.
	MoveBot(bot, bot->angle);

	if (!move_ok || (bot->target && bot->target->health <= 0)
		|| bot->movecount < 0)
	{
		bot->target = NULL;
		P_MobjSetTarget(bot->pl->mo, NULL);
		NewBotChaseDir(bot);
		bot->strafedir = M_Random() & 1;
	}
}

void BOT_DMSpawn(void)
{
#if 0
	int i, j;
	int selections;
	mobj_t *bot;
	const mobjinfo_t *info;

	selections = deathmatch_p - deathmatchstarts;
	if (selections < 4)
		I_Warning("Only %i deathmatch spots, 4 required\n", selections);

	info = DDF_MobjLookupPlayer(-2);

	for (j = 0; j < selections; j++)
	{
		i = M_Random() % selections;
		bot = P_MobjCreateObject(deathmatchstarts[i].x,
			deathmatchstarts[i].y, ONFLOORZ, info);

		if (P_CheckAbsPosition(bot, bot->x, bot->y, bot->z))
		{
			P_MobjCreateObject(bot->x, bot->y, bot->z, info->respawneffect);
			bot->side = M_Random() & 31;
			bot->side = 1 << bot->side;
			return;
		}
		P_RemoveMobj(bot);
	}
#endif
}

//
// P_BotCreate
//
// Converts the player (which should be empty, i.e. neither a network
// or console player) to a bot.
//
void P_BotCreate(player_t *p)
{
	bot_t *bot;

	p->thinker = P_BotPlayerThinker;
	P_AddPlayerToGame(p);

	bot = Z_ClearNew(bot_t, 1);
	p->data = (void *)bot;

	bot->pl = p;

	bot->next = bots;
	bots = bot;
}

//
// Reads the botcmd_t, converts it to ticcmd_t and stores the result in dest.
//
static void ConvertToTiccmd(bot_t *bot, ticcmd_t *dest, botcmd_t *src)
{
	float s,d,x,y;
	angle_t a, new_angle;

	dest->buttons = dest->extbuttons = 0;
	if (src->attack)
		dest->buttons |= BT_ATTACK;
	if (src->second_attack)
		dest->extbuttons |= EBT_SECONDATK;
	if (src->use)
		dest->buttons |= BT_USE;
	if (src->jump)
		dest->extbuttons |= EBT_JUMP;
	if (src->new_weapon != -1)
		dest->buttons |= (src->new_weapon << BT_WEAPONSHIFT) & BT_WEAPONMASK;

	switch (src->facetype)
	{
		case BOTCMD_FACE_MOBJ:
			x = src->faceobj.mo->x - bot->pl->mo->x;
			y = src->faceobj.mo->y - bot->pl->mo->y;
			new_angle = R_PointToAngle(bot->pl->mo->x, bot->pl->mo->y, src->faceobj.mo->x, src->faceobj.mo->y);
			s = (float)sqrt(x * x + y * y);
			
			if (s < 0.1f)
				s = 0.1f;

			s = (src->faceobj.mo->z - bot->pl->mo->z) / s;
			break;

		case BOTCMD_FACE_XYZ:
			x = src->faceobj.xyz.x - bot->pl->mo->x;
			y = src->faceobj.xyz.y - bot->pl->mo->y;
			new_angle = R_PointToAngle(bot->pl->mo->x, bot->pl->mo->y, src->faceobj.xyz.x, src->faceobj.xyz.y);
			s = (float)sqrt(x * x + y * y);

			if (s < 0.1f)
				s = 0.1f;

			s = (src->faceobj.xyz.z - bot->pl->mo->z) / s;
			break;

		case BOTCMD_FACE_ANGLE:
			new_angle = src->faceobj.angle.angle;
			s = src->faceobj.angle.slope;

		default:
			s = 0;
			new_angle = bot->pl->mo->angle;

			break;
	}

	dest->angleturn = (new_angle - bot->pl->mo->angle) >> 16;
	dest->vertslope = (signed char) (s * 256);

	if (src->followtype == BOTCMD_FOLLOW_NONE)
	{
		dest->sidemove = dest->forwardmove = 0;
	}
	else
	{
		// set a to the angle relative the player.
		switch (src->followtype)
		{
			case BOTCMD_FOLLOW_MOBJ:
				x = src->followobj.mo->x - bot->pl->mo->x;
				y = src->followobj.mo->y - bot->pl->mo->y;
				d = x * x + y * y;
				a = R_PointToAngle(src->followobj.mo->x, src->followobj.mo->y,
					bot->pl->mo->x, bot->pl->mo->y) - new_angle;
				break;

			case BOTCMD_FOLLOW_XY:
				x = src->followobj.xyz.x - bot->pl->mo->x;
				y = src->followobj.xyz.y - bot->pl->mo->y;
				d = x * x + y * y;
				a = R_PointToAngle(src->followobj.xyz.x, src->followobj.xyz.y,
					bot->pl->mo->x, bot->pl->mo->y) - new_angle;
				break;

			case BOTCMD_FOLLOW_DIR:
				d = src->followobj.dir.distance;
				a = src->followobj.dir.angle - new_angle;
				d *= d;
				break;

			default:
				a = 0;
				d = 0.0f;
				break;
		}

		// -ES- Fixme: Improve this code. Take momx and momy into consideration.

		// d is squared distance. Don't move if we are very close.
		if (d < 64)
		{
			dest->sidemove = dest->forwardmove = 0;
		}
		else
		{
			// if we don't have to face a specific object, diagonal strafe.
			if (src->facetype == BOTCMD_FACE_NONE)
			{
				DEV_ASSERT2(dest->angleturn == 0);
				dest->angleturn = (a - ANG45) >> 16;
				a = ANG45;
			}

			// decide direction
			if ((a + ANG45) % ANG180 < ANG90)
			{
				if (a + ANG45 < ANG90)
				{
					// forward
					dest->forwardmove = 0x32;
				}
				else
				{
					// backward
					dest->forwardmove = -0x32;
				}
				dest->sidemove = (char)(dest->forwardmove * M_Tan(a));
			}
			else
			{
				if (a - ANG45 < ANG90)
				{
					// left
					dest->sidemove = 0x32;
				}
				else
				{
					// right
					dest->sidemove = -0x32;
				}
				dest->forwardmove = (char)(dest->sidemove * M_Tan(a - ANG90));
			}
		}
	}

	// L_WriteDebug("BOT %d  thr %d  mv %d  conf %d  ftype %d\n",
	// bot->pl->pnum+1, bot->threshold, bot->movecount,
	// bot->confidence, bot->cmd.followtype);

	// L_WriteDebug("  CMD: fwd=%d side=%d\n", dest->forwardmove, dest->sidemove);
}

static void DoThink(bot_t *bot)
{
	BotThink(bot);
}

void P_BotPlayerThinker(const player_t *p, void *data, ticcmd_t *cmd)
{
	bot_t *bot = (bot_t *)data;

	if (gamestate != GS_LEVEL)
		return;

	// recalculate , but only if we have a new gametime
	if (bot->prev_gametime != gametime)
	{
		bot->prev_gametime = gametime;

		Z_Clear(&bot->cmd, botcmd_t, 1);
		bot->cmd.new_weapon = -1;

		DoThink(bot);

		ConvertToTiccmd(bot, &bot->prev_cmd, &bot->cmd);
	}
	else
	{
		// Don't turn around more: If we decided to switch angle the first tic,
		// then that's the angle we prefer, and it's bad to turn around more.
		bot->prev_cmd.angleturn = 0;
		bot->prev_cmd.vertslope = 0;
	}

	*cmd = bot->prev_cmd;
}
