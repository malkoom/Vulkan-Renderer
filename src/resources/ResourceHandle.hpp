//
// Created by marc on 17/7/26.
//

#ifndef VULKANENGINE_RESOURCEHANDLE_HPP
#define VULKANENGINE_RESOURCEHANDLE_HPP

#include <string>
#include "ResourceManager.hpp"

// Resource handle
template<typename T>
class ResourceHandle {
private:
    std::string m_ResourceId;
    ResourceManager* m_ResourceManager;

public:
    ResourceHandle() : m_ResourceId(""), m_ResourceManager(nullptr) {}

    ResourceHandle(const std::string& id, ResourceManager* manager)
        : m_ResourceId(id), m_ResourceManager(manager) {}

    ResourceHandle(const ResourceHandle& other)
    {
        m_ResourceId = other.m_ResourceId;
        m_ResourceManager = other.m_ResourceManager;

        // Si el handle copiado es válido, le decimos al manager que hay un interesado más
        if (m_ResourceManager && !m_ResourceId.empty()) {
            // Llamamos a Load<T> pasando el ID. Como el manager ya lo tiene en caché,
            // simplemente incrementará el refCount interno y no volverá a cargar el archivo.
            m_ResourceManager->Load<T>(m_ResourceId);
        }
    }

    T* Get() const {
        if (!m_ResourceManager) return nullptr;
        return m_ResourceManager->GetResource<T>(m_ResourceId);
    }

    bool IsValid() const {
        return m_ResourceManager && m_ResourceManager->HasResource<T>(m_ResourceId);
    }

    const std::string& GetId() const {
        return m_ResourceId;
    }

    // Convenience operators
    T* operator->() const {
        return Get();
    }

    T& operator*() const {
        return *Get();
    }

    operator bool() const {
        return IsValid();
    }
};

#endif //VULKANENGINE_RESOURCEHANDLE_HPP
