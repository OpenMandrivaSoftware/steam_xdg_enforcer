# steam_xdg_enforcer
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

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
