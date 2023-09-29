--@meta

LT_STARTED = 0
LT_EXIT_NORMAL = 1
LT_EXIT_UNEXPECT = 2
LT_SYSCALL_ENTER = 3
LT_SYSCALL_EXIT = 4
LT_SIG_DELIVER = 5
LT_GROUP_STOP = 6
LT_PTRACE_EVENT = 7
LT_EXEC_OCCURED = 8

-- Initialize lua trace
-- @param func The callback function
function LT_init(func) end

-- Read a cstr at given address
-- @param address of the cstr
-- @return the string
function LT_read_cstr(addr) end

-- Format buffer at address as printable string
-- @param buf address of the buffer
-- @param buf_size size of the buffer in bytes
-- @param print_size max size of printable string
-- @return a printable string representing the buffer
function LT_fmt_buffer(buf, buf_size, print_size) end

-- Format cstr at address as printable string
-- @param buf address of the buffer
-- @param print_size max size of printable string
-- @return a printable string representing the buffer
function LT_fmt_cstr(buf, print_size) end
