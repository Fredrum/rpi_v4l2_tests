# V4L2 Video to GL Hardware Path example for Raspberry Pi    

I'll be putting my Raspberry Pi v4l2 tests here if they are half decent.
The idea is to have examples that are simple to follow and read. Not to be well structured code.
I had trouble finding good examples on the web that did this so thought it might help others if I could make one.

I am working on a Raspberry Pi4b with 32bit Raspbian Buster OS.    
Kernel version,    
Linux sensitpi 5.10.17-v7l+ #1403 SMP Mon Feb 22 11:33:35 GMT 2021 armv7l GNU/Linux    
**The camera I'm using is the RaspiCam2, aka Sony imx219.**    

The first (and currently only) demo is doing camera video capture using V4L2 and setting up DMA buffers to create a 'zero copy' hardware pipeline all the way to GL texture.
I ended up using glfw as that seemed to allow for getting a gl window thrown up with minimal amount of code. There were a few non obvious tricks to getting this to work which I will describe below.

Actually first you might as well try to build and run as it's possible the issues I had have been fixed.    
    
**0. Try Just Build and run.**    
Install some dependencies    
`sudo apt install libglfw3-dev libgles2-mesa-dev`    
    
Install & Build the test program    
`git clone https://github.com/Fredrum/rpi_v4l2_tests.git`    
`cd rpi_v4l2_tests`  
`make`    

If all seems good, run,    
`glDmaTexture`    
    
If that didn't work you might want to read on...


**1. The camera driver (at the time) didn't actually allow for streaming video.**    
If you run:    
`> v4l2-compliance`    
    
near the top you should have a line saying    
`Compliance test for device /dev/video0:`    
    
then if you go to the bottom of the test printout you might see a line that looks something like this,    
`test VIDIOC_EXPBUF: OK (Not Supported)`    
    
If you get this 'not supported' message, then you'll have the same problem as I had as the systems camera driver won't allow for streaming. The solution involves making a small addition to the raspberry kernel camera driver and if you haven't done anything like that before (I hadn't) you'll need to read up a little on how to build and install one of the drivers. It's totally doable though! You'll start by downloading the full kernel tree which might take a while so you might want kick that download off soon! :)    
The source file you must edit is    
`../drivers/staging/vc04_services/bcm2835-camera/bcm2835-camera/bcm2835-camera.c`    
add a line like this    
`.vidioc_expbuf			= vb2_ioctl_expbuf,`    
in struct v4l2_ioctl_ops camera0_ioctl_ops, eg just after ".vidioc_dqbuf = vb2_ioctl_dqbuf," somewhere around line 1481.
Then build and install it on your system. A good tip I got was to make an edit in the info found at the bottom of the driver source and then you can look for that after install+reboot using,    
`modinfo bcm2835-v4l2`    
Also you can run the '4l2-compliance' command again and if its working the line near the bottom should instead say,    
`test VIDIOC_EXPBUF: OK`    
    
NOTE: If you run an general update on your raspberry system it will probably overwrite this fix and you'll have to re-install or re-do.
    
Here's a forum thread about this:  https://www.raspberrypi.org/forums/viewtopic.php?f=43&t=291940    
    
**2. It sort of works but you get only 3-4 fps!**    
I didn't realise that for the camera to be able to operate as a streaming videocamera I also had to add a special config file.
So if this happens for you too then try making a file here,    
`/etc/modprobe.d/bcm2835-v4l2.conf`    
containing the text    
`options bcm2835-v4l2 max_video_width=1920`    
`options bcm2835-v4l2 max_video_height=1080`    
    
To verify that it now works you can run these commands,    
`> v4l2-ctl -v width=1280,height=720,pixelformat=RX24`    
`> v4l2-ctl --stream-mmap --stream-count=-1 -d /dev/video0 --stream-to=/dev/null`    

My forum thread about this one:  https://www.raspberrypi.org/forums/viewtopic.php?f=43&t=298040    


**3. Don't use the imx219 dt overlay!**    
For this particular setup to work we don't want to use this driver. I tried it out and seemed to get low latency and high frame rate but it was a red herring.   
`dtoverlay=imx219`    
This driver is made for capturing raw bayered sensor data and its meant for the 'libcamera' library.    
My forum thread here:  https://www.raspberrypi.org/forums/viewtopic.php?f=107&t=293712    
    
    
**4. Goto  0.**



## Some known issues    
Strangely the RGB pixel color order seems to sometimes switch to BGR and I don't understand why yet. If this happens you can change the pixelformat in eglCreateImageKHR() from DRM_FORMAT_XRGB8888 -> DRM_FORMAT_XBGR8888 or vice versa.    
    
There's tearing in the playback. I haven't bothered trying to fix this yet as there's an as-of-yet unsolved Raspberry OS problem with the tearing. Also I didn't want to add double bufferering as my goal is low latency.    
    
Talking about the latency I think it might be faster and probably also lower system/bus bandwith use setting the camera to produce some YUV format. I didn't want to do that either as I think it would have involved setting up an ISP unit and I wanted to keep it simple to start with. The issue here is that you are limited in what pixelformats are compatible with the RPis 3d graphics, V3D.
    
    
    
Please let me know if you can spot bits that are incorrect or that can be improved!    
    
Cheers!
