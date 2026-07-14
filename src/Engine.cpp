#include "Engine.hpp"

#include <cassert>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

const char* IMAGE_PATH = "/assets/textures/image.jpg";

void Engine::run()
{
    std::cout << "[1] Inicializando ventana..." << std::endl;
    initWindow();

    std::cout << "[2] Inicializando Vulkan..." << std::endl;
    initVulkan();

    std::cout << "[3] Entrando en el bucle principal..." << std::endl;
    mainLoop();

    std::cout << "[4] Limpiando recursos..." << std::endl;
    cleanup();

    std::cout << "[5] Programa finalizado correctamente." << std::endl;
}

void Engine::initWindow()
{
    if (!glfwInit())
    {
        throw std::runtime_error("Fallo crítico: No se pudo inicializar GLFW");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    m_Window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    glfwSetWindowUserPointer(m_Window, this);
    glfwSetFramebufferSizeCallback(m_Window, framebufferResizeCallback);

    if (!m_Window)
    {
        throw std::runtime_error("Fallo crítico: La ventana GLFW es null (no se pudo crear)");
    }
}

void Engine::framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    auto app = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window));
    app->m_FramebufferResized = true;
}

void Engine::initVulkan()
{
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createDescriptorSetLayout();
    createGraphicsPipeline();
    createCommandPool();
    createColorResources();
    createDepthResources();
    createTextureImage();
    createTextureImageView();
    createTextureSampler();
    loadModel();
    createVertexBuffer();
    createIndexBuffer();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers();
    createSyncronizationObjetcs();
}

void Engine::createInstance()
{
    constexpr vk::ApplicationInfo appInfo{ .pApplicationName   = "Hello Triangle",
                .applicationVersion = VK_MAKE_VERSION( 1, 0, 0 ),
                .pEngineName        = "No Engine",
                .engineVersion      = VK_MAKE_VERSION( 1, 0, 0 ),
                .apiVersion         = vk::ApiVersion14 };

    // Get the required layers
    std::vector<char const*> requiredLayers;
    if (g_EnableValidationLayers) {
        requiredLayers.assign(g_ValidationLayers.begin(), g_ValidationLayers.end());
    }

    // Check if the required layers are supported by the Vulkan implementation.
    auto layerProperties = m_Context.enumerateInstanceLayerProperties();
    if (std::ranges::any_of(requiredLayers, [&layerProperties](auto const& requiredLayer) {
        return std::ranges::none_of(layerProperties,
                                   [requiredLayer](auto const& layerProperty)
                                   { return strcmp(layerProperty.layerName, requiredLayer) == 0; });
    }))
    {
        throw std::runtime_error("One or more required layers are not supported!");
    }

    // Get the required extensions.
    auto requiredExtensions = getRequiredInstanceExtensions();

    // Check if the required extensions are supported by the Vulkan implementation.
    auto extensionProperties = m_Context.enumerateInstanceExtensionProperties();
        auto unsupportedPropertyIt =
            std::ranges::find_if(requiredExtensions,
                                 [&extensionProperties](auto const &requiredExtension) {
                                     return std::ranges::none_of(extensionProperties,
                                                                 [requiredExtension](auto const &extensionProperty) { return strcmp(extensionProperty.extensionName, requiredExtension) == 0; });
                                 });
        if (unsupportedPropertyIt != requiredExtensions.end())
        {
            throw std::runtime_error("Required extension not supported: " + std::string(*unsupportedPropertyIt));
        }

    vk::InstanceCreateInfo createInfo{
        .pApplicationInfo        = &appInfo,
        .enabledLayerCount       = static_cast<uint32_t>(requiredLayers.size()),
        .ppEnabledLayerNames     = requiredLayers.data(),
        .enabledExtensionCount   = static_cast<uint32_t>(requiredExtensions.size()),
        .ppEnabledExtensionNames = requiredExtensions.data() };

    m_Instance = vk::raii::Instance(m_Context, createInfo);
}

std::vector<const char*> Engine::getRequiredInstanceExtensions()
{
    uint32_t glfwExtensionCount = 0;
    auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (g_EnableValidationLayers)
    {
        extensions.push_back(vk::EXTDebugUtilsExtensionName);
    }

    return extensions;
}

void Engine::setupDebugMessenger()
{
    if (!g_EnableValidationLayers) return;

    vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                                                    vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
    vk::DebugUtilsMessageTypeFlagsEXT     messageTypeFlags(
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
    vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{.messageSeverity = severityFlags,
                                                                          .messageType     = messageTypeFlags,
                                                                          .pfnUserCallback = &debugCallback};
    m_DebugMessenger = m_Instance.createDebugUtilsMessengerEXT( debugUtilsMessengerCreateInfoEXT );
}

VKAPI_ATTR vk::Bool32 VKAPI_CALL Engine::debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT       severity,
                                                  vk::DebugUtilsMessageTypeFlagsEXT              type,
                                                  const vk::DebugUtilsMessengerCallbackDataEXT * pCallbackData,
                                                  void * pUserData)
{
    std::cerr << "Validation layer: type " << to_string(type) << " msg: " << pCallbackData->pMessage << std::endl;

    return vk::False;
}

void Engine::createSurface()
{
    VkSurfaceKHR _vkSurface;
    if (glfwCreateWindowSurface(*m_Instance, m_Window, nullptr, &_vkSurface) != 0)
    {
        throw std::runtime_error("failed to create window surface!");
    }
    m_Surface = vk::raii::SurfaceKHR(m_Instance, _vkSurface);
}

