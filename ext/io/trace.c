#include "trace.h"

/*
 * dtrace proc handler / callback
*/
static void
rb_io_trace_prochandler(struct ps_prochandle *p, const char *msg, void *arg)
{
}

/*
 * dtrace error handler / callback
*/
static int
rb_io_trace_errhandler(const dtrace_errdata_t *data, void *arg)
{
    return (DTRACE_HANDLE_OK);
}

/*
 * dtrace drop handler / callback
*/
static int
rb_io_trace_drophandler(const dtrace_dropdata_t *data, void *arg)
{
    return (DTRACE_HANDLE_OK);
}

/*
 * dtrace setopt handler / callback
*/
static int
rb_io_trace_setopthandler(const dtrace_setoptdata_t *data, void *arg)
{
    return (DTRACE_HANDLE_OK);
}

/*
 * dtrace buffer handler / callback
*/
static int
rb_io_trace_bufhandler(const dtrace_bufdata_t *bufdata, void *arg)
{
    return (DTRACE_HANDLE_OK);
}

/*
 * Mark IO::Trace::Aggregation instances during GC
*/
static void rb_io_trace_mark_aggregation(io_trace_aggregation_t* r)
{
}

/*
 * Free IO::Trace::Aggregation instances during GC
*/
static void rb_io_trace_free_aggregation(io_trace_aggregation_t* a)
{
    if (a){
      xfree(a->syscall);
      xfree(a->metric);
      xfree(a->file);
      xfree(a);
    }
}

/*
 * Setup a blank io_trace_aggregation_t struct
*/
static void
rb_io_trace_init_aggregation(io_trace_aggregation_t* as)
{
    as->syscall = NULL;
    as->metric = NULL;
    as->file = NULL;
    as->fd = 0;
    as->line = 0;
    as->value = 0;
}

/*
 * Wraps a io_trace_aggregation_t struct as a Ruby object
*/
static VALUE
rb_io_trace_aggregation_wrap(io_trace_aggregation_t* as)
{
    return Data_Wrap_Struct(rb_cTraceAggregation, rb_io_trace_mark_aggregation, rb_io_trace_free_aggregation, as);
}

/*
 * IO::Trace::Aggregation#inspect => string
 *
 * Default #to_s and #inspect implementation for aggregate instances.May not always be
 * well formed.
*/
static VALUE
rb_io_trace_aggregation_inspect(VALUE obj)
{
    io_trace_aggregation_t* a = GetIOTracerAggregation(obj);
    size_t len;
    char* buf;
    bzero(buf, 0);
    if (strcmp(a->metric, "cpu") == 0 || strcmp(a->metric, "time") == 0){
      InspectAggregation(((double)a->value / 1000000), "%-40.40s %-7d %-18s %-4d %-10s %f ms\n");
    }else{
      InspectAggregation((long int)a->value, "%-40.40s %-7d %-18s %-4d %-10s %ld\n");
    }
    return rb_str_new2(buf);
}

/*
 * IO::Trace::Aggregation#syscall => string
 *
 * Probe / syscall this aggregation represents
*/
static VALUE
rb_io_trace_aggregation_syscall(VALUE obj)
{
    io_trace_aggregation_t* a = GetIOTracerAggregation(obj);
    return (a->syscall == NULL) ? Qnil : rb_str_new2(a->syscall);
}

/*
 * IO::Trace::Aggregation#metric => string
 *
 * Feature / dimension this aggregation represents
*/
static VALUE
rb_io_trace_aggregation_metric(VALUE obj)
{
    io_trace_aggregation_t* a = GetIOTracerAggregation(obj);
    return (a->metric == NULL) ? Qnil : rb_str_new2(a->metric);
}

/*
 * IO::Trace::Aggregation#file => string
 *
 * Source file this aggregation represents
*/
static VALUE
rb_io_trace_aggregation_file(VALUE obj)
{
    io_trace_aggregation_t* a = GetIOTracerAggregation(obj);
    return (a->file == NULL) ? Qnil : rb_str_new2(a->file);
}

/*
 * IO::Trace::Aggregation#fd => numeric
 *
 * File descriptor this aggregation represents
*/
static VALUE
rb_io_trace_aggregation_fd(VALUE obj)
{
    io_trace_aggregation_t* a = GetIOTracerAggregation(obj);
    return (a->fd == 0) ? Qnil : INT2NUM(a->fd);
}

