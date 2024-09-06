#pragma once

#include "function/window/window.h"

namespace Meow
{
    class RuntimeWindow : public Window
    {
    public:
        RuntimeWindow(std::size_t id);
        ~RuntimeWindow() override;

        void Tick(float dt) override;
    };
} // namespace Meow
