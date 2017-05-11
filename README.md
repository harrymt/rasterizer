# Rasterizer

Rasterizer for the University of Bristol Computer Graphics course (course work 2), links with the [Ray Tracer](https://github.com/harrymt/ray_tracer/).

See the [report](report.pdf) for a full overview.


## How to Build

- Download a copy of [GLM](http://glm.g-truc.net) and place it inside of the root directory.
- Then use [Makefile](Makefile) to build the program by running the following command in the directory.

```
make
```

- A rendered image of the [Cornell Box](https://en.wikipedia.org/wiki/Cornell_box) should appear.


## Latest Rendered Image

![Screenshot](screenshot.bmp "Rendered Image")

## Features

Basic components of a rasteriser were implemented along with the following extensions:

- Optimisations
- Shadows
- Soft Shadows
- Interpolation Changes & Basic Clipping
- Anti-Aliasing - FXAA
- Parallelisation - GPU
- Frame buffer and Deferred Rendering
- Directional Lights - Light as a Camera

### YouTube Videos

Four clips show the rasteriser in action:

- [50 shadow samples CPU vs GPU](https://youtu.be/nlUu7aPz-H4)
- [2048 * 2048 CPU vs GPU](https://youtu.be/siGBA8brz9E)
- [3 Soft Shadow Samples GPU](https://youtu.be/HQDrsd6H4bY)
- [25 Soft Shadow Samples, 2048 * 2048 Shadow Map GPU](https://www.youtube.com/watch?v=LAGSdhaoWEs)



## Setup project with Visual Studio 201X for Windows

- Get a GLM folder and the correct version of SDL, we are using SDL 1.2.5.
- Place `glm` folder inside of Source, for ease
- Extract SDL contents and place `include` and `lib` folders inside of a directory you can remember, e.g. I have mine at `c:\vs_lib`.
- Take SDL.dll and place it into your project Source folder also (we are gonna move it in a sec)
- Okay now we are ready to open VS (File -> New -> Create new project from Source -> Console app -> Select directory where you have cloned this repo)
- When its loaded, rightclick project name (not solution) and select properties


In Properties change these 3 sections (I have chosen for DEBUG: x64)

1. VC++ Directories
    - Include Directories, add `c:\vs_lib\include`
    - Library Directories, add `c:\vs_lib\lib`
2. C/C++
    - Additional Include Directories, add the location of your GLM folder, e.g. `C:\Users\harrymt\Documents\GitHub\ray_tracer\Source\glm`
3. Linked
    - Input, `SDL.lib;SDLmain.lib;`


- Based on this tutorial: (google lazyfoo SDL 1.2)
- For GLM: http://stackoverflow.com/questions/17905794/how-to-setup-the-glm-library-in-visual-studio-2012

## Technologies

Uses the following technologies:

- [SDL](http://www.libsdl.org)
- [GLM](http://glm.g-truc.net)
