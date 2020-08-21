local redis = require 'lib.redis'
M = {}
local client = redis.connect('127.0.0.1', 6379)
local response = client:ping()           -- true

function get(key)
  local value = client:get(key)
  if not value or value == '' then
    if key == 'host' then
      client:set('host', 'stackpath.com')
      value = client:get(key)
    end
    if key == 'port' then
      client:set('port', '1234')
      value = client:get(key)
    end
  end
  return value
end
  
function M.getValue(key)
    return get(key)
end

function M.setValue(key, value)
    client:set(key, value)
end

return M
