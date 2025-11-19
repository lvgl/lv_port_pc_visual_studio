# How to synchronize LVGL related submodules

First you should use `git submodule update --remote` to update the LVGL
related submodules to the latest.

Then you should open `LVGL.MaintainerTools.slnx` and run `LvglProjectFileUpdater`
project to synchronize the files from the current LVGL related submodules to
project files.

Finally you should open `LVGL.MaintainerTools.slnx` and run `LvglModuleDefinitionGenerator`
project to synchronize `LvglWindows\LvglWindows.def`.
