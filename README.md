# G@AS Engine by ChaCheeChoo
#### PSP Game Engine that does what it needs to do

## Features
 - 2D Rendering
 - 3D Rendering
 - Controller Input
 - WAV Audio
 - MP3 Audio
 - GWD Filesystem*

## Compiling
You need pspsdk to compile this

1. Run ```make all2```
2. Copy the contents of the build directory into your project
3. Link ```-lgaas``` in your project

## License
This engine is licensed under the WTFPL License, meaning you can do whatever the fuck you want with this, but putting something like "Uses G@AS Engine by ChaCheeChoo" in your project would be appreciated

## Extra
*GWD Filesystem is an optional thing that allows you to pack all your game assets into one .gwd file which you can then put in the .PSAR section of your game EBOOT. Documentation on how to use it can be found in ```source/gwd.h```
