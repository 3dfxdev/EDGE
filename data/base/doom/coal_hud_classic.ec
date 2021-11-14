//------------------------------------------
//  DOOM HUD CODE for EDGE
//  Copyright (c) 2009-2010 The Edge Team
//  Copyright (C) 1993-1996 by id Software, Inc.
//  Under the GNU General Public License
//------------------------------------------

var face_time  : float
var face_image : string


function doom_weapon_icon(slot, x, y, off_pic : string, on_pic : string) =
{
    if (player.has_weapon_slot(slot))
        hud.draw_image(x, y, on_pic)
    else
        hud.draw_image(x, y, off_pic)
}


function doom_key(x, y, card, skull,
    card_pic : string, skull_pic : string, both_pic : string) =
{
    var has_cd = player.has_key(card)
    var has_sk = player.has_key(skull)

    if (has_cd && has_sk)
    {
        hud.draw_image(x, y, both_pic)
    }
    else if (has_cd)
    {
        hud.draw_image(x, y, card_pic)
    }
    else if (has_sk)
    {
        hud.draw_image(x, y, skull_pic)
    }
}


function pain_digit() : string =
{
    var health = player.health()
    if (health > 100)
        health = 100

    var index = math.floor(4.99 * (100 - health) / 100)

    assert(index >= 0)
    assert(index <= 4)

    return "" + index
}

function turn_digit() : string =
{
    //Lobo: this never gives > 1 so face never looks left
    //var r = math.floor(math.random() * 2.99)
    
    var r = math.floor(math.random2() / 4 ) //always between 0 and 2
    
    return "" + r
}

function select_new_face() =
{
    // This routine handles the face states and their timing.
    // The precedence of expressions is:
    //
    //    dead > evil grin > turned head > straight ahead
    //

    // dead ?
    if (player.health() <= 0)
    {
        face_image = "STFDEAD0"
        face_time  = 10
        return
    }

    // evil grin when player just picked up a weapon
    if (player.is_grinning())
    {
        face_image = "STFEVL" + pain_digit()
        face_time  = 7
        return
    }

    // being attacked ?
    if (player.hurt_by())
    {
        if (player.hurt_pain() > 20)
        {
            face_image = "STFOUCH" + pain_digit()
            face_time = 26
            return
        }

        var dir = 0

        if (player.hurt_by() == "enemy" ||
            player.hurt_by() == "friend")
        {
            dir = player.hurt_dir()
        }

        if (dir < 0)
            face_image = "STFTL" + pain_digit() + "0"
        else if (dir > 0)
            face_image = "STFTR" + pain_digit() + "0"
        else
            face_image = "STFKILL" + pain_digit()

        face_time = 35
        return
    }

    // rampaging?
    if (player.is_rampaging())
    {
        face_image = "STFKILL" + pain_digit()
        face_time  = 7
        return
    }

    // god mode?
    if (player.has_power(player.INVULN))
    {
        face_image = "STFGOD0"
        face_time  = 7
        return
    }

    // default: look about the place...
    face_image = "STFST" + pain_digit() + turn_digit()
    face_time  = 17
}

function doomguy_face (x, y) =
{
    //---| doomguy_face |---

    face_time = face_time - hud.passed_time

    if (face_time <= 0)
        select_new_face()

    // FIXME faceback

    hud.draw_image(x - 1, y - 1, face_image)
}


function doom_little_ammo() =
{
    hud.text_font("YELLOW_DIGIT")
    hud.text_color(hud.NO_COLOR)

    hud.draw_num2(288, 173, 3, player.ammo(1))
    hud.draw_num2(288, 179, 3, player.ammo(2))
    hud.draw_num2(288, 185, 3, player.ammo(3))
    hud.draw_num2(288, 191, 3, player.ammo(4))

    hud.draw_num2(314, 173, 3, player.ammomax(1))
    hud.draw_num2(314, 179, 3, player.ammomax(2))
    hud.draw_num2(314, 185, 3, player.ammomax(3))
    hud.draw_num2(314, 191, 3, player.ammomax(4))
}


