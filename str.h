#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

static inline bool streq(const char *str1, const char *str2) {
	return strcmp(str1, str2) == 0;
}

static inline bool strneq(const char *str1, const char *str2, const size_t len) {
	return strncmp(str1, str2, len) == 0;
}