bool Engine::isDeviceSuitable( vk::raii::PhysicalDevice const & physicalDevice )
{
  // Check if the physicalDevice supports the Vulkan 1.3 API version
  bool supportsVulkan1_3 = physicalDevice.getProperties().apiVersion >= vk::ApiVersion13;

  // Check if any of the queue families support graphics operations
  auto queueFamilies    = physicalDevice.getQueueFamilyProperties();
  bool supportsGraphics = std::ranges::any_of( queueFamilies, []( auto const & qfp ) { return !!( qfp.queueFlags & vk::QueueFlagBits::eGraphics ); } );

  // Check if all required physicalDevice extensions are available
  auto availableDeviceExtensions = physicalDevice.enumerateDeviceExtensionProperties();
  bool supportsAllRequiredExtensions =
    std::ranges::all_of( m_RequiredDeviceExtension,
                         [&availableDeviceExtensions]( auto const & requiredDeviceExtension )
                         {
                           return std::ranges::any_of( availableDeviceExtensions,
                                                       [requiredDeviceExtension]( auto const & availableDeviceExtension )
                                                       { return strcmp( availableDeviceExtension.extensionName, requiredDeviceExtension ) == 0; } );
                         } );

  // Check if the physicalDevice supports the required features (dynamic rendering and extended dynamic state)
    auto features                 = physicalDevice.template getFeatures2<vk::PhysicalDeviceFeatures2,
                                                                         vk::PhysicalDeviceVulkan13Features,
                                                                         vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
    bool supportsRequiredFeatures = features.template get<vk::PhysicalDeviceFeatures2>().features.samplerAnisotropy &&
                                    features.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
                                    features.template get<vk::PhysicalDeviceVulkan13Features>().synchronization2 &&
                                    features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;

  // Return true if the physicalDevice meets all the criteria
  return supportsVulkan1_3 && supportsGraphics && supportsAllRequiredExtensions && supportsRequiredFeatures;
}

void Engine::pickPhysicalDevice()
{
    std::vector<vk::raii::PhysicalDevice> physicalDevices = m_Instance.enumeratePhysicalDevices();

    for (auto &dev : physicalDevices) {
        if (isDeviceSuitable(dev)) {
            m_PhysicalDevice = dev;
            m_MSaaSamples = getMaxUsableSampleCount();
            return;
        }
    }

    throw std::runtime_error("Failed to find a suitable GPU!");
}

void Engine::createLogicalDevice()
{
    // find the index of the first queue family that supports graphics
    std::vector<vk::QueueFamilyProperties> queueFamilyProperties = m_PhysicalDevice.getQueueFamilyProperties();

    // get the first index into queueFamilyProperties which supports both graphics and present
    for (uint32_t qfpIndex = 0; qfpIndex < queueFamilyProperties.size(); qfpIndex++)
    {
      if ((queueFamilyProperties[qfpIndex].queueFlags & vk::QueueFlagBits::eGraphics) &&
          m_PhysicalDevice.getSurfaceSupportKHR(qfpIndex, *m_Surface))
      {
        // found a queue family that supports both graphics and present
        m_GraphicsQueueIndex = qfpIndex;
        break;
      }
    }
    if (m_GraphicsQueueIndex == ~0)
    {
      throw std::runtime_error("Could not find a queue for graphics and present -> terminating");
    }

    // query for Vulkan 1.3 features
    vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceVulkan11Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> featureChain = {
        {.features = {.samplerAnisotropy = true}},                                   // vk::PhysicalDeviceFeatures2
        {.synchronization2 = true,
            .dynamicRendering = true},           // vk::PhysicalDeviceVulkan13Features
        {.shaderDrawParameters = true},
        {.extendedDynamicState = true}        // vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
    };

    // create a Device
    float                      queuePriority = 0.5f;
    vk::DeviceQueueCreateInfo deviceQueueCreateInfo{.queueFamilyIndex = m_GraphicsQueueIndex, .queueCount = 1, .pQueuePriorities = &queuePriority};
    vk::DeviceCreateInfo      deviceCreateInfo{.pNext                   = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
                                               .queueCreateInfoCount    = 1,
                                               .pQueueCreateInfos       = &deviceQueueCreateInfo,
                                               .enabledExtensionCount   = static_cast<uint32_t>(m_RequiredDeviceExtension.size()),
                                               .ppEnabledExtensionNames = m_RequiredDeviceExtension.data()};

    m_LogicalDevice = vk::raii::Device( m_PhysicalDevice, deviceCreateInfo );
    m_GraphicsQueue  = vk::raii::Queue(m_LogicalDevice, m_GraphicsQueueIndex, 0);
}

vk::SurfaceFormatKHR Engine::chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
    assert(!availableFormats.empty());
    const auto formatIt = std::ranges::find_if(
        availableFormats,
        [](const auto &format) { return format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear; });
    return formatIt != availableFormats.end() ? *formatIt : availableFormats[0];
}

vk::PresentModeKHR Engine::chooseSwapPresentMode(std::vector<vk::PresentModeKHR> const &availablePresentModes)
{
    assert(std::ranges::any_of(availablePresentModes, [](auto presentMode) { return presentMode == vk::PresentModeKHR::eFifo; }));
    return std::ranges::any_of(availablePresentModes,
                               [](const vk::PresentModeKHR value) { return vk::PresentModeKHR::eMailbox == value; }) ?
               vk::PresentModeKHR::eMailbox :
               vk::PresentModeKHR::eFifo;;
}

vk::Extent2D Engine::chooseSwapExtent(vk::SurfaceCapabilitiesKHR const &capabilities)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }
    int width, height;
    glfwGetFramebufferSize(m_Window, &width, &height);

    return {
        std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
    };
}

uint32_t Engine::chooseSwapMinImageCount(vk::SurfaceCapabilitiesKHR const &surfaceCapabilities)
{
    auto minImageCount = std::max(3u, surfaceCapabilities.minImageCount);
    if ((0 < surfaceCapabilities.maxImageCount) && (surfaceCapabilities.maxImageCount < minImageCount))
    {
        minImageCount = surfaceCapabilities.maxImageCount;
    }
    return minImageCount;
}

