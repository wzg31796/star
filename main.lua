local star = require "star.core"

local function add(a)
	local n = 0
	for _,v in ipairs(a) do
		n = n + v
	end
	return n
end

local t = {
	1,2,3,4,5,6,7,8,9,
	1,2,3,4,5,6,7,8,9,
	1,2,3,4,5,6,7,8,9,
	1,2,3,4,5,6,7,8,9,
	1,2,3,4,5,6,7,8,9,
	1,2,3,4,5,6,7,8,9,
}


print("star version", star.version(), "\n")

local co = coroutine.create(function ()

	local a = star.call("add", t)
	print("a:", a)

	local b = star.call("add", t)
	print("b:", b)

	local c = star.call("add", {a, b})
	print("c:", c)
end)

coroutine.resume(co)


local d = star.call("add", t)
print("d:", d)

local e = add(t)
print("e:", e)

local f = add(t)
print("f:", f)


-- 陷阱: 如果用 "star.call", call 一个没有返回值的函数, 则该协程将永远挂起
-- 比如: log 函数没有返回值, 用 star.send 就好
star.send("log", "done, byebye")


--[[

框架概述:
	Self introduction:

		star 包含一个main线程, 和多个func线程 (里面的函数 必须是全局的, 无副作用的), 

		全部数据,和大部分逻辑都在 main 线程里执行
		
		当有比较耗时的算法, 或者IO, 我们可以选择放到 func线程里执行

		经过LUA协程包装，跨线程调用和调用一个普通函数没什么区别(回调一边玩去 ^_^)

	Why do it:
	
		为什么写这么一个框架, 因为我觉得其他框架不管在实现,还是使用上,都比较复杂.

		因为它们都是多线程的, 最好用的 还是"单线程"

		star 的设计，使得使用它就像 使用单线程一样简单, 只需要在 复杂算法 和 I/O 前面 加上star.call/send

		就能提升性能, 可以想象 辅助线程(func thread) 和 主线程 (main thread) 的任务量 并不能保证绝对公平
		
		(由开发者决定, 预估), 所以 它的总体性能很难把握(我甚至能接受它比其他框架的性能低), 但是使用它

		肯定是一种享受.

	Will do:
		
		加入socket， timer到内核中去, 让它成为真正的 game-server.

]]