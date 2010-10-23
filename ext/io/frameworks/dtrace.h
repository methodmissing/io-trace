#include <dtrace.h>

#define DtraceErrorMsg(obj) dtrace_errmsg(obj->handle, dtrace_errno(obj->handle))
#define DtraceError(obj, msg) rb_raise(rb_eTraceError, (msg), DtraceErrorMsg(obj))
#define RegisterHandler(func, handler, err_msg) \
    Trace(ret = func(trace->handle, &(handler), (void*)trace)); \
    if (ret == -1) DtraceError(trace, err_msg);
