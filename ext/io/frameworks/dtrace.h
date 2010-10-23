#include <dtrace.h>

#define DtraceErrorMsg(obj) dtrace_errmsg(obj->handle, dtrace_errno(obj->handle))
#define DtraceError(obj, msg) rb_raise(rb_eTraceError, (msg), DtraceErrorMsg(obj))
#define RegisterHandler(func, handler, err_msg) \
    Trace(ret = func(trace->handle, &(handler), (void*)trace)); \
    if (ret == -1) DtraceError(trace, err_msg);

#define CompileStrategy(trace, script) \
   Trace(trace->prog = dtrace_program_strcompile(trace->handle, script, DTRACE_PROBESPEC_NAME, DTRACE_C_CPP, 0, NULL)); \
   if(trace->prog == NULL) \
     rb_raise(rb_eTraceError, "failed to compile '%s': %s", script, DtraceErrorMsg(trace));

#ifdef __APPLE__
#define RuntimeOptions(trace) \
    Trace((void) dtrace_setopt(trace->handle, "bufsize", "4m")); \
    Trace((void) dtrace_setopt(trace->handle, "aggsize", "4m")); \
    Trace((void) dtrace_setopt(trace->handle, "flowindent", 0)); \
    Trace((void) dtrace_setopt(trace->handle, "quiet", 0)); \
    Trace((void) dtrace_setopt(trace->handle, "stacksymbols", "enabled"));
#else
#define RuntimeOptions(trace) \
    Trace((void) dtrace_setopt(trace->handle, "bufsize", "4m")); \
    Trace((void) dtrace_setopt(trace->handle, "aggsize", "4m")); \
    Trace((void) dtrace_setopt(trace->handle, "flowindent", 0)); \
    Trace((void) dtrace_setopt(trace->handle, "quiet", 0));
#endif

#define RunStrategy(trace) \
    Trace(ret = dtrace_program_exec(trace->handle, trace->prog, trace->info)); \
    if (ret == -1) DtraceError(trace, "failed to enable probes"); \
    RuntimeOptions(trace); \

#define StartTrace(trace) \
    Trace(ret = dtrace_go(trace->handle)); \
    if(ret < 0) \
      DtraceError(trace, "could not enable tracing");

#define StopTrace(trace) \
    Trace(ret = dtrace_stop(trace->handle)); \
    if(ret < 0) \
      DtraceError(trace, "could not disable tracing"); \
    Trace(ret = dtrace_aggregate_snap(trace->handle)); \
    if(ret < 0) \
      DtraceError(trace, "could not snapshot aggregates"); \
    Trace(ret = dtrace_aggregate_walk(trace->handle, rb_io_trace_walk, (void*)trace)); \
    if(ret < 0) \
      DtraceError(trace, "could not walk the aggregate snapshot"); \
    Trace(dtrace_aggregate_clear(trace->handle));
