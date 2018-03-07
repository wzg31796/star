local sock = require "star.sock"

function add( a, b )
	return a + b
end


function foo()
	print("i'm foo() in func.lua")
end


function log( ... )
	print("Log:", ...)
end


function send2client(fd, data)
	sock.send(fd, data)
end