/*
 * IO::Trace::Aggregation#line => numeric
 *
 * Line number this aggregation represents
*/
static VALUE
rb_io_trace_aggregation_line(VALUE obj)
{
    io_trace_aggregation_t* a = GetIOTracerAggregation(obj);
    return (a->line == 0) ? Qnil : INT2NUM(a->line);
}

/*
 * Converts a D time metric to a Ruby float in ms
*/
static VALUE
rb_io_convert_time(uint64_t t)
{
    return rb_float_new((double) t / 1000000);
}

/*
 * IO::Trace::Aggregation#value => numeric
 *
 * Feature / dimension value for this aggregation.
 * Time and cpu is special cased to coerce to floats - everything else is coerced to Numeric
*/
static VALUE
rb_io_trace_aggregation_value(VALUE obj)
{
    io_trace_aggregation_t* a = GetIOTracerAggregation(obj);
    if (a->value == 0) return INT2NUM(a->value);
    if (strcmp(a->metric, "cpu") == 0 || strcmp(a->metric, "time") == 0){
      return rb_io_convert_time(a->value);
    }else{
      return INT2NUM(a->value);
    }
}

/*
 * IO::Trace::Aggregation#bytes? => boolean
 *
 * Predicate method for a bytes dimension
*/
static VALUE
rb_io_trace_aggregation_bytes_p(VALUE obj)
{
    io_trace_aggregation_t* a = GetIOTracerAggregation(obj);
    return AggregationTypeP("bytes");
}

/*
 * IO::Trace::Aggregation#calls? => boolean
 *
 * Predicate method for a calls / invocations dimension
*/
static VALUE
rb_io_trace_aggregation_calls_p(VALUE obj)
{
    io_trace_aggregation_t* a = GetIOTracerAggregation(obj);
    return AggregationTypeP("calls");
}

/*
 * IO::Trace::Aggregation#cpu? => boolean
 *
 * Predicate method for a cpu time dimension
*/
static VALUE
rb_io_trace_aggregation_cpu_p(VALUE obj)
{
    io_trace_aggregation_t* a = GetIOTracerAggregation(obj);
    return AggregationTypeP("cpu");
}

/*
 * IO::Trace::Aggregation#time? => boolean
 *
 * Predicate method for a time dimension
*/
static VALUE
rb_io_trace_aggregation_time_p(VALUE obj)
{
    io_trace_aggregation_t* a = GetIOTracerAggregation(obj);
    return AggregationTypeP("time");
}

/*
 * Release / close the dtrace framework handle
*/
static void 
rb_io_trace_close_handle(io_trace_t* t)
{
    if(t->closed == 0){
      switch(dtrace_status(t->handle)){
        case DTRACE_STATUS_OKAY:
        case DTRACE_STATUS_FILLED:
        case DTRACE_STATUS_STOPPED:
             Trace(dtrace_close(t->handle));
             t->closed = 1;
             break;
      }
    }
}

/*
 * Mark IO::Trace instances during GC
*/
static void
rb_io_trace_mark(io_trace_t* t)
{
    if(t){
      rb_gc_mark(t->stream);
      rb_gc_mark(t->strategy);
      rb_gc_mark(t->formatter);
      rb_gc_mark(t->aggregations);
    }
}

/*
 * Free IO::Trace instances during GC
*/
static void 
rb_io_trace_free(io_trace_t* t)
{
    if(t){
      rb_io_trace_close_handle(t);
      xfree(t);
    }
}

/*
 * Allocator for IO::Trace
 *
 * Wraps an io_trace_t struct, opens a dtrace framework handle and initializes an empty
 * aggregation buffer
*/
static VALUE
rb_io_trace_alloc(VALUE obj)
{
   VALUE trace;
   int err;
   io_trace_t* ts;
   trace = Data_Make_Struct(obj, io_trace_t, rb_io_trace_mark, rb_io_trace_free, ts);
   ts->stream = Qnil;
   ts->formatter = Qnil;
   ts->strategy = Qnil;
   ts->aggregations = Qnil;
   ts->prog = NULL;
   ts->info = NULL;
   ts->closed = 1;
   Trace(ts->handle = dtrace_open(DTRACE_VERSION, 0, &err));
   if (ts->handle == NULL)
     rb_raise(rb_eTraceError, "Cannot open dtrace library: %s\n", dtrace_errmsg(NULL, err));
   ts->closed = 0;
   ts->aggregations = rb_ary_new();
   return trace;
}

