#pragma once

#include <stdarg.h>

// Do we have a cross-platform way of doing this?
// Would a stream work better?
#ifdef _MSC_VER

#define LOG(level, fmt, ...) { dwlog(level, fmt, __FILE__, __LINE__, __FUNCSIG__, __VA_ARGS__); }

#define QLOG(level, fmt, ...) { dwlog(level, "[%lx] " fmt, __FILE__, __LINE__, __FUNCSIG__, (uint64_t)QThread::currentThreadId(), __VA_ARGS__); }

#else

#define LOG(level, fmt, args...) { dwlog(level, fmt, __FILE__, __LINE__, __PRETTY_FUNCTION__, ## args); }

#define QLOG(level, fmt, args...) { dwlog(level, "[%lx] " fmt, __FILE__, __LINE__, __PRETTY_FUNCTION__, (uint64_t)QThread::currentThreadId(), ## args); }

#endif



void dwlog(int level, char const * fmt, char const * file, int line, char const * func, ...);

void break1();
