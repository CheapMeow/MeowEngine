#pragma once

#include "function/system.h"

namespace Meow
{
    class ParticleSystem final : public System
    {
    public:
        ParticleSystem();
        ~ParticleSystem();

        void Start() override {}

        void Tick(float dt) override {}
    };
} // namespace Meow
