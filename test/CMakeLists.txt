# Jianm test suite — tests are added here as the project grows.

# Fetch GoogleTest at configure time
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)
# For Windows: prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# ---- Smoke test: the project links and runs ----
add_executable(jianm_smoke_test smoke_test.cpp)
target_link_libraries(jianm_smoke_test PRIVATE jianm_lib GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(jianm_smoke_test)
