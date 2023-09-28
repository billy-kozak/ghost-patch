-------------------------------------------------------------------------------
-- Copyright (C) 2023  Billy Kozak                                           --
--                                                                           --
-- This file is part of the ghost-patch program                              --
--                                                                           --
-- This program is free software: you can redistribute it and/or modify      --
-- it under the terms of the GNU Lesser General Public License as published  --
-- by the Free Software Foundation, either version 3 of the License, or      --
-- (at your option) any later version.                                       --
--                                                                           --
-- This program is distributed in the hope that it will be useful,           --
-- but WITHOUT ANY WARRANTY; without even the implied warranty of            --
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             --
-- GNU Lesser General Public License for more details.                       --
--                                                                           --
-- You should have received a copy of the GNU Lesser General Public License  --
-- along with this program.  If not, see <http://www.gnu.org/licenses/>.     --
-------------------------------------------------------------------------------

SYS_read = 0
SYS_write = 1
SYS_open = 2
SYS_close = 3

local function syscall_no(uregs)
	return uregs.orig_rax
end

local function syscall_retval(uregs)
	return uregs.rax
end

local function syscall_arg(uregs, arg)
	if arg == 0 then
		return uregs.rdi
	elseif arg == 1 then
		return uregs.rsi
	elseif arg == 2 then
		return uregs.rdx
	elseif arg == 3 then
		return uregs.r10
	elseif arg == 4 then
		return uregs.r8
	elseif arg == 5 then
		return uregs.r9
	end
end

local function format_syscall(pid, syscall, ret, ...)
	local args={...}

	local alist

	if #(args) ~= 0 then
		alist = tostring(args[1])
	else
		alist = ""
	end

	for i=2, #(args) do
		alist = alist .. ", " .. args[i]
	end

	return (
		"[ID: " .. pid .. "] " ..
		syscall .. "(" .. alist .. ") = " .. ret
	)
end

local function printf_syscall(pid, syscall, ret, ...)
	local args = {...}

	print(format_syscall(pid, syscall, ret, table.unpack(args)))
end

local function print_syscall(pid, uregs)

	local no = syscall_no(uregs)
	local ret = syscall_retval(uregs)

	if no == SYS_read then
	elseif no == SYS_write then
	elseif no == SYS_open then
	elseif no == SYS_close then
		printf_syscall(pid, "close", ret, syscall_arg(uregs, 0))
	else
		print("[ID: "..pid.."] syscall("..no..", ...) = "..ret)
	end
end

local function handle_ev(ev, pid, uregs)
	if ev == LT_SYSCALL_EXIT then
		print_syscall(pid, uregs)
	 end
end

LT_init(handle_ev)
