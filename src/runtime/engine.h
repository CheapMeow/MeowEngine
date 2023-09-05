#include "core/base/macro.h"

/*! \file engine.h */

namespace Meow
{
    /** \class MeowEngine
     * A test class
     */
    class LIBRARY_API MeowEngine
    {
    public:
        bool init(); /**< Init engine */
        void run();
        void shutdown();
    };
} // namespace Meow