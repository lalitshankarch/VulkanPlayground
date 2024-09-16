# Notes on Vulkan

Vulkan is an explicit API designed to give more control back to the developers, in terms of how they organize their rendering architecture. Having greater control on the application level means more potential to squeeze out as much performance from the GPU.

`VkInstance` represents the current Vulkan configuration the developer is intending to use on their system.

`VkSurface` is a connection to the underlying OS-specific window surface (Window System Integration).

`VkPhysicalDevice` refers to a physical device that is capable of running Vulkan. We enumerate physical devices and then select an appropriate one for creating a device instance (`VkDevice`).

`VkQueue` receives commands to be executed on a physical device. Before creating a logical device, we need to specify the number and type of queues we would like to create on it. Commands, more precisely command buffers are queued for processing. Commands start being processed in submission order, but can complete out of order. It's like a single way, single lane motorway in how it works compared to a traditional FIFO queue.

A queue always belongs to a specific queue family, which supports different operations (graphics, compute, transfer, etc).

`VkDevice` is a handle to the physical device with a specific configuration in mind.

`VkKHRSwapchain` provides images for rendering and presentation.

Vulkan: Uses `VK_KHR_surface` and `VK_KHR_swapchain` extensions to handle platform-specific functionality in a modular way, providing flexibility for different windowing systems.

DirectX: Uses `DXGI` and `Win32` to manage surface and swapchain functionalities, tightly integrated with the Windows operating system.

High level overview of rendering a model:

    - Create instance
    - Create surface
    - Pick a physical device
    - Select a queue family
    - Create logical device and get queue
    - Create a swapchain
    - Get swapchain images and image views

    (The steps above can be automated by `vk-bootstrap`)

    - Read SPIR-V shaders and create shader modules
    - Create graphics pipeline from the shader modules
    - Load model and setup vertex and index buffers
    - Get next swapchain image
    - Record and finish commands
    - Present current swapchain image
    - Wait for all command queues to finish work
    - Cleanup all allocated resources

Vulkan validation layers facilitate proper API usage.

The graphics pipeline is the sequence of operations that take the vertices and textures of the meshes all the way to the pixels in the render targets.

APIs like `OpenGL` and `DX 11` calculate pipeline objects automatically, while Vulkan enables the programmer to cache pipeline objects or generate them "on the fly" while rendering. Creating pipelines are expensive, so it's best to create them on a background thread or on a loading screen.

Most of the state validation in Vulkan occurs when creating objects while the render loop is relatively inexpensive.

To perform commands on the GPU, we allocate a command buffer, start encoding things on it, and then execute it by adding it to a queue.

Submitting a command buffer to a queue means it starts processing on the GPU side. Submitting mutiple command buffers on multiple queues may mean that the commands may execute in parallel.

Vulkan gives explicit control over the rendering architecture. This means that frame can be rendered offline (headless mode); over the network; or on a screen.

Swapchain image views are conceptually similar to Render Target Views on DirectX 12.

The general flow to execute commands is:

    - Allocate a `VkCommandBuffer` from a `VkCommandPool`
    - Record commands into the command buffer
    - Submit the command buffer into a `VkQueue` through `VkQueueSubmit`

Multiple threads can record commands into a single queue but only one thread should submit them to a queue. Also, each thread should use its own copy of a command allocator and command buffer.

In Vulkan, all of the rendering happens inside a `VkRenderPass`. It's possible to do compute commands outside of a renderpass. `VkRenderPass` is an object that encapsulates the state needed to setup the target for rendering, and the state of the images we will be rendering to.

A `VkRenderPass` renders into a `VkFramebuffer`. It allows to driver to know more about the state of images presenting more opportunities for optimization. The framebuffer links to the images we will render to, and it's used when starting a renderpass to set the target images for rendering.

The general use of a renderpass when encoding commands is like this:

```
vkBeginCommandBuffer(cmd, ...);

vkCmdBeginRenderPass(cmd, ...);

//rendering commands go here

vkCmdEndRenderPass(cmd);

vkEndCommandBuffer(cmd)
```

A renderpass consists of subpasses, which are a bit like rendering steps. A renderpass should contain a minimum of 1 subpass.

A renderpass performs image layout changes when entering and exiting the renderpass. For optimization purposes, GPUs compress textures or reorder pixels so that they mipmap better.

The color attachment is the description of the image we will be writing into with rendering commands.

Vulkan offers explicit synchronization structures to allow the CPU to sync execution of commands with the GPU.

`VkFence` is used for GPU -> CPU synchronization. If this is set, we can know from the CPU if the GPU has finished these operations.

Pseudocode:

```
VkFence myFence;

//start some operation on the GPU
VkSomeOperation(whatever, myFence);

// block the CPU until the GPU operation finishes
VkWaitForFences(myFence);

//fences always have to be reset before they can be used again
VkResetFences(myFence);
```

For GPU - GPU synchronization, `VkSemaphore` is used. Semaphores allow defining order of operations on GPU commands.

Requesting an image from the swapchain will block the CPU thread until the image is available. Using V-sync'd modes block the CPU for long while modes like mailbox return almost immediately.

Request image -> Reset command buffer -> Start rendering commands -> End rendering commands -> Submit to graphics queue -> Wait for fence -> Present -> Reset fence.

Unlike older APIs, shader code in Vulkan has to be specified in a bytecode format as opposed to human readable syntax like GLSL/HLSL. The bytecode, called SPIR-V (Standard, Portable Intermediate Representation).

The vertex shader runs for each vertex. It takes attributes like color, world position, normal and texture coordinates as input. The output is a final position in clip coordinates andattributes that need to be passed on to the fragment shader. The rasterizer will then interpolate the values to produce a smooth gradient.

The vertex shader receives coordinates in Normalized Device Coordinates (NDC) format. The origin is in the center, the Y axis is flipped, and Z ranges from 0 to 1 (transformed from -1 to 1).

We can directly output normalized device coordinates by outputting them as clip coordinates from the vertex shader with the last component set to 1. That way the division to transform clip coordinates to normalized device coordinates does not change anything.

`gl_Position` is a built in variable storing the final position output of the vertex shader.

`gl_VertexIndex` stores the current vertex index.