local star = require "star.core"
local sock = require "star.sock"
local timer = require "star.timer"

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

-- test base 
function REQUEST:hello()
	-- 陷阱: 如果用 "star.call", call 一个没有返回值的函数, 则该协程将永远挂起
	-- 比如: foo 函数没有返回值, 用 star.send 就好
	local result = star.call("add", 1, 6)
	print("call add:", result)

	star.send("foo")
end

-- 性能测试
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
		local threads = {}
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


-- test sleep
function REQUEST:sleep()
	print("i will sleep 5s", os.time())
	star.sleep(5000)
	print("i am sleep over", os.time())
end


-- test next version api : star.xcall
function REQUEST:xcall( )
	local co = {}
	local a,b

	co[1] = coroutine.create(function ( )
		a = star.call("eight_queens", {}, 1)
		if b then
			-- do something
			print("xcall over")
		end
	end)

	co[2] = coroutine.create(function ( )
		b = star.call("eight_queens", {}, 1)
		if a then
			-- do something
			print("xcall over")
		end
	end)

	coroutine.resume(co[1])
	coroutine.resume(co[2])

	--[[
		情景: 客户端有个请求中 有多次复杂算法的调用
			a.如果我们这样做:
				local a = star.call("aaaa", ...)  #假设cpu计算耗时 1s
				local b = star.call("bbbb", ...)  #假设cpu计算耗时 2s
				print(a, b)

				star.call 会挂起当前协程, 所以a, b的计算是串行的, 则响应时间为 3s (1s + 2s) (不考虑其他开销)

			b. 按照上面例子的写法:
				响应时间按最长一次的调用计算, 即2s. why?
					说明1：每次调用都在一个独立协程中进行
					说明2: 每个协程中的star.call 会调用其他线程(func thread) 的计算能力
					说明3: star.call 每次调用 会轮换使用不同的func线程，
							假设有func thread1, func thread2, func thread3, func thread4
							那上面的伪代码 a 会 调用 func thread1, b 会 调用 func thread2 中的函数
					结论: 通过这样的方式, 我们可以在玩家的单次请求中, 将计算任务分割到不同线程中去做, 缩短响应时间

			c. 但是 每次都这样写太麻烦了
			   所以 下版本中将加入 star.xcall 的 api来帮我们做这件事 使代码更简洁
		
			   -- in the next version
			   local a, b = star.xcall{
					{"foo1", ...},
					{"foo2", ...}
			   }

			   print(a, b)
	]]
end


-- test timer
local function test_timer()

	print("")
	print("timer test ------------>")
	print("i will sleep 3000ms", os.time())

	-- 注意: 例子中 test_timer 跑在(lua_State)初始化中, star.sleep() 会阻塞线程,
	-- 在 socket 处理函数中调用 star.sleep() 只会挂起当前协程, 不会阻塞线程
	star.sleep(3000)

	print("I woke up", os.time())
	print("")

	local id1 = timer.timeout(1000, function ( )
		print("im timeout 1", os.time())
	end)

	local id2 = timer.timeout(4000, function ( )
		print("im timeout 2", os.time())
	end)

	local id3 = timer.timeout(7000, function ( )
		print("im timeout 3", os.time())
		print("timer test ------------<")
		print("")
	end)

	print("create timer 1:", id1)
	print("create timer 2:", id2)
	print("create timer 3:", id3)
	print("")
	print("now cancel timer 1")
	timer.cancel(id1)
end


star.server{

	open = function (fd, ip, port)
		sock.send(fd, "Welcome, i'm an echo-server create by Star\n")
		print(string.format("new client:%d from:'%s:%d'", fd, ip, port))
	end,

	data = function (fd, data)
	
		if data:sub(#data, #data) == "\n" then
			data = data:sub(1, #data-1)
		end

		local f = REQUEST[data]
		if f then f() end

		print(string.format("============> client %d: %s", fd, data))

		-- 如果 star.call/send的 第一个参数是数字(>=0), 则会发到一个固定的func thread 去处理
		-- 因为我们不想客户端收到的包是乱序的
		star.send(fd, "send2client", fd, "star:"..data.."\n")
	end,

	close = function (fd)
		print(string.format("client %d close socket", fd))
	end
}

-- v0.03
--[[
	Test:
	1.open another shell
	2.nc 127.0.0.1 8888
	3.input hello and enter
	4.input test and enter
	5.input xcall and enter
]]

print("star version", star.version(), "\n")

test_timer()