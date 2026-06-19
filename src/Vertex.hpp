//
// Created by marc on 19/6/26.
//

#ifndef VULKANTUTORIAL_VERTEX_HPP
#define VULKANTUTORIAL_VERTEX_HPP

struct Vertex {
    glm::vec2 Pos;
    glm::vec3 Color;
    glm::vec2 TexCoord;

    static vk::VertexInputBindingDescription getBindingDescription()
    {
        return {.binding = 0, .stride = sizeof(Vertex), .inputRate = vk::VertexInputRate::eVertex};
    }

    static std::array<vk::VertexInputAttributeDescription, 3> getAttributeDescriptions()
    {
        return {
			    {
			        {.location = 0, .binding = 0, .format = vk::Format::eR32G32Sfloat, .offset = offsetof(Vertex, Pos)},
                    {.location = 1, .binding = 0, .format = vk::Format::eR32G32B32Sfloat, .offset = offsetof(Vertex, Color)},
                        {.location = 2, .binding = 0, .format = vk::Format::eR32G32Sfloat, .offset = offsetof(Vertex, TexCoord)}}
        };
    }
};

struct UniformBufferObject {
    alignas(16) glm::mat4 Model;
    alignas(16) glm::mat4 View;
    alignas(16) glm::mat4 Proj;
};

#endif //VULKANTUTORIAL_VERTEX_HPP
