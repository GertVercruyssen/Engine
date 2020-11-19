
#ifndef __VULKANMAIN_HPP__
#define __VULKANMAIN_HPP__
#include <android_native_app_glue.h>
#include "vulkan_wrapper.h"
#include <vector>
#include <array>
#include <optional>
#include "../glm/vec2.hpp"
#include "../glm/vec3.hpp"

struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }
    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);
        return attributeDescriptions;
    }
};

struct VulkanDeviceInfo {
    bool initialized_;

    VkInstance instance_;
    VkPhysicalDevice gpuDevice_;
    VkDevice device_;
    uint32_t queueGraphicsIndex_;
    uint32_t queuePresentIndex_;

    VkSurfaceKHR surface_;
    VkQueue graphicsQueue_;
    VkQueue presentQueue_;
};
struct VulkanSwapchainInfo {
    VkSwapchainKHR swapchain_;
    uint32_t swapchainLength_;

    VkExtent2D displaySize_;
    VkFormat displayFormat_;
    VkPresentModeKHR presentMode_;

    // array of frame buffers and views
    std::vector<VkImage> displayImages_;
    std::vector<VkImageView> displayViews_;
    std::vector<VkFramebuffer> framebuffers_;
};

struct VulkanRenderInfo {
    VkRenderPass renderPass_;
    VkCommandPool cmdPool_;
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence>imagesInFlight;
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
};

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isCompleted() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

class VulkanEngine {
public:
// Initialize vulkan device context
// after return, vulkan is ready to draw
    bool InitVulkan(android_app *app);

// delete vulkan device context when application goes away
    void DeleteVulkan(void);

// Check if vulkan is ready to draw
    bool IsVulkanReady(void);

// Ask Vulkan to Render a frame
    bool VulkanDrawFrame();

// Wait for the Drawloop to be idle
    void WaitIdle();

// window has changed (rotated), recreate swapchain
    void VulkanResize();

    VulkanEngine();
    virtual ~VulkanEngine();

private:
    void EvalVK(VkResult result, char* errormsg);
    bool CheckValidationLayerSupport();
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice gpudevice);
    bool CheckDeviceExtensionsSupported(VkPhysicalDevice physicalDevice, std::vector<const char*> extensions);
    bool isDeviceSuitable(VkPhysicalDevice gpudevice, std::vector<const char*> extensions);
    std::vector<char> LoadShaderFile(const char* shaderName);
    void CreateVulkanDevice(ANativeWindow* platformWindow, VkApplicationInfo* appInfo);
    void CreateSwapChain(void);
    VkShaderModule CreateShaderModule(const std::vector<char>& code);
    void CreateRenderPass();
    void CreateGraphicsPipeline();
    void CreateImageViews();
    void CreateFramebuffers(VkImageView depthView);
    void CreateCommandPool();
    void CreateVertexBuffer();
    void CreateCommandBuffers();
    void CreateSyncObjects();
    void CleanupSwapchain();
    void RecreateSwapchain();
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    const int MAX_FRAMES_IN_FLIGHT = 2;
    size_t currentFrame = 0;
    VulkanDeviceInfo device;
    VulkanSwapchainInfo swapchain;
    VulkanRenderInfo render;
    android_app* androidAppCtx = nullptr; // Android Native App pointer...
    AAssetManager* assManager = nullptr;
    bool framebufferResized = false;

    const std::vector<Vertex> vertices = {
            {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
            {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
            {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
    };

#ifdef NDEBUG
    const static bool enableValidationLayers = false;
#else
    const static bool enableValidationLayers = true;
#endif
    //validation layers
    const std::vector<const char*> validationLayers = {
            "VK_LAYER_KHRONOS_validation"
    };

};

#endif // __VULKANMAIN_HPP__