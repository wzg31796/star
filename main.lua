local star = require "star.core"
local sock = require "star.sock"

-- 仅仅测试用的8皇后算法, 不用看
local function eight_queens(...)
	local N=8
	local solved=false

	local function printBoard(board)
	    if solved then
	        return
	    end
	    for i=1, N do
	        for j=1, N do
	            -- io.write(board[i]==j and "X" or "-", " ")
	        end
	        -- io.write("\n")
	    end
	    -- io.write("\n")
	    solved = true
	end

	local function validPos(board, row, pos)
	    for i=1, row-1 do
	        if (board[i] == pos) or
	            (row-i == math.abs(pos - board[i])) then
	            return false
	        end
	    end
	    return true
	end

	local function addQueen(board, n)
	    if n>N then
	        printBoard(board)
	    else
	        for pos=1, N do
	            if validPos(board, n, pos) then
	                board[n]=pos
	                addQueen(board, n+1)
	            end
	        end
	    end
	end

	addQueen(...)
	
	return true
end

local REQUEST = {}

function REQUEST:hello()
	-- 陷阱: 如果用 "star.call", call 一个没有返回值的函数, 则该协程将永远挂起
	-- 比如: foo 函数没有返回值, 用 star.send 就好
	print("star version", star.version(), "\n")

	local result = star.call("add", 1, 6)
	print("call add:", result)

	star.send("foo")
end

local threads = {}

function REQUEST:test()
	-- 3.性能测试
	local function test_star_call()
		local t1 = os.clock()
		local dt

		for i=1,100 do
			star.call("eight_queens", {}, 1)
		end

		dt = os.clock() - t1
		return dt
	end

	local function test_local_call()

		local t1 = os.clock()
		local dt

		for i=1,100 do
			eight_queens({}, 1)
		end

		dt = os.clock() - t1
		
		return dt
	end

	print("")
	print("性能测试------------------->")
	local dt1 = test_star_call()
	local dt2 = test_local_call()

	print("阻塞STAR.CALL 100: "..tostring(dt1).."秒")
	print("本地CALL 100: "..tostring(dt2).."秒")

	print("LOCAL CALL < STAR.CALL: "..tostring((dt1 - dt2)*1000).."ms" )

	print("结论0: call 一次8皇后: ", dt2*1000/100, "ms")
	print("结论1: star.call 一次 八皇后算法的 额外开销:", (dt1 -dt2)*1000/100, "ms")

	local function test_star_call_2()
		local count = 0
		local t1 = os.clock()
		local dt, co

		for i=1,100 do
			threads[i] = coroutine.create(function ( )
				local r = star.call("eight_queens", {}, 1)
				count = count + 1
				if count == 100 then
					dt = os.clock() - t1
					print("开100个协程STAR.CALL: "..tostring(dt).."秒")

					print("结论2: 由于我这里是虚拟机, 所以是最慢的?, 真多核机器我猜应该是最快的...")
					print("性能测试-------------------<")
					print("")
				end
			end)
			coroutine.resume(threads[i])
		end
		return dt
	end

	test_star_call_2()
end

-- v0.02
--[[
	Test:
	1.open another shell
	2.nc 127.0.0.1 8888
	3.input hello and enter
	4.input test and enter
]]
star.server{

	open = function (fd, ip, port)
		sock.send(fd, "Welcome, i'm a echo-server make by Star\n")
		print(string.format("new client:%d from:'%s:%d'", fd, ip, port))
	end,

	data = function (fd, data)


		local msg = data
		if msg:sub(#msg, #msg) == "\n" then
			msg = msg:sub(1, #msg-1)
			local f = REQUEST[msg]
			if f then
				f()
			end
		end

		print(string.format("============> client %d: %s", fd, msg))

		-- 如果 star.call/send的 第一个参数是数字(>=0), 则会发到一个固定的func thread 去处理
		-- 因为我们不想客户端收到的包是乱序的
		star.send(fd, "send2client", fd, "star:"..data)
	end,

	close = function (fd)
		print(string.format("client %d close socket", fd))
	end
}