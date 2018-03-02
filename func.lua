local sock = require "star.sock"

function add( a, b )
	return a + b
end

function foo()
	print("i'm foo() in func.lua")
end

-- 仅仅测试用的8皇后算法, 不用看
function eight_queens(...)
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



function log( ... )
	print("Log:", ...)
end

function send2client(fd, data)
	sock.send(fd, data)
end
