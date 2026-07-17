//
// Created by marc on 17/7/26.
//

#ifndef VULKANENGINE_RESOURCE_HPP
#define VULKANENGINE_RESOURCE_HPP

#include <string>

// Resource base class
class Resource {
private:
    std::string m_ResourceId;     // Unique identifier for this resource within the system
    bool m_Loaded = false;        // Loading state flag for resource lifecycle management

public:
    explicit Resource(const std::string& id) : m_ResourceId(id) {}
    virtual ~Resource() = default;

    // Core resource identity and state access methods
    const std::string& GetId() const { return m_ResourceId; }
    bool IsLoaded() const { return m_Loaded; }

    // Virtual interface for resource-specific loading and unloading behavior
    bool Load() {
        m_Loaded = doLoad();
        return m_Loaded;
    }

    void Unload() {
        doUnload();
        m_Loaded = false;
    }

protected:
    virtual bool doLoad() = 0;
    virtual bool doUnload() = 0;
};

#endif //VULKANENGINE_RESOURCE_HPP
