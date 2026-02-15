# SWG Furniture Mover

Numpad-controlled furniture placement plugin for **SWG Restoration** (x64). Loads as a Discord RPC proxy DLL and adds hotkeys for moving and rotating furniture in-game.

## Prerequisites

- **Visual Studio 2022** or newer (the free [Community edition](https://visualstudio.microsoft.com/vs/community/) works fine)
  - Install the **"Desktop development with C++"** workload
  - The x64 MSVC compiler must be included (it is by default)
- **SWG Restoration** x64 client installed

## Building

**Option A** — Double-click `build.bat`. It auto-detects your Visual Studio installation (supports VS 2022 Community/Professional/Enterprise, VS 2025 Preview, and VS Insiders).

**Option B** — Open an **x64 Native Tools Command Prompt for VS** and run:

```
cl /nologo /EHsc /O2 /LD /DWIN32_LEAN_AND_MEAN DiscordProxy64.cpp /link /OUT:discord-rpc.dll user32.lib kernel32.lib
```

The output is `discord-rpc.dll`. If your game is installed at the default path (`C:\Games\SWG Restoration\x64\`), `build.bat` will auto-copy it there.

## Installation

1. Navigate to your SWG Restoration **x64** folder (e.g. `C:\Games\SWG Restoration\x64\`)
2. Rename the existing `discord-rpc.dll` to `discord-rpc-real.dll`
3. Copy the built `discord-rpc.dll` into that folder
4. Launch the game normally through the SWG Restoration launcher

## Controls

Target a piece of furniture first, then press **F12** to enable decoration mode.

| Key | Action |
|-----|--------|
| **F12** | Toggle decoration mode on/off (beep confirms) |
| **Numpad 8** | Move forward |
| **Numpad 2** | Move back |
| **Numpad 4** | Move left |
| **Numpad 6** | Move right |
| **Numpad +** | Move up |
| **Numpad -** | Move down |
| **Numpad 7** | Rotate left (yaw) |
| **Numpad 9** | Rotate right (yaw) |
| **Numpad \*** | Increase step size (1 &rarr; 2 &rarr; 5 &rarr; 10 &rarr; 25 &rarr; 50 &rarr; 100) |
| **Numpad /** | Decrease step size |
| **Shift** (hold) | 5x movement/rotation multiplier |
| **F11** | Show current status in chat |

## Usage Tips

- **NumLock must be on** for the numpad keys to register
- **Target the furniture first** (click on it) before pressing any movement keys
- The **game window must be focused** — the plugin ignores input when alt-tabbed
- Step size starts at **1** (finest). Use **Numpad \*** to increase for bigger moves
- Hold **Shift** for a quick 5x multiplier on any move or rotation

## Uninstalling

1. Delete `discord-rpc.dll` from the `x64` folder
2. Rename `discord-rpc-real.dll` back to `discord-rpc.dll`

## How It Works

The plugin is a proxy DLL that replaces `discord-rpc.dll`. It forwards all Discord RPC calls to the original (renamed to `discord-rpc-real.dll`) while running a background thread that polls for numpad keypresses. When a key is pressed, it simulates typing `/moveFurniture` or `/rotateFurniture` chat commands via `SendInput`.
