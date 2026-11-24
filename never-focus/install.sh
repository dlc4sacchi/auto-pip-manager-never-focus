#!/usr/bin/env bash
set -euo pipefail

cd -- "$(dirname -- "${BASH_SOURCE[0]}")"

uuid=auto-pip-manager-never-focus@saint.local
extension_dir="${XDG_DATA_HOME:-$HOME/.local/share}/gnome-shell/extensions/$uuid"
hook_dir="$HOME/.local/lib/auto-pip-manager-never-focus"
drop_in_dir="${XDG_CONFIG_HOME:-$HOME/.config}/systemd/user/org.gnome.Shell@user.service.d"

if [[ -L "$extension_dir" ]]; then
    printf 'The extension directory is a symlink. Remove the symlink before installing a separate copy.\n' >&2
    exit 1
fi

build_dir=$(mktemp -d)
hook_temp=
trap 'rm -rf -- "$build_dir"; if [[ -n "$hook_temp" ]]; then rm -f -- "$hook_temp"; fi' EXIT

"${CC:-cc}" -O2 -shared -fPIC early-scroll.c -ldl -o "$build_dir/early-scroll.so"
glib-compile-schemas --strict --targetdir="$build_dir" schemas
msgfmt --check po/ru.po -o "$build_dir/$uuid.mo"

install -d "$extension_dir/schemas" "$extension_dir/locale/ru/LC_MESSAGES" "$hook_dir" "$drop_in_dir"
install -m 644 metadata.json extension.js prefs.js "$extension_dir/"
install -m 644 schemas/*.xml "$build_dir/gschemas.compiled" "$extension_dir/schemas/"
install -m 644 "$build_dir/$uuid.mo" "$extension_dir/locale/ru/LC_MESSAGES/"

hook_temp=$(mktemp "$hook_dir/.early-scroll.XXXXXX")
install -m 755 "$build_dir/early-scroll.so" "$hook_temp"
mv -fT -- "$hook_temp" "$hook_dir/early-scroll.so"
cat > "$drop_in_dir/pip-early-scroll.conf" <<'CONF'
[Service]
Environment=LD_PRELOAD=%h/.local/lib/auto-pip-manager-never-focus/early-scroll.so
CONF
systemctl --user daemon-reload

printf 'Log out and back in, then enable with:\n  gnome-extensions enable %s\n' "$uuid"
