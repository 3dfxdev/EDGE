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

    function vlen(v : vector) : float =
    {
      return (v * v) ^ 0.5
    }

    function min(a, b) : float =
    {
      if (a < b) return a
      return b
    }

    function max(a, b) : float =
    {
      if (a > b) return a
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

    // automap options
    constant AM_GRID     = 0   // also a color
    constant AM_ALLMAP   = 1   // also a color
    constant AM_WALLS    = 2   // also a color
    constant AM_THINGS   = 3
    constant AM_FOLLOW   = 4
    constant AM_ROTATE   = 5

    // automap colors
    constant AM_STEP     = 3
    constant AM_LEDGE    = 4
    constant AM_CEIL     = 5
    constant AM_SECRET   = 6

    constant AM_PLAYER   = 7
    constant AM_MONSTER  = 8
    constant AM_CORPSE   = 9
    constant AM_ITEM     = 10
    constant AM_MISSILE  = 11
    constant AM_SCENERY  = 12

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

    function solid_box(x, y, w, h, color : vector) = native
    function solid_line(x1, y1, x2, y2, color : vector) = native
    function thin_box(x, y, w, h, color : vector) = native

    function draw_image(x, y, image : string) = native
    function stretch_image(x, y, w, h, image : string) = native
    function tile_image(x, y, w, h, image : string, offset_x, offset_y) = native
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
    // ammo
    constant BULLETS  = 1
    constant SHELLS   = 2
    constant ROCKETS  = 3
    constant CELLS    = 4
    constant PELLETS  = 5
    constant NAILS    = 6
    constant GRENADES = 7
    constant GAS      = 8

    // armors
    constant GREEN_ARMOR  = 1
    constant BLUE_ARMOR   = 2
    constant PURPLE_ARMOR = 3
    constant YELLOW_ARMOR = 4
    constant RED_ARMOR    = 5

    // powerups
    constant INVULN    = 1
    constant BERSERK   = 2
    constant INVIS     = 3
    constant ACID_SUIT = 4
    constant AUTOMAP   = 5
    constant GOGGLES   = 6
    constant JET_PACK  = 7
    constant NIGHT_VIS = 8
    constant SCUBA     = 9

    // keys
    constant BLUE_CARD    = 1
    constant YELLOW_CARD  = 2
    constant RED_CARD     = 3
    constant GREEN_CARD   = 4
    constant BLUE_SKULL   = 5
    constant YELLOW_SKULL = 6
    constant RED_SKULL    = 7
    constant GREEN_SKULL  = 8

    constant GOLD_KEY     = 9
    constant SILVER_KEY   = 10
    constant BRASS_KEY    = 11
    constant COPPER_KEY   = 12
    constant STEEL_KEY    = 13
    constant WOODEN_KEY   = 14
    constant FIRE_KEY     = 15
    constant WATER_KEY    = 16

    function num_players()  = native
    function set_who(index) = native

    function is_bot()    : float  = native
    function get_name()  : string = native
    function get_pos()   : float  = native
    function get_angle() : float  = native
    function get_mlook() : float  = native

    function health()      : float = native
    function armor(type)   : float = native
    function total_armor() : float = native
    function ammo(type)    : float = native
    function ammomax(type) : float = native
    function frags()       : float = native

    function is_swimming()  : float = native
    function is_jumping()   : float = native
    function is_crouching() : float = native
    function is_using()     : float = native
    function is_action1()   : float = native
    function is_action2()   : float = native
    function is_attacking() : float = native
    function is_rampaging() : float = native
    function is_grinning()  : float = native

    function under_water()  : float = native
    function on_ground()    : float = native
    function move_speed()   : float = native
    function air_in_lungs() : float = native

    function has_key(key)     : float = native
    function has_power(type)  : float = native
    function power_left(type) : float = native
    function has_weapon(name : string) : float = native
    function has_weapon_slot(slot) : float = native
    function cur_weapon()         : string = native
    function cur_weapon_slot(slot) : float = native

    function main_ammo(clip)   : float = native
    function ammo_type(ATK)    : float = native
    function ammo_pershot(ATK) : float = native
    function clip_ammo(ATK)    : float = native
    function clip_size(ATK)    : float = native
    function clip_is_shared()  : float = native

    function hurt_by()   : string = native
    function hurt_mon()  : string = native
    function hurt_pain()  : float = native
    function hurt_dir()   : float = native
    function hurt_angle() : float = native
}

