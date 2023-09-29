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
PRINT_SIZE = 64

SYS_read = 0
SYS_write = 1
SYS_open = 2
SYS_close = 3
SYS_mmap = 9
SYS_munmap = 11
SYS_fadvise64 = 221
SYS_clock_nanosleep = 230
SYS_openat = 257
SYS_newfstat = 262

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

local function format_addr(addr)
	if addr == 0 then
		return "NULL"
	else
		return string.format("0x%x", addr)
	end
end

local function printf_syscall(pid, syscall, ret, ...)
	local args = {...}

	print(format_syscall(pid, syscall, ret, table.unpack(args)))
end

local function print_sys_write(pid, ret, uregs)
	local len = syscall_arg(uregs, 2)
	printf_syscall(
		pid,
		"write",
		ret,
		syscall_arg(uregs, 0),
		LT_fmt_buffer(syscall_arg(uregs, 1), len, PRINT_SIZE),
		len
	)
end

local function print_sys_close(pid, ret, uregs)
	printf_syscall(pid, "close", ret, syscall_arg(uregs, 0))
end

local function print_sys_clock_nanosleep(pid, ret, uregs)
	printf_syscall(
			pid,
			"clock_nanosleep",
			ret,
			syscall_arg(uregs, 0),
			syscall_arg(uregs, 1),
			format_addr(syscall_arg(uregs, 2)),
			format_addr(syscall_arg(uregs, 3))
	)
end

local function print_sys_read(pid, ret, uregs)

	local addr = syscall_arg(uregs, 1)
	local len = syscall_arg(uregs, 2)

	printf_syscall(
		pid,
		"read",
		ret.." ("..LT_fmt_buffer(addr, ret, PRINT_SIZE)..")",
		syscall_arg(uregs, 0),
		format_addr(addr),
		len
	)
end

local function print_sys_open(pid, ret, uregs)
	printf_syscall(
		pid,
		"open",
		ret,
		LT_fmt_cstr(syscall_arg(uregs, 0), PRINT_SIZE),
		syscall_arg(uregs, 1),
		syscall_arg(uregs, 2)
	)
end

local function print_sys_mmap(pid, ret, uregs)
	printf_syscall(
		pid,
		"mmap",
		format_addr(ret),
		format_addr(syscall_arg(uregs, 0)),
		syscall_arg(uregs, 1),
		syscall_arg(uregs, 2),
		syscall_arg(uregs, 3),
		syscall_arg(uregs, 4),
		syscall_arg(uregs, 5)
	)
end

local function print_sys_openat(pid, ret, uregs)
	printf_syscall(
		pid,
		"openat",
		ret,
		syscall_arg(uregs, 0),
		LT_fmt_cstr(syscall_arg(uregs, 1), PRINT_SIZE),
		syscall_arg(uregs, 2),
		syscall_arg(uregs, 3)
	)
end

local function print_sys_newfstatat(pid, ret, uregs)
	printf_syscall(
		pid,
		"newfstatat",
		ret,
		syscall_arg(uregs, 0),
		LT_fmt_cstr(syscall_arg(uregs, 1), PRINT_SIZE),
		format_addr(syscall_arg(uregs, 2))
	)
end

local function print_sys_fadvise64(pid, ret, uregs)
	printf_syscall(
		pid,
		"fadvise64",
		ret,
		syscall_arg(uregs, 0),
		syscall_arg(uregs, 1),
		syscall_arg(uregs, 2),
		syscall_arg(uregs, 3)
	)
end

local function print_sys_munmap(pid, ret, uregs)
	printf_syscall(
		pid,
		"munmap",
		ret,
		format_addr(syscall_arg(uregs, 0)),
		syscall_arg(uregs, 1)
	)
end

local syscall_print_tbl = {
	[SYS_read] = print_sys_read,
	[SYS_write] = print_sys_write,
	[SYS_open] = print_sys_open,
	[SYS_close] = print_sys_close,
	[SYS_mmap] = print_sys_mmap,
	[SYS_munmap] = print_sys_munmap,
	[SYS_fadvise64] = print_sys_fadvise64,
	[SYS_clock_nanosleep] = print_sys_clock_nanosleep,
	[SYS_openat] = print_sys_openat,
	[SYS_newfstat] = print_sys_newfstatat
}

local function print_syscall(pid, uregs)

	local no = syscall_no(uregs)
	local ret = syscall_retval(uregs)

	local print_func = syscall_print_tbl[no]

	if print_func == nil then
		print("[ID: "..pid.."] syscall("..no..", ...) = "..ret)
	else
		print_func(pid, ret, uregs)
	end
end

local function handle_ev(ev, pid, uregs)
	if ev == LT_SYSCALL_EXIT then
		print_syscall(pid, uregs)
	 end
end

LT_init(handle_ev)
