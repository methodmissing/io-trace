#include <dtrace.h>

typedef struct {
    dtrace_hdl_t *handle;
    dtrace_prog_t *prog;
    dtrace_proginfo_t *info;
} framework_t;

#define DtraceErrorMsg(obj) dtrace_errmsg((*obj->framework).handle, dtrace_errno((*obj->framework).handle))
#define DtraceError(obj, msg) rb_raise(rb_eTraceError, (msg), DtraceErrorMsg(obj))
#define RegisterHandler(func, handler, err_msg) \
    Trace(ret = func((*trace->framework).handle, &(handler), (void*)trace)); \
    if (ret == -1) DtraceError(trace, err_msg);

#define InitFramework(trace) \
   framework_t* fw; \
   fw = ALLOC(framework_t); \
   fw->prog = NULL; \
   fw->info = NULL; \
   Trace(fw->handle = dtrace_open(DTRACE_VERSION, 0, &err)); \
   if (fw->handle == NULL) \
     rb_raise(rb_eTraceError, "Cannot open dtrace library: %s\n", dtrace_errmsg(NULL, err)); \
   trace->framework = fw; \
   ts->closed = 0;

#define RegisterHandlers(trace) \
    RegisterHandler(dtrace_handle_err, rb_io_trace_errhandler, "failed to establish error handler"); \
    RegisterHandler(dtrace_handle_drop, rb_io_trace_drophandler, "failed to establish drop handler"); \
    RegisterHandler(dtrace_handle_proc, rb_io_trace_prochandler, "failed to establish proc handler"); \
    RegisterHandler(dtrace_handle_setopt, rb_io_trace_setopthandler, "failed to establish setopt handler"); \
    RegisterHandler(dtrace_handle_buffered, rb_io_trace_bufhandler, "failed to establish buffered handler");

#define CompileStrategy(trace, script) \
   Trace((*trace->framework).prog = dtrace_program_strcompile((*trace->framework).handle, script, DTRACE_PROBESPEC_NAME, DTRACE_C_CPP, 0, NULL)); \
   if((*trace->framework).prog == NULL) \
     rb_raise(rb_eTraceError, "failed to compile '%s': %s", script, DtraceErrorMsg(trace));

#ifdef __APPLE__
#define RuntimeOptions(trace) \
    Trace((void) dtrace_setopt((*trace->framework).handle, "bufsize", "4m")); \
    Trace((void) dtrace_setopt((*trace->framework).handle, "aggsize", "4m")); \
    Trace((void) dtrace_setopt((*trace->framework).handle, "flowindent", 0)); \
    Trace((void) dtrace_setopt((*trace->framework).handle, "quiet", 0)); \
    Trace((void) dtrace_setopt((*trace->framework).handle, "stacksymbols", "enabled"));
#else
#define RuntimeOptions(trace) \
    Trace((void) dtrace_setopt((*trace->framework).handle, "bufsize", "4m")); \
    Trace((void) dtrace_setopt((*trace->framework).handle, "aggsize", "4m")); \
    Trace((void) dtrace_setopt((*trace->framework).handle, "flowindent", 0)); \
    Trace((void) dtrace_setopt((*trace->framework).handle, "quiet", 0));
#endif

#define RunStrategy(trace) \
    Trace(ret = dtrace_program_exec((*trace->framework).handle, (*trace->framework).prog, (*trace->framework).info)); \
    if (ret == -1) DtraceError(trace, "failed to enable probes"); \
    RuntimeOptions(trace); \

#define StartTrace(trace) \
    Trace(ret = dtrace_go((*trace->framework).handle)); \
    if(ret < 0) \
      DtraceError(trace, "could not enable tracing");

#define StopTrace(trace) \
    Trace(ret = dtrace_stop((*trace->framework).handle)); \
    if(ret < 0) \
      DtraceError(trace, "could not disable tracing"); \
    Trace(ret = dtrace_aggregate_snap((*trace->framework).handle)); \
    if(ret < 0) \
      DtraceError(trace, "could not snapshot aggregates"); \
    Trace(ret = dtrace_aggregate_walk((*trace->framework).handle, rb_io_trace_walk, (void*)trace)); \
    if(ret < 0) \
      DtraceError(trace, "could not walk the aggregate snapshot"); \
    Trace(dtrace_aggregate_clear((*trace->framework).handle));
