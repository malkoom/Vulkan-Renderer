// RenderGraph.hpp
#pragma once

#include <vulkan/vulkan_raii.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>

class RenderGraph {
public:
    struct Resource {
        std::string name;
        vk::Format format;
        vk::Extent2D extent;
        vk::ImageUsageFlags usage;
        vk::ImageLayout initialLayout;
        vk::ImageLayout finalLayout;

        vk::raii::Image image = nullptr;
        vk::raii::DeviceMemory memory = nullptr;
        vk::raii::ImageView view = nullptr;
    };

    struct Pass {
        std::string name;
        std::vector<std::string> inputs;
        std::vector<std::string> outputs;
        std::function<void(vk::raii::CommandBuffer&)> executeFunc;
    };

private:
    const vk::raii::Device& m_Device;
    uint32_t m_MaxFramesInFlight;
    size_t m_FrameIndex = 0;
    bool m_FramebufferResized = false;

    // Almacenamiento global de recursos y pases
    std::unordered_map<std::string, Resource> m_Resources;
    std::vector<Pass> m_Passes;

    // Estructuras calculadas durante Compile()
    std::vector<size_t> m_ExecutionOrder;
    std::vector<vk::raii::Semaphore> m_Semaphores;
    std::vector<std::pair<size_t, size_t>> m_SemaphoreSignalWaitPairs; // (Productor, Consumidor)

    // Sincronización de la Swapchain / Frames en Vuelo
    std::vector<vk::raii::Semaphore> m_ImageAvailableSemaphores;
    std::vector<vk::raii::Semaphore> m_RenderFinishedSemaphores;
    std::vector<vk::raii::Fence>     m_InFlightFences;

public:
    RenderGraph(const vk::raii::Device& device, uint32_t maxFramesInFlight);
    ~RenderGraph() = default;

    // Configuración de la topología
    void AddResource(const std::string& name, vk::Format format, vk::Extent2D extent,
                    vk::ImageUsageFlags usage, vk::ImageLayout initialLayout, vk::ImageLayout finalLayout);

    void AddPass(const std::string& name, const std::vector<std::string>& inputs,
                 const std::vector<std::string>& outputs, std::function<void(vk::raii::CommandBuffer&)> executeFunc);

    // Compilación del grafo y reserva de memoria física
    void Compile();

    // Invocación por frame
    void Execute(vk::raii::CommandBuffer& commandBuffer, vk::Queue queue,
                 uint32_t imageIndex, vk::Semaphore imageAvailable, vk::Semaphore renderFinished);

    void RenderFrame(vk::Queue& graphicsQueue, vk::raii::SwapchainKHR& swapchain,
                     const std::vector<vk::raii::CommandBuffer>& commandBuffers,
                     std::function<void(uint32_t frameIdx)> updateUBOFunc,
                     std::function<void()> recreateSwapchainFunc);

    // Acceso a recursos
    Resource* GetResource(const std::string& name);
    void SetFramebufferResized(bool resized) { m_FramebufferResized = resized; }
};