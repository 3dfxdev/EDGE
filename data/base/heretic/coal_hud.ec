//------------------------------------------
//  DOOM HUD CODE for EDGE
//  Copyright (c) 2009-2010 The Edge Team
//  Copyright (C) 1993-1996 by id Software, Inc.
//  Under the GNU General Public License
//------------------------------------------

	// PatchLTFACE = W_CacheLumpName("LTFACE", PU_STATIC); --------------------DISPLAYED--------------------
	// PatchRTFACE = W_CacheLumpName("RTFACE", PU_STATIC); --------------------DISPLAYED--------------------
	// PatchBARBACK = W_CacheLumpName("BARBACK", PU_STATIC); 
	// PatchINVBAR = W_CacheLumpName("INVBAR", PU_STATIC);
	// PatchCHAIN = W_CacheLumpName("CHAIN", PU_STATIC);
	// PatchSTATBAR = W_CacheLumpName("LIFEBAR", PU_STATIC);
	// PatchLIFEGEM = W_CacheLumpName("LIFEGEM2", PU_STATIC);
	// PatchLTFCTOP = W_CacheLumpName("LTFCTOP", PU_STATIC);
	// PatchRTFCTOP = W_CacheLumpName("RTFCTOP", PU_STATIC);
	// PatchSELECTBOX = W_CacheLumpName("SELECTBOX", PU_STATIC);
	// PatchINVLFGEM1 = W_CacheLumpName("INVGEML1", PU_STATIC);
	// PatchINVLFGEM2 = W_CacheLumpName("INVGEML2", PU_STATIC);
	// PatchINVRTGEM1 = W_CacheLumpName("INVGEMR1", PU_STATIC);
	// PatchINVRTGEM2 = W_CacheLumpName("INVGEMR2", PU_STATIC);
	// PatchBLACKSQ    =   W_CacheLumpName("BLACKSQ", PU_STATIC);
	// PatchARMCLEAR = W_CacheLumpName("ARMCLEAR", PU_STATIC);
	// PatchCHAINBACK = W_CacheLumpName("CHAINBACK", PU_STATIC);
	// startLump = W_GetNumForName("IN0");

var face_time  : float
var face_image : string

var ChainWiggle : float //0

var HealthMarker : float //0




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
/*
    //var r = math.floor(math.random() * 2.99)
    var r = math.floor(math.random() * 4.99)

    return "" + r
*/
   var r = math.random() * 2

    if (r < 0.34) return "0"
    if (r < 0.67) return "1"
    return "2"
}

function select_new_face() =
{
    // This routine handles the face states and their timing.

    // dead ?
    if (player.health() <= 0)
    {
        face_image = "FACEB0"
        face_time  = 10
        return
    }

    
    face_image = "FACEA0" //+ pain_digit() + turn_digit()

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
    hud.text_font("GREY_DIGIT")
    hud.text_color(hud.NO_COLOR)

doom_weapon_icon(2, 277, 150, "STGNUM2", "STSMNUM2")
doom_weapon_icon(3, 277, 156, "STGNUM3", "STSMNUM3")
doom_weapon_icon(4, 277, 162, "STGNUM4", "STSMNUM4")

doom_weapon_icon(5, 299, 150, "STGNUM5", "STSMNUM5")
doom_weapon_icon(6, 299, 156, "STGNUM6", "STSMNUM6")
doom_weapon_icon(7, 299, 162, "STGNUM7", "STSMNUM7")

doom_weapon_icon(2, 280, 150, "STGNCOMA", "STSMCOMA")
doom_weapon_icon(3, 280, 156, "STGNCOMA", "STSMCOMA")
doom_weapon_icon(4, 280, 162, "STGNCOMA", "STSMCOMA")

doom_weapon_icon(5, 302, 150, "STGNCOMA", "STSMCOMA")
doom_weapon_icon(6, 302, 156, "STGNCOMA", "STSMCOMA")
doom_weapon_icon(7, 302, 162, "STGNCOMA", "STSMCOMA")


    hud.draw_num2(295, 150, 3, player.ammo(1))
    hud.draw_num2(295, 156, 3, player.ammo(2))
    hud.draw_num2(295, 162, 3, player.ammo(1))

    hud.draw_num2(317, 150, 3, player.ammo(3))
    hud.draw_num2(317, 156, 3, player.ammo(4))
    hud.draw_num2(317, 162, 3, player.ammo(4))

}
// Full-Screen Heretic status bar. .
//DrawCommonBar in Hexetic source code!
// Does not even deal with the Life Chain/Gem bar!
function heretic_status_bar() =
{

		// mirror function doom_status_bar in order of operations.
		hud.draw_image(0, 148, "LTFCTOP")
		hud.draw_image(290, 148, "RTFCTOP")
		hud.draw_image(0, 158, "BARBACK")
		
		//Draw Heretic Health Digit
		//hud.text_font("HERETIC_DIGIT")
		
		
		
		// NOTE (from DoomWiki:) Health Chain (equivalent to Doom's player face)

		hud.draw_image(34, 160, "LIFEBAR")
		
		hud.draw_image(0, 190, "CHAINBAC") // changed to match Doom's centered face icon, which it isn't!
		
		//hud.set_alpha(0.0)
		
		hud.text_font("HERETIC_DIGIT")
		//hud.set_scale(1.2)
		hud.draw_num(85, 170, 9, player.health()) // 100?
		
		hud.draw_num(252, 170, 3, player.total_armor()) //
		
		// player picks up keys and stuff
		
		
	if (player.has_key(2))
    hud.draw_image(  153, 164, "ykeyicon")

	if (player.has_key(4))
    hud.draw_image(  153, 172, "gkeyicon")

	if (player.has_key(1))
    hud.draw_image(  153, 180, "bkeyicon")

		
		// weapon selection box (displays ammo, and picture of ammunition in Heretic)
	if (player.cur_weapon() == "PISTOL")
	{
		hud.draw_image(115, 170, "INAMGLD")
    hud.draw_num( 135, 162, 3, player.ammo(1))
	}
	

    if (player.cur_weapon() == "SHOTGUN")
		hud.draw_image(115, 170, "INAMBOW")
    hud.draw_num( 135, 162, 3, player.main_ammo(1))

    if (player.cur_weapon() == "CHAINGUN")
		hud.draw_image(115, 170, "INAMBST")
    hud.draw_num( 135, 162, 3, player.main_ammo(1))

    if (player.cur_weapon() == "ROCKET_LAUNCHER")
		hud.draw_image(115, 170, "INAMRAM")
    hud.draw_num( 135, 162, 3, player.main_ammo(1))

    if (player.cur_weapon() == "PLASMA_RIFLE")
		hud.draw_image(115, 170, "INAMPNX")
    hud.draw_num( 135, 162, 3, player.main_ammo(1))

    if (player.cur_weapon() == "BFG9000")
		hud.draw_image(115, 170, "INAMLOB")
    hud.draw_num( 135, 162, 3, player.main_ammo(1))

}
// 3DGE-based overlay (more advanced overlay than in default Heretic)
function heretic_overlay_status() = 
{
    hud.text_font("HERETIC_BIG")

	doomguy_face(0, 166)

	hud.draw_num2(78, 179, 3, player.health()) // 100

    //hud.draw_image(  79, 179, "STTPRCNT")

    hud.draw_num2(312, 179, 3, player.total_armor())

    hud.text_font("HERETIC")
    hud.text_color(hud.NO_COLOR)
    hud.draw_text(38, 170, "HEALTH")
	hud.draw_text(220, 170, "AMMO")
    hud.draw_text(270, 170, "ARMOR")
	

}