function doom_status_bar() =
{
    hud.draw_image(  0, 168, "STBAR")
	
    hud.draw_image(  -83, 168, "STBARL") // Widescreen border
    hud.draw_image(  320, 168, "STBARR") // Widescreen border
	
    hud.draw_image( 90, 171, "STTPRCNT")
    hud.draw_image(221, 171, "STTPRCNT")

    hud.text_font("BIG_DIGIT")

    hud.draw_num2( 44, 171, 3, player.main_ammo(1) )
    hud.draw_num2( 90, 171, 3, player.health()     )
    hud.draw_num2(221, 171, 3, player.total_armor())

    if (hud.game_mode() == "dm")
    {
        hud.draw_num2(138, 171, 2, player.frags())
    }
    else
    {
        hud.draw_image(104, 168, "STARMS")

        doom_weapon_icon(2, 111, 172, "STGNUM2", "STYSNUM2")
        doom_weapon_icon(3, 123, 172, "STGNUM3", "STYSNUM3")
        doom_weapon_icon(4, 135, 172, "STGNUM4", "STYSNUM4")

        doom_weapon_icon(5, 111, 182, "STGNUM5", "STYSNUM5")
        doom_weapon_icon(6, 123, 182, "STGNUM6", "STYSNUM6")
        doom_weapon_icon(7, 135, 182, "STGNUM7", "STYSNUM7")
    }

    doomguy_face(144, 169)

    doom_key(239, 171, 1, 5, "STKEYS0", "STKEYS3", "STKEYS6")
    doom_key(239, 181, 2, 6, "STKEYS1", "STKEYS4", "STKEYS7")
    doom_key(239, 191, 3, 7, "STKEYS2", "STKEYS5", "STKEYS8")

    doom_little_ammo()
}


function doom_overlay_status() = 
{
    hud.text_font("BIG_DIGIT")

    hud.draw_num2(100, 171, 3, player.health())

    hud.text_color(hud.YELLOW)
    hud.draw_num2( 44, 171, 3, player.main_ammo(1))

    if (player.total_armor() > 100)
        hud.text_color(hud.BLUE)
    else
        hud.text_color(hud.GREEN)

    hud.draw_num2(242, 171, 3, player.total_armor())

    doom_key(256, 171, 1, 5, "STKEYS0", "STKEYS3", "STKEYS6")
    doom_key(256, 181, 2, 6, "STKEYS1", "STKEYS4", "STKEYS7")
    doom_key(256, 191, 3, 7, "STKEYS2", "STKEYS5", "STKEYS8")

    doom_little_ammo()
}


