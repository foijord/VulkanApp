#pragma once

#define NO_COPY_OR_ASSIGNMENT(Class)                    \
  Class(Class&&) = delete;                              \
  Class(const Class&) = delete;                         \
  Class & operator=(Class&&) = delete;                  \
  Class & operator=(const Class&) = delete;             \
