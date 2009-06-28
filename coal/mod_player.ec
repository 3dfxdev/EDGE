
// PLAYER LIBRARY

function player_health() : float = { return 50; }

function player_has_weapon_slot(slot : float) : float = { return 1; }

function player_has_key(key : float) : float = { return 1; }

function player_main_ammo(clip : float) : float = { return 24; }
function player_ammo(type : float) : float = { return 120 + type; }
function player_ammomax(type : float) : float = { return 200 + type; }
function player_total_armor() : float = { return 40; }

function player_hurt_by() : string = { return "none"; }
function player_hurt_pain() : float = { return 3; }
function player_hurt_dir() : float = { return 0; }

function player_frags() : float = { return 0; }

function player_under_water() : float = { return 0; }
function player_air_in_lungs() : float = { return 3; }

function player_is_grinning() : float = { return 0; }
function player_is_rampaging() : float = { return 0; }
function player_has_power(type : string) : float = { return 0; }

