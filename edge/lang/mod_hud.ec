
// HUD LIBRARY

float hud_which;
float hud_passed_time;

void(string font) hud_text_font =
{ 
  sprint("HUD_TEXT_FONT");
  sprint(font);
};

void (float x, float y, string image) hud_draw_image =
{
  sprint("HUD_DRAW_IMAGE");
  sprint(image);
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

