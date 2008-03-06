--------------------------------------------
--  HUD LUA CODE for EDGE
--  Copyright (c) 2008 The Edge Team.
--  Under the GNU General Public License.
--------------------------------------------

function doom_weapon_icon(x, y, slot, off_pic, on_pic)
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
  -- TODO !!!

  -- faceback too

  hud.draw_image(x-1, y-1, "STFEVL0")
end


function doom_status_bar()

  hud.draw_image(  0, 168, "STBAR");
  hud.draw_image( 90, 171, "STTPRCNT");
  hud.draw_image(221, 171, "STTPRCNT");

  hud.text_font("STAT_DIGIT")

  hud.draw_number( 44, 171, 3, player.cur_ammo(1));
  hud.draw_number( 90, 171, 3, player.health());

  local armor = 0
  for i = 1,5 do
    armor = math.max(armor, player.armor(i))
  end

  hud.draw_number(221, 171, 3, armor)

  if hud.game_mode() == "dm" then

    hud.draw_number(138, 171, 2, player.frags());

  else
    hud.draw_image("STARMS", 104, 168);

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

  hud.draw_number( 3, player.ammo(1), 288, 173);
  hud.draw_number( 3, player.ammo(2), 288, 179);
  hud.draw_number( 3, player.ammo(3), 288, 185);
  hud.draw_number( 3, player.ammo(4), 288, 191);

  hud.draw_number( 3, player.ammomax(1), 314, 173);
  hud.draw_number( 3, player.ammomax(2), 314, 179);
  hud.draw_number( 3, player.ammomax(3), 314, 185);
  hud.draw_number( 3, player.ammomax(4), 314, 191);
end


function overlay_status_bar()
  -- TODO !!!!
end


function hud.draw_all()

  hud.hud_scaling(320, 200)
  hud.text_color()

  if hud.mode == "full" or hud.mode == "overlay" then
    hud.render_world(0, 0, 320, 200)
  elseif hud.mode == "normal" then
    hud.render_world(0, 0, 320, 200-32)
  elseif hud.mode == "automap" then
    hud.render_automap(0, 0, 320, 200-32)
  end

  if hud.mode == "normal" or hud.mode == "automap" then
    doom_status_bar()
  elseif hud.mode == "overlay" then
    overlay_status_bar()
  end

  -- TODO: air bar
end

