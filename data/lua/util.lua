--------------------------------------------
--  UTILITY LUA CODE for EDGE
--  Copyright (c) 2008 The Edge Team.
--  Under the GNU General Public License.
--------------------------------------------


----====| GENERAL STUFF |====----

function do_nothing()
end

function int(val)
  return math.floor(val)
end

function sel(cond, yes_val, no_val)
  if cond then
    return yes_val
  else
    return no_val
  end
end

function dist(x1,y1, x2,y2)
  return math.sqrt( (x1-x2)*(x1-x2) + (y1-y2)*(y1-y2) )
end


----====| TABLE UTILITIES |====----

function table_empty(t)
  return not next(t)
end

function table_size(t)
  local count = 0;
  for k,v in pairs(t) do count = count+1 end
  return count
end

function table_to_str(t, depth, prefix)
  if not t then return "NIL" end
  if table_empty(t) then return "{}" end

  depth = depth or 1
  prefix = prefix or ""

  local keys = {}
  for k,v in pairs(t) do
    table.insert(keys, k)
  end

  table.sort(keys, function (A,B) return tostring(A) < tostring(B) end)

  local result = "{\n"

  for idx,k in ipairs(keys) do
    local v = t[k]
    result = result .. prefix .. "  " .. tostring(k) .. " = "
    if type(v) == "table" and depth > 1 then
      result = result .. table_to_str(v, depth-1, prefix .. "  ")
    else
      result = result .. tostring(v)
    end
    result = result .. "\n"
  end

  result = result .. prefix .. "}"

  return result
end

function merge_table(dest, src)
  for k,v in pairs(src) do
    dest[k] = v
  end
  return dest
end

-- Note: shallow copy
function copy_table(t)
  return t and merge_table({}, t)
end


----====| EDGE INTERFACE |====----

function hud.printf(fmt, ...)
  if fmt then
    hud.raw_debug_print(string.format(fmt, ...))
  end
end

--[[
function hud.debugf(fmt, ...)
  if fmt then
    hud.raw_debug_print(string.format(fmt, ...))
  end
end
--]]


function stack_trace(msg)

  -- guard against very early errors
  if not hud or not hud.printf then
    return msg
  end
 
  hud.printf("\nStack Trace:\n")

  local stack_limit = 40

  local function format_source(info)
    if not info.short_src or info.currentline <= 0 then
      return ""
    end

    local base_fn = string.match(info.short_src, "[^/]*$")
 
    return string.format("@ %s:%d", base_fn, info.currentline)
  end

  for i = 1,stack_limit do
    local info = debug.getinfo(i+1)
    if not info then break end

    if i == stack_limit then
      hud.printf("(remaining stack trace omitted)\n")
      break;
    end

    if info.what == "Lua" then

      local func_name = "???"

      if info.namewhat and info.namewhat ~= "" then
        func_name = info.name or "???"
      else
        -- perform our own search of the global namespace,
        -- since the standard LUA code (5.1.2) will not check it
        -- for the topmost function (the one called by C code)
        for k,v in pairs(_G) do
          if v == info.func then
            func_name = k
            break;
          end
        end
      end

      hud.printf("  %d: %s() %s\n", i, func_name, format_source(info))

    elseif info.what == "main" then

      hud.printf("  %d: main body %s\n", i, format_source(info))

    elseif info.what == "tail" then

      hud.printf("  %d: tail call\n", i)

    elseif info.what == "C" then

      if info.namewhat and info.namewhat ~= "" then
        hud.printf("  %d: c-function %s()\n", i, info.name or "???")
      end
    end
  end

  return msg
end


----====| ENGINE CONSTANTS |====----

TICRATE = 35

KEYS =
{
  blue_card    = 1,
  yellow_card  = 2,
  red_card     = 3,
  green_card   = 4,

  blue_skull   = 5,
  yellow_skull = 6,
  red_skull    = 7,
  green_skull  = 8,

  gold   =  9,  gold_key   =  9,
  silver = 10,  silver_key = 10,
  brass  = 11,  brass_key  = 11,
  copper = 12,  copper_key = 12,

  steel  = 13,  steel_key  = 13,
  wooden = 14,  wooden_key = 14,
  fire   = 15,  fire_key   = 15,
  water  = 16,  water_key  = 16,
}

POWERUPS =
{
  invuln    = 1,
  berserk   = 2,
  invis     = 3,
  acid_suit = 4,
  automap   = 5,
  goggles   = 6,
  jet_pack  = 7,
  night_vis = 8,
  scuba     = 9,
}

AMMOS =
{
  bullets  = 1,
  shells   = 2,
  rockets  = 3,
  cells    = 4,
  pellets  = 5,
  nails    = 6,
  grenades = 7,
  gas      = 8,
}

ARMOURS =
{
  green  = 1,
  blue   = 2,
  purple = 3,
  yellow = 4,
  red    = 5,
}

