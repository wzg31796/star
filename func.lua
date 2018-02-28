function add(a)
	local n = 0
	for _,v in ipairs(a) do
		n = n + v
	end
	return n
end

function log( ... )
	print("Log:", ...)
end
