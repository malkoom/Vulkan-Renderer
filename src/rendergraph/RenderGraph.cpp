// RenderGraph.cpp
#include "RenderGraph.hpp"
#include "../services/GraphicsServices.hpp"
#include "../VulkanUtils.hpp"

#include <stdexcept>
#include <cassert>

RenderGraph::RenderGraph(const vk::raii::Device& device, uint32_t maxFramesInFlight)
    : m_Device(device), m_MaxFramesInFlight(maxFramesInFlight)
{
    // Creación de objetos de sincronización para el bucle de renderizado
    for (size_t i = 0; i < m_MaxFramesInFlight; ++i) {
        m_ImageAvailableSemaphores.emplace_back(m_Device, vk::SemaphoreCreateInfo{});
        m_InFlightFences.emplace_back(m_Device, vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled});
    }
}

void RenderGraph::AddResource(const std::string& name, vk::Format format, vk::Extent2D extent,
                              vk::ImageUsageFlags usage, vk::ImageLayout initialLayout, vk::ImageLayout finalLayout)
{
    Resource resource;
    resource.name = name;
    resource.format = format;
    resource.extent = extent;
    resource.usage = usage;
    resource.initialLayout = initialLayout;
    resource.finalLayout = finalLayout;

    m_Resources[name] = std::move(resource);
}

void RenderGraph::AddPass(const std::string& name, const std::vector<std::string>& inputs,
                          const std::vector<std::string>& outputs, std::function<void(vk::raii::CommandBuffer&)> executeFunc)
{
    Pass pass;
    pass.name = name;
    pass.inputs = inputs;
    pass.outputs = outputs;
    pass.executeFunc = std::move(executeFunc);

    m_Passes.push_back(pass);
}

void RenderGraph::Compile()
{
    m_ExecutionOrder.clear();
    m_Semaphores.clear();
    m_SemaphoreSignalWaitPairs.clear();

    // 1. Descubrimiento de Dependencias
    std::vector<std::vector<size_t>> dependencies(m_Passes.size());
    std::vector<std::vector<size_t>> dependents(m_Passes.size());
    std::unordered_map<std::string, size_t> resourceWriters;

    for (size_t i = 0; i < m_Passes.size(); ++i) {
        const auto& pass = m_Passes[i];

        for (const auto& input : pass.inputs) {
            auto it = resourceWriters.find(input);
            if (it != resourceWriters.end()) {
                dependencies[i].push_back(it->second);
                dependents[it->second].push_back(i);
            }
        }

        for (const auto& output : pass.outputs) {
            resourceWriters[output] = i;
        }
    }

    // 2. Ordenamiento Topológico y Detección de Ciclos (DFS)
    std::vector<bool> visited(m_Passes.size(), false);
    std::vector<bool> inStack(m_Passes.size(), false);

    std::function<void(size_t)> visit = [&](size_t node) {
        if (inStack[node]) {
            throw std::runtime_error("Cycle detected in RenderGraph!");
        }
        if (visited[node]) return;

        inStack[node] = true;
        for (auto dependent : dependents[node]) {
            visit(dependent);
        }
        inStack[node] = false;
        visited[node] = true;
        m_ExecutionOrder.push_back(node);
    };

    for (size_t i = 0; i < m_Passes.size(); ++i) {
        if (!visited[i]) visit(i);
    }

    // 3. Creación Autómata de Semáforos Inter-pase
    for (size_t i = 0; i < m_Passes.size(); ++i) {
        for (auto dep : dependencies[i]) {
            m_Semaphores.emplace_back(m_Device, vk::SemaphoreCreateInfo{});
            m_SemaphoreSignalWaitPairs.emplace_back(dep, i);
        }
    }

    // 4. Asignación Física de Recursos en VRAM
    for (auto& [name, resource] : m_Resources) {
        vk::ImageCreateInfo imageInfo{
            .imageType     = vk::ImageType::e2D,
            .format        = resource.format,
            .extent        = {resource.extent.width, resource.extent.height, 1},
            .mipLevels     = 1,
            .arrayLayers   = 1,
            .samples       = vk::SampleCountFlagBits::e1,
            .tiling        = vk::ImageTiling::eOptimal,
            .usage         = resource.usage,
            .sharingMode   = vk::SharingMode::eExclusive,
            .initialLayout = vk::ImageLayout::eUndefined
        };

        resource.image = vk::raii::Image(m_Device, imageInfo);

        vk::MemoryRequirements memReqs = resource.image.getMemoryRequirements();
        vk::MemoryAllocateInfo allocInfo{
            .allocationSize  = memReqs.size,
            .memoryTypeIndex = VulkanUtils::FindMemoryType(memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal)
        };

        resource.memory = vk::raii::DeviceMemory(m_Device, allocInfo);
        resource.image.bindMemory(*resource.memory, 0);

        vk::ImageViewCreateInfo viewInfo{
            .image    = *resource.image,
            .viewType = vk::ImageViewType::e2D,
            .format   = resource.format,
            .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
        };

        resource.view = vk::raii::ImageView(m_Device, viewInfo);
    }
}

