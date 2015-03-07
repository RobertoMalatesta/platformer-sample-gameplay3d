local androidCommand = Game.getInstance():getConfig():getString("debug_android_sdk_tools_dir") .. "/" .. "android"
local ndkCommand = Game.getInstance():getConfig():getString("debug_android_ndk_dir") .. "/" ..  "ndk-build"
local numCompileThreads = Game.getInstance():getConfig():getInt("debug_num_android_compile_jobs")
os.execute(androidCommand .. " update project -t 1 -p ../android -s")
os.execute(ndkCommand .. " -C ../android/gameplay -j " ..  numCompileThreads)
os.execute(ndkCommand .. " -C ../external/gameobjects-gameplay3d/android -j " ..  numCompileThreads)
os.execute(ndkCommand .. " -C ../android -j " ..  numCompileThreads)
