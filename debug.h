#pragma once

#include <stdarg.h>
#include <stdio.h>

static inline void debug(const char *format, ...) {
#ifdef NDEBUG
	(void)format;
#else
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
#endif
}

static inline void debug_path_func(const char *name, const char *path, const char *real_path) {
	debug("[%s] %s -> %s\n", name, path, real_path);
}
