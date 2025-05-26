#if !defined(DUK_TRANS_DVALUE_H_INCLUDED)
#define DUK_TRANS_DVALUE_H_INCLUDED

#include "duktape.h"

typedef struct duk_dvalue duk_dvalue;
typedef struct duk_trans_buffer duk_trans_buffer;
typedef struct duk_trans_dvalue_ctx duk_trans_dvalue_ctx;

typedef void (*duk_trans_dvalue_received_function)(duk_trans_dvalue_ctx *ctx, duk_dvalue *dv);
typedef void (*duk_trans_dvalue_cooperate_function)(duk_trans_dvalue_ctx *ctx, int block);
typedef void (*duk_trans_dvalue_handshake_function)(duk_trans_dvalue_ctx *ctx, const char *handshake_line);
typedef void (*duk_trans_dvalue_detached_function)(duk_trans_dvalue_ctx *ctx);

/* struct duk_dvalue 'tag' values, note that these have nothing to do with
 * Duktape debug protocol inital byte.  Struct fields used with the type
 * are noted next to the define.
 */
#define DUK_DVALUE_EOM         1   /* no fields */
#define DUK_DVALUE_REQ         2   /* no fields */
#define DUK_DVALUE_REP         3   /* no fields */
#define DUK_DVALUE_ERR         4   /* no fields */
#define DUK_DVALUE_NFY         5   /* no fields */
#define DUK_DVALUE_INTEGER     6   /* i: 32-bit signed integer */
#define DUK_DVALUE_STRING      7   /* buf: string data, len: string length */
#define DUK_DVALUE_BUFFER      8   /* buf: buffer data, len: buffer length */
#define DUK_DVALUE_UNUSED      9   /* no fields */
#define DUK_DVALUE_UNDEFINED   10  /* no fields */
#define DUK_DVALUE_NULL        11  /* no fields */
#define DUK_DVALUE_TRUE        12  /* no fields */
#define DUK_DVALUE_FALSE       13  /* no fields */
#define DUK_DVALUE_NUMBER      14  /* d: ieee double */
#define DUK_DVALUE_OBJECT      15  /* i: class number, buf: pointer data, len: pointer length */
#define DUK_DVALUE_POINTER     16  /* buf: pointer data, len: pointer length */
#define DUK_DVALUE_LIGHTFUNC   17  /* i: lightfunc flags, buf: pointer data, len: pointer length */
#define DUK_DVALUE_HEAPPTR     18  /* buf: pointer data, len: pointer length */

struct duk_dvalue {
	/* Could use a union for the value but the gain would be relatively small. */
	int tag;
	int i;
	double d;
	size_t len;
	unsigned char *buf;
};

struct duk_trans_buffer {
	unsigned char *base;
	size_t write_offset;
	size_t read_offset;
	size_t alloc_size;
};

struct duk_trans_dvalue_ctx {
	duk_trans_dvalue_received_function received;
	duk_trans_dvalue_cooperate_function cooperate;
	duk_trans_dvalue_handshake_function handshake;
	duk_trans_dvalue_detached_function detached;
	duk_trans_buffer send_buf;  /* sending towards Duktape (duktape read callback) */
	duk_trans_buffer recv_buf;  /* receiving from Duktape (duktape write callback) */
	int handshake_done;
	int double_byteorder;  /* 0=little endian, 1=big endian, 2=mixed endian */
};

/* Buffer size needed by duk_dvalue_to_string(). */
#define DUK_DVALUE_TOSTRING_BUFLEN 256

/* Dvalue handling. */
duk_dvalue *duk_dvalue_alloc(void);
void duk_dvalue_free(duk_dvalue *dv);
void duk_dvalue_to_string(duk_dvalue *dv, char *buf);
duk_dvalue *duk_dvalue_make_tag(int tag);
duk_dvalue *duk_dvalue_make_tag_int(int tag, int intval);
duk_dvalue *duk_dvalue_make_tag_double(int tag, double dblval);
duk_dvalue *duk_dvalue_make_tag_data(int tag, const char *buf, size_t len);
duk_dvalue *duk_dvalue_make_tag_int_data(int tag, int intval, const char *buf, size_t len);

/* Initializing and freeing the transport context. */
duk_trans_dvalue_ctx *duk_trans_dvalue_init(void);
void duk_trans_dvalue_free(duk_trans_dvalue_ctx *ctx);

/* Sending dvalues towards Duktape. */
void duk_trans_dvalue_send(duk_trans_dvalue_ctx *ctx, duk_dvalue *dv);
void duk_trans_dvalue_send_eom(duk_trans_dvalue_ctx *ctx);
void duk_trans_dvalue_send_req(duk_trans_dvalue_ctx *ctx);
void duk_trans_dvalue_send_rep(duk_trans_dvalue_ctx *ctx);
void duk_trans_dvalue_send_err(duk_trans_dvalue_ctx *ctx);
void duk_trans_dvalue_send_nfy(duk_trans_dvalue_ctx *ctx);
void duk_trans_dvalue_send_integer(duk_trans_dvalue_ctx *ctx, int val);
void duk_trans_dvalue_send_string(duk_trans_dvalue_ctx *ctx, const char *str);
void duk_trans_dvalue_send_lstring(duk_trans_dvalue_ctx *ctx, const char *str, size_t len);
void duk_trans_dvalue_send_buffer(duk_trans_dvalue_ctx *ctx, const char *buf, size_t len);
void duk_trans_dvalue_send_unused(duk_trans_dvalue_ctx *ctx);
void duk_trans_dvalue_send_undefined(duk_trans_dvalue_ctx *ctx);
void duk_trans_dvalue_send_null(duk_trans_dvalue_ctx *ctx);
void duk_trans_dvalue_send_true(duk_trans_dvalue_ctx *ctx);
void duk_trans_dvalue_send_false(duk_trans_dvalue_ctx *ctx);
void duk_trans_dvalue_send_number(duk_trans_dvalue_ctx *ctx, double val);
void duk_trans_dvalue_send_object(duk_trans_dvalue_ctx *ctx, int classnum, const char *ptr_data, size_t ptr_len);
void duk_trans_dvalue_send_pointer(duk_trans_dvalue_ctx *ctx, const char *ptr_data, size_t ptr_len);
void duk_trans_dvalue_send_lightfunc(duk_trans_dvalue_ctx *ctx, int lf_flags, const char *ptr_data, size_t ptr_len);
void duk_trans_dvalue_send_heapptr(duk_trans_dvalue_ctx *ctx, const char *ptr_data, size_t ptr_len);
void duk_trans_dvalue_send_req_cmd(duk_trans_dvalue_ctx *ctx, int cmd);

/* Duktape debug callbacks provided by the transport. */
duk_size_t duk_trans_dvalue_read_cb(void *udata, char *buffer, duk_size_t length);
duk_size_t duk_trans_dvalue_write_cb(void *udata, const char *buffer, duk_size_t length);
duk_size_t duk_trans_dvalue_peek_cb(void *udata);
void duk_trans_dvalue_read_flush_cb(void *udata);
void duk_trans_dvalue_write_flush_cb(void *udata);
void duk_trans_dvalue_detached_cb(duk_context *ctx, void *udata);

#endif  /* DUK_TRANS_DVALUE_H_INCLUDED */
