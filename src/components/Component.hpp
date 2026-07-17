//
// Created by marc on 17/7/26.
//

#ifndef VULKANENGINE_COMPONENT_HPP
#define VULKANENGINE_COMPONENT_HPP

class Entity;

class Component {
protected:
    Entity* m_Owner = nullptr;

public:
    virtual ~Component() = default;

    virtual void Initialize() {}
    virtual void Update(float deltaTime) {}
    virtual void Render() {}

    void SetOwner(Entity* entity) { m_Owner = entity; }
    [[nodiscard]]Entity* GetOwner() const { return m_Owner; }
};



#endif //VULKANENGINE_COMPONENT_HPP
