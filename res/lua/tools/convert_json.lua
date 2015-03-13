local convertCommand = _toolsRoot .. "/build/external/json-to-gameplay3d/json2gp3d"

function convert(resourceDirName, hasRules)
  local rawDir = _toolsRoot .. "/raw/" .. resourceDirName
  local resDir = _toolsRoot .. "/res/" .. resourceDirName
  for index,fileName in pairs(ls(rawDir)) do
      if string.match(fileName, "%.json") and not string.match(fileName, "rules.json") then
        local rawPath = rawDir .. "/" .. fileName
        local resPath = resDir .. "/" .. string.gsub(fileName, ".json", ".ss")
        local command = convertCommand .. " -i " .. rawPath .. " -o " .. resPath

        if hasRules then
          command = command .. " -r " .. rawDir .. "/rules.json"
        end

        os.execute(command)
      end
  end
end

convert("levels", false)
convert("spritesheets", true)
