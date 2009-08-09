
// BASICS

module sys
{
    function print(s : string) = native

    function assert(n : float) = { }
}

// FIXME temp crud
function nprint(n : float) = { sys.print("" + n) }


// MATH

module math
{
    function floor(n : float) : float = { return n - n % 1; }
    function random() : float = { return 1; }
}


// STRING

module strings
{
}


// HUD LIBRARY

module hud
{
    var which;
    var passed_time;
    var automap;

    function map_title() : string = { return "Entryway"; }
    function game_mode() : string = { return "sp"; }

    function text_color(color : string) =
    {
      sys.print("HUD_TEXT_COLOR");
      sys.print(color);
    }

    function text_font(font : string) =
    { 
      sys.print("HUD_TEXT_FONT");
      sys.print(font);
    }

    function draw_num2(x, y, w, num) =
    {
      sys.print("HUD_DRAW_NUM2");
      nprint(num);
    }

    function draw_image(x, y, image : string) =
    {
      sys.print("HUD_DRAW_IMAGE");
      sys.print(image);
    }

    function draw_text(x, y, text : string) =
    {
      sys.print("HUD_DRAW_TEXT");
      sys.print(text);
    }

    function render_world(x, y, w, h) =
    {
      sys.print("HUD_RENDER_WORLD");
    }

    function render_automap(x, y, w, h) =
    {
      sys.print("HUD_RENDER_AUTOMAP");
    }

    function coord_sys(w, h) =
    {
      sys.print("HUD_COORD_SYS:");
      nprint(w);
      nprint(h);
    }
}


// PLAYER LIBRARY

module player
{
    function health() : float = { return 50; }

    function has_weapon_slot(slot : float) : float = { return 1; }

    function has_key(key : float) : float = { return 1; }

    function main_ammo(clip : float) : float = { return 24; }
    function ammo(type : float) : float = { return 120 + type; }
    function ammomax(type : float) : float = { return 200 + type; }
    function total_armor() : float = { return 40; }

    function hurt_by() : string = { return "none"; }
    function hurt_pain() : float = { return 3; }
    function hurt_dir() : float = { return 0; }

    function frags() : float = { return 0; }

    function under_water() : float = { return 0; }
    function air_in_lungs() : float = { return 3; }

    function is_grinning() : float = { return 0; }
    function is_rampaging() : float = { return 0; }
    function has_power(type : string) : float = { return 0; }
}

