#include "core/base/macro.h"

namespace Meow
{
    class MeowEngine
    {
    public:
        LIBRARY_API bool init();
        LIBRARY_API void run();
        LIBRARY_API void shutdown();
    };
} // namespace Meow