#pragma once

#include <memory>
#include <vector>
#include <string>
#include <array>
#include <iostream>

#if defined(__INTELLISENSE__) || !defined(USE_CPP20_MODULES)
#   include <vulkan/vulkan_raii.hpp>
#else
import vulkan_hpp;
#endif

#include <GLFW/glfw3.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Includes de mi propio motor
#include "Vertex.hpp"

// Constantes globales de configuración del motor
constexpr int MAX_FRAMES_IN_FLIGHT = 2;
constexpr uint32_t WIDTH  = 800;
constexpr uint32_t HEIGHT = 600;

const std::vector<char const*> g_ValidationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
constexpr bool g_EnableValidationLayers = false;
#else
constexpr bool g_EnableValidationLayers = true;
#endif

class Engine
{
public:
    void run();

private:
    // Ventana
    GLFWwindow *m_Window = nullptr;

    // Núcleo de Vulkan RAII
    vk::raii::Context  m_Context;
    vk::raii::Instance m_Instance = nullptr;
    vk::raii::DebugUtilsMessengerEXT m_DebugMessenger = nullptr;
    vk::raii::SurfaceKHR m_Surface = nullptr;
    vk::raii::PhysicalDevice m_PhysicalDevice = nullptr;

    std::vector<const char*> m_RequiredDeviceExtension = {vk::KHRSwapchainExtensionName};
    vk::raii::Device m_LogicalDevice = nullptr;

    // Colas y Swapchain
    vk::raii::Queue m_GraphicsQueue = nullptr;
    uint32_t m_GraphicsQueueIndex = ~0;
    vk::raii::SwapchainKHR m_SwapChain = nullptr;
    std::vector<vk::Image> m_SwapChainImages;
    std::vector<vk::raii::ImageView> m_SwapChainImageViews;
    vk::SurfaceFormatKHR  m_SwapChainSurfaceFormat;
    vk::Extent2D m_SwapChainExtent;
    bool m_FramebufferResized = false;

    // Pipeline Gráfico y Descriptores
    vk::raii::DescriptorSetLayout m_DescriptorSetLayout = nullptr;
    vk::raii::PipelineLayout m_PipelineLayout = nullptr;
    vk::raii::Pipeline m_GraphicsPipeline = nullptr;

    // Buffers de Geometría
    vk::raii::Buffer m_VertexBuffer = nullptr;
    vk::raii::DeviceMemory m_VertexBufferMemory = nullptr;
    vk::raii::Buffer       m_IndexBuffer        = nullptr;
    vk::raii::DeviceMemory m_IndexBufferMemory  = nullptr;

    // MSAA buffers
    vk::raii::Image m_ColorImage = nullptr;
    vk::raii::DeviceMemory m_ColorImageMemory = nullptr;
    vk::raii::ImageView m_ColorImageView = nullptr;

    // Uniform Buffers (UBO)
    std::vector<vk::raii::Buffer>       m_UniformBuffers;
    std::vector<vk::raii::DeviceMemory> m_UniformBuffersMemory;
    std::vector<void *>                  m_UniformBuffersMapped;
    vk::raii::DescriptorPool m_DescriptorPool = nullptr;
    std::vector<vk::raii::DescriptorSet> m_DescriptorSets;

    // Texturas de la GPU
    uint32_t m_MipLevels = 0;
    vk::raii::Image        m_TextureImage       = nullptr;
    vk::raii::DeviceMemory m_TextureImageMemory = nullptr;
    vk::raii::ImageView m_TextureImageView = nullptr;
    vk::raii::Sampler m_TextureSampler = nullptr;
    vk::SampleCountFlagBits m_MSaaSamples = vk::SampleCountFlagBits::e1;


    // Depth buffering
    vk::raii::Image        m_DepthImage       = nullptr;
    vk::raii::DeviceMemory m_DepthImageMemory = nullptr;
    vk::raii::ImageView    m_DepthImageView   = nullptr;

    // Infraestructura de Comandos y Sincronización
    vk::raii::CommandPool m_CommandPool = nullptr;
    std::vector<vk::raii::CommandBuffer> m_CommandBuffers;
    std::vector<vk::raii::Semaphore> m_PresentCompleteSemaphores;
    std::vector<vk::raii::Semaphore> m_RenderFinishedSemaphores;
    std::vector<vk::raii::Fence> m_InFlightFences;
    uint32_t m_FrameIndex = 0;

