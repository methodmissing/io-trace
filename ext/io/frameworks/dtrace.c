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
 * Release / close the dtrace framework handle
*/
static void 
rb_io_trace_close_handle(io_trace_t* t)
{
    if(t->closed == 0){
      switch(dtrace_status((*t->framework).handle)){
        case DTRACE_STATUS_OKAY:
        case DTRACE_STATUS_FILLED:
        case DTRACE_STATUS_STOPPED:
             Trace(dtrace_close((*t->framework).handle));
             t->closed = 1;
             break;
      }
    }
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
