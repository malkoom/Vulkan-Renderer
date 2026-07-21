//
// Created by marc on 21/7/26.
//

#include "Swapchain.hpp"

#include "../services/GraphicsServices.hpp"
#include "../VulkanUtils.hpp"

vk::SurfaceFormatKHR Swapchain::chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &availableFormats)
{
    assert(!availableFormats.empty());
    const auto formatIt = std::ranges::find_if(
        availableFormats,
        [](const auto &format) { return format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear; });
    return formatIt != availableFormats.end() ? *formatIt : availableFormats[0];
}

vk::PresentModeKHR Swapchain::chooseSwapPresentMode(std::vector<vk::PresentModeKHR> const &availablePresentModes)
{
    assert(std::ranges::any_of(availablePresentModes, [](auto presentMode) { return presentMode == vk::PresentModeKHR::eFifo; }));
    return std::ranges::any_of(availablePresentModes,
                               [](const vk::PresentModeKHR value) { return vk::PresentModeKHR::eMailbox == value; }) ?
               vk::PresentModeKHR::eMailbox :
               vk::PresentModeKHR::eFifo;;
}

vk::Extent2D Swapchain::chooseSwapExtent(vk::SurfaceCapabilitiesKHR const &capabilities, GLFWwindow* window)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    return {
        std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
    };
}

uint32_t Swapchain::chooseSwapMinImageCount(vk::SurfaceCapabilitiesKHR const &surfaceCapabilities)
{
    auto minImageCount = std::max(3u, surfaceCapabilities.minImageCount);
    if ((0 < surfaceCapabilities.maxImageCount) && (surfaceCapabilities.maxImageCount < minImageCount))
    {
        minImageCount = surfaceCapabilities.maxImageCount;
    }
    return minImageCount;
}

void Swapchain::createSwapChain(vk::SurfaceKHR* surface, GLFWwindow* window)
{
    auto physicalDevice = GraphicsServices::GetPhysicalDevice();

    vk::SurfaceCapabilitiesKHR surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR( *surface );
    m_SwapChainExtent                                = chooseSwapExtent(surfaceCapabilities, window);
    uint32_t minImageCount                         = chooseSwapMinImageCount(surfaceCapabilities);

    std::vector<vk::SurfaceFormatKHR> availableFormats = physicalDevice.getSurfaceFormatsKHR(*surface);
    m_SwapChainSurfaceFormat                             = chooseSwapSurfaceFormat(availableFormats);

    std::vector<vk::PresentModeKHR> availablePresentModes = physicalDevice.getSurfacePresentModesKHR(*surface);

    vk::SwapchainCreateInfoKHR swapChainCreateInfo{
        .surface          = *surface,
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

    m_SwapChain       = vk::raii::SwapchainKHR( GraphicsServices::GetDevice(), swapChainCreateInfo );
    m_SwapChainImages = m_SwapChain.getImages();
}

void Swapchain::cleanupSwapChain()
{
    m_SwapChainImageViews.clear();
    m_SwapChain = nullptr;
}

void Swapchain::recreateSwapChain(vk::SurfaceKHR* surface, GLFWwindow* window)
{
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    GraphicsServices::GetDevice().waitIdle();

    cleanupSwapChain();
    createSwapChain(surface, window);
    createImageViews();
    //createColorResources();
    //createDepthResources();
}

void Swapchain::createImageViews()
{
    assert(m_SwapChainImageViews.empty());

    for (auto &image : m_SwapChainImages)
    {
        m_SwapChainImageViews.emplace_back(VulkanUtils::CreateImageView(image, 1, m_SwapChainSurfaceFormat.format));
    }
}
