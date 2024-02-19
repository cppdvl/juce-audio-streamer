include(FetchContent)

FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.5.2 # Adjust to the version you want
)
FetchContent_MakeAvailable(Catch2)

# Define test executable
add_executable(my_test
        BlockSizeAdapter.cpp
        RTPWrap.cpp
)

# Link test executable with Catch2
target_link_libraries(my_test PRIVATE Catch2::Catch2)

# Enable CTest and include Catch2's CMake integration
include(CTest)
include(Catch)
catch_discover_tests(my_test)
