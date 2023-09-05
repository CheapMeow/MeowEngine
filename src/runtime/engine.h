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
        bool Init(); /**< Init engine */
        void Run();
        void ShutDown();
    };
} // namespace Meow