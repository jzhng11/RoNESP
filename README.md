# RoN ESP

External ESP for Ready Or Not (UE5.3.2) using the DX11 overlay.

## Features

| Toggle | Default | What it does |
|---|---|---|
| Boxes | on | 2D box around each character |
| Tracers | on | Line from feet to bottom-center of screen |
| Names | on | Enemy / Innocent / Player label |
| Health | on | Vertical bar, left side of box |
| Distance | on | Meters from camera below the box |
| Dead | off | Show corpses in dimmed grey with [DEAD] |
| Crosshair | on | Center-screen reticle |

Red suspects for enemies, green for civ, SWAT is blue (though I don't think they render correctly), yellow is downed, dead is grey (you need to enable the option first)
Shot/Cuffed/Neutralized NPCs have a `[DOWNED]` label under their ESP boxes.

## Controls

| Key | Action |
|---|---|
| INSERT | Opens config menu, unlocks the cursor so you can click toggles |
| END | Exits the tool |
| F5 | Dumps nearest entity debug info to console |
| Menu | Check or uncheck anything while cursor is free |

## Build

Visual Studio 2019+ is required with the "Desktop Development" workload.

```powershell
git clone https://github.com/ocornut/imgui.git
# set IMGUI_DIR or copy imgui into the tree
cmake -B build
cmake --build build --config Release
```

(excuse any bugs as this project was developed at 4 in the morning along with non-extensive testing)

IMAGES WILL BE PROVIDED, THOUGH IM TOO LAZY TO ATTACH THEM NOW