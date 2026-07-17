//
// Created by marc on 17/7/26.
//

#ifndef VULKANENGINE_RESOURCEHANDLE_HPP
#define VULKANENGINE_RESOURCEHANDLE_HPP

#include <string>
#include "../ResourceManager.hpp"

// Resource handle
template<typename T>
class ResourceHandle {
private:
    std::string resourceId;
    ResourceManager* resourceManager;

public:
    ResourceHandle() : resourceManager(nullptr) {}

    ResourceHandle(const std::string& id, ResourceManager* manager)
        : resourceId(id), resourceManager(manager) {}

    T* Get() const {
        if (!resourceManager) return nullptr;
        return resourceManager->GetResource<T>(resourceId);
    }

    bool IsValid() const {
        return resourceManager && resourceManager->HasResource<T>(resourceId);
    }

    const std::string& GetId() const {
        return resourceId;
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
