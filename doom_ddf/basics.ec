//------------------------------------------
//  BASIC COAL DEFINITIONS for EDGE
//  Copyright (c) 2009 The Edge Team
//  Under the GNU General Public License
//------------------------------------------

// SYSTEM

module sys
{
    constant TICRATE = 35

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

    function getx(v : vector) : float = { return v * '1 0 0'; }
    function gety(v : vector) : float = { return v * '0 1 0'; }
    function getz(v : vector) : float = { return v * '0 0 1'; }

    function min(a, b) : float =
    {
      if (a < b)
        return a
      else
        return b
    }

    function max(a, b) : float =
    {
      if (a > b)
        return a
      else
        return b
    }
}


// STRINGS

module strings
{
}


// HUD LIBRARY

module hud
{
    var which
    var automap

    var now_time
    var passed_time
    var last_time = -1

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

    function game_mode() : string = native
    function game_name() : string = native
    function map_name()  : string = native
    function map_title() : string = native

    function which_hud() : float = native
    function check_automap() : float = native
    function get_time()  : float = native

    function coord_sys(w, h) = native

    function text_font(font : string) = native
    function text_color(color : vector) = native
    function set_scale(scale : float) = native
    function set_alpha(alpha : float) = native

    function draw_image(x, y, image : string) = native
    function stretch_image(x, y, w, h, image : string) = native
    function draw_text(x, y, text : string) = native
    function draw_num2(x, y, w, num) = native

    function render_world(x, y, w, h) = native
    function render_automap(x, y, w, h) = native

    function grab_times() =
    {
      now_time = get_time();
      passed_time = 0;

      if (last_time > 0 && last_time <= now_time)
      {
        passed_time = math.min(now_time - last_time, sys.TICRATE);
      }

      last_time = now_time;
    }
}


// PLAYER LIBRARY

module player
{
	  function num_players() = native
	  function set_who(index) = native
    function is_bot() = native
    function get_name() = native

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

    function is_swimming() : float = native
    function is_jumping() : float = native
    function is_crouching() : float = native
    function is_using() : float = native
    function is_action1() : float = native
    function is_action2() : float = native
    function is_attacking() : float = native
    function is_rampaging() : float = native
    function is_grinning() : float = native

    function has_power(type : string) : float = native
}

