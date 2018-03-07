--[[

Star版本:
	0.01：base
	0.02: add tcp suport
	0.03: add timer suport and star.sleep api
	0.04: add star.xcall
	will do:
		add udp suport
		...

框架概述:
	Self introduction:

		star 包含一个main线程, 和多个func线程 (里面的函数 必须是全局的, 无副作用的), 

		全部数据,和大部分逻辑都在 main 线程里执行
		
		当有比较耗时的算法, 或者IO, 我们可以选择放到 func线程里执行

		经过LUA协程包装，跨线程调用和调用一个普通函数没什么区别(回调一边玩去 ^_^)

	Why do it:
	
		为什么写这么一个框架, 因为我觉得其他框架不管在实现,还是使用上,都比较复杂.

		因为它们都是多线程的, 最好用的 还是"单线程"

		star 的设计，使得使用它就像 使用单线程一样简单, 只需要在 复杂算法 和 I/O 前面

		加上star.call/send 就能提升性能, Now just enjoy it.


Api:
	1.star.time() 		# (单位为毫秒)

	
	2.star.sleep(ti)	# (单位为毫秒)


	3.star.version()    # 返回 Star 版本号


	4.star.server()     #设置socket消息回调, 详见 main.lua


	4.star.call([thread_index,] func_name, ...)
		示例:
			local result = star.call("add", 3, 4)
		说明:
			进行一次跨线程调用(main thread call func thread), 示例中 add函数定义在 func.lua中,
			该函数必须有返回值, 否则当前协程将永久挂起


	5.star.send([thread_index,] func_name, ...)
		示例:
			star.send("log", "Server start at: 2018/3/7 10:55:02")
		说明:
			进行一次跨线程调用, 该函数必须没有返回值
			call/send, 最开始都有个可选参数 thread_index, 我们可以指定一个大于等于0的数字进去, 
			来指定一条确定的 func thread 来处理请求, 来满足特殊需求


	6. star.xcall([n,] taskList)
		示例:
			local a, b = star.xcall{
				{"add", 1, 2},
				{"add", 3, 4}
			}

		说明1: star.call的 加强版本, 我们可以将多条star.call 打包在一起, 传给star.xcall
			
			-- call 版本
			local a = star.call("add", 1, 2)
			local b = star.call("add", 3, 4)
			
			star.call 版本是串行的(第一次call 返回后才执行下一次call),
			star.xcall 版本则可以将请求平均分配给 多条func thread 去同时处理请求
			从而缩短本次的计算时间(从而缩短客户端单次请求的响应时间)

		说明2: 可选参数 数字n, 有时可能taskList非常长, 返回值很多的时候我们希望xcall给我们打包成一个表后返回
			这时我们可以这样做, local result = star.xcall(1, taskList) # n:任意数字都可以, 效果是一样的 


	7. sock.send(fd, data)
		示例:
			sock.send(5, "Hi client")
		说明:
			发送socket数据


	8. timer.timeout(delay, callback [,iterations])
		示例:
			local id = timer.timeout(1000, function()
				print("timeout")
			end)
		说明:
			delay: 		延迟执行时间, 单位为毫秒
			callback: 	回调
			iterations: 可选参数, 执行次数, 默认为1, 若传入 <= 0 则为无限次数


	9. timer.cancel(timer_id)
		说明:
			取消一个定时器, timer_id 是 timer.timeout 的返回值


	Last: 参阅 "conf.lua" -> "func.lua" -> "main.lua" and Test, See you next version. :)
]]	