# LVGL - PC Simulator using Visual Studio

This is a pre-configured Visual Studio project to try LVGL on a Windows PC. The project uses the [SDL](https://www.libsdl.org/) library which is copied and linked to the project, so you can compile it without any extra dependencies. The 64 bit libraries are used so it will work out-of-the-box on 64-bit systems.

The project is currently maintained using Visual Studio 2019.  It may well work without modification on Visual Studio 2017 but it is not actively maintained to work with that version, so please upgrade to 2019 before reporting any bugs.

**This project is not for Visual Studio Code, it is for Visual Studio 2019.**

Instructions for cloning, building and running the application are found below.

## How to Clone

This repository contains other, necessary LVGL software repositories as [git submodules](https://git-scm.com/book/en/v2/Git-Tools-Submodules).  Those submodules are not pulled in with the normal git clone command and they will be needed.  There are a couple of techniques to pull in the submodules.

### Everything at Once

This command will clone the lv_sim_visual_studio_sdl repository and all submodules in a single step.

```
git clone --recurse-submodules https://github.com/lvgl/lv_sim_visual_studio_sdl.git
```

### Main Repository First, Submodules Second

If you've already cloned the main repository you can pull in the submodules with a second command.  Both commands are shown below.

```
git clone https://github.com/lvgl/lv_sim_visual_studio_sdl.git
cd lv_sim_visual_studio_sdl
git submodule update --init --recursive
```

### Keeping Your Clone Up-To-Date

If you have cloned this repository and would like to pull in the latest changes, you will have to do this in two steps.  The first step will pull in updates to the main repo, including updated _references_ to the submodules.  The second step will update the code in the submodules to match those references.  The two commands needed to accomplish this are shown below, run these commands from inside the main repository's directory (top level lv_sim_visual_studio_sdl directory works fine).

```
git pull
git submodule update --recursive
```

If you have chosen to fork this repository then updating the fork from upstream will require a different, more involved procedure.

## How To Build & Run

Open the `lv_sim_visual_studio_sdl.sln` solution file in Visual Studio. Click on the _Local windows Debugger_ button in the top toolbar.  The included project will be built and run, launching from a cmd window.

## Trying Things Out

There are a list of possible test applications in the [main.c](visual_studio_2017_sdl/main.c) file.  Each test or demo is launched via a single function call.  By default the `lv_demo_widgets` function is the one that runs, but you can comment that one out and choose any of the others to compile and run.

Use these examples to start building your own application test code inside the simulator.

## A Note About Versions

This repository has its submodule references updated shortly after the release of new, major releases of LittlevGL's core [lvgl](https://github.com/lvgl/lvgl) project.  Occasionally it is updated to work with minor version updates.  When submodule updates take place a matching version tag is added to this repository.

If you need to pull in bug fixes in more recent changes to the submodules you will have to update the references on your own.  If source files are added or removed in the submodules then the visual studio project will likely need adjusting.  See the commit log for examples of submodule updates and associated visual studio file changes to guide you.