void RenderGraph::Execute(vk::raii::CommandBuffer& commandBuffer, vk::Queue queue,
                         uint32_t imageIndex, vk::Semaphore imageAvailable, vk::Semaphore renderFinished)
{
    std::vector<vk::Semaphore> waitSemaphores;
    std::vector<vk::PipelineStageFlags> waitStages;
    std::vector<vk::Semaphore> signalSemaphores;

    size_t firstPassIdx = m_ExecutionOrder.front();
    size_t lastPassIdx  = m_ExecutionOrder.back();

    for (auto passIdx : m_ExecutionOrder) {
        const auto& pass = m_Passes[passIdx];

        waitSemaphores.clear();
        waitStages.clear();
        signalSemaphores.clear();

        // Si es el primer pase, esperamos a que la Swapchain tenga la imagen lista
        if (passIdx == firstPassIdx) {
            waitSemaphores.push_back(imageAvailable);
            waitStages.push_back(vk::PipelineStageFlagBits::eColorAttachmentOutput);
        }

        // Recoger semáforos inter-pase (Consumidor)
        for (size_t i = 0; i < m_SemaphoreSignalWaitPairs.size(); ++i) {
            if (m_SemaphoreSignalWaitPairs[i].second == passIdx) {
                waitSemaphores.push_back(*m_Semaphores[i]);
                waitStages.push_back(vk::PipelineStageFlagBits::eColorAttachmentOutput);
            }
        }

        // Recoger semáforos inter-pase (Productor)
        for (size_t i = 0; i < m_SemaphoreSignalWaitPairs.size(); ++i) {
            if (m_SemaphoreSignalWaitPairs[i].first == passIdx) {
                signalSemaphores.push_back(*m_Semaphores[i]);
            }
        }

        // Si es el último pase, avisamos al semáforo de presentación
        if (passIdx == lastPassIdx) {
            signalSemaphores.push_back(renderFinished);
        }

        // Grabación de Comandos
        commandBuffer.begin({.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

        // Barriadas de Entrada (ShaderRead)
        for (const auto& input : pass.inputs) {
            auto& resource = m_Resources[input];
            vk::ImageMemoryBarrier barrier{
                .srcAccessMask       = vk::AccessFlagBits::eMemoryWrite,
                .dstAccessMask       = vk::AccessFlagBits::eShaderRead,
                .oldLayout           = resource.initialLayout,
                .newLayout           = vk::ImageLayout::eShaderReadOnlyOptimal,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image               = *resource.image,
                .subresourceRange    = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
            };

            commandBuffer.pipelineBarrier(
                vk::PipelineStageFlagBits::eAllCommands,
                vk::PipelineStageFlagBits::eFragmentShader,
                vk::DependencyFlagBits::eByRegion, {}, {}, barrier
            );
        }

        // Barriadas de Salida (ColorAttachment)
        for (const auto& output : pass.outputs) {
            auto& resource = m_Resources[output];
            vk::ImageMemoryBarrier barrier{
                .srcAccessMask       = vk::AccessFlagBits::eMemoryRead,
                .dstAccessMask       = vk::AccessFlagBits::eColorAttachmentWrite,
                .oldLayout           = resource.initialLayout,
                .newLayout           = vk::ImageLayout::eColorAttachmentOptimal,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image               = *resource.image,
                .subresourceRange    = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
            };

            commandBuffer.pipelineBarrier(
                vk::PipelineStageFlagBits::eAllCommands,
                vk::PipelineStageFlagBits::eColorAttachmentOutput,
                vk::DependencyFlagBits::eByRegion, {}, {}, barrier
            );
        }

        // Ejecutar Lambda del Pase
        pass.executeFunc(commandBuffer);

        // Barriadas Finales
        for (const auto& output : pass.outputs) {
            auto& resource = m_Resources[output];
            vk::ImageMemoryBarrier barrier{
                .srcAccessMask       = vk::AccessFlagBits::eColorAttachmentWrite,
                .dstAccessMask       = vk::AccessFlagBits::eMemoryRead,
                .oldLayout           = vk::ImageLayout::eColorAttachmentOptimal,
                .newLayout           = resource.finalLayout,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image               = *resource.image,
                .subresourceRange    = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
            };

            commandBuffer.pipelineBarrier(
                vk::PipelineStageFlagBits::eColorAttachmentOutput,
                vk::PipelineStageFlagBits::eAllCommands,
                vk::DependencyFlagBits::eByRegion, {}, {}, barrier
            );
        }

        commandBuffer.end();

        // Enviar al Queue
        vk::SubmitInfo submitInfo{
            .waitSemaphoreCount   = static_cast<uint32_t>(waitSemaphores.size()),
            .pWaitSemaphores      = waitSemaphores.data(),
            .pWaitDstStageMask    = waitStages.data(),
            .commandBufferCount   = 1,
            .pCommandBuffers      = &*commandBuffer,
            .signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size()),
            .pSignalSemaphores    = signalSemaphores.data()
        };

        // Solo señalamos la Fence de CPU en el ÚLTIMO pase
        vk::Fence fenceToSignal = (passIdx == lastPassIdx) ? *m_InFlightFences[m_FrameIndex] : nullptr;
        queue.submit(submitInfo, fenceToSignal);
    }
}

void RenderGraph::RenderFrame(vk::Queue& graphicsQueue, vk::raii::SwapchainKHR& swapchain,
                             const std::vector<vk::raii::CommandBuffer>& commandBuffers,
                             std::function<void(uint32_t frameIdx)> updateUBOFunc,
                             std::function<void()> recreateSwapchainFunc)
{
    // 1. Esperar a la GPU para este Frame en Vuelo
    auto fenceResult = m_Device.waitForFences(*m_InFlightFences[m_FrameIndex], vk::True, UINT64_MAX);
    if (fenceResult != vk::Result::eSuccess) {
        throw std::runtime_error("Failed to wait for fence!");
    }

    // 2. Adquirir Imagen de la Swapchain
    // Creamos dinámicamente el semáforo de render finished por imagen de swapchain si es necesario
    if (m_RenderFinishedSemaphores.size() < swapchain.getImages().size()) {
        m_RenderFinishedSemaphores.clear();
        for (size_t i = 0; i < swapchain.getImages().size(); ++i) {
            m_RenderFinishedSemaphores.emplace_back(m_Device, vk::SemaphoreCreateInfo{});
        }
    }

    auto [result, imageIndex] = swapchain.acquireNextImage(UINT64_MAX, *m_ImageAvailableSemaphores[m_FrameIndex], nullptr);

    if (result == vk::Result::eErrorOutOfDateKHR) {
        recreateSwapchainFunc();
        return;
    }

    m_Device.resetFences(*m_InFlightFences[m_FrameIndex]);

    // 3. Actualizar Transformaciones / UBOs
    updateUBOFunc(m_FrameIndex);

    // 4. Delegar la Ejecución Completa al Grafo
    Execute(
        const_cast<vk::raii::CommandBuffer&>(commandBuffers[m_FrameIndex]),
        graphicsQueue,
        imageIndex,
        *m_ImageAvailableSemaphores[m_FrameIndex],
        *m_RenderFinishedSemaphores[imageIndex]
    );

    // 5. Presentación Final
    vk::PresentInfoKHR presentInfo{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = &*m_RenderFinishedSemaphores[imageIndex],
        .swapchainCount     = 1,
        .pSwapchains        = &*swapchain,
        .pImageIndices      = &imageIndex
    };

    result = graphicsQueue.presentKHR(presentInfo);

    if (result == vk::Result::eSuboptimalKHR || result == vk::Result::eErrorOutOfDateKHR || m_FramebufferResized) {
        m_FramebufferResized = false;
        recreateSwapchainFunc();
    }

    m_FrameIndex = (m_FrameIndex + 1) % m_MaxFramesInFlight;
}

RenderGraph::Resource* RenderGraph::GetResource(const std::string& name)
{
    auto it = m_Resources.find(name);
    return (it != m_Resources.end()) ? &it->second : nullptr;
}