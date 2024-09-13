#include <vulkan/vulkan.hpp>
#include <iostream>

// Function to check if Vulkan-Headers are working by creating a Vulkan instance
bool checkVulkanHeaders()
{
    // Create an instance creation info structure
    vk::ApplicationInfo appInfo {};
    appInfo.pApplicationName = "Vulkan Headers Test";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    vk::InstanceCreateInfo createInfo {};
    createInfo.pApplicationInfo = &appInfo;

    // Try to create a Vulkan instance
    vk::Instance instance;
    try {
        instance = vk::createInstance(createInfo);
    } catch (const vk::SystemError& err) {
        std::cerr << "Failed to create Vulkan instance: " << err.what() << std::endl;
        return false;
    }

    // If instance creation is successful, clean up and return true
    instance.destroy();
    return true;
}

int main()
{
    if (checkVulkanHeaders()) {
        std::cout << "Vulkan instantiated correctly." << std::endl;
    } else {
        std::cout << "Failed to initialize Vulkan instance." << std::endl;
    }

    return 0;
}
