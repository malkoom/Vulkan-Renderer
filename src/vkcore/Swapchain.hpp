//
// Created by marc on 21/7/26.
//

#ifndef VULKANENGINE_SWAPCHAIN_HPP
#define VULKANENGINE_SWAPCHAIN_HPP

#include <GLFW/glfw3.h>
#include <vulkan/vulkan_raii.hpp>

class Swapchain {
private:
    vk::raii::SwapchainKHR m_SwapChain = nullptr;
    std::vector<vk::Image> m_SwapChainImages;
    std::vector<vk::raii::ImageView> m_SwapChainImageViews;
    vk::SurfaceFormatKHR  m_SwapChainSurfaceFormat;
    vk::Extent2D m_SwapChainExtent;

private:
    vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
    vk::PresentModeKHR chooseSwapPresentMode(std::vector<vk::PresentModeKHR> const &availablePresentModes);
    vk::Extent2D chooseSwapExtent(vk::SurfaceCapabilitiesKHR const &capabilities, GLFWwindow* window);
    uint32_t chooseSwapMinImageCount(vk::SurfaceCapabilitiesKHR const &surfaceCapabilities);
    void cleanupSwapChain();
    void createImageViews();

public:
    // Gestión de Swapchain
    void createSwapChain(vk::SurfaceKHR* surface, GLFWwindow* window);
    void recreateSwapChain(vk::SurfaceKHR* surface, GLFWwindow* window);
};


#endif //VULKANENGINE_SWAPCHAIN_HPP