void Engine::createSwapChain()
{
    vk::SurfaceCapabilitiesKHR surfaceCapabilities = m_PhysicalDevice.getSurfaceCapabilitiesKHR( *m_Surface );
    m_SwapChainExtent                                = chooseSwapExtent(surfaceCapabilities);
    uint32_t minImageCount                         = chooseSwapMinImageCount(surfaceCapabilities);

    std::vector<vk::SurfaceFormatKHR> availableFormats = m_PhysicalDevice.getSurfaceFormatsKHR(*m_Surface);
    m_SwapChainSurfaceFormat                             = chooseSwapSurfaceFormat(availableFormats);

    std::vector<vk::PresentModeKHR> availablePresentModes = m_PhysicalDevice.getSurfacePresentModesKHR(*m_Surface);

    vk::SwapchainCreateInfoKHR swapChainCreateInfo{
                                             .surface          = *m_Surface,
                                            .minImageCount    = minImageCount,
                                            .imageFormat      = m_SwapChainSurfaceFormat.format,
                                            .imageColorSpace  = m_SwapChainSurfaceFormat.colorSpace,
                                            .imageExtent      = m_SwapChainExtent,
                                            .imageArrayLayers = 1,
                                            .imageUsage       = vk::ImageUsageFlagBits::eColorAttachment,
                                            .imageSharingMode = vk::SharingMode::eExclusive,
                                            .preTransform     = surfaceCapabilities.currentTransform,
                                            .compositeAlpha   = vk::CompositeAlphaFlagBitsKHR::eOpaque,
                                            .presentMode      = chooseSwapPresentMode(availablePresentModes),
                                            .clipped          = true};

    m_SwapChain       = vk::raii::SwapchainKHR( m_LogicalDevice, swapChainCreateInfo );
    m_SwapChainImages = m_SwapChain.getImages();
}

void Engine::cleanupSwapChain()
{
    m_SwapChainImageViews.clear();
    m_SwapChain = nullptr;
}

void Engine::recreateSwapChain()
{
    int width = 0, height = 0;
    glfwGetFramebufferSize(m_Window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(m_Window, &width, &height);
        glfwWaitEvents();
    }

    m_LogicalDevice.waitIdle();

    cleanupSwapChain();
    createSwapChain();
    createImageViews();
    createColorResources();
    createDepthResources();
}

void Engine::createImageViews()
{
    assert(m_SwapChainImageViews.empty());

    for (auto &image : m_SwapChainImages)
    {
        m_SwapChainImageViews.emplace_back(createImageView(image, 1, m_SwapChainSurfaceFormat.format));
    }
}

void Engine::createGraphicsPipeline()
{
    auto shaderCode = readFile("shaders/slang.spv");
    vk::raii::ShaderModule shaderModule = createShaderModule(shaderCode);

    // Info del shader de vértices
    vk::PipelineShaderStageCreateInfo vertShaderStageInfo{ .stage = vk::ShaderStageFlagBits::eVertex, .module = shaderModule,  .pName = "vertMain" };

    // Info del shader de fragmentos
    vk::PipelineShaderStageCreateInfo fragShaderStageInfo{ .stage = vk::ShaderStageFlagBits::eFragment, .module = shaderModule, .pName = "fragMain" };

    // Etapas del pipline de renderizado
    vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    auto bindingDescription    = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();
    vk::PipelineVertexInputStateCreateInfo   vertexInputInfo{.vertexBindingDescriptionCount   = 1,
                                                             .pVertexBindingDescriptions      = &bindingDescription,
                                                             .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
                                                             .pVertexAttributeDescriptions    = attributeDescriptions.data()};

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly{.topology = vk::PrimitiveTopology::eTriangleList};

    std::vector<vk::DynamicState> dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
    vk::PipelineDynamicStateCreateInfo dynamicState{.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()), .pDynamicStates = dynamicStates.data()};

    vk::PipelineViewportStateCreateInfo viewportState{.viewportCount = 1, .scissorCount = 1};

    vk::PipelineRasterizationStateCreateInfo rasterizer{.depthClampEnable        = vk::False,
                                                        .rasterizerDiscardEnable = vk::False,
                                                        .polygonMode             = vk::PolygonMode::eFill,
                                                        .cullMode                = vk::CullModeFlagBits::eBack,
                                                        .frontFace               = vk::FrontFace::eCounterClockwise,
                                                        .depthBiasEnable         = vk::False,
                                                        .lineWidth               = 1.0f};

    vk::PipelineMultisampleStateCreateInfo multisampling{.rasterizationSamples = m_MSaaSamples, .sampleShadingEnable = vk::False};

    // Blending (1 - alpha)
    vk::PipelineColorBlendAttachmentState colorBlendAttachment{
        .blendEnable         = vk::True,
        .srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
        .dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
        .colorBlendOp        = vk::BlendOp::eAdd,
        .srcAlphaBlendFactor = vk::BlendFactor::eOne,
        .dstAlphaBlendFactor = vk::BlendFactor::eZero,
        .alphaBlendOp        = vk::BlendOp::eAdd,
        .colorWriteMask      = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA};

    vk::PipelineColorBlendStateCreateInfo colorBlending{
        .logicOpEnable = vk::False, .logicOp = vk::LogicOp::eCopy, .attachmentCount = 1, .pAttachments = &colorBlendAttachment};

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{ .setLayoutCount = 1, .pSetLayouts = &*m_DescriptorSetLayout, .pushConstantRangeCount = 0 };

    m_PipelineLayout = vk::raii::PipelineLayout(m_LogicalDevice, pipelineLayoutInfo);

    vk::PipelineDepthStencilStateCreateInfo depthStencil{
        .depthTestEnable       = vk::True,
        .depthWriteEnable      = vk::True,
        .depthCompareOp        = vk::CompareOp::eLess,
        .depthBoundsTestEnable = vk::False,
        .stencilTestEnable     = vk::False};

    vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipelineCreateInfoChain = {
        {.stageCount          = 2,
            .pStages             = shaderStages,
            .pVertexInputState   = &vertexInputInfo,
            .pInputAssemblyState = &inputAssembly,
            .pViewportState      = &viewportState,
            .pRasterizationState = &rasterizer,
            .pMultisampleState   = &multisampling,
            .pDepthStencilState = &depthStencil,
            .pColorBlendState    = &colorBlending,
            .pDynamicState       = &dynamicState,
            .layout              = m_PipelineLayout,
            .renderPass          = nullptr},
        {.colorAttachmentCount = 1, .pColorAttachmentFormats = &m_SwapChainSurfaceFormat.format, .depthAttachmentFormat = findDepthFormat()}};

    m_GraphicsPipeline = vk::raii::Pipeline(m_LogicalDevice, nullptr, pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>());
}

