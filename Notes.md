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

    (The steps above can be automated by `vk-bootstrap`)

    - Load model and setup vertex and index buffers and shaders
    - Get next swapchain image
    - Record and finish commands
    - Present current swapchain image
    - Wait for all command queues to finish work
    - Cleanup all allocated resources

Vulkan validation layers facilitate proper API usage.

APIs like `OpenGL` and `DX 11` calculate pipeline objects automatically, while Vulkan enables the programmer to cache pipeline objects or generate them "on the fly" while rendering. Creating pipelines are expensive, so it's best to create them on a background thread or on a loading screen.

Most of the state validation in Vulkan occurs when creating objects while the render loop is relatively inexpensive.

To perform commands on the GPU, we allocate a command buffer, start encoding things on it, and then execute it by adding it to a queue.

Submitting a command buffer to a queue means it starts processing on the GPU side. Submitting mutiple command buffers on multiple queues may mean that the commands may execute in parallel.

Vulkan gives explicit control over the rendering architecture. This means that frame can be rendered offline (headless mode); over the network; or on a screen.


