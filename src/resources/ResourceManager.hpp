//
// Created by marc on 17/7/26.
//

#ifndef VULKANENGINE_RESOURCEMANAGER_HPP
#define VULKANENGINE_RESOURCEMANAGER_HPP

#include <unordered_map>
#include <string>
#include <memory>
#include <typeindex>

#include "Resource.hpp"

template <typename  T>
class ResourceHandle;

// Resource manager
class ResourceManager {
private:
    // Estructura única para evitar duplicar mapas redundantes
    struct ResourceEntry {
        std::shared_ptr<Resource> resource;
        int refCount = 0;
    };

    // Un único mapa maestro de dos niveles: Tipo -> (ID -> Datos del Recurso)
    std::unordered_map<std::type_index, std::unordered_map<std::string, ResourceEntry>> m_Resources;

public:
    ~ResourceManager()
    {
        UnloadAll();
    }
    template<typename T>
    ResourceHandle<T> Load(const std::string& resourceId) {
        static_assert(std::is_base_of<Resource, T>::value, "T must derive from Resource");

        std::type_index typeIdx(typeid(T));
        auto& typeMap = m_Resources[typeIdx];
        auto it = typeMap.find(resourceId);

        // Si ya está en la caché
        if (it != typeMap.end()) {
            it->second.refCount++;
            return ResourceHandle<T>(resourceId, this);
        }

        // Si es nuevo, lo creamos
        auto resource = std::make_shared<T>(resourceId);
        if (!resource->Load()) {
            return ResourceHandle<T>(); // Invalid handle
        }

        // Guardamos en la caché única
        typeMap[resourceId] = ResourceEntry{ resource, 1 };

        return ResourceHandle<T>(resourceId, this);
    }

    template<typename T>
    T* GetResource(const std::string& resourceId) {
        std::type_index typeIdx(typeid(T));
        auto typeIt = m_Resources.find(typeIdx);

        if (typeIt != m_Resources.end()) {
            auto& typeMap = typeIt->second;
            auto it = typeMap.find(resourceId);
            if (it != typeMap.end()) {
                return static_cast<T*>(it->second.resource.get());
            }
        }
        return nullptr;
    }

    template<typename T>
    bool HasResource(const std::string& resourceId) const {
        std::type_index typeIdx(typeid(T));
        auto typeIt = m_Resources.find(typeIdx);

        if (typeIt == m_Resources.end()) {
            return false; // El tipo ni siquiera se ha registrado
        }

        const auto& typeMap = typeIt->second; // Acceso seguro por referencia constante
        return typeMap.find(resourceId) != typeMap.end();
    }

    // El Release ahora necesita saber el tipo para ser ultra rápido (O(1) en vez de buscar en bucles)
    template<typename T>
    void Release(const std::string& resourceId) {
        std::type_index typeIdx(typeid(T));
        auto typeIt = m_Resources.find(typeIdx);

        if (typeIt != m_Resources.end()) {
            auto& typeMap = typeIt->second;
            auto it = typeMap.find(resourceId);

            if (it != typeMap.end()) {
                it->second.refCount--;

                // Si ya nadie lo referencia, limpieza automática de VRAM/RAM
                if (it->second.refCount <= 0) {
                    it->second.resource->Unload();
                    typeMap.erase(it);
                }
            }
        }
    }

    void UnloadAll() {
        for (auto& [type, typeMap] : m_Resources) {
            for (auto& [id, entry] : typeMap) {
                if (entry.resource) {
                    entry.resource->Unload();
                }
            }
            typeMap.clear();
        }
        m_Resources.clear();
    }
};

#endif //VULKANENGINE_RESOURCEMANAGER_HPP