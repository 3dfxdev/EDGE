//------------------------------------------
//  DOOM HUD AND PLAYER CONTROL CODE for EDGE
//  Copyright (c) 2009-2018 The Edge Team
//  Copyright (C) 1996 by id Software, Inc.
//  Under the GNU General Public License
//------------------------------------------

var face_priority : float
var face_image : string
var face_time : float
var w_message : string
var text_float : float
var bob_z_scale : float
var bob_r_scale : float


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
	
	//var r = math.random() * 2

  //  if (r < 0.34) return "0"
  //  if (r < 0.67) return "1"
   // return "2"
    
    return "" + r
}

function select_new_face() =
{
    // This routine handles the face states and their timing.
    // The precedence of expressions is:
    //
    //    dead > evil grin > turned head > straight ahead
    //
    if (face_priority < 10 && player.health() <= 0)
    {
        // dead
        face_priority = 9
        face_image = "STFDEAD0"
        face_time = 1
    }

    if (face_priority < 9 && player.is_grinning())
    {
        // evil grin if just picked up weapon
        face_priority = 8
        face_image = "STFEVL" + pain_digit()
        face_time = 70
    }

    if (face_priority < 8 &&
        (player.hurt_by() == "enemy" || player.hurt_by() == "friend"))
    {
        // being attacked
        var dir = player.hurt_dir()
        face_priority = 7
        if (dir < 0)
            face_image = "STFTL" + pain_digit() + "0"
        else if (dir > 0)
            face_image = "STFTR" + pain_digit() + "0"
        else
            face_image = "STFKILL" + pain_digit()
        if (player.hurt_pain() > 20)
            face_image = "STFOUCH" + pain_digit()
        face_time = 35
    }

    if (face_priority < 7 && player.hurt_pain())
    {
        // getting hurt because of your own damn stupidity
        face_priority = 6
        if (player.hurt_pain() > 20)
            face_image = "STFOUCH" + pain_digit()
        else
            face_image = "STFKILL" + pain_digit()
        face_time = 35
    }

    if (face_priority < 6 && player.is_rampaging())
    {
        // rapid firing
        face_priority = 5
        face_image = "STFKILL" + pain_digit()
        face_time = 1
    }

    if (face_priority < 5 && player.has_power(player.INVULN))
    {
        // invulnerability
        face_priority = 4
        face_image = "STFGOD0"
        face_time = 1
    }

    if (!face_time)
    {
        // look left or right
        face_priority = 0
        face_image = "STFST" + pain_digit() + turn_digit()
        face_time = 17
    }
    face_time = face_time - hud.passed_time
}

function doomguy_face (x, y) =
{
    //---| doomguy_face |---
    select_new_face()

    // FIXME faceback

    hud.draw_image(x - 1, y - 1, face_image)
}

function doom_little_ammo() =
{
    hud.text_font("YELLOW_DIGIT")
    hud.text_color(hud.NO_COLOR)

    hud.draw_num(288, 173, 3, player.ammo(1))
    hud.draw_num(288, 179, 3, player.ammo(2))
    hud.draw_num(288, 185, 3, player.ammo(3))
    hud.draw_num(288, 191, 3, player.ammo(4))

    hud.draw_num(314, 173, 3, player.ammomax(1))
    hud.draw_num(314, 179, 3, player.ammomax(2))
    hud.draw_num(314, 185, 3, player.ammomax(3))
    hud.draw_num(314, 191, 3, player.ammomax(4))
}


function doom_little_ammo2() = 
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


