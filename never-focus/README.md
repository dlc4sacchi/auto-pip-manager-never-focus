# Auto PiP Manager (Never Focus fork)

Local fork of [Auto PiP Manager](https://github.com/EvilX/nextpinp) for GNOME Shell 49–51.

The **Never in Focus** preference is on by default. When a picture-in-picture
window receives keyboard focus, the extension returns focus to the most recently
used suitable window on the current workspace. If no other
window exists on that workspace, it clears window focus. During a window drag,
focus is restored when the drag ends so moving the PiP remains possible.

The **Do Not Show on Overview** preference is off by default. When enabled,
PiP stays out of Overview workspace previews and thumbnails. It remains visible
on the desktop and available to other window lists.

Scroll over a PiP window to send it to the next corner clockwise. Moving the
pointer over PiP leaves it in place. Scroll moves use the current monitor and
configured offset.
The saved starting corner only changes when you drag and snap the window.

## Install on Wayland

The installer requires GNOME Shell 49–51 with a systemd user service, a C
compiler, `glib-compile-schemas`, and `msgfmt`.

From the repository root, run:

```sh
./never-focus/install.sh
```

The installer compiles the extension schema, Russian translation, and native
scroll hook. It configures your user’s GNOME Shell service to load the hook so
plain scroll works without holding Super.

Log out and back in, then run:

```sh
gnome-extensions enable auto-pip-manager-never-focus@saint.local
```

If you use another PiP manager, disable it before enabling this extension.
Open preferences from the Extensions app or with
`gnome-extensions prefs auto-pip-manager-never-focus@saint.local`.

If the hook is absent, the extension can use GNOME’s window action key
(Super by default) with scroll.

To remove the native hook, delete
`~/.config/systemd/user/org.gnome.Shell@user.service.d/pip-early-scroll.conf`
(or the corresponding file under `$XDG_CONFIG_HOME`), run
`systemctl --user daemon-reload`, and log out and back in.
