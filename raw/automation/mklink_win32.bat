set GAME_DIR=bin/%1
CALL mklink /H "game.config" "../game.config"
CALL mklink /H "game.config" "./../../%GAME_DIR%/game.config"
CALL mklink /J "res" "../res"
CALL mklink /J "res" "../../../%GAME_DIR%/res"
