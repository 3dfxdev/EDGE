//------------------------------------------
//  HUD CODE for EDGE
//  Copyright (c) 2009 The Edge Team.
//  Under the GNU General Public License.
//------------------------------------------

float  face_time;
string face_image;


void (float slot, float x, float y, string off_pic, string on_pic) doom_weapon_icon =
{
  if (player_has_weapon_slot(slot))
    hud_draw_image(x, y, on_pic);
  else
    hud_draw_image(x, y, off_pic);
};


void(float x, float y, float card, float skull,
     string card_pic, string skull_pic, string both_pic) doom_key =
{
  local float has_cd ; has_cd = player_has_key(card);
  local float has_sk ; has_cd = player_has_key(skull);
  
  if (has_cd && has_sk)
  {
    hud_draw_image(x, y, both_pic);
  }
  else if (has_cd)
  {
    hud_draw_image(x, y, card_pic);
  }
  else if (has_sk)
  {
    hud_draw_image(x, y, skull_pic);
  }
};


  string() pain_digit =
  {
    local float health; health = player_health();
    if (health > 100)
      health = 100;

    local float index; index = math_floor(4.99 * (100 - health) / 100);

    sys_assert(index >= 0);
    sys_assert(index <= 4);

    return string_format("%d", index);
  };

  void() turn_digit =
  {
    local float r; r = math_random() * 2.99;
    r = math_floor(r);
    return string_format("%d", r);
  };

  void() select_new_face =
  {
    // dead ?
    if (player_health() <= 0)
    {
      face_image = "STFDEAD0";
      face_time  = 10;
      return;
    }

    // evil grin when player just picked up a weapon
    if (player_is_grinning())
    {
      face_image = string_concat("STFEVL", pain_digit());
      face_time  = 7;
      return;
    }

    // being attacked ?
    if (player_hurt_by())
    {
      if (player_hurt_pain() > 50)
      {
        face_image = string_concat("STFOUCH", pain_digit());
        face_time = 26;
        return;
      }

      local float dir; dir = 0;

      if (player_hurt_by() == "enemy" ||
          player_hurt_by() == "friend")
      {
        dir = player_hurt_dir();
      }

      if (dir < 0)
        face_image = string_concat3("STFTL", pain_digit(), "0");
      else if (dir > 0)
        face_image = string_concat3("STFTR", pain_digit(), "0");
      else
        face_image = string_concat("STFKILL", pain_digit());

      face_time = 35;
      return;
    }

    // rampaging?
    if (player_is_rampaging())
    {
      face_image = string_concat("STFKILL", pain_digit());
      face_time  = 7;
      return;
    }

    // god mode?
    if (player_has_power("invuln"))
    {
      face_image = "STFGOD0";
      face_time  = 7;
      return;
    }

    // default: look about the place...
    face_image = string_concat3("STFST", pain_digit(), turn_digit());
    face_time  = 17;
  };

void(float x, float y) doomguy_face =
{
  // This routine handles the face states and their timing.
  // The precedence of expressions is:
  //
  //    dead > evil grin > turned head > straight ahead

// ABOVE FUNCS SHOULD BE HERE \\

  //---| doomguy_face |---

  face_time = face_time - hud_passed_time;

  if (face_time <= 0)
    select_new_face();

  // FIXME faceback

  hud_draw_image(x - 1, y - 1, face_image);
};


void() doom_little_ammo =
{
  hud_text_font("YELLOW_DIGIT");
  hud_text_color("wtf");

  local float a;

  a = player_ammo(1); hud_draw_num2(288, 173, 3, a);
  a = player_ammo(2); hud_draw_num2(288, 179, 3, a);
  a = player_ammo(3); hud_draw_num2(288, 185, 3, a);
  a = player_ammo(4); hud_draw_num2(288, 191, 3, a);

  a = player_ammomax(1); hud_draw_num2(314, 173, 3, a);
  a = player_ammomax(2); hud_draw_num2(314, 179, 3, a);
  a = player_ammomax(3); hud_draw_num2(314, 185, 3, a);
  a = player_ammomax(4); hud_draw_num2(314, 191, 3, a);
};


