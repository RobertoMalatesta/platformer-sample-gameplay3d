local pngCrushCommand = _toolsRoot .. "/build/external/pngcrush/pngcrush"
local textureDir = _toolsRoot .. "/res/textures"
for index,fileName in pairs(ls(textureDir)) do
    if string.match(fileName, "%.png") then
      local texutrePath = textureDir .. "/" .. fileName
      os.execute(pngCrushCommand .. " -ow " .. textureDir .. "/" .. fileName)
    end
end
