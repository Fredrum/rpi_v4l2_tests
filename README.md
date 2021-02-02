I'll be putting my Raspberry Pi v4l2 tests here if they are half decent.
The idea is to have examples that are simple to follow and read. Not to be well structured code.

I am working on a Raspberry Pi4b with 32bit Raspbian Buster OS.
Kernel version,
Linux raspberrypi 5.4.72-v7l+ #1356 SMP Thu Oct 22 13:57:51 BST 2020 armv7l GNU/Linux
The camera I'm using is the RaspiCam2, aka Sony imx219.

The first demo is doing camera video capture using V4L2 and setting up DMA buffers to create a 'zero copy' hardware pipeline all the way to GL texture.
I ended up using glfw as that seemed to allow for getting a gl window thrown up with minimal amount of code. There were a few non obvious tricks to getting this to work which I will describe below.

https://www.raspberrypi.org/forums/viewtopic.php?f=43&t=291940

https://www.raspberrypi.org/forums/viewtopic.php?f=43&t=298040

v4l2-ctl -d /dev/video0 --list-formats

https://www.raspberrypi.org/forums/viewtopic.php?f=107&t=293712

sudo apt install libglfw3-dev libgles2-mesa-dev

