# steam_xdg_enforcer

Born shortly after the 10th anniversary of ValveSoftware/steam-for-linux#1890, this program allows to rewrite the paths accessed by Steam into proper ones by using [FUSE](https://www.kernel.org/doc/html/latest/filesystems/fuse.html).

This also opens up the possibility to finally create proper packages, while getting rid of the insane bootstrapping process.

## Example launch script

```sh
export STEAM_INSTALL_DIR="/usr/libexec/steam"
export STEAM_DATA_DIR="${HOME}/.local/share/Steam"
export STEAM_RUN_DIR="${XDG_RUNTIME_DIR}"

export ORIG_LD_LIBRARY_PATH="${LD_LIBRARY_PATH}"

mkdir -p ~/.steam
steam_xdg_enforcer ~/.steam

pushd .

cd "${STEAM_INSTALL_DIR}"

export LD_LIBRARY_PATH="${STEAM_INSTALL_DIR}/ubuntu12_32:${STEAM_INSTALL_DIR}/ubuntu12_32/panorama:${LD_LIBRARY_PATH-}"
ubuntu12_32/steam -noverifyfiles -nobootstrapupdate -skipinitialbootstrap -norepairfiles -nodircheck -inhibitbootstrap
export LD_LIBRARY_PATH="$ORIG_LD_LIBRARY_PATH"

umount ~/.steam

popd
```

Useful generic reference: https://wiki.fex-emu.com/index.php/Steam
