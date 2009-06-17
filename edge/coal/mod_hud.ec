
// HUD LIBRARY

float hud_which;
float hud_passed_time;
float hud_automap;

string() hud_map_title = { return "Entryway"; };
string() hud_game_mode = { return "sp"; };

void(string color) hud_text_color =
{
  sprint("HUD_TEXT_COLOR");
  sprint(color);
};

void(string font) hud_text_font =
{ 
  sprint("HUD_TEXT_FONT");
  sprint(font);
};

void(float x, float y, float w, float num) hud_draw_num2 =
{
  sprint("HUD_DRAW_NUM2");
  nprint(num);
};

void (float x, float y, string image) hud_draw_image =
{
  sprint("HUD_DRAW_IMAGE");
  sprint(image);
};

void (float x, float y, string text) hud_draw_text =
{
  sprint("HUD_DRAW_TEXT");
  sprint(text);
};

void(float x, float y, float w, float h) hud_render_world =
{
  sprint("HUD_RENDER_WORLD");
};

void(float x, float y, float w, float h) hud_render_automap =
{
  sprint("HUD_RENDER_AUTOMAP");
};

void(float w, float h) hud_coord_sys =
{
  sprint("HUD_COORD_SYS:");
  nprint(w);
  nprint(h);
};