/*
 * Allocator for IO::Trace::Aggregation
 *
 * Wraps an io_trace_aggregation_t struct and initialize it to a blank slate
*/
static VALUE
rb_io_trace_aggregation_alloc(VALUE obj)
{
    VALUE aggr;
    io_trace_aggregation_t* as;
    aggr = Data_Make_Struct(obj, io_trace_aggregation_t, rb_io_trace_mark_aggregation, rb_io_trace_free_aggregation, as);
    rb_io_trace_init_aggregation(as);
    return aggr;
}

/*
 * Initializer for IO::Trace::Aggregation
 *
 * Supports building aggregate instances from a symbolized Hash, primarily for testing
*/
static VALUE
rb_io_trace_aggregation_init(int argc, VALUE *argv, VALUE obj)
{
    VALUE values, syscall, metric, fd, file, line, value;
    io_trace_aggregation_t* a = GetIOTracerAggregation(obj);

    rb_scan_args(argc, argv, "01", &values);
    if(!NIL_P(values)){
      Check_Type(values, T_HASH);
      CoerceFromHash(syscall, RSTRING_PTR);
      CoerceFromHash(metric, RSTRING_PTR);
      CoerceFromHash(file, RSTRING_PTR);
      CoerceFromHash(fd, NUM2INT);
      CoerceFromHash(line, NUM2INT);
      CoerceFromHash(value, NUM2INT);
    }
    return obj;
}

/*
 * Initializer for IO::Trace
 *
 * Registers dtrace framework callbacks (no-ops in the interim), compiles and executes
 * the D script for the given tracing strategy (see below).
 *
 * Supports an optional strategy argument, one of :
 *
 * - IO::Trace::SUMMARY default, no FD, file or line specific contexts
 * - IO::Trace::SETUP   connect, open, close, fcntl, stat etc.
 * - IO::Trace::READ    read, receive etc.
 * - IO::Trace::WRITE   write, send etc.
 * - IO::Trace::ALL     combo of SETUP, READ and WRITE
*/
static VALUE
rb_io_trace_init(int argc, VALUE *argv, VALUE obj)
{
    io_trace_t* trace = GetIOTracer(obj);
    char *script;
    size_t len;
    int ret;

    rb_scan_args(argc, argv, "01", &trace->strategy);
    if (NIL_P(trace->strategy)) 
      trace->strategy = rb_const_get(rb_cTrace, rb_intern("SUMMARY"));
    RegisterHandler(dtrace_handle_err, rb_io_trace_errhandler, "failed to establish error handler");
    RegisterHandler(dtrace_handle_drop, rb_io_trace_drophandler, "failed to establish drop handler");
    RegisterHandler(dtrace_handle_proc, rb_io_trace_prochandler, "failed to establish proc handler");
    RegisterHandler(dtrace_handle_setopt, rb_io_trace_setopthandler, "failed to establish setopt handler");
    RegisterHandler(dtrace_handle_buffered, rb_io_trace_bufhandler, "failed to establish buffered handler");

    switch(FIX2INT(trace->strategy)){
      case IO_TRACE_SUMMARY : script = summary_script;
                              break;
      case IO_TRACE_ALL     : len = strlen(read_script) + strlen(write_script) + strlen(setup_script);
                              script = ALLOC_N(char, len + 1);
                              if (!script) TraceError("unable to allocate a script buffer");
                              snprintf(script, len + 1, "%s%s%s", read_script, write_script, setup_script);
                              break;
      case IO_TRACE_READ    : script = read_script;
                              break;
      case IO_TRACE_WRITE   : script = write_script;
                              break;
      case IO_TRACE_SETUP   : script = setup_script;
                              break;
    }

    Trace(trace->prog = dtrace_program_strcompile(trace->handle, script, DTRACE_PROBESPEC_NAME, DTRACE_C_CPP, 0, NULL));

    if(trace->prog == NULL)
      rb_raise(rb_eTraceError, "failed to compile '%s': %s", script, DtraceErrorMsg(trace));

    Trace(ret = dtrace_program_exec(trace->handle, trace->prog, trace->info));
    if (ret == -1)
      DtraceError(trace, "failed to enable probes");

    Trace((void) dtrace_setopt(trace->handle, "bufsize", "4m"));
    Trace((void) dtrace_setopt(trace->handle, "aggsize", "4m"));
    Trace((void) dtrace_setopt(trace->handle, "flowindent", 0));
    Trace((void) dtrace_setopt(trace->handle, "quiet", 0));
#ifdef __APPLE__
    Trace((void) dtrace_setopt(trace->handle, "stacksymbols", "enabled"));
#endif
   return obj;
}