function new_overlay_status() = 
 {
	
	if (player.has_key(1))
		hud.draw_image(  86, 170, "STKEYS0")

	if (player.has_key(2))
		hud.draw_image(  86, 180, "STKEYS1")

	if (player.has_key(3))
		hud.draw_image(  86, 190, "STKEYS2")

	if (player.has_key(5))
		hud.draw_image(  96, 170, "STKEYS3")

	if (player.has_key(6))
		hud.draw_image(  96, 180, "STKEYS4")

	if (player.has_key(7))
		hud.draw_image(  96, 190, "STKEYS5")


    hud.set_alpha(0.9) //**Alters Transparency of HUD Elements**
    hud.text_font("BIG_DIGIT")
	hud.set_scale(0.80)
	
    if (player.cur_weapon() == "PISTOL")
    hud.draw_num2( 295, 180, 3, player.ammo(1))

    if (player.cur_weapon() == "SHOTGUN")
    hud.draw_num2( 295, 180, 3, player.main_ammo(1))

    if (player.cur_weapon() == "SUPERSHOTGUN")
    hud.draw_num2( 295, 180, 3, player.ammo(2))

    if (player.cur_weapon() == "CHAINGUN")
    hud.draw_num2( 295, 180, 3, player.main_ammo(1))

    if (player.cur_weapon() == "ROCKET_LAUNCHER")
    hud.draw_num2( 295, 180, 3, player.main_ammo(1))

    if (player.cur_weapon() == "PLASMA_RIFLE")
    hud.draw_num2( 295, 180, 3, player.main_ammo(1))

    if (player.cur_weapon() == "BFG9000")
    hud.draw_num2( 295, 180, 3, player.main_ammo(1))
	
	hud.text_color(hud.NO_COLOR)

   if (player.health() < 35)
        hud.text_color(hud.RED) 
    else
    if (player.health() < 65)
        hud.text_color(hud.RED)
    else
    if (player.health() < 100)
        hud.text_color(hud.RED)
    else
        hud.text_color(hud.RED)
		
	hud.set_scale(0.75)
	hud.text_color(hud.NO_COLOR)
	hud.draw_num2(60, 180, 3, player.health()) // 100
	hud.text_color(hud.NO_COLOR)

//start armour hud edits

if (player.total_armor() > 0)
{
    hud.text_color(hud.NO_COLOR)

    if (player.total_armor() <= 99)
        hud.draw_image(3, 165, "ARM1A0")
    else
    if (player.total_armor() > 200)
        hud.text_color(hud.NO_COLOR)
    else
    if (player.total_armor() > 300)
        hud.text_color(hud.NO_COLOR)
    else
        hud.text_color(hud.NO_COLOR)
    
	hud.set_scale(0.75)
    hud.draw_num2(60, 165, 3, player.total_armor())
	
		if (player.armor(1))
	hud.draw_image(3, 165, "ARM1A0")
	
		if (player.armor(2))
	hud.draw_image(3, 165, "ARM2A0")

//end armour hud edits	
}
    hud.text_color(hud.NO_COLOR)

    hud.text_font("DOOM")
    hud.text_color(hud.WHITE)
	
    hud.set_scale(0.85) //scales medikit in hud
    hud.draw_image(3, 180, "MEDIA0") //HEALTH GFX in HUD
	hud.text_color(hud.NO_COLOR)


    if (player.cur_weapon() == "PISTOL")
    hud.draw_image(297, 180, "CLIPA0")

    if (player.cur_weapon() == "SHOTGUN")
    hud.draw_image(297, 180, "SHELA0")

    if (player.cur_weapon() == "SUPERSHOTGUN")
    hud.draw_image(297, 180, "SHELA0")

    if (player.cur_weapon() == "CHAINGUN")
    hud.draw_image(297, 180, "CLIPA0")

    if (player.cur_weapon() == "ROCKET_LAUNCHER")
    hud.draw_image(297, 175, "ROCKA0")

    if (player.cur_weapon() == "PLASMA_RIFLE")
    hud.draw_image(297, 180, "CELLA0")

    if (player.cur_weapon() == "BFG9000")
    hud.draw_image(297, 180, "CELLA0")

}


function doom_automap() =
{
    // Background is already black, only need to use 'solid_box'
    // when we want a different color.
    //
    // hud.solid_box(0, 0, 320, 200 - 32, '80 80 80')

    hud.render_automap(0, 0, 320, 200 - 32)
	
    new_overlay_status()

    hud.text_font("DOOM")
    hud.text_color(hud.GREEN)
    
    hud.draw_text(0, 200 - 32 - 10, hud.map_title())
    
    hud.set_scale(0.75)
    hud.draw_text(10, 20, "Kills:    " + player.kills() + "/" + player.map_enemies())

	if (player.map_secrets() > 0)
	{
		hud.draw_text(10, 25, "Secrets: " + player.secrets() + "/" + player.map_secrets())
	}
	if (player.map_items() > 0)
	{
		hud.draw_text(10, 30, "Items:    " + player.items() + "/" + player.map_items())
	}
	hud.set_scale(1.0)
}


function edge_air_bar() =
{
	var TopX = 250
	var TopY = 10
	var BarHeight = 8
	var BarLength = 51
	var BarValue = 51
	
	
    if (player.health() <= 0)
        return

    if (! player.under_water())
        return

	BarValue = math.floor(player.air_in_lungs() / 2)
	
    hud.thin_box(TopX, TopY, BarLength, BarHeight, hud.GRAY)
	
	if (BarValue > 1)
	{
		hud.gradient_box(TopX + 1, TopY + 1, BarValue - 1, BarHeight - 2, hud.BLUE, hud.LIGHTBLUE, hud.BLUE, hud.LIGHTBLUE)
	}
}

