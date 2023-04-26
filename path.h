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

#include "str.h"

#include <stdbool.h>
#include <stddef.h>

#define PATH_ROOT_N_SYMLINK (6)

bool path_init();

char *path_get_real(const char *target);

static inline const char *path_get_link(const char *target, const bool start_slash) {
	const char *ret;

	if (streq(target, "/bin")) {
		ret = "/bin32";
	} else if (streq(target, "/bin32")) {
		ret = "/root/ubuntu12_32";
	} else if (streq(target, "/bin64")) {
		ret = "/root/ubuntu12_64";
	} else if (streq(target, "/sdk32")) {
		ret = "/root/linux32";
	} else if (streq(target, "/sdk64")) {
		ret = "/root/linux64";
	} else if (streq(target, "/steam")) {
		ret = "/root";
	} else {
		return NULL;
	}

	return start_slash ? ret : ret + 1;
}

static inline const char **path_get_root_names() {
	static const char *names[] = {
		"bin",
		"bin32",
		"bin64",
		"root",
		"sdk32",
		"sdk64",
		"steam",
		NULL
	};

	return names;
}

static inline bool path_is_fixed(const char *target) {
	if (streq(target, "/") ||
		streq(target, "/bin") ||
		streq(target, "/bin32") ||
		streq(target, "/bin64") ||
		streq(target, "/root") ||
		streq(target, "/sdk32") ||
		streq(target, "/sdk64") ||
		streq(target, "/steam")) {
		return true;
	}

	return false;
}

static inline bool path_is_root(const char *target) {
	return streq(target, "/");
}

static inline bool path_is_steam_root(const char *target) {
	return streq(target, "/root");
}
