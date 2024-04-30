# How to synchronize LVGL related submodules

First you should use `git submodule update --remote` to update the LVGL
related submodules to the latest.

Then you should open `LVGL.MaintainerTools.sln` and run `LvglProjectFileUpdater`
project to synchronize the files from the current LVGL related submodules to
project files.
