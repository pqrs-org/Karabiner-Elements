#if !defined(C_EVENTLOOP_H)
#define C_EVENTLOOP_H

void eventloop_register(duk_context *ctx);
duk_ret_t eventloop_run(duk_context *ctx, void *udata);

#endif