function doom_automap() =
{
    // Background is already black, only need to use 'solid_box'
    // when we want a different color.
    //
	
    ///hud.gradient_box(0, 0, 320, 200 - 32, '80 80 80')

    hud.render_automap(0, 0, 320, 200)

	//hud.draw_image(0, 0, "AUTOPAGE")
    hud.text_color(hud.NO_COLOR)
    hud.text_font("HERETIC")
    hud.draw_text(0, 200 - 32 - 10, hud.map_title())
    hud.draw_text(0, 200 - 32 - 10, hud.map_title())

    heretic_status_bar()
}

function heretic_chain_bar() =
{
/*     if (player.health() <= 0)
        return */

    var wiggle = player.health()

    wiggle = math.floor(1 + 21 * ((100 - wiggle) / 100.1))

    var barname : string;
    
/*     if (air < 10)
        barname = "AIRBAR0" + wiggle
    else */
        barname = "CHAIN"


    hud.draw_image(0, 190, barname) //finally draw the god damn chain
}


function edge_air_bar() =
{
    if (player.health() <= 0)
        return

    if (! player.under_water())
        return

    var air = player.air_in_lungs()

    air = math.floor(1 + 21 * ((100 - air) / 100.1))

    var barname : string;
    
    if (air < 10)
        barname = "AIRBAR0" + air
    else
        barname = "AIRBAR" + air

    hud.draw_image(0, 0, barname)
}


function begin_level() =
{
}

function draw_all() =
{
    hud.coord_sys(320, 200) //Didn't Heretic draw at 320x240?
    hud.grab_times()

    //hud.render_world(0, 0, 320, 200)

    if (hud.check_automap())
    {
       doom_automap()
        return
    }
	// there are three standard HUDs
	var which = hud.which_hud() % 3

    if (which == 1) //heretic_overlay_status()
        hud.render_world(0, 0, 320, 200)// 0, 0, 320, 200. . //0, 200 - 32 - 10,
    else //heretic_status_bar()
        hud.render_world(0, 0, 320, 200 - 30)

    if (which == 1)
        heretic_overlay_status()
    else if (which == 2)
		
        heretic_status_bar()
		heretic_chain_bar()
		
	
	cam.set_vert_bob(1.0)

	//if (player.get_side_move() != 0)
	//cam.set_roll_bob(0.25)
	//else
	cam.set_roll_bob(0.0)
	

    edge_air_bar()
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
	
        hud.render_world(0, 0, 320, 200)

        heretic_overlay_status()

    edge_air_bar()
	// message_ticker()
}