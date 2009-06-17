
// PLAYER LIBRARY

float() player_health = { return 50; };

float(float slot) player_has_weapon_slot = { return 1; };

float(float key) player_has_key = { return 1; };

float(float clip) player_main_ammo = { return 24; };
float(float type) player_ammo = { return 120 + type; };
float(float type) player_ammomax = { return 200 + type; };
float() player_total_armor = { return 40; };

string() player_hurt_by = { return "none"; };
float() player_hurt_pain = { return 3; };
float() player_hurt_dir = { return 0; };

float() player_frags = { return 0; };

float() player_under_water = { return 0; };
float() player_air_in_lungs = { return 3; };

float() player_is_grinning = { return 0; };
float() player_is_rampaging = { return 0; };
float(string type) player_has_power = { return 0; };

