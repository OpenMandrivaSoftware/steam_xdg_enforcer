#include "path.h"

#include "str.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cwalk.h>

#define ARRAY_SIZE(arr) (sizeof((arr)) / sizeof((arr)[0]))

#define ENV_VAR_INSTALL_DIR ENV_VAR_PREFIX "INSTALL_DIR"
#define ENV_VAR_DATA_DIR ENV_VAR_PREFIX "DATA_DIR"
#define ENV_VAR_RUN_DIR ENV_VAR_PREFIX "RUN_DIR"

static struct {
	char *install;
	char *data;
	char *run;
} g_roots;

static struct PathSpec {
	const char *match;
	const char *redir;
	const char *root;
} g_specs[18];

static bool is_strict_match(const char *str, const size_t len) {
	// 0x03 corresponds to ETX (End of TeXt) in ASCII.
	return str[len - 1] == '\x03';
}

static char *build_path(const char *target, const struct PathSpec *spec) {
	const size_t match_len = strlen(spec->match);

	size_t path_size;
	if (is_strict_match(spec->match, match_len)) {
		path_size = cwk_path_join(spec->root, spec->redir, NULL, 0);
	} else {
		path_size = snprintf(NULL, 0, "%s/%s%s", spec->root, spec->redir, target + strlen(spec->match));
	}

	if (!path_size) {
		return NULL;
	}

	char *path = malloc(++path_size);
	if (!path) {
		return NULL;
	}

	if (is_strict_match(spec->match, match_len)) {
		cwk_path_join(spec->root, spec->redir, path, path_size);
	} else {
		snprintf(path, path_size, "%s/%s%s", spec->root, spec->redir, target + strlen(spec->match));
	}

	cwk_path_normalize(path, path, path_size);

	return path;
}

bool path_init() {
	g_roots.install = getenv(ENV_VAR_INSTALL_DIR);
	g_roots.data = getenv(ENV_VAR_DATA_DIR);
	g_roots.run = getenv(ENV_VAR_RUN_DIR);

	if (!(g_roots.install && g_roots.data && g_roots.run)) {
		printf("Please define all environment variables:\n\n"
		ENV_VAR_INSTALL_DIR "\n"
		ENV_VAR_DATA_DIR "\n"
		ENV_VAR_RUN_DIR "\n");
		return false;
	}

	g_specs[0]  = (struct PathSpec){ .match = "/registry.vdf",                     .redir = "/config/registry.vdf",            .root = g_roots.data    };
	g_specs[1]  = (struct PathSpec){ .match = "/starting\x03",                     .redir = "/starting",                       .root = g_roots.data    };
	g_specs[2]  = (struct PathSpec){ .match = "/steam.config\x03",                 .redir = "/config/steam.config",            .root = g_roots.data    };
	g_specs[3]  = (struct PathSpec){ .match = "/steam.pid\x03",                    .redir = "/steam.pid",                      .root = g_roots.run     };
	g_specs[4]  = (struct PathSpec){ .match = "/steam.pipe\x03",                   .redir = "/steam.pipe",                     .root = g_roots.run     };
	g_specs[5]  = (struct PathSpec){ .match = "/steam.token\x03",                  .redir = "/steam.token",                    .root = g_roots.run     };
	g_specs[6]  = (struct PathSpec){ .match = "/root/.crash\x03",                  .redir = "/config/.crash",                  .root = g_roots.data    };
	g_specs[7]  = (struct PathSpec){ .match = "/root/.forceupdate\x03",            .redir = "/config/.forceupdate",            .root = g_roots.data    };
	g_specs[8]  = (struct PathSpec){ .match = "/root/appcache",                    .redir = "/appcache",                       .root = g_roots.data    };
	g_specs[9]  = (struct PathSpec){ .match = "/root/compatibilitytools.d",        .redir = "/compatibilitytools.d",           .root = g_roots.data    };
	g_specs[10] = (struct PathSpec){ .match = "/root/config",                      .redir = "/config",                         .root = g_roots.data    };
	g_specs[11] = (struct PathSpec){ .match = "/root/depotcache",                  .redir = "/depotcache",                     .root = g_roots.data    };
	g_specs[12] = (struct PathSpec){ .match = "/root/logs",                        .redir = "/logs",                           .root = g_roots.data    };
	g_specs[13] = (struct PathSpec){ .match = "/root/music",                       .redir = "/music",                          .root = g_roots.data    };
	g_specs[14] = (struct PathSpec){ .match = "/root/shader_cache",                .redir = "/shader_cache",                   .root = g_roots.data    };
	g_specs[15] = (struct PathSpec){ .match = "/root/steamapps",                   .redir = "/steamapps",                      .root = g_roots.data    };
	g_specs[16] = (struct PathSpec){ .match = "/root/update_hosts_cached.vdf\x03", .redir = "/config/update_hosts_cached.vdf", .root = g_roots.data    };
	g_specs[17] = (struct PathSpec){ .match = "/root/userdata",                    .redir = "/userdata",                       .root = g_roots.data    };

	return true;
}

char *path_get_real(const char *target) {
	size_t target_len = strlen(target);
	char *target_norm = strndup(target, target_len);
	target_len = cwk_path_normalize(target, target_norm, target_len + 1);
	target = target_norm;

	char *ret = NULL;

	for (uint8_t i = 0; i < ARRAY_SIZE(g_specs); ++i) {
		const struct PathSpec *spec = &g_specs[i];

		size_t match_len = strlen(spec->match);
		if (is_strict_match(spec->match, match_len)) {
			if (strlen(target) != --match_len) {
				continue;
			}
		}

		if (strneq(target, spec->match, match_len)) {
			ret = build_path(target, spec);
			goto RET;
		}
	}

	if (cwk_path_is_relative(target)) {
		return target_norm;
	}

	if (strneq(target, "/root", 5)) {
		target += 5;

		if (target_len <= 5 || target[0] == '/') {
			ret = build_path(target, &(struct PathSpec){ .match = "\x03",.redir = target, .root = g_roots.install });
		}
	}
RET:
	free(target_norm);

	return ret;
}
