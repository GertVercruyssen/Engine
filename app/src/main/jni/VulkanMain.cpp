// Copyright 2016 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "VulkanMain.hpp"
#include <android/log.h>
#include <cassert>
#include <cstring>
#include <string>
#include <set>

// Android log function wrappers
static const char* kTAG = "Vulkan-Engine";
#define LOGI(...) \
  ((void)__android_log_print(ANDROID_LOG_INFO, kTAG, __VA_ARGS__))
#define LOGW(...) \
  ((void)__android_log_print(ANDROID_LOG_WARN, kTAG, __VA_ARGS__))
#define LOGE(...) \
  ((void)__android_log_print(ANDROID_LOG_ERROR, kTAG, __VA_ARGS__))

// Vulkan call wrapper
#define CALL_VK(func)                                                 \
  if (VK_SUCCESS != (func)) {                                         \
    __android_log_print(ANDROID_LOG_ERROR, "Tutorial ",               \
                        "Vulkan error. File[%s], line[%d]", __FILE__, \
                        __LINE__);                                    \
    assert(false);                                                    \
  }

// Global Variables ...



void VulkanEngine::EvalVK(VkResult result, char* errormsg) {
    if (result != VK_SUCCESS) {
        LOGE("Error evaluating VK call");
        __android_log_print(ANDROID_LOG_ERROR, kTAG, "%s", errormsg);
        assert(false);
    }
}

bool VulkanEngine::CheckValidationLayerSupport()
{
    unsigned int layercount = 0;
    vkEnumerateInstanceLayerProperties(&layercount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layercount);
    vkEnumerateInstanceLayerProperties(&layercount,availableLayers.data());

    for(const char* layerName : validationLayers)
    {
        bool layerfound = false;

        for (const auto&  layerProperties: availableLayers)
        {
            if (strcmp(layerName, layerProperties.layerName) == 0){
                layerfound = true;
                break;
            }
        }
        if(!layerfound)
            return false;
    }

    return true;
}

QueueFamilyIndices VulkanEngine::findQueueFamilies(VkPhysicalDevice gpudevice)
{
    QueueFamilyIndices indices;
    uint32_t queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(gpudevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(gpudevice, &queueFamilyCount, queueFamilyProperties.data());
    int i = 0;
    for(const auto& queueFamily : queueFamilyProperties) {
        if(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
            device.queueGraphicsIndex_ = i;
        }
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(gpudevice, i, device.surface_, &presentSupport);
        if(presentSupport) {
            indices.presentFamily = i;
            device.queuePresentIndex_ = i;
        }
        if(indices.isCompleted())
            break;
        i++;
    }
    return indices;
}

bool VulkanEngine::CheckDeviceExtensionsSupported(VkPhysicalDevice physicalDevice, std::vector<const char*> extensions) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr,&extensionCount, availableExtensions.data());
    std::set<const char*> requiredExtensions (extensions.begin(), extensions.end());
    for(const auto& extension : extensions) {
        requiredExtensions.erase(extension);
    }
    return requiredExtensions.empty();
}

bool VulkanEngine::isDeviceSuitable(VkPhysicalDevice gpudevice, std::vector<const char*> extensions)
{
    //engine specific requests go here, but for now we'll take any vulkan compatible gpu
    QueueFamilyIndices indices = findQueueFamilies(gpudevice);

    bool extensionsSupported = CheckDeviceExtensionsSupported(gpudevice, extensions);

    return indices.isCompleted() && extensionsSupported;
}

//Helper function to load shaders
//name example: "shaders/tri.vert.spv"
std::vector<char> VulkanEngine::LoadShaderFile(const char* shaderName) {
    AAsset* file = AAssetManager_open(assManager, shaderName, AASSET_MODE_BUFFER);
    size_t glslShaderLen = AAsset_getLength(file);
    std::vector<char> glslShader;
    glslShader.resize(glslShaderLen);

    AAsset_read(file, static_cast<void*>(glslShader.data()), glslShaderLen);
    AAsset_close(file);
    return glslShader;
}

