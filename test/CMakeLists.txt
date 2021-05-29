include(FetchContent)
FetchContent_Declare(doctest
        GIT_REPOSITORY https://github.com/onqtam/doctest.git
        GIT_TAG 2.4.6)
FetchContent_MakeAvailable(doctest)

macro(MAKE_BOOST_CHANNELS_TEST)
    cmake_parse_arguments(mbct "" "NAME" "SRCS" ${ARGN})
    if("${mbct_NAME}" STREQUAL "")
        message(FATAL_ERROR "MAKE_BOOST_CHANNELS_TEST: missing NAME")
    endif()
    if("${mbct_SRCS}" STREQUAL "")
        message(FATAL_ERROR "MAKE_BOOST_CHANNELS_TEST: missing SRCS")
    endif()
    message(STATUS "make_boost_channels_test (NAME ${PROJECT_NAME}-${mbct_NAME}) SRCS ${mbct_SRCS}")
    add_executable("${PROJECT_NAME}-${mbct_NAME}" ${mbct_SRCS})
    target_include_directories("${PROJECT_NAME}-${mbct_NAME}" PRIVATE .)
    target_link_libraries("${PROJECT_NAME}-${mbct_NAME}" PRIVATE Boost::channels doctest::doctest)
    add_test(NAME "${PROJECT_NAME}-${mbct_NAME}" COMMAND "${PROJECT_NAME}-${mbct_NAME}")
endmacro()

file(GLOB_RECURSE boost_channels_test_srcs $<$<VERSION_GREATER_EQUAL:${CMAKE_VERSION},3.12>:CONFIGURE_DEPENDS>
        "*.cpp")
message(STATUS "pre: ${boost_channels_test_srcs}")
list(FILTER boost_channels_test_srcs EXCLUDE REGEX "^(.*)/test_main\\.cpp$")
message(STATUS "post: ${boost_channels_test_srcs}")

foreach(src IN LISTS boost_channels_test_srcs)
    get_filename_component(root_name "${src}" NAME_WE)
    MAKE_BOOST_CHANNELS_TEST(NAME "${root_name}" SRCS "${src}" test_main.cpp)
endforeach()