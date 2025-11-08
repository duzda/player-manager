# Player Manager

Showcase application on how to handle MPRIS activities for [Waybar](https://github.com/Alexays/Waybar) using [playerctl](https://github.com/altdesktop/playerctl).

Sharing as I had trouble finding examples in C/C++, hopefully someone appreciates this.

## Functionality

Removes _(feat. artist)_ and _(ft. artist)_ from the song title.

Truncates length of _title - artist_ to the maximum number of characters, that can be defined during config, `artist_length` is the preferred maximum length of artist string, the same goes with `title_length`, in case of a long artist, but way shorter title, the allowed number of characters for artist grows for every unused character of title.

Shows activity for specified players only.

To see the options run:

```
meson configure
```

## Building

Make sure to grab the dependencies:

```
sudo apt install ninja-build meson gcc playerctl clang-format clang-tidy
```

Then build with meson:

```
meson setup build --buildtype release
cd build && (
    meson configure -Dsupported_players="['Deezer', 'DeaDBeeF']"
    meson configure -DG_DISABLE_ASSERT
    meson compile -C build
)
```

Replace `supported_players` with your own array of players.

## Waybar

The configuration for Waybar can look as follows:

```
    "custom/mpris": {
        "format": "   {text}",
        "exec": "~/.config/scripts/player-manager",
        "hide-empty-text": true,
        "return-type": "json",
        "on-click": "playerctl play-pause",
        "on-click-middle": "playerctl previous",
        "on-click-right": "playerctl next",
        "on-click-backward": "playerctl previous",
        "on-click-forward": "playerctl next"
    },
```

## Development

In case of changes, I've added `clang-tidy` and `clang-format` that help me with making the codebase consistent, readable and bug-free.

### Debugging

For vscode there is defined a custom task `Debug with Meson`, that can be used, be vary, as in the case of change in dependencies, the `tasks.json` file has to be edited (there is probably simpler way to debug Meson, I just don't know about yet).

### clang-tidy

```
meson setup build
cd build
clang-tidy ../main.cpp
```

### clang-format

```
meson setup build
ninja -C build clang-format
```
