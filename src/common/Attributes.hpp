#pragma once

// GCC may warn that [[maybe_unused]] is ignored on non-static data members.
#if defined(__GNUC__) && !defined(__clang__)
#define JIANM_MAYBE_UNUSED __attribute__((unused))
#else
#define JIANM_MAYBE_UNUSED [[maybe_unused]]
#endif
