#include <ruby.h>
#include <dtrace.h>
#include "probes.h"

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
#else
#define Trace(wrap) wrap
#endif

typedef struct {
    dtrace_hdl_t *handle;
    dtrace_prog_t *prog;
    dtrace_proginfo_t *info;
    VALUE aggregations;
    VALUE stream;
    VALUE strategy;
} io_trace_t;

typedef struct {
    char* probe;
    char* feature;
    int fd;
    char* file;
    int line;
    uint64_t value;
} io_trace_aggregation_t;

ID id_new, id_call;
VALUE rb_cTrace, rb_eTraceError, rb_cTraceAggregation;

#define GetIOTracer(obj) (Check_Type(obj, T_DATA), (io_trace_t*)DATA_PTR(obj))
#define GetIOTracerAggregation(obj) (Check_Type(obj, T_DATA), (io_trace_aggregation_t*)DATA_PTR(obj))
#define DtraceErrorMsg(obj) dtrace_errmsg(obj->handle, dtrace_errno(obj->handle))
#define DtraceError(obj, msg) rb_raise(rb_eTraceError, (msg), DtraceErrorMsg(obj))
#define TraceError(msg) rb_raise(rb_eTraceError, (msg))
#define BlockRequired() if (!rb_block_given_p()) rb_raise(rb_eArgError, "block required!")
#define CoerceFromHash(obj, macro) \
    if(!NIL_P(obj = rb_hash_aref(values, ID2SYM(rb_intern(#obj))))) \
      a->obj = macro(obj);
#define InspectAggregation(val, fmt) \
    len = snprintf(buf, 0, (fmt), a->file, a->line, a->probe, a->fd, a->feature, (val)); \
    buf = ALLOC_N(char, len + 1); \
    snprintf(buf, len + 1, (fmt), a->file, a->line, a->probe, a->fd, a->feature, (val));
#define AggregationTypeP(agg) (strcmp(a->feature, agg) == 0) ? Qtrue : Qfalse;
#define RegisterHandler(func, handler, err_msg) \
    Trace(ret = func(trace->handle, &(handler), (void*)trace)); \
    if (ret == -1) \
       DtraceError(trace, err_msg);

#define IO_TRACE_SUMMARY 1
#define IO_TRACE_ALL 2
#define IO_TRACE_READ 3
#define IO_TRACE_WRITE 4
#define IO_TRACE_SETUP 5

/* Not exposing scripts to users in the interim */

static char* summary_script =
    "syscall::read*:entry,"
    "syscall::pread*:entry,"
    "syscall::readv*:entry,"
    "syscall::recv*:entry,"
    "syscall::write*:entry,"
    "syscall::pwrite*:entry,"
    "syscall::writev*:entry,"
    "syscall::send*:entry,"
    "syscall::bind*:entry,"
    "syscall::socketpair*:entry,"
    "syscall::getpeername*:entry,"
    "syscall::shutdown*:entry,"
    "syscall::open*:entry,"
    "syscall::stat*:entry,"
    "syscall::lstat*:entry"
    "/pid == $pid/"
    "{"
    "  self->ts = timestamp;"
    "  self->vts = vtimestamp;"
    "  @calls[probefunc,\"(all)\",0,-1] = count();"
    "}"
    "syscall::read*:return,"
    "syscall::pread*:return,"
    "syscall::readv*:return,"
    "syscall::recv*:return,"
    "syscall::write*:return,"
    "syscall::pwrite*:return,"
    "syscall::writev*:return,"
    "syscall::send*:return"
    "/pid == $pid && self->ts/"
    "{"
    "  this->elapsed = timestamp - self->ts;"
    "  this->cpu = vtimestamp - self->vts;"
    "  @cpu[probefunc,\"(all)\",0,-1] = sum(this->cpu);"
    "  @time[probefunc,\"(all)\",0,-1] = sum(this->elapsed);"
    "  this->elapsed = 0;"
    "  this->cpu = 0;"
    "  self->ts = 0;"
    "  self->vts = 0;"
    "  @bytes[probefunc,\"(all)\",0,-1] = sum(arg0);"
    "}"
    "syscall::bind*:return,"
    "syscall::socketpair*:return,"
    "syscall::getpeername*:return,"
    "syscall::shutdown*:return,"
    "syscall::open*:return,"
    "syscall::close*:return,"
    "syscall::stat*:return,"
    "syscall::lstat*:return"
    "/pid == $pid/"
    "{"
    "  this->elapsed = timestamp - self->ts;"
    "  this->cpu = vtimestamp - self->vts;"
    "  @cpu[probefunc,\"(all)\",0,-1] = sum(this->cpu);"
    "  @time[probefunc,\"(all)\",0,-1] = sum(this->elapsed);"
    "  this->elapsed = 0;"
    "  this->cpu = 0;"
    "  self->ts = 0;"
    "  self->vts = 0;"
    "  self->line = 0;"
    "  self->file = 0;"
    "}";

static char* read_script =
    "ruby*:::line"
    "/pid == $pid/"
    "{"
    "  self->file = copyinstr(arg0);"
    "  self->line = arg1;"
    "}"
    "syscall::read*:entry,"
    "syscall::pread*:entry,"
    "syscall::readv*:entry,"
    "syscall::recv*:entry"
    "/pid == $pid && self->line/"
    "{"
    "  self->ts = timestamp;"
    "  self->vts = vtimestamp;"
    "  self->fd = arg0;"
    "  @calls[probefunc,self->file,self->line,self->fd] = count();"
    "}"
    "syscall::read*:return,"
    "syscall::pread*:return,"
    "syscall::readv*:return,"
    "syscall::recv*:return"
    "/pid == $pid && self->line && self->ts/"
    "{"
    "  this->elapsed = timestamp - self->ts;"
    "  this->cpu = vtimestamp - self->vts;"
    "  @cpu[probefunc,self->file,self->line,self->fd] = sum(this->cpu);"
    "  @time[probefunc,self->file,self->line,self->fd] = sum(this->elapsed);"
    "  @bytes[probefunc,self->file,self->line,self->fd] = sum(arg0);"
    "  this->elapsed = 0;"
    "  this->cpu = 0;"
    "  self->ts = 0;"
    "  self->vts = 0;"
    "  self->line = 0;"
    "  self->file = 0;"
    "}";

static char* write_script =
    "ruby*:::line"
    "/pid == $pid/"
    "{"
    "  self->file = copyinstr(arg0);"
    "  self->line = arg1;"
    "}"
    "syscall::write*:entry,"
    "syscall::pwrite*:entry,"
    "syscall::writev*:entry,"
    "syscall::send*:entry"
    "/pid == $pid && self->line/"
    "{"
    "  self->ts = timestamp;"
    "  self->vts = vtimestamp;"
    "  self->fd = arg0;"
    "  @calls[probefunc,self->file,self->line,self->fd] = count();"
    "}"
    "syscall::write*:return,"
    "syscall::pwrite*:return,"
    "syscall::writev*:return,"
    "syscall::send*:return"
    "/pid == $pid && self->line && self->ts/"
    "{"
    "  this->elapsed = timestamp - self->ts;"
    "  this->cpu = vtimestamp - self->vts;"
    "  @cpu[probefunc,self->file,self->line,self->fd] = sum(this->cpu);"
    "  @time[probefunc,self->file,self->line,self->fd] = sum(this->elapsed);"
    "  @bytes[probefunc,self->file,self->line,self->fd] = sum(arg0);"
    "  this->elapsed = 0;"
    "  this->cpu = 0;"
    "  self->ts = 0;"
    "  self->vts = 0;"
    "  self->line = 0;"
    "  self->file = 0;"
    "}";

static char* setup_script =
    "ruby*:::line"
    "/pid == $pid/"
    "{"
    "  self->file = copyinstr(arg0);"
    "  self->line = arg1;"
    "}"
    "syscall::bind*:entry,"
    "syscall::socketpair*:entry,"
    "syscall::getpeername*:entry,"
    "syscall::shutdown*:entry,"
    "syscall::open*:entry,"
    "syscall::stat*:entry,"
    "syscall::lstat*:entry,"
    "syscall::fstat*:entry,"
    "syscall::select*:entry,"
    "syscall::poll*:entry"
    "/pid == $pid && self->line/"
    "{"
    "  self->ts = timestamp;"
    "  self->vts = vtimestamp;"
    "  self->fd = -1;"
    "  @calls[probefunc,self->file,self->line,self->fd] = count();"
    "}"
    "syscall::close*:entry,"
    "syscall::connect*:entry,"
    "syscall::fsync*:entry,"
    "syscall::accept*:entry,"
    "syscall::fcntl*:entry"
    "/pid == $pid && self->line/"
    "{"
    "  self->ts = timestamp;"
    "  self->vts = vtimestamp;"
    "  self->fd = arg0;"
    "  @calls[probefunc,self->file,self->line,self->fd] = count();"
    "}"
    "syscall::bind*:return,"
    "syscall::socketpair*:return,"
    "syscall::getpeername*:return,"
    "syscall::shutdown*:return,"
    "syscall::open*:return,"
    "syscall::close*:return,"
    "syscall::stat*:return,"
    "syscall::lstat*:return,"
    "syscall::fstat*:return,"
    "syscall::select*:return,"
    "syscall::poll*:return,"
    "syscall::connect*:return,"
    "syscall::fsync*:return,"
    "syscall::accept*:return,"
    "syscall::fcntl*:return"
    "/pid == $pid && self->line && self->ts/"
    "{"
    "  this->elapsed = timestamp - self->ts;"
    "  this->cpu = vtimestamp - self->vts;"
    "  @cpu[probefunc,self->file,self->line,self->fd] = sum(this->cpu);"
    "  @time[probefunc,self->file,self->line,self->fd] = sum(this->elapsed);"
    "  this->elapsed = 0;"
    "  this->cpu = 0;"
    "  self->ts = 0;"
    "  self->vts = 0;"
    "  self->line = 0;"
    "  self->file = 0;"
    "}";