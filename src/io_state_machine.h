#pragma once
#include <stddef.h>

enum state_machine_push_result {
  want_send = 0,
  want_recv,
  want_close,
};

struct io_state_machine_vtable {
  enum state_machine_push_result (*push)(void*);
  void (*send)(void*, char*, size_t);
  void (*recv)(void*, char*, size_t);
};

struct io_state_machine {
  void* ptr;
  const struct io_state_machine_vtable* vtable;
};
