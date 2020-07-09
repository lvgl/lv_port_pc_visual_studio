@echo off

:: Batch which will rename the current solution and dependent files to another name inputted by the user
:: It expects that solution name is the same as the project folder and the project name, example:
::  visual_studio_2017_sdl.sln, visual_studio_2017_sdl\visual_studio_2017_sdl.vcxproj.


:: count nr of *.sln files and exit if there is not exactly one *.sln file!
:: see https://stackoverflow.com/questions/6474738/batch-file-for-f-doesnt-work-if-path-has-spaces
setlocal ENABLEDELAYEDEXPANSION
set /a SLN_CNT=0
for /f %%x in ('CALL dir *.sln /b') do (
	if 0 equ !SLN_CNT! (
		:: get the name-only (no extension) of the 1st sln file
		SET old_name=%%~nx
	)
	set /a SLN_CNT+=1
)

:: exit if none or more found
if not [1] == [%SLN_CNT%] (
	echo  
	echo Expected exactly 1 *.sln file in this folder, but found %SLN_CNT%! Exiting...
	pause
	exit /b
)


set /P new_name="Enter the name of new solution? "
echo Confirm that "%old_name%" should be replaced with "%new_name%"!
pause


echo 1. Renamimg current folder files
ren %old_name%.* %new_name%.*
::pause

echo 2. Renamimg current subfolder
ren %old_name% %new_name%
::pause

echo 3. Renamimg subfolder files
ren %new_name%\%old_name%.* %new_name%.*
::pause

echo 4. Replacing in files
SETLOCAL
CALL :ReplaceInFile %new_name%.sln
::pause
CALL :ReplaceInFile %new_name%\%new_name%.vcxproj
::pause
CALL :ReplaceInFile %new_name%\%new_name%.vcxproj.user
echo 5. All done!
echo  
pause
EXIT /B %ERRORLEVEL%


:ReplaceInFile
:: https://stackoverflow.com/questions/23075953/batch-script-to-find-and-replace-a-string-in-text-file-without-creating-an-extra/23076141
setlocal enableextensions disabledelayedexpansion
	set "replacedFile=%~1"
	for /f "delims=" %%i in ('type "%replacedFile%" ^& break ^> "%replacedFile%" ') do (
		set "line=%%i"
		setlocal enabledelayedexpansion
		>>"%replacedFile%" echo(!line:%old_name%=%new_name%!
		endlocal
	)
EXIT /B 0
echo Unexpected execution point!
echo  
pause
