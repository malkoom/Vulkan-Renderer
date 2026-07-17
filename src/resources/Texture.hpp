//
// Created by marc on 17/7/26.
//

#ifndef VULKANENGINE_TEXTURE_HPP
#define VULKANENGINE_TEXTURE_HPP

#include "Resource.hpp"
#include <vulkan/vulkan_raii.hpp>

class Texture : public Resource {
private:
    // Core Vulkan GPU resources for texture representation
    vk::raii::Image m_Image = nullptr;              // GPU image object containing pixel data
    vk::raii::DeviceMemory m_Memory = nullptr;      // GPU memory allocation backing the image
    vk::DeviceSize m_Offset {};        // Offset within the memory allocation for this texture
    vk::raii::ImageView m_ImageView = nullptr;      // Shader-accessible view into the image
    vk::raii::Sampler m_Sampler = nullptr;          // Sampling configuration (filtering, wrapping, etc.)

    // Texture metadata for validation and debugging
    int m_Width = 0;                // Image width in pixels
    int m_Height = 0;               // Image height in pixels
    int m_Channels = 0;             // Number of color channels (RGB=3, RGBA=4, etc.)
    uint32_t m_MipLevels = 1;

private:
    unsigned char* LoadImageData(const std::string& filePath, int* width, int* height, int* channels);

    void CreateVulkanImage(unsigned char* pixels, int width, int height, int channels);
    void GenerateMipmaps(vk::raii::Image         &image,
                     vk::Format               imageFormat,
                     int32_t                  texWidth,
                     int32_t                  texHeight,
                     uint32_t                 mipLevels);

public:
    explicit Texture(const std::string& id) : Resource(id) {}

    ~Texture() override {
        Unload();                 // Ensure proper cleanup when object is destroyed
    }

protected:
    bool doLoad() override;
    bool doUnload() override;
};


#endif //VULKANENGINE_TEXTURE_HPP
