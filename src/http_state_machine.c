#include <event2/buffer.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "io_state_machine.h"
#include "http_state_machine.h"

//struct io_state_machine_vtable http_state_machine_vtable = { .push = http_push, .send = http_send, .recv = http_recv };

int http_empty_response_fun(evutil_socket_t sockfd) {
  return 0;
}

enum state_machine_push_result http_push(void* ptr) {
  struct http_state_machine* http_machine = (struct http_state_machine*) ptr;

  while (1) {
    switch (http_machine->state) {
      case http_state_request_line:
        {
          size_t len;
          char* request_line = evbuffer_readln(http_machine->recv_buf, &len, EVBUFFER_EOL_CRLF_STRICT);
          if ((!request_line && evbuffer_get_length(http_machine->recv_buf) >= 8192) || len >= 8192) {
              char* str = "HTTP/1.1 414 URI Too Long\r\nConnection: close\r\n\r\n";
              evbuffer_add(http_machine->send_buf, str, strlen(str));
              http_machine->state = http_state_closing;
              return want_send;
          } else if (!request_line) {
            return want_recv;
          } else {
            free(request_line); //todo
            //evbuffer_add_printf(http_machine->send_buf, "YAY\r\n");

            http_machine->state = http_state_headers;
            http_machine->header_len_so_far = 0;
          }
        }
        break;
      case http_state_headers:
        {
          size_t len;
          char* header = evbuffer_readln(http_machine->recv_buf, &len, EVBUFFER_EOL_CRLF_STRICT);
          http_machine->header_len_so_far += len;
          if ((!header && evbuffer_get_length(http_machine->recv_buf) >= 8192) || http_machine->header_len_so_far >= 8192) {
              char* str = "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n";
              evbuffer_add(http_machine->send_buf, str, strlen(str));
              http_machine->state = http_state_closing;
              return want_send;
          } else if (!header) {
            return want_recv;
          }
          printf("Header len %zu\n", len);
          if (len == 0) {
            printf("End of headers\n");
            http_machine->state = http_state_closing;
            return want_close;
          }

          // TODO Store header
          // go again
        }
        break;
      case http_state_closing:
        {
          printf("headers2\n");
          if (evbuffer_get_length(http_machine->send_buf) > 0) {
            return want_send;
          }
          return want_close;
        }
        break;
    }
  }
}

//void http_send(void*, char*, size_t);
//void http_recv(void*, char*, size_t);
