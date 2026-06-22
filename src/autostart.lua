local posix = require("posix")

local m = {}

function m.autostart(tbl)
	for _, item in ipairs(tbl) do
		if posix.unistd.fork() == 0 then
			posix.unistd.execp("/bin/sh", { "-c", item })
			os.exit(0)
		end
	end
end

return m