vk::raii::ShaderModule Engine::createShaderModule(const std::vector<char>& code) const
{
    vk::ShaderModuleCreateInfo createInfo{ .codeSize = code.size() * sizeof(char), .pCode = reinterpret_cast<const uint32_t*>(code.data()) };
    return vk::raii::ShaderModule(m_LogicalDevice, createInfo);
}

void Engine::createVertexBuffer()
{
    vk::DeviceSize bufferSize = sizeof(m_Vertices[0]) * m_Vertices.size();

    auto [stagingBuffer, stagingBufferMemory] =
    createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    void *dataStaging = stagingBufferMemory.mapMemory(0, bufferSize);
    memcpy(dataStaging, m_Vertices.data(), bufferSize);
    stagingBufferMemory.unmapMemory();

    std::tie(m_VertexBuffer, m_VertexBufferMemory) =
    createBuffer(bufferSize, vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal);

    copyBuffer(stagingBuffer, m_VertexBuffer, bufferSize);
}

void Engine::createIndexBuffer()
{
    vk::DeviceSize bufferSize = sizeof(m_Indices[0]) * m_Indices.size();

    auto [stagingBuffer, stagingBufferMemory] =
        createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    void *data = stagingBufferMemory.mapMemory(0, bufferSize);
    memcpy(data, m_Indices.data(), (size_t) bufferSize);
    stagingBufferMemory.unmapMemory();

    std::tie(m_IndexBuffer, m_IndexBufferMemory) =
        createBuffer(bufferSize, vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal);

    copyBuffer(stagingBuffer, m_IndexBuffer, bufferSize);
}

void Engine::createUniformBuffers()
{
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vk::DeviceSize bufferSize = sizeof(UniformBufferObject);
        auto [buffer, bufferMem]  = createBuffer(
            bufferSize, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
        m_UniformBuffers.emplace_back(std::move(buffer));
        m_UniformBuffersMemory.emplace_back(std::move(bufferMem));
        m_UniformBuffersMapped.emplace_back( m_UniformBuffersMemory.back().mapMemory(0, bufferSize));
    }
}

std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> Engine::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties)
{
    vk::BufferCreateInfo   bufferInfo{.size = size, .usage = usage, .sharingMode = vk::SharingMode::eExclusive};
    vk::raii::Buffer       buffer          = vk::raii::Buffer(m_LogicalDevice, bufferInfo);
    vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();
    vk::MemoryAllocateInfo allocInfo{.allocationSize = memRequirements.size, .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties)};
    vk::raii::DeviceMemory bufferMemory = vk::raii::DeviceMemory(m_LogicalDevice, allocInfo);
    buffer.bindMemory(*bufferMemory, 0);
    return {std::move(buffer), std::move(bufferMemory)};
}

void Engine::copyBuffer(vk::raii::Buffer& srcBuffer, vk::raii::Buffer& dstBuffer, vk::DeviceSize size)
{
    vk::raii::CommandBuffer commandCopyBuffer = beginSingleTimeCommands();
    commandCopyBuffer.copyBuffer(*srcBuffer, *dstBuffer, vk::BufferCopy{.size = size});
    endSingleTimeCommands(std::move(commandCopyBuffer));
}

uint32_t Engine::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties)
{
    vk::PhysicalDeviceMemoryProperties memProperties = m_PhysicalDevice.getMemoryProperties();

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

//UBO
void Engine::createDescriptorSetLayout()
{
    std::array<vk::DescriptorSetLayoutBinding, 2> bindings{
    {{.binding = 0, .descriptorType = vk::DescriptorType::eUniformBuffer, .descriptorCount = 1, .stageFlags = vk::ShaderStageFlagBits::eVertex},
     {.binding = 1, .descriptorType = vk::DescriptorType::eCombinedImageSampler, .descriptorCount = 1, .stageFlags = vk::ShaderStageFlagBits::eFragment}}};

    vk::DescriptorSetLayoutCreateInfo layoutInfo{.bindingCount = static_cast<uint32_t>(bindings.size()), .pBindings = bindings.data()};

    m_DescriptorSetLayout = vk::raii::DescriptorSetLayout(m_LogicalDevice, layoutInfo);
}

void Engine::createDescriptorPool()
{
    std::array<vk::DescriptorPoolSize, 2> poolSize{{{.type = vk::DescriptorType::eUniformBuffer, .descriptorCount = MAX_FRAMES_IN_FLIGHT},
                                                    {.type = vk::DescriptorType::eCombinedImageSampler, .descriptorCount = MAX_FRAMES_IN_FLIGHT}}};
    vk::DescriptorPoolCreateInfo          poolInfo{.flags         = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
                                                   .maxSets       = MAX_FRAMES_IN_FLIGHT,
                                                   .poolSizeCount = static_cast<uint32_t>(poolSize.size()),
                                                   .pPoolSizes    = poolSize.data()};

    m_DescriptorPool = vk::raii::DescriptorPool(m_LogicalDevice, poolInfo);
}

void Engine::createDescriptorSets()
{
    std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_DescriptorSetLayout);
    vk::DescriptorSetAllocateInfo allocInfo{
        .descriptorPool = *m_DescriptorPool,
        .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
        .pSetLayouts = layouts.data()};

    m_DescriptorSets = m_LogicalDevice.allocateDescriptorSets(allocInfo);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vk::DescriptorBufferInfo bufferInfo{ .buffer = m_UniformBuffers[i], .offset = 0, .range = sizeof(UniformBufferObject) };
        vk::DescriptorImageInfo  imageInfo{.sampler = m_TextureSampler, .imageView = m_TextureImageView, .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal};

        std::array<vk::WriteDescriptorSet, 2> descriptorWrites{{{.dstSet          = m_DescriptorSets[i],
                                                                 .dstBinding      = 0,
                                                                 .dstArrayElement = 0,
                                                                 .descriptorCount = 1,
                                                                 .descriptorType  = vk::DescriptorType::eUniformBuffer,
                                                                 .pBufferInfo     = &bufferInfo},
                                                                {.dstSet          = m_DescriptorSets[i],
                                                                 .dstBinding      = 1,
                                                                 .dstArrayElement = 0,
                                                                 .descriptorCount = 1,
                                                                 .descriptorType  = vk::DescriptorType::eCombinedImageSampler,
                                                                 .pImageInfo      = &imageInfo}}};
        m_LogicalDevice.updateDescriptorSets(descriptorWrites, {});
    }
}

