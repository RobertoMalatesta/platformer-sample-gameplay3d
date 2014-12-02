os.execute(Game.getInstance():getConfig():getString("debug_android_sdk_tools_dir") .. "/" .. "android update project -t 1 -p ../android -s")
os.execute(Game.getInstance():getConfig():getString("debug_android_ndk_dir") .. "/" ..  "ndk-build -C ../android -j " ..  Game.getInstance():getConfig():getInt("debug_num_android_compile_jobs"))
