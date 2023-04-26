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

#include "filesystem.h"

#include "debug.h"
#include "path.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/xattr.h>

#include <fuse.h>

#define FD_ROOT (UINT64_MAX)

#define FREE_REAL_PATH(path) \
	free(real_##path);

#define FREE_REAL_PATH_2(path1, path2) \
	FREE_REAL_PATH(path1) \
	FREE_REAL_PATH(path2)

#define GET_REAL_PATH_NO_RET(path) \
	char *real_##path = path_get_real(path); \
	debug_path_func(__func__, path, real_##path);

#define GET_REAL_PATH(path) \
	GET_REAL_PATH_NO_RET(path) \
	if (!real_##path) return -ENOENT;

#define GET_REAL_PATH_2(path1, path2) \
	GET_REAL_PATH(path1)              \
	GET_REAL_PATH_NO_RET(path2)       \
	if (!real_##path2) {              \
		FREE_REAL_PATH(path1)         \
		return -ENOENT;               \
	}

static int fs_getattr(const char *path, struct stat *buf, struct fuse_file_info *fi) {
	if (fi) {
		return fstat((int)fi->fh, buf) == 0 ? 0 : -errno;
	}

	if (path_is_root(path)) {
		buf->st_mode = S_IFDIR | 0755;
		buf->st_nlink = PATH_ROOT_N_SYMLINK;
		return 0;
	}

	const char *link = path_get_link(path, false);
	if (link) {
		buf->st_mode = S_IFLNK | 0777;
		buf->st_nlink = 1;
		buf->st_size = strlen(link);
		return 0;
	}

	GET_REAL_PATH(path)
	const int ret = lstat(real_path, buf) == 0 ? 0 : -errno;
	FREE_REAL_PATH(path)

	return ret;
}

static int fs_rename(const char *old, const char *new, unsigned int flags) {
	if (path_is_fixed(old) || path_is_fixed(new)) {
		return -EACCES;
	}

	GET_REAL_PATH_2(old, new)
	const int ret = renameat2(AT_FDCWD, real_old, AT_FDCWD, real_new, flags) == 0 ? 0 : -errno;
	FREE_REAL_PATH_2(old, new)

	return ret;
}

static int fs_unlink(const char *path) {
	if (path_is_fixed(path)) {
		return -EACCES;
	}

	GET_REAL_PATH(path)
	const int ret = unlink(real_path) == 0 ? 0 : -errno;
	FREE_REAL_PATH(path)

	return ret;
}

static int fs_rmdir(const char *path) {
	if (path_is_fixed(path)) {
		return -EACCES;
	}

	GET_REAL_PATH(path)
	const int ret = rmdir(real_path) == 0 ? 0 : -errno;
	FREE_REAL_PATH(path)

	return ret;
}

static int fs_symlink(const char *from, const char *to) {
	if (path_is_fixed(to)) {
		return -EACCES;
	}

	GET_REAL_PATH_2(from, to)
	const int ret = symlink(real_from, real_to) == 0 ? 0 : -errno;
	FREE_REAL_PATH_2(from, to)

	return ret;
}

static int fs_link(const char *from, const char *to) {
	if (path_is_fixed(to)) {
		return -EACCES;
	}

	GET_REAL_PATH_2(from, to)
	const int ret = link(real_from, real_to) == 0 ? 0 : -errno;
	FREE_REAL_PATH_2(from, to)

	return ret;
}

static int fs_release(const char *path, struct fuse_file_info *fi) {
	(void)path;

	int fd = (int)fi->fh;
	if (fd == -1) {
		return -EBADF;
	}

	return close(fd) == 0 ? 0 : -errno;
}

static int fs_open(const char *path, struct fuse_file_info *fi) {
	if (path_is_root(path)) {
		fi->fh = FD_ROOT;
		return 0;
	}

	const char *link = path_get_link(path, true);
	if (link) {
		return fs_open(link, fi);
	}

	GET_REAL_PATH(path)
	const int ret = open(real_path, fi->flags);
	FREE_REAL_PATH(path)

	if (ret == -1) {
		return -errno;
	}

	fi->fh = ret;

	return 0;
}

static int fs_read(const char *path, char *buf, size_t size, off_t off, struct fuse_file_info *fi) {
	(void)path;

	const ssize_t ret = pread((int)fi->fh, buf, size, off);

	return ret >= 0 ? ret : -errno;
}

static int fs_write(const char *path, const char *buf, size_t size, off_t off, struct fuse_file_info *fi) {
	(void)path;

	const ssize_t ret = pwrite((int)fi->fh, buf, size, off);

	return ret >= 0 ? ret : -errno;
}

static int fs_fsync(const char *path, int datasync, struct fuse_file_info *fi) {
	(void)path;

	if (datasync) {
		return fdatasync((int)fi->fh) == 0 ? 0 : -errno;
	} else {
		return fsync((int)fi->fh) == 0 ? 0 : -errno;
	}
}

static int fs_flush(const char *path, struct fuse_file_info *fi) {
	(void)path;
	(void)fi;

	return 0;
}

static int fs_statfs(const char *path, struct statvfs *buf) {
	if (path_is_fixed(path) && !path_is_steam_root(path)) {
		return 0;
	}

	GET_REAL_PATH(path)
	const int ret = statvfs(real_path, buf) == 0 ? 0 : -errno;
	FREE_REAL_PATH(path)

	return ret;
}

static int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t off, struct fuse_file_info *fi, enum fuse_readdir_flags flags) {
	(void)path;
	(void)off;
	(void)flags;

	if (fi->fh == FD_ROOT) {
		const char **names = path_get_root_names();
		for (const char *name = *names; name; name = *(++names)) {
			if (filler(buf, name, NULL, 0, 0) == 1) {
				break;
			}
		}

		return 0;
	}

	int fd = dup((int)fi->fh);
	if (fd == -1) {
		return -errno;
	}

	DIR *dir = fdopendir(fd);
	if (!dir) {
		return -errno;
	}

	errno = 0;

	struct dirent *ent;
	while ((ent = readdir(dir)) != NULL) {
		if (filler(buf, ent->d_name, NULL, 0, 0) == 1) {
			break;
		}
	}

	closedir(dir);

	return -errno;
}

static int fs_chmod(const char *path, mode_t mode, struct fuse_file_info *fi) {
	if (fi) {
		return fchmod((int)fi->fh, mode) == 0 ? 0 : -errno;
	}

	if (path_is_fixed(path)) {
		return -EACCES;
	}

	GET_REAL_PATH(path)
	const int ret = chmod(real_path, mode) == 0 ? 0 : -errno;
	FREE_REAL_PATH(path)

	return ret;
}

static int fs_chown(const char *path, uid_t uid, gid_t gid, struct fuse_file_info *fi) {
	if (fi) {
		return fchown((int)fi->fh, uid, gid) == 0 ? 0 : -errno;
	}

	if (path_is_fixed(path)) {
		return -EACCES;
	}

	GET_REAL_PATH(path)
	const int ret = chown(real_path, uid, gid) == 0 ? 0 : -errno;
	FREE_REAL_PATH(path)

	return ret;
}

static int fs_truncate(const char *path, off_t size, struct fuse_file_info *fi) {
	if (fi) {
		return ftruncate((int)fi->fh, size) == 0 ? 0 : -errno;
	}

	if (path_is_fixed(path)) {
		return -EACCES;
	}

	GET_REAL_PATH(path)
	const int ret = truncate(real_path, size) == 0 ? 0 : -errno;
	FREE_REAL_PATH(path)

	return ret;
}

static int fs_utimens(const char *path, const struct timespec tv[2], struct fuse_file_info *fi) {
	if (fi) {
		return futimens((int)fi->fh, tv) == 0 ? 0 : -errno;
	}

	if (path_is_fixed(path)) {
		return -EACCES;
	}

	GET_REAL_PATH(path)
	const int ret = utimensat(AT_FDCWD, real_path, tv, 0) == 0 ? 0 : -errno;
	FREE_REAL_PATH(path)

	return ret;
}

static int fs_access(const char *path, int mask) {
	if (path_is_fixed(path)) {
		return 0;
	}

	GET_REAL_PATH(path)
	const int ret = access(real_path, mask) == 0 ? 0 : -errno;
	FREE_REAL_PATH(path)

	return ret;
}

static int fs_readlink(const char *path, char *buf, size_t len) {
	const char *link = path_get_link(path, false);
	if (link) {
		strncpy(buf, link, len);
		return 0;
	}

	GET_REAL_PATH(path)
	const int ret = (int)readlink(real_path, buf, len > 0 ? len - 1 : 0);
	FREE_REAL_PATH(path)

	if (ret < 0) {
		return -errno;
	}

	if (len > 0) {
		buf[ret] = '\0';
	}

	return 0;
}

static int fs_mknod(const char *path, mode_t mode, dev_t rdev) {
	if (path_is_fixed(path)) {
		return -EACCES;
	}

	GET_REAL_PATH(path)
	const int ret = mknod(real_path, mode, rdev) == 0 ? 0 : -errno;
	FREE_REAL_PATH(path)

	return ret;
}

static int fs_mkdir(const char *path, mode_t mode) {
	if (path_is_fixed(path)) {
		return -EACCES;
	}

	GET_REAL_PATH(path)
	const int ret = mkdir(real_path, mode) == 0 ? 0 : -errno;
	FREE_REAL_PATH(path)

	return ret;
}

static int fs_setxattr(const char *path, const char *name, const char *value, size_t size, int flags) {
	if (path_is_fixed(path)) {
		return -EACCES;
	}

	GET_REAL_PATH(path)
	const int ret = setxattr(real_path, name, value, size, flags) == 0 ? 0 : -errno;
	FREE_REAL_PATH(path);

	return ret;
}

static int fs_getxattr(const char *path, const char *name, char *value, size_t size) {
	if (path_is_fixed(path)) {
		return 0;
	}

	GET_REAL_PATH(path)
	const int ret = (int)getxattr(real_path, name, value, size);
	FREE_REAL_PATH(path)

	return ret >= 0 ? 0 : -errno;
}

static int fs_listxattr(const char *path, char *list, size_t size) {
	if (path_is_fixed(path)) {
		return 0;
	}

	GET_REAL_PATH(path)
	const int ret = (int)listxattr(real_path, list, size);
	FREE_REAL_PATH(path)

	return ret >= 0 ? 0 : -errno;
}

static int fs_removexattr(const char *path, const char *name) {
	if (path_is_fixed(path)) {
		return -EACCES;
	}

	GET_REAL_PATH(path)
	const int ret = removexattr(real_path, name) == 0 ? 0 : -errno;
	FREE_REAL_PATH(path)

	return ret;
}

static int fs_fallocate(const char *path, int mode, off_t offset, off_t length, struct fuse_file_info *fi) {
	(void)path;

	return fallocate((int)fi->fh, mode, offset, length) == 0 ? 0 : -errno;
}

static ssize_t fs_copy_file_range(const char *path_in, struct fuse_file_info *fi_in, off_t off_in,
								  const char *path_out, struct fuse_file_info *fi_out, off_t off_out,
								  size_t len, int flags) {
	(void)path_in;
	(void)path_out;

	const ssize_t ret = copy_file_range((int)fi_in->fh, &off_in, (int)fi_out->fh, &off_out, len, flags);

	return ret >= 0 ? ret : -errno;
}

static off_t fs_lseek(const char *path, off_t off, int whence, struct fuse_file_info *fi) {
	(void)path;

	const off_t ret = lseek((int)fi->fh, off, whence);

	return ret >= 0 ? ret : -errno;
}

static void *fs_init(struct fuse_conn_info *conn, struct fuse_config *cfg) {
	(void)conn;

	cfg->nullpath_ok = 1;

	return NULL;
}

static const struct fuse_operations operations = {
	.getattr = fs_getattr,
	.readlink = fs_readlink,
	.mknod = fs_mknod,
	.mkdir = fs_mkdir,
	.unlink = fs_unlink,
	.rmdir = fs_rmdir,
	.symlink = fs_symlink,
	.rename = fs_rename,
	.link = fs_link,
	.chmod = fs_chmod,
	.chown = fs_chown,
	.truncate = fs_truncate,
	.open = fs_open,
	.read = fs_read,
	.write = fs_write,
	.statfs = fs_statfs,
	.flush = fs_flush,
	.release = fs_release,
	.fsync = fs_fsync,
	.setxattr = fs_setxattr,
	.getxattr = fs_getxattr,
	.listxattr = fs_listxattr,
	.removexattr = fs_removexattr,
	.opendir = fs_open,
	.readdir = fs_readdir,
	.releasedir = fs_release,
	.fsyncdir = fs_fsync,
	.init = fs_init,
	.access = fs_access,
	.utimens = fs_utimens,
	.fallocate = fs_fallocate,
	.copy_file_range = fs_copy_file_range,
	.lseek = fs_lseek
};

int filesystem_exec(int argc, char *argv[]) {
	if (!path_init()) {
		return 1;
	}

	return fuse_main(argc, argv, &operations, NULL);
}
