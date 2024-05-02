#include <asm-generic/errno.h>
#include <errno.h>
#include <signal.h>
#include <event2/util.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <pthread.h>
#include <event2/event.h>
#include <event2/thread.h>

#include "http_state_machine.h"
#include "io_state_machine.h"

struct state_events {
  struct event_base* base;
  //push_machine
  //recv_machine
  //send_machine
};

void do_socket(evutil_socket_t sockfd, short triggered_events, void* ptr) {
  struct http_state_machine* http_machine = (struct http_state_machine*) ptr;

  while (1) {
    enum state_machine_push_result result = http_push(ptr);
    printf("res %d\n", result);
    if (result == want_send) {
      if (evbuffer_get_length(http_machine->send_buf) == 0) {
        printf("empty buffer?");
        exit(EXIT_FAILURE);
      }
      int write_res = evbuffer_write(http_machine->send_buf, sockfd);
      if (write_res == -1) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
          return; // expected exit condition
        }
        perror("evbuffer_write");
        exit(EXIT_FAILURE);
      }
    } else if (result == want_recv) {
      int read_res = evbuffer_read(http_machine->recv_buf, sockfd, 8192);
      if (read_res == 0) {
        // nothing more to read
        // todo finish writing these before closing, this is a bad implementation
        evbuffer_free(http_machine->send_buf);
        evbuffer_free(http_machine->recv_buf);
        event_del(http_machine->ev);
        free(http_machine);
        close(sockfd);
      } else if (read_res == -1) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
          return; // expected exit condition
        }
        perror("evbuffer_read");
        exit(EXIT_FAILURE);
      }
    } else if (result == want_close) {
      // todo finish writing these before closing, this is a bad implementation
      evbuffer_free(http_machine->send_buf);
      evbuffer_free(http_machine->recv_buf);
      event_del(http_machine->ev);
      free(http_machine);
      close(sockfd);
      return;
    }
  }
}

void do_accept(evutil_socket_t sockfd, short triggered_events, void* ptr) {
  int err;

  struct state_events* state = (struct state_events*) ptr;

  while (1) {
    int acceptfd = accept(sockfd, NULL, NULL);
    if (acceptfd == -1) {
      int accepterr = errno;
      if (accepterr == EAGAIN || accepterr == EWOULDBLOCK) {
        return;
      } else {
        perror("accept");
        continue;
      }
    }
    printf("accepting\n");

    err = evutil_make_socket_nonblocking(acceptfd);
    if (err == -1) {
      exit(EXIT_FAILURE);
    }

    struct http_state_machine* machine = malloc(sizeof *machine);
    machine->state = http_state_request_line;
    machine->recv_buf = evbuffer_new();
    machine->send_buf = evbuffer_new();

    struct event *accept_event = event_new(state->base, acceptfd, EV_READ | EV_WRITE | EV_PERSIST | EV_ET, do_socket, machine);
    machine->ev = accept_event;
    if (!accept_event) {
      exit(EXIT_FAILURE);
    }

    err = event_add(accept_event, NULL);
    if (err == -1) {
      exit(EXIT_FAILURE);
    }
  }
}

void* do_loop(void *base) {
  event_base_dispatch((struct event_base*) base);
  pthread_exit(NULL);
}

int main(void) {
  int err;

  signal(SIGPIPE, SIG_IGN);

  struct event_base *base = event_base_new();
  if (!base) {
    exit(EXIT_FAILURE);
  }

  evutil_socket_t sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
  if (sockfd == -1) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  err = evutil_make_socket_nonblocking(sockfd);
  if (err == -1) {
    perror("evutil_make_socket_nonblocking");
    exit(EXIT_FAILURE);
  }

  err = evutil_make_listen_socket_reuseable(sockfd);
  if (err == -1) {
    perror("evutil_make_listen_socket_reuseable");
    exit(EXIT_FAILURE);
  }

  struct sockaddr_in sin;
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = 0;
  sin.sin_port = htons(7000);

  err = bind(sockfd, (struct sockaddr*) &sin, sizeof(sin));
  if (err == -1) {
    perror("bind");
    exit(EXIT_FAILURE);
  }

  err = listen(sockfd, 4096);
  if (err == -1) {
    perror("listen");
    exit(EXIT_FAILURE);
  }

  struct state_events state = { .base = base };

  struct event *listener_event = event_new(base, sockfd, EV_READ | EV_PERSIST | EV_ET, do_accept, &state);
  if (!listener_event) {
    exit(EXIT_FAILURE);
  }

  err = event_add(listener_event, NULL);
  if (err == -1) {
    exit(EXIT_FAILURE);
  }

  printf("Listening\n");
  event_base_dispatch(base);
}
