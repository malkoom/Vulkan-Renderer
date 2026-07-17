//
// Created by marc on 17/7/26.
//

#ifndef VULKANENGINE_GRAPHICSSERVICES_HPP
#define VULKANENGINE_GRAPHICSSERVICES_HPP

// GraphicsServices.hpp
#pragma once
#include <vulkan/vulkan_raii.hpp>
#include <cassert>

class GraphicsServices {
private:
    inline static const vk::raii::PhysicalDevice* s_PhysicalDevice = nullptr;
    inline static const vk::raii::Device*         s_Device         = nullptr;
    inline static const vk::raii::CommandPool*    s_CommandPool    = nullptr;
    inline static const vk::raii::Queue*          s_GraphicsQueue  = nullptr;

public:
    // El Engine llamará a esto para registrar sus objetos al arrancar
    static void RegisterServices(
        const vk::raii::PhysicalDevice& physicalDevice,
        const vk::raii::Device&         device,
        const vk::raii::CommandPool&    commandPool,
        const vk::raii::Queue&          graphicsQueue)
    {
        s_PhysicalDevice = &physicalDevice;
        s_Device         = &device;
        s_CommandPool    = &commandPool;
        s_GraphicsQueue  = &graphicsQueue;
    }

    // Métodos globales de acceso rápido que devuelven referencias seguras
    static const vk::raii::PhysicalDevice& GetPhysicalDevice() { assert(s_PhysicalDevice); return *s_PhysicalDevice; }
    static const vk::raii::Device&         GetDevice()         { assert(s_Device);         return *s_Device; }
    static const vk::raii::CommandPool&    GetCommandPool()    { assert(s_CommandPool);    return *s_CommandPool; }
    static const vk::raii::Queue&          GetGraphicsQueue()  { assert(s_GraphicsQueue);  return *s_GraphicsQueue; }
};

#endif //VULKANENGINE_GRAPHICSSERVICES_HPP
