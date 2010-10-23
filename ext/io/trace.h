#include <ruby.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "probes.h"
#ifdef HAVE_DTRACE
#include "frameworks/dtrace.h"
#endif

#ifndef RUBY_VM
#include <rubysig.h>
#include <node.h>
#include <env.h>
typedef rb_event_t rb_event_flag_t;
#define rb_sourcefile() (ruby_current_node ? ruby_current_node->nd_file : 0)
#define rb_sourceline() (ruby_current_node ? nd_line(ruby_current_node) : 0)
/* Handle potentially blocking syscalls */
#define Trace(wrap) \
    TRAP_BEG; \
    (wrap); \
    TRAP_END;
#ifndef RSTRING_PTR
#define RSTRING_PTR(obj) RSTRING(obj)->ptr
#endif

#ifndef RSTRING_LEN
#define RSTRING_LEN(obj) RSTRING(obj)->len
#endif
#else
#define Trace(wrap) wrap
#endif

typedef struct {
    framework_t *framework;
    short int closed;
    VALUE aggregations;
    VALUE stream;
    VALUE formatter;
    VALUE strategy;
} io_trace_t;

typedef struct {
    char *syscall;
    char *metric;
    char *file;
    int fd;
    int line;
    uint64_t value;
    short int fmt_as_time;
} io_trace_aggregation_t;

ID id_new, id_call, id_formatters, id_default;
VALUE rb_cTrace, rb_eTraceError, rb_cTraceAggregation;
static FILE *devnull;

#define GetIOTracer(obj) (Check_Type(obj, T_DATA), (io_trace_t*)DATA_PTR(obj))
#define GetIOTracerAggregation(obj) (Check_Type(obj, T_DATA), (io_trace_aggregation_t*)DATA_PTR(obj))
#define TraceError(msg) rb_raise(rb_eTraceError, (msg))
#define BlockRequired() if (!rb_block_given_p()) rb_raise(rb_eArgError, "block required!")
#define CoerceStringFromHash(obj) \
    if(!NIL_P(obj = rb_hash_aref(values, ID2SYM(rb_intern(#obj))))){ \
      a->obj = ALLOC_N(char, strlen(RSTRING_PTR(obj))); \
      if (!a->obj) TraceError("unable to allocate a buffer"); \
      strcpy(a->obj, RSTRING_PTR(obj)); \
    }
#define CoerceNumericFromHash(obj) \
    if(!NIL_P(obj = rb_hash_aref(values, ID2SYM(rb_intern(#obj))))) \
       a->obj = NUM2INT(obj);
#define InspectAggregation(val, fmt) \
    file = a->file; \
    if((len = strlen(file)) >= 40) sprintf(file, "%.*s", 40, &file[len-40]); \
    len = fprintf(devnull, (fmt), file, a->line, a->syscall, a->fd, a->metric, (val)) + 1; \
    buf = ALLOC_N(char, len); \
    len = sprintf(buf, (fmt), file, a->line, a->syscall, a->fd, a->metric, (val));
#define AggregationTypeP(agg) (strcmp(a->metric, agg) == 0) ? Qtrue : Qfalse;

#define IO_TRACE_SUMMARY 1
#define IO_TRACE_ALL 2
#define IO_TRACE_READ 3
#define IO_TRACE_WRITE 4
#define IO_TRACE_SETUP 5

#include "scripts.h"