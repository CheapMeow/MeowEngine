#include "core/base/macro.h"
#include "core/base/non_copyable.h"

namespace Meow
{
    /**
     * @brief Engine entry.
     */
    class LIBRARY_API MeowEngine : NonCopyable
    {
    public:
        bool Init(); /**< Init engine */
        void Run();
        void ShutDown();
    };
} // namespace Meow