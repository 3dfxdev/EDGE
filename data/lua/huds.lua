--------------------------------------------
--  HUD LUA CODE for EDGE
--  Copyright (c) 2008 The Edge Team.
--  Under the GNU General Public License.
--------------------------------------------

face_time = 0
face_image = "STFST01"


function doom_weapon_icon(slot, x, y, off_pic, on_pic)
  if player.has_weapon_slot(slot) then
    hud.draw_image(x, y, on_pic)
  else
    hud.draw_image(x, y, off_pic)
  end
end


function doom_key(x, y, card, skull, card_pic, skull_pic, both_pic)

  local has_sk = player.has_key(skull)
  
  if player.has_key(card) then
    hud.draw_image(x, y, sel(has_sk, both_pic, card_pic))

  elseif has_sk then
    hud.draw_image(x, y, skull_pic)
  end
end


function doomguy_face(x, y)

  -- This routine handles the face states and their timing.
  -- The precedence of expressions is:
  --
  --    dead > evil grin > turned head > straight ahead

  local function pain_digit()

    local base = 100; --!!!! FIXME  player.healthmax();
    local health = math.min(base, player.health())
    
    local index = int(4.99 * (base - health) / base);

    assert(index >= 0)
    assert(index <= 4)

    return tostring(index)
  end


  local function turn_digit()
    return tostring(1 + int(math.random() * 2.99))
  end


  local function select_new_face()

    -- dead ?
    if player.health() <= 0 then
      face_image = "STFDEAD0"
      face_time  = 10
      return
    end

    -- evil grin when player just picked up a weapon
    if player.is_grinning() then
      face_image = "STFEVL" .. pain_digit()
      face_time  = 7
      return
    end

    -- being attacked ?
    if player.hurt_by() then
      
      if player.hurt_pain() > 50 then
        face_image = "STFOUCH" .. pain_digit()
        face_time = 35
        return
      end

      local dir = 0

      if player.hurt_by() == "enemy" then
        dir = player.hurt_dir()
      end

      if dir < 0 then
        face_image = "STFTL" .. pain_digit()
      elseif dir > 0 then
        face_image = "STFTR" .. pain_digit()
      else
        face_image = "STFKILL" .. pain_digit()
      end

      face_time = 35
      return
    end

    -- rampaging?
    if player.is_rampaging() then
      face_image = "STFKILL" .. pain_digit()
      face_time  = 7
      return
    end

    -- god mode?
    if player.has_power(POWERS.invuln) then
      face_image = "STFGOD0"
      face_time  = 7
      return
    end

    -- default: look about the place...
    face_image = "STFST" .. pain_digit() .. turn_digit()
    face_time  = 17
  end


  ---| doomguy_face |---

  face_time = face_time - hud.passed_time

  if face_time <= 0 then
    select_new_face()
  end

  -- FIXME faceback

  hud.draw_image(x-1, y-1, face_image)
end


function doom_status_bar()

  hud.draw_image(  0, 168, "STBAR");
  hud.draw_image( 90, 171, "STTPRCNT");
  hud.draw_image(221, 171, "STTPRCNT");

  hud.text_font("BIG_DIGIT")

  hud.draw_num2( 44, 171, 3, player.main_ammo(1));
  hud.draw_num2( 90, 171, 3, player.health());
  hud.draw_num2(221, 171, 3, player.total_armor())

  if hud.game_mode() == "dm" then

    hud.draw_num2(138, 171, 2, player.frags());

  else
    hud.draw_image(104, 168, "STARMS");

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

  hud.text_font("YELLOW_DIGIT")

  hud.draw_num2(288, 173, 3, player.ammo(1));
  hud.draw_num2(288, 179, 3, player.ammo(2));
  hud.draw_num2(288, 185, 3, player.ammo(3));
  hud.draw_num2(288, 191, 3, player.ammo(4));

  hud.draw_num2(314, 173, 3, player.ammomax(1));
  hud.draw_num2(314, 179, 3, player.ammomax(2));
  hud.draw_num2(314, 185, 3, player.ammomax(3));
  hud.draw_num2(314, 191, 3, player.ammomax(4));
end


function doom_overlay_status()
  -- TODO !!!!
end


function doom_automap()

--[[ 
  hud.automap_colors(
  {
    grid    = "#006666",

    wall    = "#FF0000",
    step    = "#C08050",
    ledge   = "#77DD77",
    ceil    = "#DDDD00",
    secret  = "#00CCCC",
    allmap  = "#888888",

    player  = "#FFFFFF",
    monster = "#00DD00",
    corpse  = "#DD0000",
    item    = "#0000FF",
    missile = "#FFBB00",
    scenery = "#773311",
  })
--]]

  -- Background is already black, only need to use 'solid_box'
  -- when we want a different color.
  --
  -- hud.solid_box(0, 0, 320, 200-32, "#505050")

  hud.render_automap(0, 0, 320, 200-32)

  doom_status_bar()

  hud.text_font("DOOM")
  hud.draw_text(0, 200-32-10, hud.map_title())
end


function edge_air_bar()
  if player.health() <= 0 or not player.under_water() then
    return
  end

  local air = player.air_in_lungs()

  air = int(1 + 21 * ((100-air) / 100.1))

  hud.draw_image(0, 0, string.format("AIRBAR%02d", air))
end


function hud.draw_all()

  hud.coord_sys(320, 200)

  if hud.automap then
    doom_automap()
    return
  end

  -- there are three standard HUDs
  hud.which = hud.which % 3

  if hud.which == 0 then
    hud.render_world(0, 0, 320, 200-32)
  else
    hud.render_world(0, 0, 320, 200)
  end

  if hud.which == 0 then
    doom_status_bar()
  elseif hud.which == 2 then
    doom_overlay_status()
  end

  edge_air_bar()
end

