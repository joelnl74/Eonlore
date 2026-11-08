# Eonlore

A JRPG built with [Hydro](https://github.com/joelnl74/Hydro), a Vulkan game engine, and its editor Oxygen.

This repository is a Hydro **workspace**: the game's code (`Game/src`), its project (`Project/Eonlore.hyproj` with
`Assets/`), its scene and art tools (`scripts/`), docs (`docs/`) and rule tests (`Tools/Tests/`). The engine lives in
its own repository and is not copied in here.

## Setup

1. Clone Hydro and this repository **next to each other** (or set `HYDRO_DIR` to your Hydro checkout):

   ```text
   C:\Users\<you>\Hydro      git clone --recursive https://github.com/joelnl74/Hydro
   C:\Users\<you>\Eonlore    git clone https://github.com/joelnl74/Eonlore
   ```

2. Run Hydro's `scripts\Setup.bat` once (Vulkan SDK, premake, Python).
3. Build this workspace: `scripts\Build.bat Debug` (the solution is `build\Hydro.sln`, binaries in `bin\`).
4. Start the editor on the game: `scripts\RunOxygen.bat`, or double-click
   `bin\Debug-windows-x86_64\Oxygen\Oxygen.exe` and open `Project\Eonlore.hyproj` from the project hub.

`engine.lock` names the Hydro commit this game was last built and tested with. CI builds against exactly that commit;
locally a different Hydro checkout only prints a warning, so you can change the engine and the game together. After
an engine change the game needs, update `engine.lock` to the new Hydro commit (engine first, then the game).

## Packaging

`scripts\Package.bat Release` (or Oxygen's **File > Build**) writes `Builds\Eonlore-Release\` with `Eonlore.exe`.
