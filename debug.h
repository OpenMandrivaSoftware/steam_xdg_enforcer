/*
 * steam_xdg_enforcer
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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
