#include <event2/buffer.h>
#include <stdio.h>
#include <string.h>

#include "io_state_machine.h"
#include "http_state_machine.h"

struct io_state_machine_vtable http_state_machine_vtable = { .push = http_push, .send = http_send, .recv = http_recv };

enum state_machine_push_result http_push(void* ptr) {
  struct http_state_machine* http_machine = (struct http_state_machine*) ptr;
  while (1) {
    switch (http_machine->state) {
      case request_line:
        {
          size_t len;
          char* request_line = evbuffer_readln(http_machine->recv_buf, &len, EVBUFFER_EOL_CRLF_STRICT);
          if (!request_line) {
            if (evbuffer_get_length(http_machine->recv_buf) > 8192) {
              char* http_414 = "HTTP/1.1 414 URI Too Long\r\n\r\n";
              evbuffer_add(http_machine->send_buf, http_414, strlen(http_414));
              // return 414 and terminate connection
            }
            return want_recv;
          }

          printf("%-.*s", (int) len, request_line);
        }
    }
  }
}

void http_send(void*, char*, size_t);
void http_recv(void*, char*, size_t);

