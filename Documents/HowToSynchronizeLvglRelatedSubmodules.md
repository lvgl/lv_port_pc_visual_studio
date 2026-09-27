# How to synchronize LVGL related submodules

First, run `git submodule update --remote` to update the LVGL related
submodules to the latest commits on their configured branches.

Then, build the affected Visual Studio projects. The MSBuild tasks
automatically synchronize the project files and filters with the current
submodule contents. Configuration migration updates `lv_conf.h` from the
LVGL template and the project's `lv_conf.defaults` when the Git migration
check indicates that migration is required.

If a `.vcxproj` file changes, the build intentionally stops after migration.
Review the generated changes, reload the updated projects if necessary,
and build again.

The `LvglWindows.def` file is generated automatically in the intermediate
directory when building `LvglWindows`. Build `LvglWindowsStatic` successfully
for the same configuration and platform first.

If the MSBuild task assembly needs to be rebuilt, open
`LVGL.MaintainerTools.slnx` and build `Lvgl.Build.Tasks`. The build copies
`Lvgl.Build.Tasks.dll` to `LvglPlatform` automatically.
