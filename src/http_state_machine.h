#pragma once
#include "io_state_machine.h"
#include <event2/buffer.h>

enum state_machine_push_result http_push(void*);
void http_send(void*, char*, size_t);
void http_recv(void*, char*, size_t);

extern struct io_state_machine_vtable http_state_machine_vtable;

enum http_state {
  request_line,
};

struct http_state_machine {
  enum http_state state;
  struct evbuffer *recv_buf;
  struct evbuffer *send_buf;
  char* method;
  size_t method_len;
};
