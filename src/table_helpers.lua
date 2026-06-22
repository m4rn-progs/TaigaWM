local m = {}

function m.table_index_of(tbl, sought)
	for i, v in ipairs(tbl) do
		if v == sought then
			return i
		end
	end
	return 0
end

function m.table_filter_inplace(tbl, pred)
	local removed = 0
	for i = 1, #tbl do
		if pred(tbl[i]) then
			tbl[i - removed] = tbl[i]
		else
			removed = removed + 1
		end
		if removed > 0 then
			tbl[i] = nil
		end
	end
	return tbl
end

return m
