# Building

## Generating project files

In order to start you will need to download <a href="https://premake.github.io/">Premake</a>

<a href="https://premake.github.io/download">Download</a> ·
<a href="https://premake.github.io/docs/What-Is-Premake">Learn More</a>

*You may want to add the premake executable to you PATH environment variable.*

On Windows, Visual Studio 2022 is recommended as Saturn is officially untested on other development environments.

On macOS, Unix Makefiles can be used or Xcode.
Please note that if you use Xcode you'll need to generate Unix Makefiles when compiling your game.

So for generating the project files on Visual Studio 2022 you'd do `premake5.exe vs2022` or on Linux use `premake5 gmake2`.

For macOS, please use `premake5 --file=premake5-AArch.lua [action]`. Where `[action]` is `xcode4` or `gmake`
*Please note that depending on your premake version you may want to use `gmake2`*

## Compiling (Windows)

To compile the engine simply open the newly generated project files and build the entire solution.

## Compiling (macOS)

For macOS building the engine depends on your build system:

Using Unix Makefiles:

- To build the editor invoke `make -j Saturn-Editor config=buildconfig`
- To build the project browser invoke `make -j Saturn-ProjectBrowser config=buildconfig`
- Or you can just build everything.

`buildconfig` is the config you want to build, either `debug-asan`, `debug`, `release` or `dist`

Using Xcode:

- Select the proudct you want to build and do `Cmd+B`

## Compiling SaturnBuildTool (non-windows platforms)

On platforms other than windows compiling the SaturnBuildTool is different.

First on macOS, invoke `premake5 vs2022 --file=SaturnBuildTool/SBT-ForNonWindows.lua --os=macosx`

Or on Linux, invoke `premake5 vs2022 --file=SaturnBuildTool/SBT-ForNonWindows.lua --os=linux`

And to build invoke `dotnet build -c [config] SaturnBuildTool/SaturnBuildTool.sln`

Where `[config]` is either `Debug-ASan`, `Debug`, `Release` or `Dist`.

### Running the Editor

Before launching the editor you must create a new project as this repo does not contain a default project.

On Visual Studio you may need to set Saturn-ProjectBrowser as the startup project and make sure to set it back to the Editor once your done.

You can also specify a path to a projet file in the command line argument e.g. `D:\MyProjects\Project1\Project1.sproject` or someting like `/Users/user/MyProjects/Project1/Project1.sproject` on macOS.

For macOS and Linux you will need to make sure the working directory is set correctly.

For example if you are running the Editor, make sure the working directory is set to `[repo_path]/Saturn-Editor`. Same applies to Saturn-ProjectBrowser.

## Note

The macOS port is experimental and is very unstable.
The macOS port is only available on Apple Silicon (Arm based CPUs).
