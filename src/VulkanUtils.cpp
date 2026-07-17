//
// Created by marc on 17/7/26.
//

#include "VulkanUtils.hpp"
#include "services/GraphicsServices.hpp"

std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> VulkanUtils::CreateBuffer(vk::DeviceSize size,
    vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties)
{
    vk::BufferCreateInfo   bufferInfo{.size = size, .usage = usage, .sharingMode = vk::SharingMode::eExclusive};
    vk::raii::Buffer       buffer          = vk::raii::Buffer(GraphicsServices::GetDevice(), bufferInfo);
    vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();
    vk::MemoryAllocateInfo allocInfo{.allocationSize = memRequirements.size, .memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties)};
    vk::raii::DeviceMemory bufferMemory = vk::raii::DeviceMemory(GraphicsServices::GetDevice(), allocInfo);
    buffer.bindMemory(*bufferMemory, 0);
    return {std::move(buffer), std::move(bufferMemory)};
}

std::pair<vk::raii::Image, vk::raii::DeviceMemory> VulkanUtils::CreateImage(uint32_t width, uint32_t height,
    uint32_t mipLevels, vk::SampleCountFlagBits numSamples, vk::Format format, vk::ImageTiling tiling,
    vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties)
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

    vk::raii::Image image = vk::raii::Image(GraphicsServices::GetDevice(), imageInfo);

    vk::MemoryRequirements memRequirements = image.getMemoryRequirements();
    vk::MemoryAllocateInfo allocInfo{.allocationSize  = memRequirements.size,
                                     .memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties)};
    vk::raii::DeviceMemory imageMemory = vk::raii::DeviceMemory(GraphicsServices::GetDevice(), allocInfo);
    image.bindMemory(imageMemory, 0);

    return {std::move(image), std::move(imageMemory)};
}

void VulkanUtils::TransitionImageLayout(const vk::raii::Image &image, vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout, uint32_t mipLevels)
{
    auto commandBuffer = BeginSingleTimeCommands();

    vk::ImageMemoryBarrier barrier{
        .oldLayout        = oldLayout,
        .newLayout        = newLayout,
        .image            = *image,
        .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, mipLevels, 0, 1}
    };

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
    commandBuffer.pipelineBarrier(sourceStage, destinationStage, {}, {}, nullptr, barrier);
    EndSingleTimeCommands(std::move(commandBuffer));
}

void VulkanUtils::CopyBufferToImage(const vk::raii::Buffer &buffer, vk::raii::Image &image, uint32_t width,
    uint32_t height)
{
}

vk::raii::CommandBuffer VulkanUtils::BeginSingleTimeCommands()
{
    vk::CommandBufferAllocateInfo allocInfo{.commandPool = GraphicsServices::GetCommandPool(), .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = 1};
    vk::raii::CommandBuffer       commandBuffer = std::move(vk::raii::CommandBuffers(GraphicsServices::GetDevice(), allocInfo).front());

    vk::CommandBufferBeginInfo beginInfo{.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit};
    commandBuffer.begin(beginInfo);

    return std::move(commandBuffer);
}

void VulkanUtils::EndSingleTimeCommands(vk::raii::CommandBuffer &&commandBuffer)
{
    commandBuffer.end();

    vk::SubmitInfo submitInfo{.commandBufferCount = 1, .pCommandBuffers = &*commandBuffer};
    GraphicsServices::GetGraphicsQueue().submit(submitInfo, nullptr);
    GraphicsServices::GetGraphicsQueue().waitIdle();
}

uint32_t VulkanUtils::FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties)
{
    vk::PhysicalDeviceMemoryProperties memProperties = GraphicsServices::GetPhysicalDevice().getMemoryProperties();

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}
