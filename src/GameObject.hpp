//
// Created by marc on 16/7/26.
//

#ifndef VULKANENGINE_GAMEOBJECT_HPP
#define VULKANENGINE_GAMEOBJECT_HPP
#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

class GameObject {

public:
    glm::vec3 Position{};
    glm::vec3 Scale{};
    glm::vec3 Rotation{};

    // Uniform buffer for this object (one per frame in flight)
    std::vector<vk::raii::Buffer> UniformBuffers;
    std::vector<vk::raii::DeviceMemory> UniformBuffersMemory;
    std::vector<void*> UniformBuffersMapped;

    // Descriptor sets for this object (one per frame in flight)
    std::vector<vk::raii::DescriptorSet> DescriptorSets;


    // Calculate model matrix based on position, rotation, and scale
    glm::mat4 getModelMatrix() const {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, Position);
        model = glm::rotate(model, Rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, Rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, Rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, Scale);
        return model;
    }

};


#endif //VULKANENGINE_GAMEOBJECT_HPP