    // Datos estáticos de la escena
    std::vector<Vertex> m_Vertices;
    std::vector<uint32_t> m_Indices;

    //3D Models
    const uint32_t WIDTH                = 800;
    const uint32_t HEIGHT               = 600;
    const std::string  MODEL_PATH           = "assets/models/viking_room.obj";
    const std::string  TEXTURE_PATH         = "assets/textures/viking_room.png";

    // Funciones de Inicialización y Ciclo de vida
    void initWindow();
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
    void initVulkan();
    void mainLoop();
    void cleanup();

    // Métodos de Configuración de Vulkan
    void createInstance();
    std::vector<const char*> getRequiredInstanceExtensions();
    void setupDebugMessenger();
    static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(
        vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
        vk::DebugUtilsMessageTypeFlagsEXT type,
        const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);

    void createSurface();
    bool isDeviceSuitable(vk::raii::PhysicalDevice const & physicalDevice);
    void pickPhysicalDevice();
    void createLogicalDevice();

    // Gestión de Swapchain
    vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
    vk::PresentModeKHR chooseSwapPresentMode(std::vector<vk::PresentModeKHR> const &availablePresentModes);
    vk::Extent2D chooseSwapExtent(vk::SurfaceCapabilitiesKHR const &capabilities);
    uint32_t chooseSwapMinImageCount(vk::SurfaceCapabilitiesKHR const &surfaceCapabilities);
    void createSwapChain();
    void cleanupSwapChain();
    void recreateSwapChain();
    void createImageViews();

    // Pipeline, Buffers y Texturas
    void createGraphicsPipeline();
    [[nodiscard]] vk::raii::ShaderModule createShaderModule(const std::vector<char>& code) const;
    void createVertexBuffer();
    void createIndexBuffer();
    void createUniformBuffers();
    std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);
    void copyBuffer(vk::raii::Buffer& srcBuffer, vk::raii::Buffer& dstBuffer, vk::DeviceSize size);
    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
    vk::SampleCountFlagBits getMaxUsableSampleCount();

    // Modelos 3D
    void loadModel();

    // Recursos de Texturizado, Imágenes y Depth Buffer
    void createTextureImage();
    void generateMipmaps(
        vk::raii::CommandBuffer &commandBuffer, vk::raii::Image &image, vk::Format imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
    std::pair<vk::raii::Image, vk::raii::DeviceMemory> createImage(
        uint32_t width, uint32_t height, uint32_t mipLevels, vk::SampleCountFlagBits numSamples, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties);
    void copyBufferToImage(vk::raii::CommandBuffer &commandBuffer, const vk::raii::Buffer &buffer, vk::raii::Image &image, uint32_t width, uint32_t height);
    void createTextureImageView();
    vk::raii::ImageView createImageView(vk::Image const &image, uint32_t mipLevels, vk::Format format, vk::ImageAspectFlags aspectFlags = vk::ImageAspectFlagBits::eColor);
    void createTextureSampler();
    void createDepthResources();
    vk::Format findDepthFormat();
    vk::Format findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);

    // MSAA
    void createColorResources();

    // Descriptores (UBO & Sampler)
    void createDescriptorSetLayout();
    void createDescriptorPool();
    void createDescriptorSets();

    // Comandos y Sincronización
    void createCommandPool();
    void createCommandBuffers();
    vk::raii::CommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(vk::raii::CommandBuffer &&commandBuffer);
    void recordCommandBuffer(uint32_t imageIndex);
    void drawFrame();
    void updateUniformBuffer(uint32_t currentImage);

    // Sincronización de Layouts de imágenes (Barriers)
    void transition_image_layout(
        vk::Image image,
        vk::ImageLayout old_layout,
        vk::ImageLayout new_layout,
        vk::AccessFlags2 src_access_mask,
        vk::AccessFlags2 dst_access_mask,
        vk::PipelineStageFlags2 src_stage_mask,
        vk::PipelineStageFlags2 dst_stage_mask,
        vk::ImageAspectFlags    image_aspect_flags);

    void transitionImageLayout(vk::raii::CommandBuffer &commandBuffer, const vk::raii::Image &image, uint32_t mipLevels, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);
    void createSyncronizationObjetcs();

    // Helpers
    static std::vector<char> readFile(const std::string& filename);
};