/*
 * IO::Trace#aggregations => array
 *
 * Returns an Array of IO::Trace::Aggregation objects instantiated during tracing
*/
static VALUE
rb_io_trace_aggregations(VALUE obj)
{
    io_trace_t* trace = GetIOTracer(obj);
    return trace->aggregations;
}

/*
 * IO::Trace#close => true
 *
 * Close the allocated dtrace handle and clear the aggregations array.
*/
static VALUE
rb_io_trace_close(VALUE obj)
{
    io_trace_t* trace = GetIOTracer(obj);
    rb_io_trace_close_handle(trace);
    rb_ary_clear(trace->aggregations);
    return Qtrue;
}

/*
 * Callback for walking aggregate data
 *
 * Instantiates IO::Trace::Aggregation objects and appends them to IO::Trace#aggregations
*/
static int
rb_io_trace_walk(const dtrace_aggdata_t *data, void * arg)
{
    io_trace_t* trace = (io_trace_t*)arg;
    dtrace_aggdesc_t *ad = data->dtada_desc;
    dtrace_probedesc_t *pd = data->dtada_pdesc;
    dtrace_recdesc_t *fdr, *fr, *lr, *kr, *dr;
    io_trace_aggregation_t *a;
    a = ALLOC(io_trace_aggregation_t);
    if (!a) TraceError("unable to allocate an aggregation structure");
    rb_io_trace_init_aggregation(a);
    fr = &ad->dtagd_rec[2];
    lr = &ad->dtagd_rec[3];
    fdr = &ad->dtagd_rec[4];
    dr = &ad->dtagd_rec[5];
    a->syscall = ALLOC_N(char, strlen(pd->dtpd_func));
    if (!a->syscall) TraceError("unable to allocate a syscall name buffer");
    strcpy(a->syscall, pd->dtpd_func);
    a->metric = ALLOC_N(char, strlen(ad->dtagd_name));
    if (!a->metric) TraceError("unable to allocate a metricname buffer");
    strcpy(a->metric, ad->dtagd_name);
    a->file = ALLOC_N(char, strlen((data->dtada_data + fr->dtrd_offset)));
    if (!a->file) TraceError("unable to allocate a file name buffer");
    strcpy(a->file, (data->dtada_data + fr->dtrd_offset));
    a->fd = *(int *)(data->dtada_data + fdr->dtrd_offset);
    a->line = *(int *)(data->dtada_data + lr->dtrd_offset);
    a->value = *(uint64_t *)(data->dtada_data + dr->dtrd_offset);
    rb_ary_push(trace->aggregations, rb_io_trace_aggregation_wrap(a));
    return (DTRACE_AGGWALK_NEXT);
}

/*
 * Our MRI event hook
 *
 * Callback for the RUBY_EVENT_LINE interpreter event.This drives the ruby:::line probe
 * which is a predicate for all other instrumentation (except SUMMARY, which is callsite
 * agnostic)
*/
static void
#ifndef RUBY_VM
rb_io_trace_event_hook(rb_event_flag_t event, NODE *node, VALUE self, ID mid, VALUE klass)
#else
rb_io_trace_event_hook(rb_event_flag_t event, VALUE data, VALUE self, ID mid, VALUE klass)
#endif
{
   if(RUBY_LINE_ENABLED()) RUBY_LINE(rb_sourcefile(), rb_sourceline());
}

/*
 * Fetches a formatter defined in Ruby code
 *
 * Expects a IO::Trace::FORMATTERS to be a symbolized Hash.
*/
static VALUE
rb_io_trace_formatter(io_trace_t* trace){
    VALUE formatters, formatter;
    formatters = rb_const_get(rb_cTrace, id_formatters);
    Check_Type(formatters, T_HASH);
    if (NIL_P(trace->formatter)) trace->formatter = ID2SYM(id_default);
    formatter = rb_hash_aref(formatters, trace->formatter);
    if (!rb_respond_to(formatter, id_call)) TraceError("formatter does not respond to #call");
    return formatter;
}

