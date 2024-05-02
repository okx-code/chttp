#pragma once
#include "io_state_machine.h"
#include <event2/buffer.h>
#include <event2/util.h>
#include <stdint.h>

enum state_machine_push_result http_push(void*);
void http_send(void*, char*, size_t);
void http_recv(void*, char*, size_t);

extern struct io_state_machine_vtable http_state_machine_vtable;

enum http_state {
  http_state_request_line,
  http_state_headers,
  http_state_closing,
};

struct http_state_machine {
  struct event* ev;
  enum http_state state;
  struct evbuffer *recv_buf;
  struct evbuffer *send_buf;

  // headers state
  size_t header_len_so_far;

  // response state TODO
  //char* response_status_and_headers;
  //int (*response_write_fn)(evutil_socket_t sockfd);
};
