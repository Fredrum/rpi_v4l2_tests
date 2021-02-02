I'll be putting my Raspberry Pi v4l2 tests here if they are half decent.
The idea is to have examples that are simple to follow and read. Not to be well structured code.
I had trouble finding something on the web that did this so thought it might help others if I could do it instead.

I am working on a Raspberry Pi4b with 32bit Raspbian Buster OS.
Kernel version,
Linux raspberrypi 5.4.72-v7l+ #1356 SMP Thu Oct 22 13:57:51 BST 2020 armv7l GNU/Linux
The camera I'm using is the RaspiCam2, aka Sony imx219.

The first (and only) demo is doing camera video capture using V4L2 and setting up DMA buffers to create a 'zero copy' hardware pipeline all the way to GL texture.
I ended up using glfw as that seemed to allow for getting a gl window thrown up with minimal amount of code. There were a few non obvious tricks to getting this to work which I will describe below.

**1. The camera driver (at the time of writing) didn't actually allow for streaming video.**
If you run:    
`> 4l2-compliance`    
near the top you should have a line saying    
`Compliance test for device /dev/video0:`    
then if you go to the bottom of the test printout you should see a line that looks something like this,    
`test VIDIOC_EXPBUF: OK (Not Supported)`    
If you get this 'not supported' message, then you'll have the same problem as I had as the systems camera driver won't allow for streaming. The solution involves making a small addition to the raspberry kernel camera driver and if you haven't done anything like that before (I hadn't) you'll need to do read up a little on how to build and install one of the drivers. It's totally doable though! You'll start by downloading the full kernel tree which might take a while so you might want kick that download off soon! :)    
The source file you must edit is    
`drivers/staging/vc04_services/bcm2835-camera/bcm2835-camera/bcm2835-camera.c`    
add a line like this    
`.vidioc_expbuf			= vb2_ioctl_expbuf,`    
in struct v4l2_ioctl_ops camera0_ioctl_ops, eg just after ".vidioc_dqbuf = vb2_ioctl_dqbuf," somewhere around line 1481.
The build an install it on your system. A got tip I got was to make some edit in the info found at the bottom of the driver source and then you can look for that after install + reboot using,    
`modinfo bcm2835-v4l2`




https://www.raspberrypi.org/forums/viewtopic.php?f=43&t=291940

https://www.raspberrypi.org/forums/viewtopic.php?f=43&t=298040

v4l2-ctl -d /dev/video0 --list-formats

https://www.raspberrypi.org/forums/viewtopic.php?f=107&t=293712

sudo apt install libglfw3-dev libgles2-mesa-dev

