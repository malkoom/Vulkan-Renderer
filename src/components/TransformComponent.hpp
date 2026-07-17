//
// Created by marc on 17/7/26.
//

#ifndef VULKANENGINE_TRANSFORMCOMPONENT_HPP
#define VULKANENGINE_TRANSFORMCOMPONENT_HPP

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "Component.hpp"

class TransformComponent : public Component {
private:
    glm::vec3 m_Position = glm::vec3(0.0f);
    glm::quat m_Rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // Identity quaternion
    glm::vec3 m_Scale = glm::vec3(1.0f);

    // Cached transformation matrix
    mutable glm::mat4 m_TransformMatrix = glm::mat4(1.0f);
    mutable bool m_TransformDirty = true;

public:
    void SetPosition(const glm::vec3& pos) {
        m_Position = pos;
        m_TransformDirty = true;
    }

    void SetRotation(const glm::quat& rot) {
        m_Rotation = rot;
        m_TransformDirty = true;
    }

    void SetScale(const glm::vec3& s) {
        m_Scale = s;
        m_TransformDirty = true;
    }

    const glm::vec3& GetPosition() const { return m_Position; }
    const glm::quat& GetRotation() const { return m_Rotation; }
    const glm::vec3& GetScale() const { return m_Scale; }

    glm::mat4 GetTransformMatrix() const {
        if (m_TransformDirty) {
            // Calculate transformation matrix
            glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), m_Position);
            glm::mat4 rotationMatrix = glm::mat4_cast(m_Rotation);
            glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), m_Scale);

            m_TransformMatrix = translationMatrix * rotationMatrix * scaleMatrix;
            m_TransformDirty = false;
        }
        return m_TransformMatrix;
    }
};

#endif //VULKANENGINE_TRANSFORMCOMPONENT_HPP