void Engine::createCommandPool()
{
    vk::CommandPoolCreateInfo poolCreateInfo {.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer, .queueFamilyIndex = m_GraphicsQueueIndex};
    m_CommandPool = vk::raii::CommandPool(m_LogicalDevice, poolCreateInfo);
}

void Engine::createCommandBuffers()
{
    vk::CommandBufferAllocateInfo allocInfo{.commandPool = m_CommandPool, .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = MAX_FRAMES_IN_FLIGHT};
    m_CommandBuffers = vk::raii::CommandBuffers( m_LogicalDevice, allocInfo );
}

vk::raii::CommandBuffer Engine::beginSingleTimeCommands()
{
    vk::CommandBufferAllocateInfo allocInfo{.commandPool = m_CommandPool, .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = 1};
    vk::raii::CommandBuffer       commandBuffer = std::move(vk::raii::CommandBuffers(m_LogicalDevice, allocInfo).front());

    vk::CommandBufferBeginInfo beginInfo{.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit};
    commandBuffer.begin(beginInfo);

    return std::move(commandBuffer);
}

void Engine::endSingleTimeCommands(vk::raii::CommandBuffer &&commandBuffer)
{
    commandBuffer.end();

    vk::SubmitInfo submitInfo{.commandBufferCount = 1, .pCommandBuffers = &*commandBuffer};
    m_GraphicsQueue.submit(submitInfo, nullptr);
    m_GraphicsQueue.waitIdle();
}

void Engine::recordCommandBuffer(uint32_t imageIndex)
{
    auto &commandBuffer = m_CommandBuffers[m_FrameIndex];
    commandBuffer.begin({});

    // Before starting rendering, transition the swapchain image to COLOR_ATTACHMENT_OPTIMAL
    transition_image_layout(
        m_SwapChainImages[imageIndex],
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal,
        {},                                                        // srcAccessMask (no need to wait for previous operations)
        vk::AccessFlagBits2::eColorAttachmentWrite,                // dstAccessMask
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,        // srcStage
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,        // dstStage
        vk::ImageAspectFlagBits::eColor);
    // Transition the multisampled color image to COLOR_ATTACHMENT_OPTIMAL
    transition_image_layout(
        *m_ColorImage,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::ImageAspectFlagBits::eColor);
    // Transition the depth image to DEPTH_ATTACHMENT_OPTIMAL
    transition_image_layout(
        *m_DepthImage,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eDepthAttachmentOptimal,
        vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
        vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
        vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
        vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
        vk::ImageAspectFlagBits::eDepth);

    vk::ClearValue clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
    vk::ClearValue clearDepth = vk::ClearDepthStencilValue(1.0f, 0);

    vk::RenderingAttachmentInfo colorAttachmentInfo = {
        .imageView   = m_ColorImageView,
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .resolveMode = vk::ResolveModeFlagBits::eAverage,
        .resolveImageView = m_SwapChainImageViews[imageIndex],
        .resolveImageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp      = vk::AttachmentLoadOp::eClear,
        .storeOp     = vk::AttachmentStoreOp::eStore,
        .clearValue  = clearColor
    };

    vk::RenderingAttachmentInfo depthAttachmentInfo = {
        .imageView   = m_DepthImageView,
        .imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
        .loadOp      = vk::AttachmentLoadOp::eClear,
        .storeOp     = vk::AttachmentStoreOp::eDontCare,
        .clearValue  = clearDepth};

    vk::RenderingInfo renderingInfo = {
        .renderArea           = {.offset = {0, 0}, .extent = m_SwapChainExtent},
        .layerCount           = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments    = &colorAttachmentInfo,
        .pDepthAttachment     = &depthAttachmentInfo};

    commandBuffer.beginRendering(renderingInfo);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *m_GraphicsPipeline);
    commandBuffer.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(m_SwapChainExtent.width), static_cast<float>(m_SwapChainExtent.height), 0.0f, 1.0f));
    commandBuffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), m_SwapChainExtent));

    commandBuffer.bindVertexBuffers(0, *m_VertexBuffer, {0}),
    commandBuffer.bindIndexBuffer(*m_IndexBuffer, 0, vk::IndexTypeValue<decltype(m_Indices)::value_type>::value);

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *m_PipelineLayout, 0, *m_DescriptorSets[m_FrameIndex], {});

    commandBuffer.drawIndexed(static_cast<uint32_t>(m_Indices.size()), 1, 0, 0, 0);

    commandBuffer.endRendering();

    // Transition the swapchain image back to vk::ImageLayout::ePresentSrcKHR
    transition_image_layout(
    m_SwapChainImages[imageIndex],
    vk::ImageLayout::eColorAttachmentOptimal,
    vk::ImageLayout::ePresentSrcKHR,
    vk::AccessFlagBits2::eColorAttachmentWrite,             // srcAccessMask
    {},                                                     // dstAccessMask
    vk::PipelineStageFlagBits2::eColorAttachmentOutput,     // srcStage
    vk::PipelineStageFlagBits2::eBottomOfPipe,               // dstStage
    vk::ImageAspectFlagBits::eColor
    );

    commandBuffer.end();
}

// ?? Pasar de imágen con layout sin definir a layout de COLOR_ATTACHMENT ????
// Attachment = render target
void Engine::transition_image_layout(
    vk::Image               image,
    vk::ImageLayout         old_layout,
    vk::ImageLayout         new_layout,
    vk::AccessFlags2        src_access_mask,
    vk::AccessFlags2        dst_access_mask,
    vk::PipelineStageFlags2 src_stage_mask,
    vk::PipelineStageFlags2 dst_stage_mask,
    vk::ImageAspectFlags    image_aspect_flags)
{
    vk::ImageMemoryBarrier2 barrier = {
        .srcStageMask        = src_stage_mask,
        .srcAccessMask       = src_access_mask,
        .dstStageMask        = dst_stage_mask,
        .dstAccessMask       = dst_access_mask,
        .oldLayout           = old_layout,
        .newLayout           = new_layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image               = image,
        .subresourceRange    = {
            .aspectMask     = image_aspect_flags,
            .baseMipLevel   = 0,
            .levelCount     = 1,
            .baseArrayLayer = 0,
            .layerCount     = 1}};
    vk::DependencyInfo dependencyInfo = {
        .dependencyFlags         = {},
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers    = &barrier};
    m_CommandBuffers[m_FrameIndex].pipelineBarrier2(dependencyInfo);
}