void() doom_status_bar =
{
  local float a;

  hud_draw_image(  0, 168, "STBAR");
  hud_draw_image( 90, 171, "STTPRCNT");
  hud_draw_image(221, 171, "STTPRCNT");

  hud_text_font("BIG_DIGIT");

  a = player_main_ammo(1) ; hud_draw_num2( 44, 171, 3, a);
  a = player_health()     ; hud_draw_num2( 90, 171, 3, a);
  a = player_total_armor(); hud_draw_num2(221, 171, 3, a);

  if (hud_game_mode() == "dm")
  {
    a = player_frags();
    hud_draw_num2(138, 171, 2, a);
  }
  else
  {
    hud_draw_image(104, 168, "STARMS");

    doom_weapon_icon(2, 111, 172, "STGNUM2", "STYSNUM2"); 
    doom_weapon_icon(3, 123, 172, "STGNUM3", "STYSNUM3");
    doom_weapon_icon(4, 135, 172, "STGNUM4", "STYSNUM4");

    doom_weapon_icon(5, 111, 182, "STGNUM5", "STYSNUM5");
    doom_weapon_icon(6, 123, 182, "STGNUM6", "STYSNUM6");
    doom_weapon_icon(7, 135, 182, "STGNUM7", "STYSNUM7");
  }

  doomguy_face(144, 169);

  doom_key(239, 171, 1, 5, "STKEYS0", "STKEYS3", "STKEYS6");
  doom_key(239, 181, 2, 6, "STKEYS1", "STKEYS4", "STKEYS7");
  doom_key(239, 191, 3, 7, "STKEYS2", "STKEYS5", "STKEYS8");

  doom_little_ammo();
};


void() doom_overlay_status =
{
  local float a;

  hud_text_font("BIG_DIGIT");

  a = player_health();
  hud_draw_num2(100, 171, 3, a);

  a = player_main_ammo(1);
  hud_text_color("TEXT_YELLOW");
  hud_draw_num2( 44, 171, 3, a);

  a = player_total_armor();
  if (player_total_armor() > 100)
    hud_text_color("TEXT_BLUE");
  else
    hud_text_color("TEXT_GREEN");

  hud_draw_num2(242, 171, 3, a);

  doom_key(256, 171, 1, 5, "STKEYS0", "STKEYS3", "STKEYS6");
  doom_key(256, 181, 2, 6, "STKEYS1", "STKEYS4", "STKEYS7");
  doom_key(256, 191, 3, 7, "STKEYS2", "STKEYS5", "STKEYS8");

  doom_little_ammo();
};


void() doom_automap =
{
  // Background is already black, only need to use 'solid_box'
  // when we want a different color.
  //
  // hud_solid_box(0, 0, 320, 200-32, "#505050")

  hud_render_automap(0, 0, 320, 200 - 32);

  doom_status_bar();

  local string title; title = hud_map_title();

  hud_text_font("DOOM");
  hud_draw_text(0, 200 - 32 - 10, title);
};


void() edge_air_bar =
{
  if (player_health() <= 0)
    return;

  if (! player_under_water())
    return;

  local float air; air = player_air_in_lungs();

  air = math_floor(1 + 21 * ((100 - air) / 100.1));

  local string barname; barname = string_format("AIRBAR%02d", air);

  hud_draw_image(0, 0, barname);
};


void() draw_all =
{
  hud_coord_sys(320, 200);

  if (hud_automap)
  {
    doom_automap();
    return;
  }

  // there are three standard HUDs
  hud_which = math_mod(hud_which, 3);

  if (hud_which == 0)
    hud_render_world(0, 0, 320, 200 - 32);
  else
    hud_render_world(0, 0, 320, 200);

  if (hud_which == 0)
    doom_status_bar();
  else if (hud_which == 2)
    doom_overlay_status();

  edge_air_bar();
};