// Create vulkan device
void VulkanEngine::CreateVulkanDevice(ANativeWindow* platformWindow, VkApplicationInfo* appInfo) {
    std::vector<const char*> instance_extensions;
    std::vector<const char*> device_extensions;

    instance_extensions.push_back("VK_KHR_surface");
    instance_extensions.push_back("VK_KHR_android_surface");

    device_extensions.push_back("VK_KHR_swapchain");

    // **********************************************************
    // Create the Vulkan instance
    if(enableValidationLayers && !CheckValidationLayerSupport())
        LOGE("validation layers requested, but not available!");

    VkInstanceCreateInfo instanceCreateInfo {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pNext = nullptr;
    instanceCreateInfo.pApplicationInfo = appInfo;
    instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(instance_extensions.size());
    instanceCreateInfo.ppEnabledExtensionNames = instance_extensions.data();
    if(enableValidationLayers)
    {
        instanceCreateInfo.enabledLayerCount = static_cast<unsigned int>(validationLayers.size());
        instanceCreateInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        instanceCreateInfo.enabledLayerCount = 0;
        instanceCreateInfo.ppEnabledLayerNames = nullptr;
    }
    CALL_VK(vkCreateInstance(&instanceCreateInfo, nullptr, &device.instance_));
    
    VkAndroidSurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    createInfo.window = platformWindow;
    CALL_VK(vkCreateAndroidSurfaceKHR(device.instance_, &createInfo, nullptr,
                                      &device.surface_));
    // Find one GPU to use:
    // On Android, every GPU device is equal -- supporting
    // graphics/compute/present
    // for this sample, we use the very first GPU device found on the system
    uint32_t gpuCount = 0;
    CALL_VK(vkEnumeratePhysicalDevices(device.instance_, &gpuCount, nullptr));
    if(gpuCount == 0 )
        LOGE("No GPU found with vulkan support");
    VkPhysicalDevice tmpGpus[gpuCount];
    //TODO: test on phone - emulator
    for(int i = 0;i<gpuCount;i++)
        LOGI("Gpu found!");
    CALL_VK(vkEnumeratePhysicalDevices(device.instance_, &gpuCount, tmpGpus));
    VkPhysicalDevice gpu = VK_NULL_HANDLE;
    for(const auto& gpudevice : tmpGpus)
    {
        if(isDeviceSuitable(gpudevice, device_extensions)) { //Pick the first suitable one (android is just first one mostly)
            gpu = gpudevice;
            break;
        }
    }
    if(gpu== VK_NULL_HANDLE)
        LOGE("No compatible GPU found.");
    else {
        device.gpuDevice_ = gpu;
    }

    // Create a logical device (vulkan device)
    float priorities =  1.0f;
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {device.queueGraphicsIndex_, device.queuePresentIndex_};

    for(uint32_t queueFamily : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.flags = 0;
        queueCreateInfo.pQueuePriorities = &priorities;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pNext = nullptr;
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceCreateInfo.enabledLayerCount = 0; //deprecated
    deviceCreateInfo.ppEnabledLayerNames = nullptr; //deprecated
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = device_extensions.data();
    deviceCreateInfo.pEnabledFeatures = nullptr;

    CALL_VK(vkCreateDevice(device.gpuDevice_, &deviceCreateInfo, nullptr,
                           &device.device_));

    //these will probably always be the same, but lets make life diffcult for ourselves
    vkGetDeviceQueue(device.device_, device.queueGraphicsIndex_, 0, &device.graphicsQueue_);
    vkGetDeviceQueue(device.device_, device.queuePresentIndex_, 0, &device.presentQueue_);
}

void VulkanEngine::CreateSwapChain(void) {
    LOGI("->createSwapChain");
    memset(&swapchain, 0, sizeof(swapchain));

    // **********************************************************
    // Get the surface capabilities because:
    //   - It contains the minimal and max length of the chain, we will need it
    //   - It's necessary to query the supported surface format (R8G8B8A8 for
    //   instance ...)
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.gpuDevice_, device.surface_,
                                              &surfaceCapabilities);
    // Query the list of supported surface format and choose one we like
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device.gpuDevice_, device.surface_,&formatCount, nullptr);
    VkSurfaceFormatKHR* formats = new VkSurfaceFormatKHR[formatCount];
    vkGetPhysicalDeviceSurfaceFormatsKHR(device.gpuDevice_, device.surface_,&formatCount, formats);
    LOGI("Got %d formats", formatCount);

    uint32_t chosenFormat;
    for (chosenFormat = 0; chosenFormat < formatCount; chosenFormat++) {
        if (formats[chosenFormat].format == VK_FORMAT_R8G8B8A8_UNORM) break; //might want to add additional formats for screens with hdr
    }
    assert(chosenFormat < formatCount);

    //Querry the list of supported presentation modes and choose one we like
    uint32_t presentCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device.gpuDevice_,device.surface_, &presentCount, nullptr);
    VkPresentModeKHR* presentModes = new VkPresentModeKHR[presentCount];
    vkGetPhysicalDeviceSurfacePresentModesKHR(device.gpuDevice_,device.surface_, &presentCount, presentModes);
    LOGI("Got %d presentation modes", presentCount);
    uint32_t chosenPresent;
    for(chosenPresent = 0; chosenPresent < presentCount; chosenPresent++) {
        if(presentModes[chosenPresent] == VK_PRESENT_MODE_MAILBOX_KHR)
            break;
    }
    if(chosenPresent == presentCount) {//No support for tripple buffering found, find alway supported Fifo
        swapchain.presentMode_ = VK_PRESENT_MODE_FIFO_KHR;
        LOGI("No triple buffering support found.");
    }
    swapchain.displaySize_ = surfaceCapabilities.currentExtent;
    swapchain.displayFormat_ = formats[chosenFormat].format;
    swapchain.presentMode_ = presentModes[chosenPresent];

    // **********************************************************
    // Create a swap chain (here we choose the minimum available number of surface
    // in the chain)
    //TODO: wide color gamut support
    //https://developer.android.com/training/wide-color-gamut#vulkan

    uint32_t imageCount = surfaceCapabilities.minImageCount +1;
    if(surfaceCapabilities.maxImageCount > 0 && imageCount > surfaceCapabilities.maxImageCount)
        imageCount = surfaceCapabilities.maxImageCount;

    VkSwapchainCreateInfoKHR swapchainCreateInfo{};
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.pNext = nullptr;
    swapchainCreateInfo.surface = device.surface_;
    swapchainCreateInfo.minImageCount = imageCount;
    swapchainCreateInfo.imageFormat = formats[chosenFormat].format;
    swapchainCreateInfo.imageColorSpace = formats[chosenFormat].colorSpace;
    swapchainCreateInfo.imageExtent = surfaceCapabilities.currentExtent;
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; //render straight to sawpchain, change this if you want to do post processing
    swapchainCreateInfo.imageArrayLayers = 1;
    uint32_t familyIndices[] = {device.queueGraphicsIndex_, device.queuePresentIndex_};
    if(device.queueGraphicsIndex_ != device.queuePresentIndex_) {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchainCreateInfo.queueFamilyIndexCount = 2;
        swapchainCreateInfo.pQueueFamilyIndices = familyIndices;
    } else {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainCreateInfo.queueFamilyIndexCount = 0; // Optional
        swapchainCreateInfo.pQueueFamilyIndices = nullptr; //Optional
    }
    swapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR; //i think this means do nothing
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR; //VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR didn't work on android;
    swapchainCreateInfo.presentMode = swapchain.presentMode_;
    swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;
    swapchainCreateInfo.clipped = VK_FALSE; //mobile phones don't have overlapping windows so this can be false

    CALL_VK(vkCreateSwapchainKHR(device.device_, &swapchainCreateInfo, nullptr, &swapchain.swapchain_));
    // Get the length of the created swap chain
    CALL_VK(vkGetSwapchainImagesKHR(device.device_, swapchain.swapchain_, &swapchain.swapchainLength_, nullptr));
    swapchain.displayImages_.resize(swapchain.swapchainLength_);
    CALL_VK(vkGetSwapchainImagesKHR(device.device_, swapchain.swapchain_, &swapchain.swapchainLength_, swapchain.displayImages_.data()));

    delete[] formats;
    delete[] presentModes;
    LOGI("<-createSwapChain");
}

