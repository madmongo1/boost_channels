include(FetchContent)
FetchContent_Declare(doctest
        GIT_REPOSITORY https://github.com/onqtam/doctest.git
        GIT_TAG 2.4.6)
FetchContent_MakeAvailable(doctest)

macro(MAKE_BOOST_CHANNELS_TEST)
    cmake_parse_arguments(mbct "" "NAME" "SRCS" ${ARGN})
    if ("${mbct_NAME}" STREQUAL "")
        message(FATAL_ERROR "MAKE_BOOST_CHANNELS_TEST: missing NAME")
    endif ()
    if ("${mbct_SRCS}" STREQUAL "")
        message(FATAL_ERROR "MAKE_BOOST_CHANNELS_TEST: missing SRCS")
    endif ()
    add_executable("${PROJECT_NAME}-${mbct_NAME}" ${mbct_SRCS})
    target_include_directories("${PROJECT_NAME}-${mbct_NAME}" PRIVATE .)
    target_link_libraries("${PROJECT_NAME}-${mbct_NAME}" PRIVATE Boost::channels doctest::doctest)
    add_test(NAME "${PROJECT_NAME}-${mbct_NAME}" COMMAND "${PROJECT_NAME}-${mbct_NAME}")
endmacro()

#
# Build a test containing a cpp file corresponding to each header file in the include directory
# Each cpp file is generated and simply includes the header file. Compiling this cpp file
# will check that each header file takes care of all its own include dependencies
#

file(GLOB_RECURSE includes $<$<VERSION_GREATER_EQUAL:${CMAKE_VERSION},3.12>:CONFIGURE_DEPENDS>
        RELATIVE "${PROJECT_SOURCE_DIR}/include"
        "${PROJECT_SOURCE_DIR}/include/*.hpp")
set(self_contained_srcs)
foreach (include IN LISTS includes)
    string(REGEX REPLACE "[\\\\/]" "__" src "${include}")
    string(REGEX REPLACE "\\.hpp$" ".cpp" src "${src}")
    set(src "${CMAKE_CURRENT_BINARY_DIR}/test_self_contained_${src}")
    configure_file(test_self_contained.cpp.in "${src}" @ONLY)
    list(APPEND self_contained_srcs "${src}")
endforeach ()
MAKE_BOOST_CHANNELS_TEST(NAME test_self_contained SRCS "${self_contained_srcs}" test_main.cpp)

#
# Build the individual file tests
#
file(GLOB_RECURSE boost_channels_test_srcs $<$<VERSION_GREATER_EQUAL:${CMAKE_VERSION},3.12>:CONFIGURE_DEPENDS>
        "*.cpp")
list(FILTER boost_channels_test_srcs EXCLUDE REGEX "^(.*)/test_main\\.cpp$")

foreach (src IN LISTS boost_channels_test_srcs)
    get_filename_component(root_name "${src}" NAME_WE)
    MAKE_BOOST_CHANNELS_TEST(NAME "${root_name}" SRCS "${src}" test_main.cpp)
endforeach ()