#include "core/base/macro.h"

/*! \file engine.h */

namespace Meow
{
    /** \class MeowEngine
     * A test class
     */
    class MeowEngine
    {
    public:
        LIBRARY_API bool init(); /**< Init engine */
        LIBRARY_API void run();
        LIBRARY_API void shutdown();
    };
} // namespace Meow