VkShaderModule VulkanEngine::CreateShaderModule(const std::vector<char>& code)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    CALL_VK(vkCreateShaderModule(device.device_, &createInfo, nullptr, &shaderModule));

    return shaderModule;
}

void VulkanEngine::CreateRenderPass() {
    VkAttachmentDescription attachmentDescriptions{
            .format = swapchain.displayFormat_,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };

    VkAttachmentReference colourReference = {
            .attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    VkSubpassDescription subpassDescription{
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colourReference
    };

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask =VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassCreateInfo{
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .pNext = nullptr,
            .attachmentCount = 1,
            .pAttachments = &attachmentDescriptions,
            .subpassCount = 1,
            .pSubpasses = &subpassDescription,
            .dependencyCount = 1,
            .pDependencies = &dependency
    };
    CALL_VK(vkCreateRenderPass(device.device_, &renderPassCreateInfo, nullptr, &render.renderPass_));


}

void VulkanEngine::CreateGraphicsPipeline() {
    std::vector<char> vertShaderCode = LoadShaderFile("shaders/triangle.vert.spv");
    std::vector<char> fragShaderCode = LoadShaderFile("shaders/triangle.frag.spv");
    VkShaderModule vertShaderModule = CreateShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = CreateShaderModule(fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo {};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = nullptr;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions = nullptr;

    VkPipelineInputAssemblyStateCreateInfo assemblyStateInfo{};
    assemblyStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    assemblyStateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; //TODO: change this when moving on to objs files
    assemblyStateInfo.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0;
    viewport.y = 0;
    viewport.height = (float) swapchain.displaySize_.height;
    viewport.width = (float) swapchain.displaySize_.width;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.extent = swapchain.displaySize_;

    VkPipelineViewportStateCreateInfo viewportStateInfo{};
    viewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateInfo.scissorCount = 1;
    viewportStateInfo.pScissors = &scissor;
    viewportStateInfo.viewportCount = 1;
    viewportStateInfo.pViewports = &viewport;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL; //TODO: setting to toggle this (requires GPU feature)
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable =VK_FALSE;
    multisampling.alphaToOneEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    //In case no transparency is used anywhere
//    colorBlendAttachment.blendEnable = VK_FALSE;
//    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
//    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
//    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
//    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
//    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
//    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f; // Optional
    colorBlending.blendConstants[1] = 0.0f; // Optional
    colorBlending.blendConstants[2] = 0.0f; // Optional
    colorBlending.blendConstants[3] = 0.0f; // Optional

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pSetLayouts = nullptr;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;

    CALL_VK(vkCreatePipelineLayout(device.device_,&pipelineLayoutInfo, nullptr,&render.pipelineLayout));

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &assemblyStateInfo;
    pipelineInfo.pViewportState = &viewportStateInfo;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDepthStencilState = nullptr;
    pipelineInfo.pDynamicState = nullptr;
    pipelineInfo.layout = render.pipelineLayout;
    pipelineInfo.renderPass = render.renderPass_;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;
    CALL_VK(vkCreateGraphicsPipelines(device.device_, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &render.pipeline));

    vkDestroyShaderModule(device.device_, vertShaderModule, nullptr);
    vkDestroyShaderModule(device.device_, fragShaderModule, nullptr);
}

void VulkanEngine::CreateImageViews() {
    // create image view for each swapchain image
    swapchain.displayViews_.resize(swapchain.swapchainLength_);
    for (uint32_t i = 0; i < swapchain.swapchainLength_; i++) {
        VkImageViewCreateInfo viewCreateInfo = {};
        viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewCreateInfo.pNext = nullptr;
        viewCreateInfo.image = swapchain.displayImages_[i];
        viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewCreateInfo.format = swapchain.displayFormat_;
        viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewCreateInfo.subresourceRange.baseMipLevel = 0;
        viewCreateInfo.subresourceRange.levelCount = 1;
        viewCreateInfo.subresourceRange.baseArrayLayer = 0;
        viewCreateInfo.subresourceRange.layerCount = 1;
        viewCreateInfo.flags = 0;
        CALL_VK(vkCreateImageView(device.device_, &viewCreateInfo, nullptr, &swapchain.displayViews_[i]));
    }
}

void VulkanEngine::CreateFramebuffers(VkImageView depthView = VK_NULL_HANDLE) {
    // create a framebuffer from each swapchain image
    swapchain.framebuffers_.resize(swapchain.swapchainLength_);
    for (uint32_t i = 0; i < swapchain.swapchainLength_; i++) {
        VkImageView attachments[] = {
                swapchain.displayViews_[i]
        };
        VkFramebufferCreateInfo fbCreateInfo{};
        fbCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbCreateInfo.renderPass = render.renderPass_;
        fbCreateInfo.layers = 1;
        fbCreateInfo.attachmentCount = 1;  // 2 if using depth
        fbCreateInfo.pAttachments = attachments;
        fbCreateInfo.width = swapchain.displaySize_.width;
        fbCreateInfo.height = swapchain.displaySize_.height;

        CALL_VK(vkCreateFramebuffer(device.device_, &fbCreateInfo, nullptr,&swapchain.framebuffers_[i]));
    }
}

void VulkanEngine::CreateCommandPool() {
    VkCommandPoolCreateInfo cmdPoolCreateInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,//VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, command buffer doesn't change during drawing so just 0;
            .queueFamilyIndex = device.queueGraphicsIndex_,
    };
    CALL_VK(vkCreateCommandPool(device.device_, &cmdPoolCreateInfo, nullptr, &render.cmdPool_));
}

void VulkanEngine::CreateCommandBuffers() {
    render.commandBuffers.resize(swapchain.framebuffers_.size());

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = render.cmdPool_;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)render.commandBuffers.size();
    CALL_VK(vkAllocateCommandBuffers(device.device_, &allocInfo, render.commandBuffers.data()));

    for(size_t i =0; i<render.commandBuffers.size();i++) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;//optional
        beginInfo.pInheritanceInfo = nullptr; //optional
        CALL_VK(vkBeginCommandBuffer(render.commandBuffers[i], &beginInfo));

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = render.renderPass_;
        renderPassInfo.framebuffer = swapchain.framebuffers_[i];
        renderPassInfo.renderArea.offset = {0,0};
        renderPassInfo.renderArea.extent = swapchain.displaySize_;
        VkClearValue backgroundcolor{{{0.0f, 0.0f, 0.0f, 1.0f}}}; //TODO: mess with this
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &backgroundcolor;
        vkCmdBeginRenderPass(render.commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(render.commandBuffers[i],VK_PIPELINE_BIND_POINT_GRAPHICS, render.pipeline);
        vkCmdDraw(render.commandBuffers[i],3,1,0,0);
        vkCmdEndRenderPass(render.commandBuffers[i]);
        CALL_VK(vkEndCommandBuffer(render.commandBuffers[i]));
    }
}

