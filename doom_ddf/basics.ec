//------------------------------------------
//  BASIC COAL DEFINITIONS for EDGE
//  Copyright (c) 2009 The Edge Team
//  Under the GNU General Public License
//------------------------------------------

// SYSTEM

module sys
{
    function print(s : string) = native
    function assert(n : float) = native
}


// MATH

module math
{
    constant pi = 3.1415926535897932384

    function floor(n : float) : float = native
    function ceil (n : float) : float = native
    function random() : float = native
}


// STRINGS

module strings
{
}


// HUD LIBRARY

module hud
{
    var which;
    var passed_time;
    var automap;

    // handy colors
    constant NO_COLOR = '-1 -1 -1'

    constant BLACK  = '0 0 0'
    constant WHITE  = '255 255 255'
    constant RED    = '255 0 0'
    constant GREEN  = '0 255 0'
    constant BLUE   = '0 0 255'
    constant YELLOW = '255 255 0'
    constant PURPLE = '255 0 255'
    constant CYAN   = '0 255 255'
    constant ORANGE = '255 160 0'

    function map_title() : string = native
    function game_mode() : string = native

    function coord_sys(w, h) = native

    function text_color(color : vector) = native
    function text_font(font : string) = native

    function draw_num2(x, y, w, num) = native
    function draw_image(x, y, image : string) = native
    function draw_text(x, y, text : string) = native

    function render_world(x, y, w, h) = native
    function render_automap(x, y, w, h) = native
}


// PLAYER LIBRARY

module player
{
    function health() : float = native

    function has_weapon_slot(slot : float) : float = native
    function has_key(key : float) : float = native

    function main_ammo(clip : float) : float = native
    function ammo(type : float) : float = native
    function ammomax(type : float) : float = native
    function total_armor() : float = native

    function hurt_by() : string = native
    function hurt_pain() : float = native
    function hurt_dir() : float = native

    function frags() : float = native
    function under_water() : float = native
    function air_in_lungs() : float = native

    function is_grinning() : float = native
    function is_rampaging() : float = native
    function has_power(type : string) : float = native
}

