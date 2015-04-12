set GAME_DIR=bin/%1

call :make_links Debug
call :make_links MinSizeRel
call :make_links Release
call :make_links RelWithDebInfo

goto :exit

:make_links
set CONFIG_BIN_DIR=%GAME_DIR%\%1
mkdir "%CONFIG_BIN_DIR%"
CALL mklink /H "%CONFIG_BIN_DIR%/OpenAL.dll" "../external/GamePlay/bin/windows/OpenAL.dll"
CALL mklink /H "%CONFIG_BIN_DIR%/game.config" "../game.config"
CALL mklink /H "%CONFIG_BIN_DIR%/user.config" "../user.config"
CALL mklink /H "%CONFIG_BIN_DIR%/default_user.config" "../default_user.config"
CALL mklink /J "%CONFIG_BIN_DIR%/res" "../res"
goto:eof

:exit