/*
 * IO::Trace#run(stream = nil, formatter = nil)
 *
 * - Enables tracing
 * - Setup an MRI event hook for RUBY_EVENT_LINE
 * - yields the block
 * - Unregister event hook
 * - Stops tracing
 * - Snapshot collected aggregates
 * - Walk collected aggregates
 * - Dump results to a stream via a given formatter (both optional)
 *
 * Stream expects a #<<(data) and formatters a lambda{|stream, lambda| ... } contract
*/
static VALUE
rb_io_trace_run(int argc, VALUE *argv, VALUE obj)
{
    io_trace_t* trace = GetIOTracer(obj);
    VALUE formatter, result;
    int ret;
    result = Qnil;

    BlockRequired();

    rb_scan_args(argc, argv, "02", &trace->stream, &trace->formatter);

    Trace(ret = dtrace_go(trace->handle));
    if(ret < 0)
      DtraceError(trace, "could not enable tracing");
#ifndef RUBY_VM
    rb_add_event_hook(rb_io_trace_event_hook, RUBY_EVENT_LINE);
#else
    rb_add_event_hook(rb_io_trace_event_hook, RUBY_EVENT_LINE, Qnil);
#endif
    result = rb_yield(Qnil);
    if (rb_remove_event_hook(rb_io_trace_event_hook) == -1)
      TraceError("could not remove event hook");
    Trace(ret = dtrace_stop(trace->handle));
    if(ret < 0)
      DtraceError(trace, "could not disable tracing");
    Trace(ret = dtrace_aggregate_snap(trace->handle));
    if(ret < 0)
      DtraceError(trace, "could not snapshot aggregates");
    Trace(ret = dtrace_aggregate_walk(trace->handle, rb_io_trace_walk, (void*)trace));
    if(ret < 0)
      DtraceError(trace, "could not walk the aggregate snapshot");
    Trace(dtrace_aggregate_clear(trace->handle));
    if(!NIL_P(trace->stream)){
      formatter = rb_io_trace_formatter(trace);
      rb_funcall(formatter, id_call, 2, obj, trace->stream);
    }
    return result;
}

void
Init_trace()
{
    id_new = rb_intern("new");
    id_call = rb_intern("call");
    id_formatters = rb_intern("FORMATTERS");
    id_default = rb_intern("default");

    rb_cTrace = rb_define_class_under(rb_cIO, "Trace", rb_cObject);
    rb_define_alloc_func(rb_cTrace, rb_io_trace_alloc);
    rb_define_method(rb_cTrace, "initialize", rb_io_trace_init, -1);
    rb_define_method(rb_cTrace, "run", rb_io_trace_run, -1);
    rb_define_method(rb_cTrace, "aggregations", rb_io_trace_aggregations, 0);
    rb_define_method(rb_cTrace, "close", rb_io_trace_close, 0);

    rb_eTraceError = rb_define_class_under(rb_cIO, "TraceError", rb_eIOError);

    rb_define_const(rb_cTrace, "SUMMARY", INT2NUM(IO_TRACE_SUMMARY));
    rb_define_const(rb_cTrace, "ALL", INT2NUM(IO_TRACE_ALL));
    rb_define_const(rb_cTrace, "READ", INT2NUM(IO_TRACE_READ));
    rb_define_const(rb_cTrace, "WRITE", INT2NUM(IO_TRACE_WRITE));
    rb_define_const(rb_cTrace, "SETUP", INT2NUM(IO_TRACE_SETUP));

    rb_cTraceAggregation = rb_define_class_under(rb_cTrace, "Aggregation", rb_cObject);
    rb_define_alloc_func(rb_cTraceAggregation, rb_io_trace_aggregation_alloc);
    rb_define_method(rb_cTraceAggregation, "initialize", rb_io_trace_aggregation_init, -1);
    rb_define_method(rb_cTraceAggregation, "inspect", rb_io_trace_aggregation_inspect, 0);
    rb_define_method(rb_cTraceAggregation, "to_s", rb_io_trace_aggregation_inspect, 0);
    rb_define_method(rb_cTraceAggregation, "syscall", rb_io_trace_aggregation_syscall, 0);
    rb_define_method(rb_cTraceAggregation, "metric", rb_io_trace_aggregation_metric, 0);
    rb_define_method(rb_cTraceAggregation, "file", rb_io_trace_aggregation_file, 0);
    rb_define_method(rb_cTraceAggregation, "fd", rb_io_trace_aggregation_fd, 0);
    rb_define_method(rb_cTraceAggregation, "line", rb_io_trace_aggregation_line, 0);
    rb_define_method(rb_cTraceAggregation, "value", rb_io_trace_aggregation_value, 0);
    rb_define_method(rb_cTraceAggregation, "time?", rb_io_trace_aggregation_time_p, 0);
    rb_define_method(rb_cTraceAggregation, "cpu?", rb_io_trace_aggregation_cpu_p, 0);
    rb_define_method(rb_cTraceAggregation, "calls?", rb_io_trace_aggregation_calls_p, 0);
    rb_define_method(rb_cTraceAggregation, "bytes?", rb_io_trace_aggregation_bytes_p, 0);
}