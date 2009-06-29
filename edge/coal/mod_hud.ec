
// HUD LIBRARY

var hud_which;
var hud_passed_time;
var hud_automap;

function hud_map_title() : string = { return "Entryway"; }
function hud_game_mode() : string = { return "sp"; }

function hud_text_color(color : string) =
{
  sprint("HUD_TEXT_COLOR");
  sprint(color);
}

function hud_text_font(font : string) =
{ 
  sprint("HUD_TEXT_FONT");
  sprint(font);
}

function hud_draw_num2(x, y, w, num) =
{
  sprint("HUD_DRAW_NUM2");
  nprint(num);
}

function hud_draw_image(x, y, image : string) =
{
  sprint("HUD_DRAW_IMAGE");
  sprint(image);
}

function hud_draw_text(x, y, text : string) =
{
  sprint("HUD_DRAW_TEXT");
  sprint(text);
}

function hud_render_world(x, y, w, h) =
{
  sprint("HUD_RENDER_WORLD");
}

function hud_render_automap(x, y, w, h) =
{
  sprint("HUD_RENDER_AUTOMAP");
}

function hud_coord_sys(w, h) =
{
  sprint("HUD_COORD_SYS:");
  nprint(w);
  nprint(h);
}