void Engine::transitionImageLayout(vk::raii::CommandBuffer &commandBuffer, const vk::raii::Image &image, uint32_t mipLevels,vk::ImageLayout oldLayout, vk::ImageLayout newLayout)
{
    vk::ImageMemoryBarrier barrier{.oldLayout           = oldLayout,
                                   .newLayout           = newLayout,
                                   .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
                                   .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
                                   .image               = image,
                                   .subresourceRange    = {.aspectMask = vk::ImageAspectFlagBits::eColor, .levelCount = mipLevels, .layerCount = 1}};

    vk::PipelineStageFlags sourceStage;
    vk::PipelineStageFlags destinationStage;

    if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal)
    {
        barrier.srcAccessMask = {};
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

        sourceStage      = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eTransfer;
    }
    else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
    {
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        sourceStage      = vk::PipelineStageFlagBits::eTransfer;
        destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
    }
    else
    {
        throw std::invalid_argument("unsupported layout transition!");
    }
    commandBuffer.pipelineBarrier(    sourceStage, destinationStage, {}, {}, nullptr, barrier);
}

void Engine::createTextureImage()
{
    int            texWidth, texHeight, texChannels;
    stbi_uc       *pixels    = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    m_MipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
    vk::DeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels)
    {
        throw std::runtime_error("failed to load texture image!");
    }

    auto [stagingBuffer, stagingBufferMemory] =
    createBuffer(imageSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    void* data = stagingBufferMemory.mapMemory(0, imageSize);
    memcpy(data, pixels, imageSize);
    stagingBufferMemory.unmapMemory();

    stbi_image_free(pixels);

    std::tie(m_TextureImage, m_TextureImageMemory) = createImage(
                                                                texWidth,
                                                                texHeight,
                                                                m_MipLevels,
                                                                vk::SampleCountFlagBits::e1,
                                                                vk::Format::eR8G8B8A8Srgb,
                                                                vk::ImageTiling::eOptimal,
                                                                vk::ImageUsageFlagBits::eTransferDst |
                                                                vk::ImageUsageFlagBits::eSampled |
                                                                vk::ImageUsageFlagBits::eTransferSrc,
                                                                vk::MemoryPropertyFlagBits::eDeviceLocal);

    vk::raii::CommandBuffer commandBuffer = beginSingleTimeCommands();
    transitionImageLayout(commandBuffer, m_TextureImage, m_MipLevels, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
    copyBufferToImage(commandBuffer, stagingBuffer, m_TextureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
    generateMipmaps(commandBuffer, m_TextureImage, vk::Format::eR8G8B8A8Srgb, texWidth, texHeight, m_MipLevels);


    endSingleTimeCommands(std::move(commandBuffer));
}

void Engine::generateMipmaps(vk::raii::CommandBuffer &commandBuffer,
                     vk::raii::Image         &image,
                     vk::Format               imageFormat,
                     int32_t                  texWidth,
                     int32_t                  texHeight,
                     uint32_t                 mipLevels)
{
    vk::FormatProperties formatProperties = m_PhysicalDevice.getFormatProperties(imageFormat);

    if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear))
    {
        throw std::runtime_error("texture image format does not support linear blitting!");
    }

    vk::ImageMemoryBarrier barrier = {
        .srcAccessMask       = vk::AccessFlagBits::eTransferWrite,
            .dstAccessMask       = vk::AccessFlagBits::eTransferRead,
            .oldLayout           = vk::ImageLayout::eTransferDstOptimal,
            .newLayout           = vk::ImageLayout::eTransferSrcOptimal,
            .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
            .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
            .image               = image,
            .subresourceRange    = {
                .aspectMask     = vk::ImageAspectFlagBits::eColor, // ¡Le decimos que es COLOR!
                .baseMipLevel   = 0,
                .levelCount     = 1,                               // 1 nivel por cada paso
                .baseArrayLayer = 0,
                .layerCount     = 1                                // 1 capa de textura
            }
    };

    int32_t mipWidth  = texWidth;
    int32_t mipHeight = texHeight;

    for (uint32_t i = 1; i < mipLevels; i++)
    {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout                     = vk::ImageLayout::eTransferDstOptimal;
        barrier.newLayout                     = vk::ImageLayout::eTransferSrcOptimal;
        barrier.srcAccessMask                 = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask                 = vk::AccessFlagBits::eTransferRead;

        commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, barrier);

        vk::ImageBlit blit = {.srcSubresource = {.aspectMask = vk::ImageAspectFlagBits::eColor, .mipLevel = i - 1, .layerCount = 1},
                      .srcOffsets     = std::array<vk::Offset3D, 2>({{}, {mipWidth, mipHeight, 1}}),
                      .dstSubresource = {.aspectMask = vk::ImageAspectFlagBits::eColor, .mipLevel = i, .layerCount = 1},
                      .dstOffsets     = std::array<vk::Offset3D, 2>({{}, {1 < mipWidth ? mipWidth / 2 : 1, 1 < mipHeight ? mipHeight / 2 : 1, 1}})};

        commandBuffer.blitImage(image, vk::ImageLayout::eTransferSrcOptimal, image, vk::ImageLayout::eTransferDstOptimal, blit, vk::Filter::eLinear);

        barrier.oldLayout     = vk::ImageLayout::eTransferSrcOptimal;
        barrier.newLayout     = vk::ImageLayout::eShaderReadOnlyOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, barrier);

        if (1 < mipWidth)
        {
            mipWidth /= 2;
        }
        if (1 < mipHeight)
        {
            mipHeight /= 2;
        }
    }

    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout                     = vk::ImageLayout::eTransferDstOptimal;
    barrier.newLayout                     = vk::ImageLayout::eShaderReadOnlyOptimal;
    barrier.srcAccessMask                 = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask                 = vk::AccessFlagBits::eShaderRead;

    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, barrier);
}

