#pragma once

#define NO_COPY_OR_ASSIGNMENT(__Class__)                        \
  __Class__(const __Class__&) = delete;                         \
  __Class__(const __Class__&&) = delete;                        \
  __Class__ & operator=(const __Class__&) = delete;             \
  __Class__ & operator=(const __Class__&&) = delete;            \
  virtual ~__Class__() = default                                \
