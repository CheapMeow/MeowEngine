#pragma once

namespace Meow
{
    class System
    {
    public:
        /**
         * @brief Start is called behind of constructor function,
         * to solve the initialization dependency problem between systems.
         */
        virtual void Start() {}

        /**
         * @brief Tick is called every frame.
         */
        virtual void Tick(float dt) {}

        /**
         * @brief Shutdown is called before the destructor function,
         * to solve the destruction dependency problem between systems.
         */
        virtual void Shutdown() {}
    };
} // namespace Meow
