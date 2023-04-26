cmake_minimum_required(VERSION 3.0)

project(steam_xdg_enforcer)

set(ENV_VAR_PREFIX "STEAM_")

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)

pkg_check_modules(FUSE REQUIRED fuse3)

add_subdirectory(3rdparty)

add_executable(steam_xdg_enforcer
	"main.c"

	"debug.h"
	"filesystem.c"
	"filesystem.h"
	"path.c"
	"path.h"
	"str.h"
)

target_compile_definitions(steam_xdg_enforcer
	PRIVATE
		"ENV_VAR_PREFIX=\"${ENV_VAR_PREFIX}\""
		"FUSE_USE_VERSION=35"
		"_GNU_SOURCE"
)

target_include_directories(steam_xdg_enforcer
	PRIVATE
		${FUSE_INCLUDE_DIRS}
)

target_compile_options(steam_xdg_enforcer
	PRIVATE
		"-Wall"
		"-Wextra"

		${FUSE_CFLAGS}
)

target_link_libraries(steam_xdg_enforcer
	PRIVATE
		cwalk

		${FUSE_LINK_LIBRARIES}
)