function doom_status_bar() =
{
    hud.draw_image(  0, 168, "STBAR")
	
	hud.draw_image(  -83, 168, "STBARL") // Widescreen border
    hud.draw_image(  320, 168, "STBARR") // Widescreen border
	
    hud.draw_image( 90, 171, "STTPRCNT")
    hud.draw_image(221, 171, "STTPRCNT")

    hud.text_font("BIG_DIGIT")

    hud.draw_num( 44, 171, 3, player.main_ammo(1) )
    hud.draw_num( 90, 171, 3, player.health()     )
    hud.draw_num(221, 171, 3, player.total_armor())

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
	if (player.has_power(player.GOGGLES))
        hud.draw_image(247, 0, "STTPOWR6")
    if (player.has_power(player.NIGHT_VIS))
        hud.draw_image(247, 0, "STTPOWR1")
    if (player.has_power(player.ACID_SUIT))
        hud.draw_image(247, 25, "STTPOWR2")
    if (player.has_power(player.INVIS))
        hud.draw_image(247, 50, "STTPOWR3")
    if (player.has_power(player.BERSERK))
        hud.draw_image(247, 75, "STTPOWR4")
    if (player.has_power(player.INVULN))
        hud.draw_image(247, 100, "STTPOWR5")

    if (player.power_left(player.NIGHT_VIS) > 114)
        hud.draw_image(247, 0, "POWBAR01")
else
    if (player.power_left(player.NIGHT_VIS) > 108)
        hud.draw_image(247, 0, "POWBAR02")
else
    if (player.power_left(player.NIGHT_VIS) > 102)
        hud.draw_image(247, 0, "POWBAR03")
else
    if (player.power_left(player.NIGHT_VIS) > 96)
        hud.draw_image(247, 0, "POWBAR04")
else
    if (player.power_left(player.NIGHT_VIS) > 90)
        hud.draw_image(247, 0, "POWBAR05")
else
    if (player.power_left(player.NIGHT_VIS) > 84)
        hud.draw_image(247, 0, "POWBAR06")
else
    if (player.power_left(player.NIGHT_VIS) > 78)
        hud.draw_image(247, 0, "POWBAR07")
else
    if (player.power_left(player.NIGHT_VIS) > 72)
        hud.draw_image(247, 0, "POWBAR08")
else
    if (player.power_left(player.NIGHT_VIS) > 66)
        hud.draw_image(247, 0, "POWBAR09")
else
    if (player.power_left(player.NIGHT_VIS) > 60)
        hud.draw_image(247, 0, "POWBAR10")
else
    if (player.power_left(player.NIGHT_VIS) > 54)
        hud.draw_image(247, 0, "POWBAR11")
else
    if (player.power_left(player.NIGHT_VIS) > 48)
        hud.draw_image(247, 0, "POWBAR12")
else
    if (player.power_left(player.NIGHT_VIS) > 42)
        hud.draw_image(247, 0, "POWBAR13")
else
    if (player.power_left(player.NIGHT_VIS) > 36)
        hud.draw_image(247, 0, "POWBAR14")
else
    if (player.power_left(player.NIGHT_VIS) > 30)
        hud.draw_image(247, 0, "POWBAR15")
else
    if (player.power_left(player.NIGHT_VIS) > 24)
        hud.draw_image(247, 0, "POWBAR16")
else
    if (player.power_left(player.NIGHT_VIS) > 18)
        hud.draw_image(247, 0, "POWBAR17")
else
    if (player.power_left(player.NIGHT_VIS) > 12)
        hud.draw_image(247, 0, "POWBAR18")
else
    if (player.power_left(player.NIGHT_VIS) > 6)
        hud.draw_image(247, 0, "POWBAR19")
else
    if (player.power_left(player.NIGHT_VIS) > 1)
        hud.draw_image(247, 0, "POWBAR20")

    if (player.power_left(player.ACID_SUIT) > 57)
        hud.draw_image(247, 25, "POWBAR01")
else
    if (player.power_left(player.ACID_SUIT) > 54)
        hud.draw_image(247, 25, "POWBAR02")
else
    if (player.power_left(player.ACID_SUIT) > 51)
        hud.draw_image(247, 25, "POWBAR03")
else
    if (player.power_left(player.ACID_SUIT) > 48)
        hud.draw_image(247, 25, "POWBAR04")
else
    if (player.power_left(player.ACID_SUIT) > 45)
        hud.draw_image(247, 25, "POWBAR05")
else
    if (player.power_left(player.ACID_SUIT) > 42)
        hud.draw_image(247, 25, "POWBAR06")
else
    if (player.power_left(player.ACID_SUIT) > 39)
        hud.draw_image(247, 25, "POWBAR07")
else
    if (player.power_left(player.ACID_SUIT) > 36)
        hud.draw_image(247, 25, "POWBAR08")
else
    if (player.power_left(player.ACID_SUIT) > 33)
        hud.draw_image(247, 25, "POWBAR09")
else
    if (player.power_left(player.ACID_SUIT) > 30)
        hud.draw_image(247, 25, "POWBAR10")
else
    if (player.power_left(player.ACID_SUIT) > 27)
        hud.draw_image(247, 25, "POWBAR11")
else
    if (player.power_left(player.ACID_SUIT) > 24)
        hud.draw_image(247, 25, "POWBAR12")
else
    if (player.power_left(player.ACID_SUIT) > 21)
        hud.draw_image(247, 25, "POWBAR13")
else
    if (player.power_left(player.ACID_SUIT) > 18)
        hud.draw_image(247, 25, "POWBAR14")
else
    if (player.power_left(player.ACID_SUIT) > 15)
        hud.draw_image(247, 25, "POWBAR15")
else
    if (player.power_left(player.ACID_SUIT) > 12)
        hud.draw_image(247, 25, "POWBAR16")
else
    if (player.power_left(player.ACID_SUIT) > 9)
        hud.draw_image(247, 25, "POWBAR17")
else
    if (player.power_left(player.ACID_SUIT) > 6)
        hud.draw_image(247, 25, "POWBAR18")
else
    if (player.power_left(player.ACID_SUIT) > 3)
        hud.draw_image(247, 25, "POWBAR19")
else
    if (player.power_left(player.ACID_SUIT) > 1)
        hud.draw_image(247, 25, "POWBAR20")

    if (player.power_left(player.INVIS) > 95)
        hud.draw_image(247, 50, "POWBAR01")
else
    if (player.power_left(player.INVIS) > 90)
        hud.draw_image(247, 50, "POWBAR02")
else
    if (player.power_left(player.INVIS) > 85)
        hud.draw_image(247, 50, "POWBAR03")
else
    if (player.power_left(player.INVIS) > 80)
        hud.draw_image(247, 50, "POWBAR04")
else
    if (player.power_left(player.INVIS) > 75)
        hud.draw_image(247, 50, "POWBAR05")
else
    if (player.power_left(player.INVIS) > 70)
        hud.draw_image(247, 50, "POWBAR06")
else
    if (player.power_left(player.INVIS) > 65)
        hud.draw_image(247, 50, "POWBAR07")
else
    if (player.power_left(player.INVIS) > 60)
        hud.draw_image(247, 50, "POWBAR08")
else
    if (player.power_left(player.INVIS) > 55)
        hud.draw_image(247, 50, "POWBAR09")
else
    if (player.power_left(player.INVIS) > 50)
        hud.draw_image(247, 50, "POWBAR10")
else
    if (player.power_left(player.INVIS) > 45)
        hud.draw_image(247, 50, "POWBAR11")
else
    if (player.power_left(player.INVIS) > 40)
        hud.draw_image(247, 50, "POWBAR12")
else
    if (player.power_left(player.INVIS) > 35)
        hud.draw_image(247, 50, "POWBAR13")
else
    if (player.power_left(player.INVIS) > 30)
        hud.draw_image(247, 50, "POWBAR14")
else
    if (player.power_left(player.INVIS) > 25)
        hud.draw_image(247, 50, "POWBAR15")
else
    if (player.power_left(player.INVIS) > 20)
        hud.draw_image(247, 50, "POWBAR16")
else
    if (player.power_left(player.INVIS) > 15)
        hud.draw_image(247, 50, "POWBAR17")
else
    if (player.power_left(player.INVIS) > 10)
        hud.draw_image(247, 50, "POWBAR18")
else
    if (player.power_left(player.INVIS) > 5)
        hud.draw_image(247, 50, "POWBAR19")
else
    if (player.power_left(player.INVIS) > 1)
        hud.draw_image(247, 50, "POWBAR20")

    if (player.power_left(player.BERSERK) > 57)
        hud.draw_image(247, 75, "POWBAR01")
else
    if (player.power_left(player.BERSERK) > 54)
        hud.draw_image(247, 75, "POWBAR02")
else
    if (player.power_left(player.BERSERK) > 51)
        hud.draw_image(247, 75, "POWBAR03")
else
    if (player.power_left(player.BERSERK) > 48)
        hud.draw_image(247, 75, "POWBAR04")
else
    if (player.power_left(player.BERSERK) > 45)
        hud.draw_image(247, 75, "POWBAR05")
else
    if (player.power_left(player.BERSERK) > 42)
        hud.draw_image(247, 75, "POWBAR06")
else
    if (player.power_left(player.BERSERK) > 39)
        hud.draw_image(247, 75, "POWBAR07")
else
    if (player.power_left(player.BERSERK) > 36)
        hud.draw_image(247, 75, "POWBAR08")
else
    if (player.power_left(player.BERSERK) > 33)
        hud.draw_image(247, 75, "POWBAR09")
else
    if (player.power_left(player.BERSERK) > 30)
        hud.draw_image(247, 75, "POWBAR10")
else
    if (player.power_left(player.BERSERK) > 27)
        hud.draw_image(247, 75, "POWBAR11")
else
    if (player.power_left(player.BERSERK) > 24)
        hud.draw_image(247, 75, "POWBAR12")
else
    if (player.power_left(player.BERSERK) > 21)
        hud.draw_image(247, 75, "POWBAR13")
else
    if (player.power_left(player.BERSERK) > 18)
        hud.draw_image(247, 75, "POWBAR14")
else
    if (player.power_left(player.BERSERK) > 15)
        hud.draw_image(247, 75, "POWBAR15")
else
    if (player.power_left(player.BERSERK) > 12)
        hud.draw_image(247, 75, "POWBAR16")
else
    if (player.power_left(player.BERSERK) > 9)
        hud.draw_image(247, 75, "POWBAR17")
else
    if (player.power_left(player.BERSERK) > 6)
        hud.draw_image(247, 75, "POWBAR18")
else
    if (player.power_left(player.BERSERK) > 3)
        hud.draw_image(247, 75, "POWBAR19")
else
    if (player.power_left(player.BERSERK) > 1)
        hud.draw_image(247, 75, "POWBAR20")

    if (player.power_left(player.INVULN) > 42)
        hud.draw_image(247, 100, "POWBAR01")
else
    if (player.power_left(player.INVULN) > 40)
        hud.draw_image(247, 100, "POWBAR02")
else
    if (player.power_left(player.INVULN) > 38)
        hud.draw_image(247, 100, "POWBAR03")
else
    if (player.power_left(player.INVULN) > 36)
        hud.draw_image(247, 100, "POWBAR04")
else
    if (player.power_left(player.INVULN) > 34)
        hud.draw_image(247, 100, "POWBAR05")
else
    if (player.power_left(player.INVULN) > 32)
        hud.draw_image(247, 100, "POWBAR06")
else
    if (player.power_left(player.INVULN) > 30)
        hud.draw_image(247, 100, "POWBAR07")
else
    if (player.power_left(player.INVULN) > 28)
        hud.draw_image(247, 100, "POWBAR08")
else
    if (player.power_left(player.INVULN) > 26)
        hud.draw_image(247, 100, "POWBAR09")
else
    if (player.power_left(player.INVULN) > 24)
        hud.draw_image(247, 100, "POWBAR10")
else
    if (player.power_left(player.INVULN) > 22)
        hud.draw_image(247, 100, "POWBAR11")
else
    if (player.power_left(player.INVULN) > 20)
        hud.draw_image(247, 100, "POWBAR12")
else
    if (player.power_left(player.INVULN) > 18)
        hud.draw_image(247, 100, "POWBAR13")
else
    if (player.power_left(player.INVULN) > 16)
        hud.draw_image(247, 100, "POWBAR14")
else
    if (player.power_left(player.INVULN) > 14)
        hud.draw_image(247, 100, "POWBAR15")
else
    if (player.power_left(player.INVULN) > 12)
        hud.draw_image(247, 100, "POWBAR16")
else
    if (player.power_left(player.INVULN) > 10)
        hud.draw_image(247, 100, "POWBAR17")
else
    if (player.power_left(player.INVULN) > 8)
        hud.draw_image(247, 100, "POWBAR18")
else
    if (player.power_left(player.INVULN) > 6)
        hud.draw_image(247, 100, "POWBAR19")
else
    if (player.power_left(player.INVULN) > 4)
        hud.draw_image(247, 100, "POWBAR19")
else
    if (player.power_left(player.INVULN) > 2)
        hud.draw_image(247, 100, "POWBAR20")
else
    if (player.power_left(player.INVULN) > 1)
        hud.draw_image(247, 100, "POWBAR20")
// DOOM NV Goggles time bars

    if (player.power_left(player.GOGGLES) > 114)
        hud.draw_image(247, 0, "POWBAR01")
else
    if (player.power_left(player.GOGGLES) > 108)
        hud.draw_image(247, 0, "POWBAR02")
else
    if (player.power_left(player.GOGGLES) > 102)
        hud.draw_image(247, 0, "POWBAR03")
else
    if (player.power_left(player.GOGGLES) > 96)
        hud.draw_image(247, 0, "POWBAR04")
else
    if (player.power_left(player.GOGGLES) > 90)
        hud.draw_image(247, 0, "POWBAR05")
else
    if (player.power_left(player.GOGGLES) > 84)
        hud.draw_image(247, 0, "POWBAR06")
else
    if (player.power_left(player.GOGGLES) > 78)
        hud.draw_image(247, 0, "POWBAR07")
else
    if (player.power_left(player.GOGGLES) > 72)
        hud.draw_image(247, 0, "POWBAR08")
else
    if (player.power_left(player.GOGGLES) > 66)
        hud.draw_image(247, 0, "POWBAR09")
else
    if (player.power_left(player.GOGGLES) > 60)
        hud.draw_image(247, 0, "POWBAR10")
else
    if (player.power_left(player.GOGGLES) > 54)
        hud.draw_image(247, 0, "POWBAR11")
else
    if (player.power_left(player.GOGGLES) > 48)
        hud.draw_image(247, 0, "POWBAR12")
else
    if (player.power_left(player.GOGGLES) > 42)
        hud.draw_image(247, 0, "POWBAR13")
else
    if (player.power_left(player.GOGGLES) > 36)
        hud.draw_image(247, 0, "POWBAR14")
else
    if (player.power_left(player.GOGGLES) > 30)
        hud.draw_image(247, 0, "POWBAR15")
else
    if (player.power_left(player.GOGGLES) > 24)
        hud.draw_image(247, 0, "POWBAR16")
else
    if (player.power_left(player.GOGGLES) > 18)
        hud.draw_image(247, 0, "POWBAR17")
else
    if (player.power_left(player.GOGGLES) > 12)
        hud.draw_image(247, 0, "POWBAR18")
else
    if (player.power_left(player.GOGGLES) > 6)
        hud.draw_image(247, 0, "POWBAR19")
else
    if (player.power_left(player.GOGGLES) > 1)
        hud.draw_image(247, 0, "POWBAR20")

    doomguy_face(0, 166)

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

    doom_little_ammo2()

    hud.set_alpha(0.1)
    if (player.has_power(player.ACID_SUIT))
        hud.draw_image(0, 0, "STSUIT")

    hud.set_alpha(0.7)
    hud.text_font("BIG_DIGIT")

    if (player.cur_weapon() == "PISTOL")
    hud.draw_num2( 266, 179, 3, player.ammo(1))

    if (player.cur_weapon() == "SHOTGUN")
    hud.draw_num2( 266, 179, 3, player.main_ammo(1))

    if (player.cur_weapon() == "SUPERSHOTGUN")
    hud.draw_num2( 266, 179, 3, player.ammo(2))

    if (player.cur_weapon() == "CHAINGUN")
    hud.draw_num2( 266, 179, 3, player.main_ammo(1))

    if (player.cur_weapon() == "ROCKET_LAUNCHER")
    hud.draw_num2( 266, 179, 3, player.main_ammo(1))

    if (player.cur_weapon() == "PLASMA_RIFLE")
    hud.draw_num2( 266, 179, 3, player.main_ammo(1))

    if (player.cur_weapon() == "BFG9000")
    hud.draw_num2( 266, 179, 3, player.main_ammo(1))

   if (player.health() < 35)
        hud.text_color(hud.RED)
    else
    if (player.health() < 65)
        hud.text_color(hud.YELLOW)
    else
    if (player.health() < 200)
        hud.text_color(hud.GREEN)
    else
        hud.text_color(hud.BLUE)

hud.draw_num2(78, 179, 3, player.health()) // 100

    hud.text_color(hud.NO_COLOR)

    if (player.total_armor() > 100)
        hud.text_color(hud.BLUE)
    else
    if (player.total_armor() > 200)
        hud.text_color(hud.YELLOW)
    else
    if (player.total_armor() > 300)
        hud.text_color(hud.RED)
    else
        hud.text_color(hud.GREEN)

    hud.draw_num2(316, 179, 3, player.total_armor())

    hud.text_color(hud.NO_COLOR)

    hud.text_font("DOOM")
    hud.text_color(hud.WHITE)
    hud.draw_text(34, 170, "HEALTH")
    hud.draw_text(274, 170, "ARMOR")

    if (player.cur_weapon() == "PISTOL")
    hud.draw_text(227, 170, "AMMO")

    if (player.cur_weapon() == "SHOTGUN")
    hud.draw_text(227, 170, "AMMO")

    if (player.cur_weapon() == "SUPERSHOTGUN")
    hud.draw_text(227, 170, "AMMO")

    if (player.cur_weapon() == "CHAINGUN")
    hud.draw_text(227, 170, "AMMO")

    if (player.cur_weapon() == "ROCKET_LAUNCHER")
    hud.draw_text(227, 170, "AMMO")

    if (player.cur_weapon() == "PLASMA_RIFLE")
    hud.draw_text(227, 170, "AMMO")

    if (player.cur_weapon() == "BFG9000")
    hud.draw_text(227, 170, "AMMO")

}

