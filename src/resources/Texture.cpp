//
// Created by marc on 17/7/26.
//

#include "Texture.hpp"
#include "../services/GraphicsServices.hpp"
#include "../VulkanUtils.hpp"

#include <stb/stb_image.h>
#include <cmath>

unsigned char* Texture::LoadImageData(const std::string &filePath, int *width, int *height, int *channels)
{
    stbi_uc      *pixels = stbi_load(filePath.c_str(), &m_Width, &m_Height, &m_Channels, STBI_rgb_alpha);

    m_MipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(m_Width, m_Height)))) + 1;

    if (!pixels)
    {
        throw std::runtime_error("failed to load texture image!");
    }

    return pixels;

}

void Texture::CreateVulkanImage(unsigned char *pixels, int width, int height, int channels)
{
    vk::DeviceSize imageSize = m_Width * m_Height * 4;
    vk::raii::Buffer       stagingBuffer       = nullptr;
    vk::raii::DeviceMemory stagingBufferMemory = nullptr;

    std::tie(stagingBuffer, stagingBufferMemory) = VulkanUtils::CreateBuffer(imageSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    void *data = stagingBufferMemory.mapMemory(0, imageSize);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    stagingBufferMemory.unmapMemory();

    stbi_image_free(data);

    // CORREGIDO: Usamos m_MipLevels en la creación de la imagen física
    std::tie(m_Image, m_Memory) = VulkanUtils::CreateImage(
        m_Width, m_Height, m_MipLevels, vk::SampleCountFlagBits::e1,
        vk::Format::eR8G8B8A8Srgb, vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    );

    // Ahora transitionImageLayout leerá m_MipLevels correctamente y abarcará toda la pirámide
    VulkanUtils::TransitionImageLayout(m_Image, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, m_MipLevels);

    VulkanUtils::CopyBufferToImage(stagingBuffer, m_Image, static_cast<uint32_t>(m_Width), static_cast<uint32_t>(m_Height));

    GenerateMipmaps(m_Image, vk::Format::eR8G8B8A8Srgb, m_Width, m_Height, m_MipLevels);

    // Implementation to create Vulkan image, allocate memory, and upload data
    // This involves complex Vulkan operations including:
    // - Format selection based on channel count and data type
    // - Memory allocation with appropriate usage flags
    // - Image creation with optimal tiling and layout
    // - Data upload via staging buffers for efficiency
    // - Image view creation for shader access
    // - Sampler creation with appropriate filtering settings
    // ...
}

void Texture::GenerateMipmaps(vk::raii::Image &image, vk::Format imageFormat, int32_t texWidth, int32_t texHeight,
    uint32_t mipLevels)
{
    auto commandBuffer = VulkanUtils::BeginSingleTimeCommands();

    vk::FormatProperties formatProperties = GraphicsServices::GetPhysicalDevice().getFormatProperties(imageFormat);

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

    VulkanUtils::EndSingleTimeCommands(std::move(commandBuffer));
}

bool Texture::doLoad()
{
    // Step 2a: Construct file path using resource ID and expected format
    std::string filePath = "textures/" + GetId() + ".png";

    // Step 2b: Load raw image data from disk with format detection
    unsigned char* data = LoadImageData(filePath, &m_Width, &m_Height, &m_Channels);
    if (!data) {
        return false;           // Failed to load - return failure without partial state
    }

    // Step 2c: Transform raw pixel data into Vulkan GPU resources
    CreateVulkanImage(data, m_Width, m_Height, m_Channels);

    return true;    // Mark resource as successfully loaded
}

bool Texture::doUnload()
{
    return true;
}