void VulkanEngine::CreateSyncObjects() {
    render.imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    render.renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    render.inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    render.imagesInFlight.resize(swapchain.displayImages_.size(), VK_NULL_HANDLE);
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for(size_t i =0; i<MAX_FRAMES_IN_FLIGHT;i++) {
        CALL_VK(vkCreateSemaphore(device.device_, &semaphoreInfo, nullptr, &render.imageAvailableSemaphores[i]));
        CALL_VK(vkCreateSemaphore(device.device_, &semaphoreInfo, nullptr, &render.renderFinishedSemaphores[i]));
        CALL_VK(vkCreateFence(device.device_, & fenceInfo, nullptr, &render.inFlightFences[i]));
    }
}

// InitVulkan:
//   Initialize Vulkan Context when android application window is created
//   upon return, vulkan is ready to draw frames
bool VulkanEngine::InitVulkan(android_app* app) {
    androidAppCtx = app;
    assManager = app->activity->assetManager;

//    if (!InitVulkan()) {
//        LOGW("Vulkan is unavailable, install vulkan and re-start");
//        return false;
//    }

    VkApplicationInfo appInfo = {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pNext = nullptr,
            .apiVersion = VK_MAKE_VERSION(1, 0, 0),
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
            .pApplicationName = "VulkanEngine",
            .pEngineName = "VulkanEngine",
    };

    CreateVulkanDevice(app->window, &appInfo);
    CreateSwapChain();
    CreateImageViews();
    CreateRenderPass();
    CreateGraphicsPipeline();
    CreateFramebuffers();
    CreateCommandPool();
    CreateCommandBuffers();
    CreateSyncObjects();

    device.initialized_ = true;
    return true;
}

