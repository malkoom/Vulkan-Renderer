//
// Created by marc on 17/7/26.
//

#ifndef VULKANENGINE_ENTITY_HPP
#define VULKANENGINE_ENTITY_HPP

#include <vector>
#include <memory>
#include "Component.hpp"

class Entity {
private:
    std::string m_Name;
    bool m_Active = true;
    std::vector<std::unique_ptr<Component>> m_Components;

public:
    explicit Entity(const std::string& entityName) : m_Name(entityName) {}

    const std::string& GetName() const { return m_Name; }
    bool IsActive() const { return m_Active; }
    void SetActive(bool isActive) { m_Active = isActive; }

    void Initialize() {
        for (auto& component : m_Components) {
            component->Initialize();
        }
    }

    void Update(float deltaTime) {
        if (!m_Active) return;

        for (auto& component : m_Components) {
            component->Update(deltaTime);
        }
    }

    void Render() {
        if (!m_Active) return;

        for (auto& component : m_Components) {
            component->Render();
        }
    }

    template<typename T, typename... Args>
    T* AddComponent(Args&&... args) {
        static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");

        // Create new component
        auto component = std::make_unique<T>(std::forward<Args>(args)...);
        T* componentPtr = component.get();
        componentPtr->SetOwner(this);
        m_Components.push_back(std::move(component));
        return componentPtr;
    }

    template<typename T>
    T* GetComponent() {
        for (auto& component : m_Components) {
            if (T* result = dynamic_cast<T*>(component.get())) {
                return result;
            }
        }
        return nullptr;
    }

    template<typename T>
    bool RemoveComponent() {
        for (auto it = m_Components.begin(); it != m_Components.end(); ++it) {
            if (dynamic_cast<T*>(it->get())) {
                m_Components.erase(it);
                return true;
            }
        }
        return false;
    }
};


#endif //VULKANENGINE_ENTITY_HPP
