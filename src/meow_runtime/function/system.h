#pragma once

namespace Meow
{
    class System
    {
    public:
        virtual void Start() {};

        virtual void Tick(float dt) {};
    };
} // namespace Meow