std::pair<vk::raii::Image, vk::raii::DeviceMemory> Engine::createImage(
    uint32_t width, uint32_t height, uint32_t mipLevels, vk::SampleCountFlagBits numSamples, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties )
{
    vk::ImageCreateInfo imageInfo{.imageType   = vk::ImageType::e2D,
                                  .format      = format,
                                  .extent      = {width, height, 1},
                                  .mipLevels   = mipLevels,
                                  .arrayLayers = 1,
                                  .samples     = numSamples,
                                  .tiling      = tiling,
                                  .usage       = usage,
                                  .sharingMode = vk::SharingMode::eExclusive};

    vk::raii::Image image = vk::raii::Image(m_LogicalDevice, imageInfo);

    vk::MemoryRequirements memRequirements = image.getMemoryRequirements();
    vk::MemoryAllocateInfo allocInfo{.allocationSize  = memRequirements.size,
                                     .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties)};
    vk::raii::DeviceMemory imageMemory = vk::raii::DeviceMemory(m_LogicalDevice, allocInfo);
    image.bindMemory(imageMemory, 0);

    return {std::move(image), std::move(imageMemory)};
}

void Engine::copyBufferToImage(vk::raii::CommandBuffer &commandBuffer, const vk::raii::Buffer &buffer, vk::raii::Image &image, uint32_t width, uint32_t height)
{
    vk::BufferImageCopy region{.bufferOffset      = 0,
                               .bufferRowLength   = 0,
                               .bufferImageHeight = 0,
                               .imageSubresource  = {.aspectMask = vk::ImageAspectFlagBits::eColor, .mipLevel = 0, .baseArrayLayer = 0, .layerCount = 1},
                               .imageOffset       = {0, 0, 0},
                               .imageExtent       = {width, height, 1}};
    commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, region);
}

void Engine::createTextureImageView()
{
    m_TextureImageView = createImageView(*m_TextureImage, m_MipLevels, vk::Format::eR8G8B8A8Srgb);
}

vk::raii::ImageView Engine::createImageView(vk::Image const &image, uint32_t mipLevels, vk::Format format, vk::ImageAspectFlags aspectFlags)
{
    vk::ImageViewCreateInfo viewInfo{
        .image            = image,
        .viewType         = vk::ImageViewType::e2D,
        .format           = format,
        .subresourceRange = {.aspectMask = aspectFlags, .baseMipLevel = 0, .levelCount = mipLevels, .baseArrayLayer = 0, .layerCount = 1}};

    return vk::raii::ImageView(m_LogicalDevice, viewInfo);
}

void Engine::createTextureSampler()
{
    vk::PhysicalDeviceProperties properties = m_PhysicalDevice.getProperties();
    vk::SamplerCreateInfo        samplerInfo{
        .magFilter        = vk::Filter::eLinear,
        .minFilter        = vk::Filter::eLinear,
        .mipmapMode       = vk::SamplerMipmapMode::eLinear,
        .addressModeU     = vk::SamplerAddressMode::eRepeat,
        .addressModeV     = vk::SamplerAddressMode::eRepeat,
        .addressModeW     = vk::SamplerAddressMode::eRepeat,
        .mipLodBias       = 0.0f,
        .anisotropyEnable = vk::True,
        .maxAnisotropy    = properties.limits.maxSamplerAnisotropy,
        .compareEnable    = vk::False,
        .compareOp        = vk::CompareOp::eAlways,
        .minLod           = 0.0f,
        .maxLod           = vk::LodClampNone};
    m_TextureSampler = vk::raii::Sampler(m_LogicalDevice, samplerInfo);
}

void Engine::createDepthResources()
{
    vk::Format depthFormat = findDepthFormat();
    std::tie(m_DepthImage, m_DepthImageMemory) = createImage(m_SwapChainExtent.width, m_SwapChainExtent.height, 1, m_MSaaSamples, depthFormat, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal);
    m_DepthImageView = createImageView(*m_DepthImage, 1, depthFormat, vk::ImageAspectFlagBits::eDepth);
}

vk::Format Engine::findDepthFormat()
{
    return findSupportedFormat({vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
                               vk::ImageTiling::eOptimal,
                               vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}

vk::SampleCountFlagBits Engine::getMaxUsableSampleCount()
{
    vk::PhysicalDeviceProperties physicalDeviceProperties = m_PhysicalDevice.getProperties();

    vk::SampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
    if (counts & vk::SampleCountFlagBits::e64) { return vk::SampleCountFlagBits::e64; }
    if (counts & vk::SampleCountFlagBits::e32) { return vk::SampleCountFlagBits::e32; }
    if (counts & vk::SampleCountFlagBits::e16) { return vk::SampleCountFlagBits::e16; }
    if (counts & vk::SampleCountFlagBits::e8) { return vk::SampleCountFlagBits::e8; }
    if (counts & vk::SampleCountFlagBits::e4) { return vk::SampleCountFlagBits::e4; }
    if (counts & vk::SampleCountFlagBits::e2) { return vk::SampleCountFlagBits::e2; }

    return vk::SampleCountFlagBits::e1;
}

vk::Format Engine::findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features) {
    for (const auto format : candidates) {
        vk::FormatProperties props = m_PhysicalDevice.getFormatProperties(format);

        if (((tiling == vk::ImageTiling::eLinear) && ((props.linearTilingFeatures & features) == features)) ||
            ((tiling == vk::ImageTiling::eOptimal) && ((props.optimalTilingFeatures & features) == features)))
        {
            return format;
        }
    }

    throw std::runtime_error("failed to find supported format!");
}

void Engine::createColorResources()
{
    vk::Format colorFormat = m_SwapChainSurfaceFormat.format;

    std::tie(m_ColorImage, m_ColorImageMemory) = createImage(m_SwapChainExtent.width, m_SwapChainExtent.height, 1, m_MSaaSamples, colorFormat, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment,  vk::MemoryPropertyFlagBits::eDeviceLocal);
    m_ColorImageView = createImageView(m_ColorImage, 1, colorFormat, vk::ImageAspectFlagBits::eColor);
}

void Engine::createSyncronizationObjetcs()
{
    assert(m_PresentCompleteSemaphores.empty() && m_RenderFinishedSemaphores.empty() && m_InFlightFences.empty());

    for (size_t i = 0; i < m_SwapChainImages.size(); i++)
    {
        m_RenderFinishedSemaphores.emplace_back(m_LogicalDevice, vk::SemaphoreCreateInfo());
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        m_PresentCompleteSemaphores.emplace_back(m_LogicalDevice, vk::SemaphoreCreateInfo());
        m_InFlightFences.emplace_back(m_LogicalDevice, vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled});
    }
}

