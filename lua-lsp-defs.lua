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