// IsVulkanReady():
//    native app poll to see if we are ready to draw...
bool VulkanEngine::IsVulkanReady(void) { return device.initialized_; }

void VulkanEngine::CleanupSwapchain() {
    for(auto framebuffer : swapchain.framebuffers_)
        vkDestroyFramebuffer(device.device_, framebuffer, nullptr);
    vkFreeCommandBuffers(device.device_, render.cmdPool_, (uint32_t)render.commandBuffers.size(), render.commandBuffers.data());
    vkDestroyPipeline(device.device_, render.pipeline, nullptr);
    vkDestroyPipelineLayout(device.device_,render.pipelineLayout,nullptr);
    vkDestroyRenderPass(device.device_, render.renderPass_, nullptr);
    for(auto imageview : swapchain.displayViews_)
        vkDestroyImageView(device.device_, imageview, nullptr);
    for(auto displayImage : swapchain.displayImages_)
        vkDestroyImage(device.device_, displayImage, nullptr);
    vkDestroySwapchainKHR(device.device_, swapchain.swapchain_, nullptr);
}

void VulkanEngine::DeleteVulkan(void) {
    CleanupSwapchain();

    for(size_t i = 0; i<MAX_FRAMES_IN_FLIGHT;i++) {
        vkDestroySemaphore(device.device_, render.imageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(device.device_, render.renderFinishedSemaphores[i], nullptr);
        vkDestroyFence(device.device_, render.inFlightFences[i], nullptr);
    }

    vkDestroyCommandPool(device.device_, render.cmdPool_, nullptr);
    vkDestroySurfaceKHR(device.instance_, device.surface_, nullptr);
    vkDestroyDevice(device.device_, nullptr);
    vkDestroyInstance(device.instance_, nullptr);

    device.initialized_ = false;
}

void VulkanEngine::RecreateSwapchain() { //TODO: check if this gets called twice during phone rotation
    vkDeviceWaitIdle(device.device_);

    CleanupSwapchain();

    CreateSwapChain();
    CreateImageViews();
    CreateRenderPass();
    CreateGraphicsPipeline();
    CreateFramebuffers();
    CreateCommandBuffers();
}

// Draw one frame
bool VulkanEngine::VulkanDrawFrame() {
    vkWaitForFences(device.device_, 1, &render.inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device.device_,swapchain.swapchain_,UINT64_MAX,render.imageAvailableSemaphores[currentFrame],VK_NULL_HANDLE, &imageIndex);
    if(result == VK_ERROR_OUT_OF_DATE_KHR){
        RecreateSwapchain();
        return false;
    } else if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        LOGE("Failed to acquire swapchain image!");
        assert(false);
    }

    // Check if a previous frame is using this image (i.e. there is its fence to wait on)
    if(render.imagesInFlight[imageIndex] != VK_NULL_HANDLE)
        vkWaitForFences(device.device_, 1, &render.imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    // Mark the image as now being in use by this frame
    render.imagesInFlight[imageIndex] = render.inFlightFences[currentFrame];

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkSemaphore waitSemaphores[] = {render.imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &render.commandBuffers[imageIndex];
    VkSemaphore signalSemaphores[] = {render.renderFinishedSemaphores[currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;


    vkResetFences(device.device_, 1, &render.inFlightFences[currentFrame]);
    CALL_VK(vkQueueSubmit(device.graphicsQueue_, 1, &submitInfo, render.inFlightFences[currentFrame]));

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    VkSwapchainKHR swapChains[] = {swapchain.swapchain_};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;
    result = vkQueuePresentKHR(device.presentQueue_, &presentInfo);

    if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized){
        framebufferResized = false;
        RecreateSwapchain();
    } else if(result != VK_SUCCESS) {
        LOGE("Failed to present swapchain image!");
        assert(false);
    }

    currentFrame = (currentFrame +1) % MAX_FRAMES_IN_FLIGHT;
    return true;
}

void VulkanEngine::WaitIdle() {
    vkDeviceWaitIdle(device.device_);
}

void VulkanEngine::VulkanResize() { //TODO: check if this gets called twice during flip
    framebufferResized = true;
}

VulkanEngine::VulkanEngine() {
    //??
}

VulkanEngine::~VulkanEngine() {
    //??
}
