//
// Created by marc on 17/7/26.
//

#ifndef VULKANENGINE_VULKANUTILS_HPP
#define VULKANENGINE_VULKANUTILS_HPP

#include <vulkan/vulkan_raii.hpp>

class VulkanUtils {
public:
    [[nodiscard]] static std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> CreateBuffer(
        vk::DeviceSize size,
        vk::BufferUsageFlags usage,
        vk::MemoryPropertyFlags properties);

    static std::pair<vk::raii::Image, vk::raii::DeviceMemory> CreateImage(
        uint32_t width,
        uint32_t height,
        uint32_t mipLevels,
        vk::SampleCountFlagBits numSamples,
        vk::Format format,
        vk::ImageTiling tiling,
        vk::ImageUsageFlags usage,
        vk::MemoryPropertyFlags properties);

    static vk::raii::ImageView CreateImageView(vk::Image const &image, uint32_t mipLevels, vk::Format format, vk::ImageAspectFlags aspectFlags = vk::ImageAspectFlagBits::eColor);

    static void TransitionImageLayout(const vk::raii::Image& image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t mipLevels);
    static void CopyBufferToImage(const vk::raii::Buffer& buffer, vk::raii::Image& image, uint32_t width, uint32_t height);

    static vk::raii::CommandBuffer BeginSingleTimeCommands();
    static void EndSingleTimeCommands(vk::raii::CommandBuffer&& commandBuffer);
    static uint32_t FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);

private:
};

#endif //VULKANENGINE_VULKANUTILS_HPP
