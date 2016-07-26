#pragma once

#include <stdarg.h>

#define LOG(level, fmt, args...) { dwlog(level, fmt, __FILE__, __LINE__, __PRETTY_FUNCTION__, ## args); }

#define QLOG(level, fmt, args...) { dwlog(level, "[%lx] " fmt, __FILE__, __LINE__, __PRETTY_FUNCTION__, (uint64_t)QThread::currentThreadId(), ## args); }

void dwlog(int level, char const * fmt, char const * file, int line, char const * func, ...);

void break1();
