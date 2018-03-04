--[[




Star版本:
	0.01：base
	0.02: add tcp suport
	0.03: add timer suport and star.sleep api
	will do:
		add udp suport
		add star.xcall api  #并行调用多个star.call
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

		


]]