//------------------------------------------
//  HUD CODE for EDGE
//  Copyright (c) 2009 The Edge Team.
//  Under the GNU General Public License.
//------------------------------------------

float  face_time;
string face_image;

float hud_which;


function doom_weapon_icon(slot, x, y, off_pic, on_pic)
  if player_has_weapon_slot(slot) then
    hud_draw_image(x, y, on_pic)
  else
    hud_draw_image(x, y, off_pic)
  end
end


function doom_key(x, y, card, skull, card_pic, skull_pic, both_pic)

  local has_sk = player_has_key(skull)
  
  if player_has_key(card) then
    hud_draw_image(x, y, sel(has_sk, both_pic, card_pic))

  elseif has_sk then
    hud_draw_image(x, y, skull_pic)
  end
end


function doomguy_face(x, y)

  // This routine handles the face states and their timing.
  // The precedence of expressions is:
  //
  //    dead > evil grin > turned head > straight ahead

  local function pain_digit()

    local health = math.min(100, player_health())
    
    local index = int(4.99 * (100 - health) / 100);

    assert(index >= 0)
    assert(index <= 4)

    return tostring(index)
  end


  local function turn_digit()
    return tostring(int(math.random() * 2.99))
  end


  local function select_new_face()

    // dead ?
    if player_health() <= 0 then
      face_image = "STFDEAD0"
      face_time  = 10
      return
    end

    // evil grin when player just picked up a weapon
    if player_is_grinning() then
      face_image = "STFEVL" .. pain_digit()
      face_time  = 7
      return
    end

    // being attacked ?
    if player_hurt_by() then
      
      if player_hurt_pain() > 50 then
        face_image = "STFOUCH" .. pain_digit()
        face_time = 26
        return
      end

      local dir = 0

      if player_hurt_by() == "enemy" or player_hurt_by() == "friend" then
        dir = player_hurt_dir()
      end

      if dir < 0 then
        face_image = "STFTL" .. pain_digit() .. "0"
      elseif dir > 0 then
        face_image = "STFTR" .. pain_digit() .. "0"
      else
        face_image = "STFKILL" .. pain_digit()
      end

      face_time = 35
      return
    end

    // rampaging?
    if player_is_rampaging() then
      face_image = "STFKILL" .. pain_digit()
      face_time  = 7
      return
    end

    // god mode?
    if player_has_power(POWERS.invuln) then
      face_image = "STFGOD0"
      face_time  = 7
      return
    end

    // default: look about the place...
    face_image = "STFST" .. pain_digit() .. turn_digit()
    face_time  = 17
  end


  //---| doomguy_face |---

  face_time = face_time - hud_passed_time

  if face_time <= 0 then
    select_new_face()
  end

  // FIXME faceback

  hud_draw_image(x - 1, y - 1, face_image)
end


void() doom_little_ammo =
{
  hud_text_font("YELLOW_DIGIT")
  hud_text_color("");

  a = player_ammo(1); hud_draw_num2(288, 173, 3, a);
  a = player_ammo(2); hud_draw_num2(288, 179, 3, a);
  a = player_ammo(3); hud_draw_num2(288, 185, 3, a);
  a = player_ammo(4); hud_draw_num2(288, 191, 3, a);

  a = player_ammomax(1); hud_draw_num2(314, 173, 3, a);
  a = player_ammomax(2); hud_draw_num2(314, 179, 3, a);
  a = player_ammomax(3); hud_draw_num2(314, 185, 3, a);
  a = player_ammomax(4); hud_draw_num2(314, 191, 3, a);
}


void() doom_status_bar =
{
  local float a;

  hud_draw_image(  0, 168, "STBAR");
  hud_draw_image( 90, 171, "STTPRCNT");
  hud_draw_image(221, 171, "STTPRCNT");

  hud_text_font("BIG_DIGIT")

  a = player_main_ammo(1) ; hud_draw_num2( 44, 171, 3, a);
  a = player_health()     ; hud_draw_num2( 90, 171, 3, a);
  a = player_total_armor(); hud_draw_num2(221, 171, 3, a)

  if hud_game_mode() == "dm" then

    a = player_frags()
    hud_draw_num2(138, 171, 2, a);

  else
    hud_draw_image(104, 168, "STARMS");

    doom_weapon_icon(2, 111, 172, "STGNUM2", "STYSNUM2"); 
    doom_weapon_icon(3, 123, 172, "STGNUM3", "STYSNUM3");
    doom_weapon_icon(4, 135, 172, "STGNUM4", "STYSNUM4");

    doom_weapon_icon(5, 111, 182, "STGNUM5", "STYSNUM5");
    doom_weapon_icon(6, 123, 182, "STGNUM6", "STYSNUM6");
    doom_weapon_icon(7, 135, 182, "STGNUM7", "STYSNUM7");
  end

  doomguy_face(144, 169)

  doom_key(239, 171, 1, 5, "STKEYS0", "STKEYS3", "STKEYS6")
  doom_key(239, 181, 2, 6, "STKEYS1", "STKEYS4", "STKEYS7")
  doom_key(239, 191, 3, 7, "STKEYS2", "STKEYS5", "STKEYS8")

  doom_little_ammo()
}


void() doom_overlay_status =
{
  local float a

  hud_text_font("BIG_DIGIT")

  a = player_health()
  hud_draw_num2(100, 171, 3, health);

  a = player_main_ammo(1)
  hud_text_color("TEXT_YELLOW");
  hud_draw_num2( 44, 171, 3, a);

  a = player_total_armor()
  if player_total_armor() > 100 then
    hud_text_color("TEXT_BLUE");
  else
    hud_text_color("TEXT_GREEN");
  end
  hud_draw_num2(242, 171, 3, a)

  doom_key(256, 171, 1, 5, "STKEYS0", "STKEYS3", "STKEYS6")
  doom_key(256, 181, 2, 6, "STKEYS1", "STKEYS4", "STKEYS7")
  doom_key(256, 191, 3, 7, "STKEYS2", "STKEYS5", "STKEYS8")

  doom_little_ammo()
}


void() doom_automap =
{
  // Background is already black, only need to use 'solid_box'
  // when we want a different color.
  //
  // hud_solid_box(0, 0, 320, 200-32, "#505050")

  hud_render_automap(0, 0, 320, 200 - 32)

  doom_status_bar()

  local string title = hud_map_title()

  hud_text_font("DOOM")
  hud_draw_text(0, 200 - 32 - 10, title)
}


void() edge_air_bar =
{
  if player_health() <= 0 || ! player_under_water() then
    return
  end

  local float air; air = player_air_in_lungs()

  air = math_floor(1 + 21 * ((100 - air) / 100.1))

  local string barname = string_format("AIRBAR%02d", air)

  hud_draw_image(0, 0, barname, air))
end


void() draw_all() =
{
  hud_coord_sys(320, 200)

  if hud_automap() then
    doom_automap()
    return
  end

  // there are three standard HUDs
  hud_which = hud_which % 3

  if hud_which == 0 then
    hud_render_world(0, 0, 320, 200 - 32)
  else
    hud_render_world(0, 0, 320, 200)
  end

  if hud_which == 0 then
    doom_status_bar()
  elseif hud_which == 2 then
    doom_overlay_status()
  end

  edge_air_bar()
end

