#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include "ngx_http_jsonnet_module.h"

static ngx_command_t ngx_http_jsonnet_commands[] = {
  {
    ngx_string("jsonnet"),
    NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
    ngx_http_jsonnet,
    0,
    0,
    NULL
  },

  ngx_null_command
};

static ngx_http_module_t ngx_http_jsonnet_module_ctx = {
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};

ngx_module_t ngx_http_jsonnet_module = {
  NGX_MODULE_V1,
  &ngx_http_jsonnet_module_ctx,
  ngx_http_jsonnet_commands,
  NGX_HTTP_MODULE,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NGX_MODULE_V1_PADDING
};

void ngx_http_jsonnet_init(ngx_http_request_t *r) {
  u_char *full_body;
  u_char *src;
  u_int full_body_len;
  ngx_chain_t *bb;
  ngx_chain_t out;
  ngx_int_t rc;
  ngx_buf_t *buffer;

  if (r->request_body == NULL) {
    ngx_http_finalize_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
    return;
  }

  if (r->request_body->temp_file) {
    return;
  }

  if (r->request_body->bufs->next == NULL) {
    full_body_len = (u_int) (r->request_body->bufs->buf->last - r->request_body->bufs->buf->pos);
    full_body = ngx_pcalloc(r->pool, (u_int) (full_body_len+1));
    memcpy(full_body, r->request_body->bufs->buf->pos, full_body_len);
  } else {
    for (full_body_len = 0, bb = r->request_body->bufs; bb; bb = bb->next) {
      full_body_len += (bb->buf->last - bb->buf->pos);
    }
    full_body = ngx_pcalloc(r->pool, full_body_len+1);
    src = full_body;
    if (!full_body) {
      return;
    }
    for (bb = r->request_body->bufs; bb; bb = bb->next) {
      full_body = ngx_cpymem(full_body, bb->buf->pos, bb->buf->last - bb->buf->pos);
    }
    full_body = src;
  }

  r->headers_out.content_type.len  = sizeof("application/json; charset=utf-8") - 1;
  r->headers_out.content_type.data = (u_char *) "application/json; charset=utf-8";

  int error;
  char *evaluated;
  struct JsonnetVM *vm;
  double gc_growth_trigger = 2;
  unsigned max_stack = 500, gc_min_objects = 1000, max_trace = 20;

  vm = jsonnet_make();
  jsonnet_max_stack(vm, max_stack);
  jsonnet_gc_min_objects(vm, gc_min_objects);
  jsonnet_max_trace(vm, max_trace);
  jsonnet_gc_growth_trigger(vm, gc_growth_trigger);
  evaluated = jsonnet_evaluate_snippet(vm, "", full_body, &error);

  buffer = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
  if (buffer == NULL) {
    return NGX_HTTP_INTERNAL_SERVER_ERROR;
  }

  out.buf = buffer;
  out.next = NULL;
  buffer->pos = evaluated;
  buffer->last = evaluated + strlen(evaluated);
  buffer->memory = 1;
  buffer->last_buf = 1;

  r->headers_out.status = NGX_HTTP_OK;
  r->headers_out.content_length_n = strlen(evaluated);

  jsonnet_destroy(vm);

  rc = ngx_http_send_header(r);
  if (rc == NGX_ERROR || rc > NGX_OK || r->header_only) {
    return rc;
  }

  rc = ngx_http_output_filter(r, &out);

  ngx_http_finalize_request(r, rc);
}

static ngx_int_t ngx_http_jsonnet_handler(ngx_http_request_t *r) {
  ngx_int_t rc;

  rc = ngx_http_read_client_request_body(r, ngx_http_jsonnet_init);

  if (rc >= NGX_HTTP_SPECIAL_RESPONSE) {
    return rc;
  }

  return NGX_DONE;
}

static char * ngx_http_jsonnet(ngx_conf_t *cf, ngx_command_t *cmd, void *conf) {
  ngx_http_core_loc_conf_t *clcf;

  clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
  clcf->handler = ngx_http_jsonnet_handler;

  return NGX_CONF_OK;
}
