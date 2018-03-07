## Star

Star is a lightweight multithreaded concurrent framework for Lua.

## Build

For Linux:

```
git clone https://github.com/HYbutterfly/star.git
cd star
make
```

## Test

Run these:

```
./star conf.lua
```

## About Lua version

Star now uses lua 5.3.4

## Process Architecture

```txt
+--------------------------------------------------+
|                                                  |
|               Lua Game Application               |
|                                                  |
+--------------------------------------------------+
|                   C Driver                       |
|                                                  |
|               star / sock / timer          	   |
|                                                  |
+--------------------------------------------------+ 
|                                                  |
| +-------------------------------------------+    |  single thread   +---------+      
| |          open / data / close        <<---------------------------<<  Sock   <<------<< Client
| |                                           |    |                  +---------+      
| |          sleep / awake  --------+         |    |
| |                                  \        |    |  single thread   +---------+      +-- Sleep Coroutine List
| |           (" MAIN THREAD ")       + <<--------------------------->>  Timer  +------| 
| |                                  /        |    |                  +---------+      +-- Timer List
| |          timer new / cancel-----+         |    |
| |                                           |    |  multi  thread   +---------+      +-- func1
| |          call / xcall / send        <<--------------------------->>  Func   +------|   
| +-------------------------------------------+    |                  +---------+      +-- func2
|                                                  |
+--------------------------------------------------+
```

## How To Use

* contact my QQ: 707298413