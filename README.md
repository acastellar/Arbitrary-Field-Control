Arbitrary Field Control (AFC) currently implements a GPU based particle and mesh Vulkan renderer and compute pipeline. It can simulate and render large numbers of particles in real-time, limited by the capability of the GPU. The compute pipeline updates particle storage buffers directly on the GPU to avoid the roundtrip. My GPU (Nvidia 4080 RTX) can simulate 50,000,000 particles with a moving point of mass at 60 fps and is limited by the graphics pipeline (on Linux).


To build this project you will need to install the Vulkan SDK, GLFW, and GLM (if not already installed).  
System specific instructions:  
* Windows you should to modify CMakeLists with the library paths for GLFW and GLM
* Linux you should install the libraries with your package manager of choice
* Mac has not been tested but should be possible with MoltenVK given some minor modifications


There is no configuration support as this is a prototype, but editing the code to change the number of particles, vertices, uniform buffers, and shaders is easy. 


I intend to come back to this prototype in the future to add actual configuration, a variety of fields, and control mechnisims.

An additional note: if you are on Linux with Nvidia drivers you may have a smoother experience if you force the present mode to mailbox vsync. This can be done by passing in '1' to the rendering engine's constructor.