function doom_splitscreen_status() = 
    {
	if (player.has_power(player.GOGGLES))
        hud.draw_image(247, 0, "STTPOWR6")
    if (player.has_power(player.NIGHT_VIS))
        hud.draw_image(247, 0, "STTPOWR1")
    if (player.has_power(player.ACID_SUIT))
        hud.draw_image(247, 25, "STTPOWR2")
    if (player.has_power(player.INVIS))
        hud.draw_image(247, 50, "STTPOWR3")
    if (player.has_power(player.BERSERK))
        hud.draw_image(247, 75, "STTPOWR4")
    if (player.has_power(player.INVULN))
        hud.draw_image(247, 100, "STTPOWR5")

    if (player.power_left(player.NIGHT_VIS) > 114)
        hud.draw_image(247, 0, "POWBAR01")
else
    if (player.power_left(player.NIGHT_VIS) > 108)
        hud.draw_image(247, 0, "POWBAR02")
else
    if (player.power_left(player.NIGHT_VIS) > 102)
        hud.draw_image(247, 0, "POWBAR03")
else
    if (player.power_left(player.NIGHT_VIS) > 96)
        hud.draw_image(247, 0, "POWBAR04")
else
    if (player.power_left(player.NIGHT_VIS) > 90)
        hud.draw_image(247, 0, "POWBAR05")
else
    if (player.power_left(player.NIGHT_VIS) > 84)
        hud.draw_image(247, 0, "POWBAR06")
else
    if (player.power_left(player.NIGHT_VIS) > 78)
        hud.draw_image(247, 0, "POWBAR07")
else
    if (player.power_left(player.NIGHT_VIS) > 72)
        hud.draw_image(247, 0, "POWBAR08")
else
    if (player.power_left(player.NIGHT_VIS) > 66)
        hud.draw_image(247, 0, "POWBAR09")
else
    if (player.power_left(player.NIGHT_VIS) > 60)
        hud.draw_image(247, 0, "POWBAR10")
else
    if (player.power_left(player.NIGHT_VIS) > 54)
        hud.draw_image(247, 0, "POWBAR11")
else
    if (player.power_left(player.NIGHT_VIS) > 48)
        hud.draw_image(247, 0, "POWBAR12")
else
    if (player.power_left(player.NIGHT_VIS) > 42)
        hud.draw_image(247, 0, "POWBAR13")
else
    if (player.power_left(player.NIGHT_VIS) > 36)
        hud.draw_image(247, 0, "POWBAR14")
else
    if (player.power_left(player.NIGHT_VIS) > 30)
        hud.draw_image(247, 0, "POWBAR15")
else
    if (player.power_left(player.NIGHT_VIS) > 24)
        hud.draw_image(247, 0, "POWBAR16")
else
    if (player.power_left(player.NIGHT_VIS) > 18)
        hud.draw_image(247, 0, "POWBAR17")
else
    if (player.power_left(player.NIGHT_VIS) > 12)
        hud.draw_image(247, 0, "POWBAR18")
else
    if (player.power_left(player.NIGHT_VIS) > 6)
        hud.draw_image(247, 0, "POWBAR19")
else
    if (player.power_left(player.NIGHT_VIS) > 1)
        hud.draw_image(247, 0, "POWBAR20")

    if (player.power_left(player.ACID_SUIT) > 57)
        hud.draw_image(247, 25, "POWBAR01")
else
    if (player.power_left(player.ACID_SUIT) > 54)
        hud.draw_image(247, 25, "POWBAR02")
else
    if (player.power_left(player.ACID_SUIT) > 51)
        hud.draw_image(247, 25, "POWBAR03")
else
    if (player.power_left(player.ACID_SUIT) > 48)
        hud.draw_image(247, 25, "POWBAR04")
else
    if (player.power_left(player.ACID_SUIT) > 45)
        hud.draw_image(247, 25, "POWBAR05")
else
    if (player.power_left(player.ACID_SUIT) > 42)
        hud.draw_image(247, 25, "POWBAR06")
else
    if (player.power_left(player.ACID_SUIT) > 39)
        hud.draw_image(247, 25, "POWBAR07")
else
    if (player.power_left(player.ACID_SUIT) > 36)
        hud.draw_image(247, 25, "POWBAR08")
else
    if (player.power_left(player.ACID_SUIT) > 33)
        hud.draw_image(247, 25, "POWBAR09")
else
    if (player.power_left(player.ACID_SUIT) > 30)
        hud.draw_image(247, 25, "POWBAR10")
else
    if (player.power_left(player.ACID_SUIT) > 27)
        hud.draw_image(247, 25, "POWBAR11")
else
    if (player.power_left(player.ACID_SUIT) > 24)
        hud.draw_image(247, 25, "POWBAR12")
else
    if (player.power_left(player.ACID_SUIT) > 21)
        hud.draw_image(247, 25, "POWBAR13")
else
    if (player.power_left(player.ACID_SUIT) > 18)
        hud.draw_image(247, 25, "POWBAR14")
else
    if (player.power_left(player.ACID_SUIT) > 15)
        hud.draw_image(247, 25, "POWBAR15")
else
    if (player.power_left(player.ACID_SUIT) > 12)
        hud.draw_image(247, 25, "POWBAR16")
else
    if (player.power_left(player.ACID_SUIT) > 9)
        hud.draw_image(247, 25, "POWBAR17")
else
    if (player.power_left(player.ACID_SUIT) > 6)
        hud.draw_image(247, 25, "POWBAR18")
else
    if (player.power_left(player.ACID_SUIT) > 3)
        hud.draw_image(247, 25, "POWBAR19")
else
    if (player.power_left(player.ACID_SUIT) > 1)
        hud.draw_image(247, 25, "POWBAR20")

    if (player.power_left(player.INVIS) > 95)
        hud.draw_image(247, 50, "POWBAR01")
else
    if (player.power_left(player.INVIS) > 90)
        hud.draw_image(247, 50, "POWBAR02")
else
    if (player.power_left(player.INVIS) > 85)
        hud.draw_image(247, 50, "POWBAR03")
else
    if (player.power_left(player.INVIS) > 80)
        hud.draw_image(247, 50, "POWBAR04")
else
    if (player.power_left(player.INVIS) > 75)
        hud.draw_image(247, 50, "POWBAR05")
else
    if (player.power_left(player.INVIS) > 70)
        hud.draw_image(247, 50, "POWBAR06")
else
    if (player.power_left(player.INVIS) > 65)
        hud.draw_image(247, 50, "POWBAR07")
else
    if (player.power_left(player.INVIS) > 60)
        hud.draw_image(247, 50, "POWBAR08")
else
    if (player.power_left(player.INVIS) > 55)
        hud.draw_image(247, 50, "POWBAR09")
else
    if (player.power_left(player.INVIS) > 50)
        hud.draw_image(247, 50, "POWBAR10")
else
    if (player.power_left(player.INVIS) > 45)
        hud.draw_image(247, 50, "POWBAR11")
else
    if (player.power_left(player.INVIS) > 40)
        hud.draw_image(247, 50, "POWBAR12")
else
    if (player.power_left(player.INVIS) > 35)
        hud.draw_image(247, 50, "POWBAR13")
else
    if (player.power_left(player.INVIS) > 30)
        hud.draw_image(247, 50, "POWBAR14")
else
    if (player.power_left(player.INVIS) > 25)
        hud.draw_image(247, 50, "POWBAR15")
else
    if (player.power_left(player.INVIS) > 20)
        hud.draw_image(247, 50, "POWBAR16")
else
    if (player.power_left(player.INVIS) > 15)
        hud.draw_image(247, 50, "POWBAR17")
else
    if (player.power_left(player.INVIS) > 10)
        hud.draw_image(247, 50, "POWBAR18")
else
    if (player.power_left(player.INVIS) > 5)
        hud.draw_image(247, 50, "POWBAR19")
else
    if (player.power_left(player.INVIS) > 1)
        hud.draw_image(247, 50, "POWBAR20")

    if (player.power_left(player.BERSERK) > 57)
        hud.draw_image(247, 75, "POWBAR01")
else
    if (player.power_left(player.BERSERK) > 54)
        hud.draw_image(247, 75, "POWBAR02")
else
    if (player.power_left(player.BERSERK) > 51)
        hud.draw_image(247, 75, "POWBAR03")
else
    if (player.power_left(player.BERSERK) > 48)
        hud.draw_image(247, 75, "POWBAR04")
else
    if (player.power_left(player.BERSERK) > 45)
        hud.draw_image(247, 75, "POWBAR05")
else
    if (player.power_left(player.BERSERK) > 42)
        hud.draw_image(247, 75, "POWBAR06")
else
    if (player.power_left(player.BERSERK) > 39)
        hud.draw_image(247, 75, "POWBAR07")
else
    if (player.power_left(player.BERSERK) > 36)
        hud.draw_image(247, 75, "POWBAR08")
else
    if (player.power_left(player.BERSERK) > 33)
        hud.draw_image(247, 75, "POWBAR09")
else
    if (player.power_left(player.BERSERK) > 30)
        hud.draw_image(247, 75, "POWBAR10")
else
    if (player.power_left(player.BERSERK) > 27)
        hud.draw_image(247, 75, "POWBAR11")
else
    if (player.power_left(player.BERSERK) > 24)
        hud.draw_image(247, 75, "POWBAR12")
else
    if (player.power_left(player.BERSERK) > 21)
        hud.draw_image(247, 75, "POWBAR13")
else
    if (player.power_left(player.BERSERK) > 18)
        hud.draw_image(247, 75, "POWBAR14")
else
    if (player.power_left(player.BERSERK) > 15)
        hud.draw_image(247, 75, "POWBAR15")
else
    if (player.power_left(player.BERSERK) > 12)
        hud.draw_image(247, 75, "POWBAR16")
else
    if (player.power_left(player.BERSERK) > 9)
        hud.draw_image(247, 75, "POWBAR17")
else
    if (player.power_left(player.BERSERK) > 6)
        hud.draw_image(247, 75, "POWBAR18")
else
    if (player.power_left(player.BERSERK) > 3)
        hud.draw_image(247, 75, "POWBAR19")
else
    if (player.power_left(player.BERSERK) > 1)
        hud.draw_image(247, 75, "POWBAR20")

    if (player.power_left(player.INVULN) > 42)
        hud.draw_image(247, 100, "POWBAR01")
else
    if (player.power_left(player.INVULN) > 40)
        hud.draw_image(247, 100, "POWBAR02")
else
    if (player.power_left(player.INVULN) > 38)
        hud.draw_image(247, 100, "POWBAR03")
else
    if (player.power_left(player.INVULN) > 36)
        hud.draw_image(247, 100, "POWBAR04")
else
    if (player.power_left(player.INVULN) > 34)
        hud.draw_image(247, 100, "POWBAR05")
else
    if (player.power_left(player.INVULN) > 32)
        hud.draw_image(247, 100, "POWBAR06")
else
    if (player.power_left(player.INVULN) > 30)
        hud.draw_image(247, 100, "POWBAR07")
else
    if (player.power_left(player.INVULN) > 28)
        hud.draw_image(247, 100, "POWBAR08")
else
    if (player.power_left(player.INVULN) > 26)
        hud.draw_image(247, 100, "POWBAR09")
else
    if (player.power_left(player.INVULN) > 24)
        hud.draw_image(247, 100, "POWBAR10")
else
    if (player.power_left(player.INVULN) > 22)
        hud.draw_image(247, 100, "POWBAR11")
else
    if (player.power_left(player.INVULN) > 20)
        hud.draw_image(247, 100, "POWBAR12")
else
    if (player.power_left(player.INVULN) > 18)
        hud.draw_image(247, 100, "POWBAR13")
else
    if (player.power_left(player.INVULN) > 16)
        hud.draw_image(247, 100, "POWBAR14")
else
    if (player.power_left(player.INVULN) > 14)
        hud.draw_image(247, 100, "POWBAR15")
else
    if (player.power_left(player.INVULN) > 12)
        hud.draw_image(247, 100, "POWBAR16")
else
    if (player.power_left(player.INVULN) > 10)
        hud.draw_image(247, 100, "POWBAR17")
else
    if (player.power_left(player.INVULN) > 8)
        hud.draw_image(247, 100, "POWBAR18")
else
    if (player.power_left(player.INVULN) > 6)
        hud.draw_image(247, 100, "POWBAR19")
else
    if (player.power_left(player.INVULN) > 4)
        hud.draw_image(247, 100, "POWBAR19")
else
    if (player.power_left(player.INVULN) > 2)
        hud.draw_image(247, 100, "POWBAR20")
else
    if (player.power_left(player.INVULN) > 1)
        hud.draw_image(247, 100, "POWBAR20")
// DOOM NV Goggles time bars

    if (player.power_left(player.GOGGLES) > 114)
        hud.draw_image(247, 0, "POWBAR01")
else
    if (player.power_left(player.GOGGLES) > 108)
        hud.draw_image(247, 0, "POWBAR02")
else
    if (player.power_left(player.GOGGLES) > 102)
        hud.draw_image(247, 0, "POWBAR03")
else
    if (player.power_left(player.GOGGLES) > 96)
        hud.draw_image(247, 0, "POWBAR04")
else
    if (player.power_left(player.GOGGLES) > 90)
        hud.draw_image(247, 0, "POWBAR05")
else
    if (player.power_left(player.GOGGLES) > 84)
        hud.draw_image(247, 0, "POWBAR06")
else
    if (player.power_left(player.GOGGLES) > 78)
        hud.draw_image(247, 0, "POWBAR07")
else
    if (player.power_left(player.GOGGLES) > 72)
        hud.draw_image(247, 0, "POWBAR08")
else
    if (player.power_left(player.GOGGLES) > 66)
        hud.draw_image(247, 0, "POWBAR09")
else
    if (player.power_left(player.GOGGLES) > 60)
        hud.draw_image(247, 0, "POWBAR10")
else
    if (player.power_left(player.GOGGLES) > 54)
        hud.draw_image(247, 0, "POWBAR11")
else
    if (player.power_left(player.GOGGLES) > 48)
        hud.draw_image(247, 0, "POWBAR12")
else
    if (player.power_left(player.GOGGLES) > 42)
        hud.draw_image(247, 0, "POWBAR13")
else
    if (player.power_left(player.GOGGLES) > 36)
        hud.draw_image(247, 0, "POWBAR14")
else
    if (player.power_left(player.GOGGLES) > 30)
        hud.draw_image(247, 0, "POWBAR15")
else
    if (player.power_left(player.GOGGLES) > 24)
        hud.draw_image(247, 0, "POWBAR16")
else
    if (player.power_left(player.GOGGLES) > 18)
        hud.draw_image(247, 0, "POWBAR17")
else
    if (player.power_left(player.GOGGLES) > 12)
        hud.draw_image(247, 0, "POWBAR18")
else
    if (player.power_left(player.GOGGLES) > 6)
        hud.draw_image(247, 0, "POWBAR19")
else
    if (player.power_left(player.GOGGLES) > 1)
        hud.draw_image(247, 0, "POWBAR20")

    // doomguy_face(0, 166)
	hud.draw_image(0, 166, "MEDIA0")

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

    doom_little_ammo2()

    hud.set_alpha(0.1)
    if (player.has_power(player.ACID_SUIT))
        hud.draw_image(0, 0, "STSUIT")

    hud.set_alpha(0.7)
    hud.text_font("BIG_DIGIT")

    if (player.cur_weapon() == "PISTOL")
    hud.draw_num2( 266, 179, 3, player.ammo(1))

    if (player.cur_weapon() == "SHOTGUN")
    hud.draw_num2( 266, 179, 3, player.main_ammo(1))

    if (player.cur_weapon() == "SUPERSHOTGUN")
    hud.draw_num2( 266, 179, 3, player.ammo(2))

    if (player.cur_weapon() == "CHAINGUN")
    hud.draw_num2( 266, 179, 3, player.main_ammo(1))

    if (player.cur_weapon() == "ROCKET_LAUNCHER")
    hud.draw_num2( 266, 179, 3, player.main_ammo(1))

    if (player.cur_weapon() == "PLASMA_RIFLE")
    hud.draw_num2( 266, 179, 3, player.main_ammo(1))

    if (player.cur_weapon() == "BFG9000")
    hud.draw_num2( 266, 179, 3, player.main_ammo(1))

   if (player.health() < 35)
        hud.text_color(hud.RED)
    else
    if (player.health() < 65)
        hud.text_color(hud.YELLOW)
    else
    if (player.health() < 200)
        hud.text_color(hud.GREEN)
    else
        hud.text_color(hud.BLUE)

hud.draw_num2(78, 179, 3, player.health()) // 100

    hud.text_color(hud.NO_COLOR)

    if (player.total_armor() > 100)
        hud.text_color(hud.BLUE)
    else
    if (player.total_armor() > 200)
        hud.text_color(hud.YELLOW)
    else
    if (player.total_armor() > 300)
        hud.text_color(hud.RED)
    else
        hud.text_color(hud.GREEN)

    hud.draw_num2(316, 179, 3, player.total_armor())

    hud.text_color(hud.NO_COLOR)

    hud.text_font("DOOM")
    hud.text_color(hud.WHITE)
    hud.draw_text(34, 170, "HEALTH")
    hud.draw_text(274, 170, "ARMOR")

    if (player.cur_weapon() == "PISTOL")
    hud.draw_text(227, 170, "AMMO")

    if (player.cur_weapon() == "SHOTGUN")
    hud.draw_text(227, 170, "AMMO")

    if (player.cur_weapon() == "SUPERSHOTGUN")
    hud.draw_text(227, 170, "AMMO")

    if (player.cur_weapon() == "CHAINGUN")
    hud.draw_text(227, 170, "AMMO")

    if (player.cur_weapon() == "ROCKET_LAUNCHER")
    hud.draw_text(227, 170, "AMMO")

    if (player.cur_weapon() == "PLASMA_RIFLE")
    hud.draw_text(227, 170, "AMMO")

    if (player.cur_weapon() == "BFG9000")
    hud.draw_text(227, 170, "AMMO")

}


