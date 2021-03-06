#include "trace.h"

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
    as->fmt_as_time = -1;
}

/*
 * Wraps a io_trace_aggregation_t struct as a Ruby object
*/
static VALUE
rb_io_trace_aggregation_wrap(io_trace_aggregation_t* as)
{
    return Data_Wrap_Struct(rb_cTraceAggregation, rb_io_trace_mark_aggregation, rb_io_trace_free_aggregation, as);
}

short int
rb_io_trace_aggregation_format_time_p(io_trace_aggregation_t* a)
{
    if (a->fmt_as_time != -1) return a->fmt_as_time;
    if (strcmp(a->metric, "cpu") == 0 || strcmp(a->metric, "time") == 0){
      a->fmt_as_time = 1;
    }else{
      a->fmt_as_time = 0;
    }
    return a->fmt_as_time;
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
    char* buf = NULL;
    char* file = NULL;
    double val;
    if (rb_io_trace_aggregation_format_time_p(a)){
      val = ((double)a->value / 1000000);
      if (val >= 1000){
        InspectAggregation(val / 1000, "%-40.40s %-7d %-18s %-4d %-10s %.02f s\n");
      }else{
        InspectAggregation(val, "%-40.40s %-7d %-18s %-4d %-10s %.03f ms\n");
      }
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
    if (rb_io_trace_aggregation_format_time_p(a)){
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
#ifdef HAVE_DTRACE
      rb_io_trace_close_handle(t);
#endif
      if (t->framework) xfree(t->framework);
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
   ts->closed = 1;
   InitFramework(ts);
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
      CoerceStringFromHash(syscall);
      CoerceStringFromHash(metric);
      CoerceStringFromHash(file);
      CoerceNumericFromHash(fd);
      CoerceNumericFromHash(line);
      CoerceNumericFromHash(value);
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
    char *script = NULL;
    size_t len;
    int ret;

    rb_scan_args(argc, argv, "01", &trace->strategy);
    if (NIL_P(trace->strategy)) 
      trace->strategy = rb_const_get(rb_cTrace, rb_intern("SUMMARY"));
    RegisterHandlers(trace);

    switch(FIX2INT(trace->strategy)){
      case IO_TRACE_SUMMARY : script = summary_script;
                              break;
      case IO_TRACE_ALL     : script = all_script;
                              break;
      case IO_TRACE_READ    : script = read_script;
                              break;
      case IO_TRACE_WRITE   : script = write_script;
                              break;
      case IO_TRACE_SETUP   : script = setup_script;
                              break;
    }

    CompileStrategy(trace, script);
    RunStrategy(trace);
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
#ifdef HAVE_DTRACE
    rb_io_trace_close_handle(trace);
#endif
    rb_ary_clear(trace->aggregations);
    return Qtrue;
}

/*
 * Our MRI event hook
 *
 * Callback for the RUBY_EVENT_LINE interpreter event.This drives the ruby:::line probe
 * which is a predicate for all other instrumentation (except SUMMARY, which is callsite
 * agnostic)
*/
static void
#ifdef RUBY_VM
rb_io_trace_event_hook(rb_event_flag_t event, VALUE data, VALUE self, ID mid, VALUE klass)
#else
rb_io_trace_event_hook(rb_event_flag_t event, NODE *node, VALUE self, ID mid, VALUE klass)
#endif
{
   if(RUBY_LINE_ENABLED()){
#ifdef RUBY_VM
     if (strncmp(rb_sourcefile(), "<internal:lib/", 13) != 0)
#endif
     RUBY_LINE((char*)rb_sourcefile(), (int)rb_sourceline());
   }
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
    int ret, status;
    result = Qnil;

    BlockRequired();

    rb_scan_args(argc, argv, "02", &trace->stream, &trace->formatter);
    StartTrace(trace);
#ifdef RUBY_VM
    rb_add_event_hook(rb_io_trace_event_hook, RUBY_EVENT_LINE, Qnil);
#else
    rb_add_event_hook(rb_io_trace_event_hook, RUBY_EVENT_LINE);
#endif
    result = rb_protect(rb_yield, obj, &status);
    rb_remove_event_hook(rb_io_trace_event_hook);
    StopTrace(trace);
    if(!NIL_P(trace->stream)){
      formatter = rb_io_trace_formatter(trace);
      rb_funcall(formatter, id_call, 2, obj, trace->stream);
    }
    return result;
}

#define define_strategy(strategy) \
    rb_define_const(rb_cTrace, #strategy, INT2NUM(IO_TRACE_##strategy));

void
Init_trace()
{

    id_new = rb_intern("new");
    id_call = rb_intern("call");
    id_formatters = rb_intern("FORMATTERS");
    id_default = rb_intern("default");
    devnull = fopen("/dev/null", "w");

    rb_cTrace = rb_define_class_under(rb_cIO, "Trace", rb_cObject);
    rb_define_alloc_func(rb_cTrace, rb_io_trace_alloc);
    rb_define_method(rb_cTrace, "initialize", rb_io_trace_init, -1);
    rb_define_method(rb_cTrace, "run", rb_io_trace_run, -1);
    rb_define_method(rb_cTrace, "aggregations", rb_io_trace_aggregations, 0);
    rb_define_method(rb_cTrace, "close", rb_io_trace_close, 0);

    rb_eTraceError = rb_define_class_under(rb_cIO, "TraceError", rb_eIOError);

    define_strategy(SUMMARY);
    define_strategy(ALL);
    define_strategy(READ);
    define_strategy(WRITE);
    define_strategy(SETUP);

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