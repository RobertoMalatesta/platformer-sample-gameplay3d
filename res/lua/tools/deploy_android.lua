os.execute(Game.getInstance():getConfig():getString("debug_ant_dir") .. "/" .. "ant -buildfile ../android/build.xml debug install")