function zdoom_overlay_status() = 
 {
	if (player.has_power(player.GOGGLES))
        hud.draw_image(247, 0, "STTPOWR6")
    if (player.has_power(player.NIGHT_VIS))
        hud.draw_image(247, 0, "STTPOWR1")
    if (player.has_power(player.ACID_SUIT))
        hud.draw_image(247, 25, "STTPOWR2")
    if (player.has_power(player.INVIS))
        hud.draw_image(247, 50, "STTPOWR3")
    if (player.has_power(player.BERSERK))
        hud.draw_image(247, 75, "STTPOWR4")
    if (player.has_power(player.INVULN))
        hud.draw_image(247, 100, "STTPOWR5")

    if (player.power_left(player.NIGHT_VIS) > 114)
        hud.draw_image(247, 0, "POWBAR01")
else
    if (player.power_left(player.NIGHT_VIS) > 108)
        hud.draw_image(247, 0, "POWBAR02")
else
    if (player.power_left(player.NIGHT_VIS) > 102)
        hud.draw_image(247, 0, "POWBAR03")
else
    if (player.power_left(player.NIGHT_VIS) > 96)
        hud.draw_image(247, 0, "POWBAR04")
else
    if (player.power_left(player.NIGHT_VIS) > 90)
        hud.draw_image(247, 0, "POWBAR05")
else
    if (player.power_left(player.NIGHT_VIS) > 84)
        hud.draw_image(247, 0, "POWBAR06")
else
    if (player.power_left(player.NIGHT_VIS) > 78)
        hud.draw_image(247, 0, "POWBAR07")
else
    if (player.power_left(player.NIGHT_VIS) > 72)
        hud.draw_image(247, 0, "POWBAR08")
else
    if (player.power_left(player.NIGHT_VIS) > 66)
        hud.draw_image(247, 0, "POWBAR09")
else
    if (player.power_left(player.NIGHT_VIS) > 60)
        hud.draw_image(247, 0, "POWBAR10")
else
    if (player.power_left(player.NIGHT_VIS) > 54)
        hud.draw_image(247, 0, "POWBAR11")
else
    if (player.power_left(player.NIGHT_VIS) > 48)
        hud.draw_image(247, 0, "POWBAR12")
else
    if (player.power_left(player.NIGHT_VIS) > 42)
        hud.draw_image(247, 0, "POWBAR13")
else
    if (player.power_left(player.NIGHT_VIS) > 36)
        hud.draw_image(247, 0, "POWBAR14")
else
    if (player.power_left(player.NIGHT_VIS) > 30)
        hud.draw_image(247, 0, "POWBAR15")
else
    if (player.power_left(player.NIGHT_VIS) > 24)
        hud.draw_image(247, 0, "POWBAR16")
else
    if (player.power_left(player.NIGHT_VIS) > 18)
        hud.draw_image(247, 0, "POWBAR17")
else
    if (player.power_left(player.NIGHT_VIS) > 12)
        hud.draw_image(247, 0, "POWBAR18")
else
    if (player.power_left(player.NIGHT_VIS) > 6)
        hud.draw_image(247, 0, "POWBAR19")
else
    if (player.power_left(player.NIGHT_VIS) > 1)
        hud.draw_image(247, 0, "POWBAR20")

    if (player.power_left(player.ACID_SUIT) > 57)
        hud.draw_image(247, 25, "POWBAR01")
else
    if (player.power_left(player.ACID_SUIT) > 54)
        hud.draw_image(247, 25, "POWBAR02")
else
    if (player.power_left(player.ACID_SUIT) > 51)
        hud.draw_image(247, 25, "POWBAR03")
else
    if (player.power_left(player.ACID_SUIT) > 48)
        hud.draw_image(247, 25, "POWBAR04")
else
    if (player.power_left(player.ACID_SUIT) > 45)
        hud.draw_image(247, 25, "POWBAR05")
else
    if (player.power_left(player.ACID_SUIT) > 42)
        hud.draw_image(247, 25, "POWBAR06")
else
    if (player.power_left(player.ACID_SUIT) > 39)
        hud.draw_image(247, 25, "POWBAR07")
else
    if (player.power_left(player.ACID_SUIT) > 36)
        hud.draw_image(247, 25, "POWBAR08")
else
    if (player.power_left(player.ACID_SUIT) > 33)
        hud.draw_image(247, 25, "POWBAR09")
else
    if (player.power_left(player.ACID_SUIT) > 30)
        hud.draw_image(247, 25, "POWBAR10")
else
    if (player.power_left(player.ACID_SUIT) > 27)
        hud.draw_image(247, 25, "POWBAR11")
else
    if (player.power_left(player.ACID_SUIT) > 24)
        hud.draw_image(247, 25, "POWBAR12")
else
    if (player.power_left(player.ACID_SUIT) > 21)
        hud.draw_image(247, 25, "POWBAR13")
else
    if (player.power_left(player.ACID_SUIT) > 18)
        hud.draw_image(247, 25, "POWBAR14")
else
    if (player.power_left(player.ACID_SUIT) > 15)
        hud.draw_image(247, 25, "POWBAR15")
else
    if (player.power_left(player.ACID_SUIT) > 12)
        hud.draw_image(247, 25, "POWBAR16")
else
    if (player.power_left(player.ACID_SUIT) > 9)
        hud.draw_image(247, 25, "POWBAR17")
else
    if (player.power_left(player.ACID_SUIT) > 6)
        hud.draw_image(247, 25, "POWBAR18")
else
    if (player.power_left(player.ACID_SUIT) > 3)
        hud.draw_image(247, 25, "POWBAR19")
else
    if (player.power_left(player.ACID_SUIT) > 1)
        hud.draw_image(247, 25, "POWBAR20")

    if (player.power_left(player.INVIS) > 95)
        hud.draw_image(247, 50, "POWBAR01")
else
    if (player.power_left(player.INVIS) > 90)
        hud.draw_image(247, 50, "POWBAR02")
else
    if (player.power_left(player.INVIS) > 85)
        hud.draw_image(247, 50, "POWBAR03")
else
    if (player.power_left(player.INVIS) > 80)
        hud.draw_image(247, 50, "POWBAR04")
else
    if (player.power_left(player.INVIS) > 75)
        hud.draw_image(247, 50, "POWBAR05")
else
    if (player.power_left(player.INVIS) > 70)
        hud.draw_image(247, 50, "POWBAR06")
else
    if (player.power_left(player.INVIS) > 65)
        hud.draw_image(247, 50, "POWBAR07")
else
    if (player.power_left(player.INVIS) > 60)
        hud.draw_image(247, 50, "POWBAR08")
else
    if (player.power_left(player.INVIS) > 55)
        hud.draw_image(247, 50, "POWBAR09")
else
    if (player.power_left(player.INVIS) > 50)
        hud.draw_image(247, 50, "POWBAR10")
else
    if (player.power_left(player.INVIS) > 45)
        hud.draw_image(247, 50, "POWBAR11")
else
    if (player.power_left(player.INVIS) > 40)
        hud.draw_image(247, 50, "POWBAR12")
else
    if (player.power_left(player.INVIS) > 35)
        hud.draw_image(247, 50, "POWBAR13")
else
    if (player.power_left(player.INVIS) > 30)
        hud.draw_image(247, 50, "POWBAR14")
else
    if (player.power_left(player.INVIS) > 25)
        hud.draw_image(247, 50, "POWBAR15")
else
    if (player.power_left(player.INVIS) > 20)
        hud.draw_image(247, 50, "POWBAR16")
else
    if (player.power_left(player.INVIS) > 15)
        hud.draw_image(247, 50, "POWBAR17")
else
    if (player.power_left(player.INVIS) > 10)
        hud.draw_image(247, 50, "POWBAR18")
else
    if (player.power_left(player.INVIS) > 5)
        hud.draw_image(247, 50, "POWBAR19")
else
    if (player.power_left(player.INVIS) > 1)
        hud.draw_image(247, 50, "POWBAR20")

    if (player.power_left(player.BERSERK) > 57)
        hud.draw_image(247, 75, "POWBAR01")
else
    if (player.power_left(player.BERSERK) > 54)
        hud.draw_image(247, 75, "POWBAR02")
else
    if (player.power_left(player.BERSERK) > 51)
        hud.draw_image(247, 75, "POWBAR03")
else
    if (player.power_left(player.BERSERK) > 48)
        hud.draw_image(247, 75, "POWBAR04")
else
    if (player.power_left(player.BERSERK) > 45)
        hud.draw_image(247, 75, "POWBAR05")
else
    if (player.power_left(player.BERSERK) > 42)
        hud.draw_image(247, 75, "POWBAR06")
else
    if (player.power_left(player.BERSERK) > 39)
        hud.draw_image(247, 75, "POWBAR07")
else
    if (player.power_left(player.BERSERK) > 36)
        hud.draw_image(247, 75, "POWBAR08")
else
    if (player.power_left(player.BERSERK) > 33)
        hud.draw_image(247, 75, "POWBAR09")
else
    if (player.power_left(player.BERSERK) > 30)
        hud.draw_image(247, 75, "POWBAR10")
else
    if (player.power_left(player.BERSERK) > 27)
        hud.draw_image(247, 75, "POWBAR11")
else
    if (player.power_left(player.BERSERK) > 24)
        hud.draw_image(247, 75, "POWBAR12")
else
    if (player.power_left(player.BERSERK) > 21)
        hud.draw_image(247, 75, "POWBAR13")
else
    if (player.power_left(player.BERSERK) > 18)
        hud.draw_image(247, 75, "POWBAR14")
else
    if (player.power_left(player.BERSERK) > 15)
        hud.draw_image(247, 75, "POWBAR15")
else
    if (player.power_left(player.BERSERK) > 12)
        hud.draw_image(247, 75, "POWBAR16")
else
    if (player.power_left(player.BERSERK) > 9)
        hud.draw_image(247, 75, "POWBAR17")
else
    if (player.power_left(player.BERSERK) > 6)
        hud.draw_image(247, 75, "POWBAR18")
else
    if (player.power_left(player.BERSERK) > 3)
        hud.draw_image(247, 75, "POWBAR19")
else
    if (player.power_left(player.BERSERK) > 1)
        hud.draw_image(247, 75, "POWBAR20")

    if (player.power_left(player.INVULN) > 42)
        hud.draw_image(247, 100, "POWBAR01")
else
    if (player.power_left(player.INVULN) > 40)
        hud.draw_image(247, 100, "POWBAR02")
else
    if (player.power_left(player.INVULN) > 38)
        hud.draw_image(247, 100, "POWBAR03")
else
    if (player.power_left(player.INVULN) > 36)
        hud.draw_image(247, 100, "POWBAR04")
else
    if (player.power_left(player.INVULN) > 34)
        hud.draw_image(247, 100, "POWBAR05")
else
    if (player.power_left(player.INVULN) > 32)
        hud.draw_image(247, 100, "POWBAR06")
else
    if (player.power_left(player.INVULN) > 30)
        hud.draw_image(247, 100, "POWBAR07")
else
    if (player.power_left(player.INVULN) > 28)
        hud.draw_image(247, 100, "POWBAR08")
else
    if (player.power_left(player.INVULN) > 26)
        hud.draw_image(247, 100, "POWBAR09")
else
    if (player.power_left(player.INVULN) > 24)
        hud.draw_image(247, 100, "POWBAR10")
else
    if (player.power_left(player.INVULN) > 22)
        hud.draw_image(247, 100, "POWBAR11")
else
    if (player.power_left(player.INVULN) > 20)
        hud.draw_image(247, 100, "POWBAR12")
else
    if (player.power_left(player.INVULN) > 18)
        hud.draw_image(247, 100, "POWBAR13")
else
    if (player.power_left(player.INVULN) > 16)
        hud.draw_image(247, 100, "POWBAR14")
else
    if (player.power_left(player.INVULN) > 14)
        hud.draw_image(247, 100, "POWBAR15")
else
    if (player.power_left(player.INVULN) > 12)
        hud.draw_image(247, 100, "POWBAR16")
else
    if (player.power_left(player.INVULN) > 10)
        hud.draw_image(247, 100, "POWBAR17")
else
    if (player.power_left(player.INVULN) > 8)
        hud.draw_image(247, 100, "POWBAR18")
else
    if (player.power_left(player.INVULN) > 6)
        hud.draw_image(247, 100, "POWBAR19")
else
    if (player.power_left(player.INVULN) > 4)
        hud.draw_image(247, 100, "POWBAR19")
else
    if (player.power_left(player.INVULN) > 2)
        hud.draw_image(247, 100, "POWBAR20")
else
    if (player.power_left(player.INVULN) > 1)
        hud.draw_image(247, 100, "POWBAR20")
// DOOM NV Goggles time bars

    if (player.power_left(player.GOGGLES) > 114)
        hud.draw_image(247, 0, "POWBAR01")
else
    if (player.power_left(player.GOGGLES) > 108)
        hud.draw_image(247, 0, "POWBAR02")
else
    if (player.power_left(player.GOGGLES) > 102)
        hud.draw_image(247, 0, "POWBAR03")
else
    if (player.power_left(player.GOGGLES) > 96)
        hud.draw_image(247, 0, "POWBAR04")
else
    if (player.power_left(player.GOGGLES) > 90)
        hud.draw_image(247, 0, "POWBAR05")
else
    if (player.power_left(player.GOGGLES) > 84)
        hud.draw_image(247, 0, "POWBAR06")
else
    if (player.power_left(player.GOGGLES) > 78)
        hud.draw_image(247, 0, "POWBAR07")
else
    if (player.power_left(player.GOGGLES) > 72)
        hud.draw_image(247, 0, "POWBAR08")
else
    if (player.power_left(player.GOGGLES) > 66)
        hud.draw_image(247, 0, "POWBAR09")
else
    if (player.power_left(player.GOGGLES) > 60)
        hud.draw_image(247, 0, "POWBAR10")
else
    if (player.power_left(player.GOGGLES) > 54)
        hud.draw_image(247, 0, "POWBAR11")
else
    if (player.power_left(player.GOGGLES) > 48)
        hud.draw_image(247, 0, "POWBAR12")
else
    if (player.power_left(player.GOGGLES) > 42)
        hud.draw_image(247, 0, "POWBAR13")
else
    if (player.power_left(player.GOGGLES) > 36)
        hud.draw_image(247, 0, "POWBAR14")
else
    if (player.power_left(player.GOGGLES) > 30)
        hud.draw_image(247, 0, "POWBAR15")
else
    if (player.power_left(player.GOGGLES) > 24)
        hud.draw_image(247, 0, "POWBAR16")
else
    if (player.power_left(player.GOGGLES) > 18)
        hud.draw_image(247, 0, "POWBAR17")
else
    if (player.power_left(player.GOGGLES) > 12)
        hud.draw_image(247, 0, "POWBAR18")
else
    if (player.power_left(player.GOGGLES) > 6)
        hud.draw_image(247, 0, "POWBAR19")
else
    if (player.power_left(player.GOGGLES) > 1)
        hud.draw_image(247, 0, "POWBAR20")


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



    hud.set_alpha(0.1)
    if (player.has_power(player.ACID_SUIT))
        hud.draw_image(0, 0, "STSUIT")

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

    hud.render_automap(0, 0, 320, 200)

    hud.text_font("DOOM")
    hud.draw_text(0, 200 - 32 - 10, hud.map_title())
    hud.draw_text(0, 200 - 32 - 10, hud.map_title())

        doom_overlay_status()
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
	//cam.set_player_camera()
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
    var which = hud.which_hud() % 4

    if (which == 0)
        //hud.render_world(0, 0, 320, 200 - 32)
		hud.render_world(0, 0, 320, 200)// - 16)
    else
        hud.render_world(0, 0, 320, 200)

    if (which == 0)
        doom_status_bar()
    else if (which == 2)
        doom_overlay_status()
	else if (which == 3)
		zdoom_overlay_status()

    edge_air_bar()
	
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
