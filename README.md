# Visual Studio 2017 using SDL on x64 architecture

A pre-configured Visual Studio Project to try LittlevGL on PC. The project uses the [SDL](https://www.libsdl.org/) library which is copied and linked to the project, so you can compile it without any extra steps. The 64 bit libraries are used so it will work out-of-the-box on 64-bit systems.

Instructions for cloning, building and running the demo are found below.

## How to Clone

This repos contains other, necessary LittleVGL software repositories as [git sub-modules](https://git-scm.com/book/en/v2/Git-Tools-Submodules).  Those sub-modules are not pulled in with the basic git clone command and they will be needed.  There are a couple of ways to pull in those submodules.

### Everything at Once

This command will clone this top level repo and all submodules in a single step.

```
git clone --recurse-submodules https://github.com/littlevgl/pc_simulator_sdl_visual_studio.git
```

### Main Repo First, Sub-modules Second

If you've already cloned the main repo you can pull in the sub-modules with a second command.  Both commands are shown below.

```
git clone https://github.com/littlevgl/pc_simulator_sdl_visual_studio.git
cd pc_simulator_sdl_visual_studio
git submodule update --init --recursive
```

## How To Build & Run

Open the `pc_simulator_sdl_visual_studio` solution file in Visual Studio. Click on the _Local windows Debugger_ button in the top toolbar.  The included project will be built and run, launching from a cmd window.

## Trying Things Out

There are a list of possible test applications in the [main.c](visual_studio_2017_sdl/main.c) file.  Each test or demo is launched via a single function call.  By default the `demo_create` function is the one that runs, but you can comment that one out and choose any of the others to compile and run.

Use these examples to start building your own application testing code inside this pc_simulator.
