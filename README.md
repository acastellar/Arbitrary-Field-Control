Arbitrary Field Control (AFC) currently implements a GPU based particle and mesh Vulkan renderer and compute pipeline. It can simulate and render large numbers of particles in real-time, limited by the capability of the GPU. The compute pipeline updates particle storage buffers directly on the GPU to avoid the roundtrip. The Nvidia 4080 RTX can simulate ~50,000,000 particles with a moving point of mass at 60+ fps (on Linux) and is limited by the graphics pipeline.


To build this project you will need to install the Vulkan SDK, GLFW, and GLM (if not already installed).  
System specific instructions:  
* Windows you should modify CMakeLists with the library paths for GLFW and GLM
* Linux you should install the libraries with your package manager of choice
* Mac you should install MoltenVK, GLFW, and GLM with brew (or manually) 


There is no file configuration support as this is a prototype, but editing the code to change the number of particles, vertices, uniform buffers, and shaders is easy. 


I intend to come back to this prototype in the future to add file configuration, fields, and control mechanisms.

An additional note: When on linux the program forces mailbox present mode to avoid a driver bug on current Nvidia drivers. If you are encountering issues you can disable this by setting forceLinuxPresentMode to false in main.
