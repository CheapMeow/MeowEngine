#pragma once

#if defined(_MSC_VER) && !defined(_WIN64)
#    define NON_DISPATCHABLE_HANDLE_TO_UINT64_CAST(type, x) static_cast<type>(x)
#else
#    define NON_DISPATCHABLE_HANDLE_TO_UINT64_CAST(type, x) reinterpret_cast<uint64_t>(static_cast<type>(x))
#endif