# SC - the Terminal based Spread Sheet Calculator

`sc` can be accessed through a terminal emulator, and has a simple interface
and keyboard shortcuts resembling the key bindings of the Vim text editor.

- Original author(s): James Gosling
- Developer: Mark Weiser, Robert Bond, Chuck Martin
- Stable release: 7.16 / September 20, 2002

[Read history here](https://en.wikipedia.org/wiki/Sc_(spreadsheet_calculator))

![Screenshot](screen.png)

## Manual

[Mini Manual](manual.pdf)

## Building

On Linux:

- install bison (e.g. `apt install bison`)
- `make`

## Troubleshooting

### Colors / cell highlighting not working

This is usually a `$TERM` mismatch. Check what your terminal reports:

```sh
echo $TERM
```

If you are running inside **tmux**, it typically sets `TERM=tmux-256color`, which may not have a complete terminfo entry on all systems. To test, launch sc with:

```sh
TERM=xterm-256color sc
```

If that fixes it, add one of the following to your `~/.tmux.conf`:

```
# Option A — use xterm-256color inside tmux sessions
set -g default-terminal "xterm-256color"

# Option B — keep tmux-256color but add RGB override
set -g default-terminal "tmux-256color"
set -ag terminal-overrides ",tmux-256color:RGB"
```

Then restart tmux for the change to take effect.

