# Rasterizer

Rasterizer for the University of Bristol Computer Graphics course (course work 2).

# Extensions - 75% predicted grade

### Ray tracer - 90% * 0.5
    - Base : 50%
    - Optimisations : 4%
    - Parallelisation : 4%
    - Soft shadows : 4%
    - Anti Aliasing : 4%
    - Photon mapping / ~Caustics~ / Colour bleeding : 20%
    - General Model loading 4%
    - Spacial datastructures (Photon map)

### Rasterizer - 58% * 0.5
    - Base : 50%
    - Parallelisation : 4% *Questionable - not sure if I'll have time to port to kernels*
    - Optimisations : 4%
   

### TODO
http://www.cs.cmu.edu/afs/cs/academic/class/15869-f11/www/ <- Great resource
    - Parrellisation, different sort
    - Shadows (DOING)
    - Anti aliasing -(FXAA or MLAA)

## How to Build

Simply use the [Makefile](Makefile) to build the program by running the following command in the directory.

```
make
```

A rendered image of the [Cornell Box](https://en.wikipedia.org/wiki/Cornell_box) should appear.
Note: SDL should be installed on the machine.

## Latest Rendered Image

![Screenshot](screenshot.bmp "Rendered Image")

## How to setup project for Visual Studio 201X for Windows

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
