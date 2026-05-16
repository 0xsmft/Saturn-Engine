<h3 align="center">Saturn</h3>

<p align=center>
    <a href="https://github.com/BEASTSM96/Saturn-Engine/blob/vulkan/LICENSE"><img alt="License" src="https://img.shields.io/badge/license-MIT-green.svg"></a>
    <a href="https://img.shields.io/github/repo-size/BEASTSM96/Saturn-Engine"><img alt="Repo Size" src="https://img.shields.io/github/repo-size/BEASTSM96/Saturn-Engine"></a>
    <a href="https://github.com/BEASTSM96/Saturn-Engine/actions/workflows/Windows.yml"><img alt="Repo Size" src="https://github.com/BEASTSM96/Saturn-Engine/actions/workflows/Windows.yml/badge.svg"></a>
    <a href="https://trello.com/b/baqP3fvB/saturn-engine"><img alt="Trello" src="https://img.shields.io/badge/Trello-saturn--engine-blue"></a>
</p>

<p align=center>
    Saturn is primarily an early-stage game engine for Windows with a Linux implemention in the works.
    <br>
</p>

## Platforms

| Platform | Supported | Architecture |
| -------- | --------- | ------------ |
| Windows 10+ | ✅ | x86_64
| Linux | 🕗 *[SOON](https://trello.com/c/o43wueQO/9-preliminary-linux-support)* | x86_64
| macOS | ❌ | AArch64
| Xbox | ❌ | x86_64
| PS4/5 | ❌ | x86_64

## Features

- Renderer
  - PBR Rendering
  - SSAO
  - Bloom *[(done the proper way)](https://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare/)*
  - Shadow Mapping
  - Forward+
  - Skeletal Animation
  - Font rendering, Screenspace/Worldspace
  - Normal Mapping
  - Instanced Rendering

- Editor
  - Node Editors for Materials, Sounds, Animation and Behaviour Trees.
  - Undo/Redo tracking
  - Asset Browser
  - World outliner panel
  - Many built in Asset Viewers
  - Autosaves
  - Project Browser

- Engine
  - AI Pathfinding and Navmesh generation using [Recast](https://github.com/recastnavigation/recastnavigation/tree/main)
  - Behaviour Trees (Node Editor and Custom Tasks via C++)
  - C++ Scripting, using an Unreal-like `UClass` and `UObject` reflection system with a Custom Build Tool.
  - Physics System via [JoltPhysics](https://github.com/jrouwe/JoltPhysics)
  - Distribution i.e. shipping the game as a standalone executable via our Asset Bundle system
  - Audio System with a SoundGraph
  - Game UI system (Alura)
  - Job system
  - ECS
  - Keybindings
  - Online subsystem (Steamworks API)

## Roadmap

For upcoming features and a general status of Saturn it is best to view the [Trello](https://trello.com/b/baqP3fvB/saturn-engine)

## Getting Started

Visual Studio 2022 is recommended as Saturn is officially untested on other development environments whilst we focus on a Windows build.

First, start by cloning the repository with `git clone --recursive https://github.com/BEASTSM96/Saturn-Engine`.

If the repository was previously cloned non-recursively then use `git submodule update --init` to clone the necessary submodules.

Make sure to check that you are on the branch `vulkan`. If not you can run `git checkout vulkan`

## Generating project files

In order to start you will need to download <a href="https://premake.github.io/">Premake</a>

<a href="https://premake.github.io/download">Download</a> ·
<a href="https://premake.github.io/docs/What-Is-Premake">Learn More</a>

*You may have to add the premake executable to you PATH environment variable.*

To generate the project files, you can run the premake executable that you downloaded, if you already have premake installed make sure it can support generating Visual Studio 2022 project files (premake version v5.0.0-beta1 onwards).

So for generating the project files on Visual Studio 2022 you'd do `premake5.exe vs2022` or on Linux use `premake5 gmake2`.

## Compiling the engine (Windows)

To compile the engine simply open the newly generated project files and build the entire solution.

### Running the engine

Before launching the editor you must create a new project as this repo does not contain a default project.
So, set the project browser as the startup project and run (F5), create a new project and launch it from the browser. After that you can set the Editor (Saturn-Editor) as the startup project and it will automatically load the most recent project. Or a project file specified in the command line argument.

## License

This project is licensed under the MIT License, for more information check the [LICENSE](https://github.com/BEASTSM96/Saturn-Engine/blob/vulkan/LICENSE) file.