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

	并发模型和框架有很多 比如 actor模型(skynet), csp模型(golang)
	
	star 则属于 "函数式并发"

		star 包含一个main线程, 和多个func线程 (里面的函数 必须是全局的, 无副作用的), 

		全部数据,和大部分逻辑都在 main 线程里执行
		
		当有比较耗时的算法, 或者IO, 我们可以选择放到 func线程里执行

		经过LUA协程包装，跨线程调用和调用一个普通函数没什么区别(回调一边玩去 ^_^)

并发模型对比:
	本人做过几个月的skynet开发, golang 也仅仅是完成了官方的基本教程, 现在列出本人的看法, 可能不太准确

	actor模型:
		1.首先是序列化的问题, actor 之间通信 必须序列化 反序列化
		2.同步问题, skynet 的假并发 曾经让我头疼不已

	csp模型:
		1.还是序列化, channel 的发送者 接收者 即使在同一线程 也要序列化.
		2.协程间 相互调用 还得自己去实现一套协议(通过channel)，还不如 actor 呢
	
	函数式:
		为什么上面2种 我认为不管在实现,还是使用上,都比较复杂.
		因为它们都是多线程的, 最好用的 还是"单线程"

		star 的设计，使得使用它就像 使用单线程一样简单, 只需要在 复杂算法 和 I/O 前面 加上star.call/send

		就能提升性能, 可以想象 辅助线程(func thread) 和 主线程 (main thread) 的任务量 并不能保证绝对公平
		
		(由开发者决定, 预估), 所以 函数式模型的性能很难把握(我甚至能接受它比其他框架的性能低), 但是使用它

		肯定是一种享受.

]]