//***********************
// Start footsteps code
var walk_speed = 12
var wait_time  : float

function DoesNameStartWith(TheName : string, ThePart : string) : float =
{
	var tempstr : string
	var templen : float
	
	tempstr = strings.sub(TheName, 1, strings.len(ThePart))
	
	//hud.draw_text(10, 10, tempstr)
	
	if (tempstr == ThePart)
		return 1
	
	return 0
}

function edge_footsteps() =
{
    if (player.is_swimming())
        return
        
    if (player.is_jumping())
        return    

	if (! player.on_ground())
	return 

	if (player.move_speed() <= 2)
	{	
		wait_time  = walk_speed
		return
	}
    
    wait_time = wait_time - hud.passed_time
    
   	/*
   	hud.text_font("DOOM")
    hud.draw_text(10, 10, player.floor_flat()) 
    hud.draw_text(10, 50,"speed:" + player.move_speed())
    hud.draw_text(10, 30,"sector tag:" + player.sector_tag())
    */
    if (wait_time > 0)
    	return
    
    if (DoesNameStartWith(player.floor_flat(), "FWATER") == 1)
    	hud.play_sound("FSWAT?")
    else
   	if (DoesNameStartWith(player.floor_flat(), "NUKAGE") == 1)
    	hud.play_sound("FSWAT?")
    else
    if (DoesNameStartWith(player.floor_flat(), "BLOOD") == 1)
    	hud.play_sound("FSWAT?")
    else //slime is a special case :(
    if (player.floor_flat() == "SLIME1")
    	hud.play_sound("FSWAT?")
    else
    if (player.floor_flat() == "SLIME2")
    	hud.play_sound("FSWAT?")
    else
    if (player.floor_flat() == "SLIME3")
    	hud.play_sound("FSWAT?")
    else
    if (player.floor_flat() == "SLIME4")
    	hud.play_sound("FSWAT?")
    else
    if (player.floor_flat() == "SLIME5")
    	hud.play_sound("FSWAT?")
    else
    if (player.floor_flat() == "SLIME6")
    	hud.play_sound("FSWAT?")
    else
    if (player.floor_flat() == "SLIME7")
    	hud.play_sound("FSWAT?")
        else
    if (player.floor_flat() == "SLIME8")
    	hud.play_sound("FSWAT?")
  
    
    
    //var loopCounter = 0
    //for (loopCounter = 1, 5) 
    //{
    //	hud.draw_text(loopCounter * 10, 30, ".")
    //}
    
    if (player.move_speed() > 10) //we're running so speed up time between sfx
	{	
		wait_time  = walk_speed - 3
	}	
	else
	{
		wait_time  = walk_speed
    }
}
// End footsteps code
//***********************


function begin_level() =
{
}

function draw_all() =
{
    hud.coord_sys(320, 200)
    hud.grab_times()

    if (hud.check_automap())
    {
        doom_automap()
        return
    }
	
	

    // there are three standard HUDs
    var which = hud.which_hud() % 3

    if (which == 0)
        hud.render_world(0, 0, 320, 200 - 32)
    else
        hud.render_world(0, 0, 320, 200)

    if (which == 0)
        doom_status_bar()
    else if (which == 2)
        //doom_overlay_status()
		new_overlay_status()
		
		
	cam.set_vert_bob(1.0)
	
	//if (player.get_side_move() != 0)
	//cam.set_roll_bob(0.25)
	//else
	cam.set_roll_bob(0.0)
   
}

function draw_split() =
{
    hud.coord_sys(320, 200)
    hud.grab_times()


    if (hud.check_automap())
    {
        doom_automap()
        return
    }
	
	// there are three standard HUDs
    var which = hud.which_hud() % 3
	
        hud.render_world(0, 0, 320, 200)
		
		if (which == 0)
        doom_splitscreen_status()
		else if (which == 2)
		zdoom_overlay_status()

    edge_air_bar()
	cam.set_vert_bob(1.0)
	cam.set_roll_bob(0.0)
	// message_ticker()
}
