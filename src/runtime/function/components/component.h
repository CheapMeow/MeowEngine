#pragma once

#include <memory>

namespace Meow
{
    class GameObject;

    class Component
    {
    public:
        virtual void Start() {};
        virtual void Tick(float dt) {};

    protected:
        std::weak_ptr<GameObject> m_parent_object;
    };
} // namespace Meow