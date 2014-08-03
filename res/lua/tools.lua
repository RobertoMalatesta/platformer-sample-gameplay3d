_toolsRoot = io.popen"pwd":read'*l'

if Game.getInstance():getConfig():getString("debug_os") == "windows" then
    _toolsRoot = io.popen"cd":read'*l'
end

_toolsRoot = string.gsub(_toolsRoot, "\\", "/")
_toolsRoot = string.gsub(_toolsRoot, "/build", "")
_autoGeneratedWarningPrefix = "This file was automatically generated by "

function ls(directory)
    local i, files, popen, command = 0, {}, io.popen, "ls"

    if Game.getInstance():getConfig():getString("debug_os") == "windows" then
        commmand = "dir /B"
    end

    local fileListFile = popen(command .. ' "'..directory..'"')

    for filename in fileListFile:lines() do
        i = i + 1
        files[i] = filename
    end

    fileListFile:close()

    return files
end

function rm(pattern)
    local commmand = "rm -rf "
    if Game.getInstance():getConfig():getString("debug_os") == "windows" then
        commmand = "del /S /Q "
    end
    os.execute(commmand .. pattern)
end

function mkdir(dir)
    os.execute("mkdir " .. dir)
end

function runTool(toolName)
    if Game.getInstance():getConfig():getBool("debug_" .. toolName) then
        Game.getInstance():getScriptController():loadScript("res/lua/tools/" .. toolName .. ".lua")
    end
end

runTool("generate_lua_bindings")
runTool("generate_android")
runTool("clean_android")
runTool("build_android")
runTool("deploy_android")
