# Notes on Vulkan

## Guides

1. [Vulkan Tutorial](https://vulkan-tutorial.com/)
2. [Vulkan in 30 minutes](https://renderdoc.org/vulkan-in-30-minutes.html)

## What is Vulkan?

Vulkan is a:

1. Cross-platform
2. Compute and graphics API 

that aims to reduce driver overhead while rendering. Vulkan is an open standard that maps well to how modern GPUs function. By providing low-level control and support for multithreading, applications written in Vulkan can efficiently utilize hardware resources. 

## Steps required to draw the first triangle

1. Create a VkInstance
2. Select a supported graphics card (VkPhysicalDevice)
3. Create a VkDevice and VkQueue for drawing and presentation
4. Create a window, window surface and swap chain
5. Wrap the swap chain images into VkImageView
6. Create a render pass that specifies the render targets and usage
7. Create framebuffers for the render pass
8. Set up the graphics pipeline
9. Allocate and record a command buffer with the draw commands for every swap chain image
10. Draw frames by acquiring images, submitting the right draw command buffer and returning the images back to the swap chain

The general pattern that object creation function parameters in Vulkan follow is:

- Pointer to struct with creation info
- Pointer to custom allocator callbacks
- Pointer to the variable that stores the handle to the new object

Vulkan validation layers are optional components that hook into Vulkan function calls to enforce proper API usage.

Vulkan needs Window System Integration (WSI) extensions to interface with the underlying windowing system.

Vulkan does not have the concept of a "default framebuffer", hence it requires an infrastructure that will own the buffers we will render to before we visualize them on the screen. This interface is called a swap chain. The swap chain is essentially a queue of images waiting to be presented to the screen.

An image view describes how to access the image and which part of the image to access.

All GPU commands in Vulkan are sent through command buffers which get executed on a queue.

Command buffers are allocated from a command pool, which should be unique per thread. Also, multiple threads can record commands, but only one thread can submit it.

A GPU can execute multiple queues in parallel. Commands in a queue are guaranteed to start in submission order, but can end out of order.

`VkFence` is used for CPU - GPU synchronization. If this is set, we can know from the CPU if the GPU has finished these operations.

For GPU - GPU synchronization, `VkSemaphore` is used. Semaphores allow defining order of operations on GPU commands.

GPUs store images in different formats, and image layout transitions are required to have the image in a valid state before rendering.

Request image -> Reset command buffer -> Start rendering commands -> End rendering commands -> Submit to graphics queue -> Wait for fence -> Present -> Reset fence.

The graphics pipeline is a collection of programmable and fixed-function stages.

Programmable stages include:

1. Vertex shader: Runs for every vertex. Performs transformation operations to turn position from model space to screen space. Also passes per-vertex data down the pipeline.
2. Tesselation shader: Subdivides geometry based on certain rules to increase mesh quality.
3. Geometry shader: Runs for every primitive and can discardit or output more primitives than came in.
4. Fragment shader: Invoked for each fragment. Determines the color values of a fragment using the interpolated data from the vertex shader.

Fixed-function stages include:

1. Input assemble: The first stage which takes in vertex and index buffers and generates primitives like triangles, triangle strips, etc.
2. Rasterizer: Discretizes the primitives into fragments. Discards fargments through depth testing and clipping.
3. Color blending: Applies operations to mix different fragments that map to the same pixel in the framebuffer. Fragments can simply overwrite each other, add up, or be mixed based upon transparency.

The graphics pipeline in Vulkan is almost completely immutable, compared to older APIs, where it can be adjusted dynamically. This allows for greater optimization by the driver.

Shader code in Vulkan is specified in Standard Portable Intermediate Representation (SPIR-V) format as opposed to a textual format like HLSL or GLSL. SPIR-V also allows different shader formats to be converted to each other easily. Bytecode format reduces driver complexity and facilitates conformance between different drivers.

A clip coordinate is a 4D vector from the vertex shader that is subsequently turned into a normalized device coordinate by dividing the whole vector by its last component. In Vulkan, the NDC Y is flipped, Z ranges from 0 to 1 and X ranges from -1 to 1.

Vulkan allows us to configure fixed function stages to a limited extent with limited dynamic configuration.