void Engine::drawFrame()
{
    // Note: inFlightFences, presentCompleteSemaphores, and commandBuffers are indexed by frameIndex,
    //        while renderFinishedSemaphores is indexed by imageIndex

    // Frame index depende de los frames al vuelo
    // Image index de la imagen física de la swapchain

    // Esperamos a que la fence del frame concreto termine al acabar el renderizado
    auto fenceResult = m_LogicalDevice.waitForFences(*m_InFlightFences[m_FrameIndex], vk::True, UINT64_MAX);
    if (fenceResult != vk::Result::eSuccess) {
        throw std::runtime_error("failed to wait for fence!");
    }

    // Sacamos la imagen física de la swapchain sobre la que se va a pintar
    // Al acabar de sacar la imagen de la swapchain, le decimos al submitInfo del comando de renderizado que ya la puede
    // empezar a renderizar
    auto [result, imageIndex] = m_SwapChain.acquireNextImage(UINT64_MAX, *m_PresentCompleteSemaphores[m_FrameIndex], nullptr);

    if (result == vk::Result::eErrorOutOfDateKHR)
    {
        recreateSwapChain();
        return;
    }
    if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
    {
        assert(result == vk::Result::eTimeout || result == vk::Result::eNotReady);
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    // La bloqueamos de nuevo para que no haya condición de carrera al no haber acabado de renderizar el frame
    m_LogicalDevice.resetFences(*m_InFlightFences[m_FrameIndex]);

    // Volvemos a reescribir el comando de pintado (el comando depende de la imagen)
    m_CommandBuffers[m_FrameIndex].reset();
    recordCommandBuffer(imageIndex);

    // Esperaamos a la señal del semaforo de presentacion una vez llegamos a la parte de
    // volcar los píxeles en el buffer del frame actual
    vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);

    updateUniformBuffer(m_FrameIndex);

    // Esperamos a que esté el frame listo sobre el que pintar y al acabar lo comunicamos al presentInfo
    const vk::SubmitInfo   submitInfo{.waitSemaphoreCount   = 1,
                                      .pWaitSemaphores      = &*m_PresentCompleteSemaphores[m_FrameIndex],
                                      .pWaitDstStageMask    = &waitDestinationStageMask,
                                      .commandBufferCount   = 1,
                                      .pCommandBuffers      = &*m_CommandBuffers[m_FrameIndex],
                                      .signalSemaphoreCount = 1,
                                      .pSignalSemaphores    = &*m_RenderFinishedSemaphores[imageIndex]};
    // Enviamos el commando de renderizado a la cola de gráficos y le damos la señal a la fence para
    // que pueda empezar a adquirir la imagen de la swapchain
    m_GraphicsQueue.submit(submitInfo, *m_InFlightFences[m_FrameIndex]);

    // Parte de presentar a la pantalla

    // No presentamos a la pantalla hasta que el frame esté pintado del todo
    const vk::PresentInfoKHR presentInfoKHR{.waitSemaphoreCount = 1,
                                            .pWaitSemaphores    = &*m_RenderFinishedSemaphores[imageIndex],
                                            .swapchainCount     = 1,
                                            .pSwapchains        = &*m_SwapChain,
                                            .pImageIndices      = &imageIndex};
    result = m_GraphicsQueue.presentKHR(presentInfoKHR);

    if ((result == vk::Result::eSuboptimalKHR) || (result == vk::Result::eErrorOutOfDateKHR) || m_FramebufferResized)
    {
        m_FramebufferResized = false;
        recreateSwapChain();
    }
    else
    {
        // There are no other success codes than eSuccess; on any error code, presentKHR already threw an exception.
        assert(result == vk::Result::eSuccess);
    }
    m_FrameIndex = (m_FrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Engine::updateUniformBuffer(uint32_t currentImage)
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time       = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo{};
    ubo.Model = rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.View = lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.Proj =
    glm::perspective(glm::radians(45.0f), static_cast<float>(m_SwapChainExtent.width) / static_cast<float>(m_SwapChainExtent.height), 0.1f, 10.0f);
    ubo.Proj[1][1] *= -1;
    memcpy(m_UniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

void Engine::mainLoop() {
    while (!glfwWindowShouldClose(m_Window)) {
        glfwPollEvents();
        drawFrame();
    }

    m_LogicalDevice.waitIdle();
}

void Engine::cleanup()
{
    cleanupSwapChain();

    if (m_Window) {
        glfwDestroyWindow(m_Window);
    }
}

void Engine::loadModel()
{
    tinyobj::attrib_t                attrib;
    std::vector<tinyobj::shape_t>    shapes;
    std::vector<tinyobj::material_t> materials;
    std::string                      warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str()))
    {
        throw std::runtime_error(warn + err);
    }

    for (const auto& shape : shapes)
    {
        for (const auto& index : shape.mesh.indices)
        {
            Vertex vertex{};

            vertex.Pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]};

            vertex.TexCoord = {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};

            vertex.Color = {1.0f, 1.0f, 1.0f};

            m_Vertices.push_back(vertex);
            m_Indices.push_back(m_Indices.size());
        }
    }
}

std::vector<char> Engine::readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("[RUNTIME_ERROR] failed to open file: " + filename);
    }

    // Reservamos memoria leyendo la posición del final
    std::vector<char> buffer(file.tellg());

    file.seekg(0, std::ios::beg);
    file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));

    file.close();

    std::cout << "file opened successfully, size: " << buffer.size() << " bytes";

    return buffer;
}