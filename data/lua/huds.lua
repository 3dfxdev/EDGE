--------------------------------------------
--  HUD LUA CODE for EDGE
--  Copyright (c) 2008 The Edge Team.
--  Under the GNU General Public License.
--------------------------------------------

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
  -- TODO !!!

  -- faceback too

  hud.draw_image(x-1, y-1, "STFEVL0")
end


function doom_status_bar()

  hud.draw_image(  0, 168, "STBAR");
  hud.draw_image( 90, 171, "STTPRCNT");
  hud.draw_image(221, 171, "STTPRCNT");

  hud.text_font("BIG_DIGIT")

  hud.draw_num2(44, 171, 3, player.cur_ammo(1));
  hud.draw_num2(90, 171, 3, player.health());

  local armor = 0
  for i = 1,5 do
    armor = math.max(armor, player.armor(i))
  end

  hud.draw_num2(221, 171, 3, armor)

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


function overlay_status_bar()
  -- TODO !!!!
end


function hud.draw_all()

  hud.mode = "normal" --!!!!!

  hud.scaling(320, 200)
  hud.text_color()

  if hud.mode == "full" or hud.mode == "overlay" then
    hud.render_world(0, 0, 320, 200)
  elseif hud.mode == "normal" then
    hud.render_world(0, 32, 320, 200-32)
  elseif hud.mode == "automap" then
    hud.render_automap(0, 32, 320, 200-32)
  end

  if hud.mode == "normal" or hud.mode == "automap" then
    doom_status_bar()
  elseif hud.mode == "overlay" then
    overlay_status_bar()
  end

  -- TODO: air bar
end

