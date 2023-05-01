# steam_xdg_enforcer

Born shortly after the 10th anniversary of ValveSoftware/steam-for-linux#1890, this program allows to rewrite the paths accessed by Steam into proper ones by using [FUSE](https://www.kernel.org/doc/html/latest/filesystems/fuse.html).

This also opens up the possibility to finally create proper packages, while getting rid of the insane bootstrapping process.

## About `~/.steam`

<details>
<summary>Problem</summary>

The infamous path is hardcoded in the `steam` binary. The relevant code is pretty much as follows:

```c
char path[FILENAME_MAX];
snprintf(path, sizeof(path), "%s/%s", getenv("HOME"), ".steam");
```

And at least [one script](ValveSoftware/steam-for-linux#9345) is also affected.

You may've noticed that, in most cases, the installation actually lives at another location:

```
lrwxrwxrwx 1 user user  23 Apr 22 01:02 bin -> /home/user/.steam/bin32
lrwxrwxrwx 1 user user  41 Apr 22 01:02 bin32 -> /home/user/.local/share/Steam/ubuntu12_32
lrwxrwxrwx 1 user user  41 Apr 22 01:02 bin64 -> /home/user/.local/share/Steam/ubuntu12_64
-rwxrwxr-x 1 user user 305 Apr 22 01:03 registry.vdf
lrwxrwxrwx 1 user user  29 Apr 22 01:02 root -> /home/user/.local/share/Steam
lrwxrwxrwx 1 user user  37 Apr 22 01:02 sdk32 -> /home/user/.local/share/Steam/linux32
lrwxrwxrwx 1 user user  37 Apr 22 01:02 sdk64 -> /home/user/.local/share/Steam/linux64
lrwxrwxrwx 1 user user  29 Apr 22 01:02 steam -> /home/user/.local/share/Steam
-rw-rw-r-- 1 user user   6 Apr 22 01:02 steam.pid
prw------- 1 user user   0 Apr 22 01:01 steam.pipe
-r-------- 1 user user  16 Apr 22 01:02 steam.token
```

Basically, the undesired directory is still around strictly for compatibility with legacy installations.

![](assets/always_has_been.png)
</details>

<details>
<summary>Solution</summary>

Setting `HOME` to an arbitrary path is a popular workaround, but there are a few quirks with it:

1. Anything that "normally" resides in `~` will be redirected as well, which may be desiderable. However, it is a problem for configuration files, such as the sound server's.  
   Symlinking the directories (e.g. `~/.config/pulse/`) and/or files generally works.
2. `.steam` still needs to be "real" (along with its structure), albeit at another location. This means that you have to either keep it around or rebuild it every time you want to launch Steam.
3. Steam may alter the directory's contents without a warning. This is going to be a problem if you want the installation to reside at a location other than the default (`~/.local/share/Steam`).

With this program you can have an actually empty `.steam`, meaning that you can just create it when launching Steam and delete it right after shutting it down.

If you don't want to see `.steam` in your home folder at all, you can still use an arbitrary `HOME`.  
Please note though that the quirk described in 1. still applies, along with its fix.
</details>

## Example launch script

```sh
export STEAM_INSTALL_DIR="/usr/libexec/steam"
export STEAM_DATA_DIR="${HOME}/.local/share/steam"
export STEAM_RUN_DIR="${XDG_RUNTIME_DIR}/steam"

MOUNT_POINT="${STEAM_RUN_DIR}/.steam"

mkdir -p "${MOUNT_POINT}" "${STEAM_DATA_DIR}"

./steam_xdg_enforcer "${MOUNT_POINT}"

pushd .

cd "${STEAM_INSTALL_DIR}"

ORIG_HOME="${HOME}"
ORIG_LD_LIBRARY_PATH="${LD_LIBRARY_PATH}"

export HOME="${STEAM_RUN_DIR}"
export LD_LIBRARY_PATH="${STEAM_INSTALL_DIR}/ubuntu12_32:${STEAM_INSTALL_DIR}/ubuntu12_32/panorama:${LD_LIBRARY_PATH-}"

ubuntu12_32/steam -noverifyfiles -nobootstrapupdate -skipinitialbootstrap -norepairfiles -nodircheck -inhibitbootstrap

export LD_LIBRARY_PATH="${ORIG_LD_LIBRARY_PATH}"
export HOME="${ORIG_HOME}"

umount "${MOUNT_POINT}"

popd
```

Useful generic reference: https://wiki.fex-emu.com/index.php/Steam
