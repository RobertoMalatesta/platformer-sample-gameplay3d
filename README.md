#2D platformer sample v1.1.0

A 2D platformer made using the open source game framework Gameplay3D, other freely available cross platform tools and free to use assets.

![ScreenShot](https://raw.githubusercontent.com/louis-mclaughlin/platformer-sample-gameplay3d/master/raw/textures/platformer_big.jpg)

### Usage
Download the external dependencies:
```
git submodule update --init
```

Download the gameplay dependencies:
```
sudo apt-get install build-essential gcc cmake libglu1-mesa-dev libogg-dev libopenal-dev libgtk2.0-dev curl libpcrecpp0:i386 lib32z1-dev
cd external/GamePlay
./install.sh
```
Build the sample:
```
cd ../..
mkdir build
cd build
cmake ..
make
```

Run the sample:
```
./platformer
```
- See 'game.config' for configuarable debug options and tools
- Use either a gamepad or the keyboard (directional arrows/WASD + space) to make the player move and jump

### Tools used
- [Audacity](http://audacity.sourceforge.net/)
- [GIMP](http://www.gimp.org/)
- [json-to-gameplay3d](https://github.com/louis-mclaughlin/json-to-gameplay3d)
- [SFXR](http://www.drpetter.se/project_sfxr.html)
- [TexturePacker (lite)](https://www.codeandweb.com/texturepacker)
- [Tiled](http://www.mapeditor.org/)

### Content pipeline
- Audio: SFXR -> (.wav) -> Audacity (.ogg)
- Levels: Tiled (.json) -> json-to-gameplay3d (.level)
- Spritesheets: TexturePacker (.json) -> json-to-gameplay3d (.ss)

### External assets used
- [https://www.google.com/fonts/specimen/Open+Sans](https://www.google.com/fonts/specimen/Open+Sans)
- [http://kenney.nl/assets](http://kenney.nl/assets)
- [https://github.com/blackberry/GamePlay/blob/master/gameplay/res/logo_black.png](https://github.com/blackberry/GamePlay/blob/master/gameplay/res/logo_black.png)
