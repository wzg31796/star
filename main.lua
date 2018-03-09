local star = require "star.core"
local sock = require "star.sock"
local timer = require "star.timer"

local REQUEST = {}

-- star.call 测试
function REQUEST:call()
	print("3 + 4 =", star.call("add", 3, 4))
end


-- star.xcall 测试
function REQUEST:xcall()
	local a,b = star.xcall{
		{"add", 1, 2},
		{"add", 3, 4}
	}
	print("a + b = ", a + b)
end


-- star.send 测试
function REQUEST:send()
	star.send("log", "Author: July Wind, QQ: 707298413")
end


-- timer模块 测试
function REQUEST:timer()
	local timer1 = timer.timeout(1000, function ( )
		print("im timeout 1")
	end)

	local timer2 = timer.timeout(1000, function ( )
		print("im timeout 2, i will start a tick")

		local time = 0
		timer.timeout(1000, function ( )
			time = time + 1
			print("tick:", time)
		end, -1)
	end)

	timer.cancel(timer1)
end


-- 一起测试
function REQUEST:test()
	for cmd, f in pairs(REQUEST) do
		if cmd ~= "test" then
			print("")
			print("test "..cmd..":")
			f()
			print("")
			star.sleep(5000)
		end
	end
end


-- 设置 sock 回调 (TCP)
star.server{

	open = function (fd, ip, port)
		sock.send(fd, "Welcome, i'm an echo-server create by Star\n")
		print(string.format("new client:%d from:'%s:%d'", fd, ip, port))
	end,


	data = function (fd, data)
	
		if data:sub(#data, #data) == "\n" then
			data = data:sub(1, #data-1)
		end

		local f = REQUEST[data] if f then f() end
		
		print(string.format("===========> client %d: %s", fd, data))
		star.send(fd, "send2client", fd, "star:"..data.."\n")
	end,


	close = function (fd)
		print(string.format("client %d close socket", fd))
	end
}

-- UDP

-- star.server{

-- 	data = function (from, data)

-- 		if data:sub(#data, #data) == "\n" then
-- 			data = data:sub(1, #data-1)
-- 		end

-- 		local ip, port = sock.udp_address(from)

-- 		print(string.format("===========> sock(udp) data: %s from '%s:%d'", data, ip, port))

-- 		sock.send(from, 'Star:'..data.."\n")
-- 	end
-- }

--[[
	启动: ./star conf.lua

	Test:
		用客户端连接服务器 (In linux, use nc 127.0.0.1 8888)
		输入 test 并发送   (其他客户端, 你需要发送的字符串为 'test\n')

	Test2:
		将"conf.lua" 中的server改成 "udp", 打开上面 star.server (UDP)的注释并重新运行
		use nc -u 127.0.0.1 8888 连接, 并输入些什么

]]