#
# Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
# Copyright (c) 2021 Richard Hodges (hodges.r@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#
# Official repository: https://github.com/madmongo1/boost_channels
#

cmake_minimum_required(VERSION 3.5...3.20)

set(BOOST_CHANNELS_VERSION ${BOOST_SUPERPROJECT_VERSION})

if (NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 20)
endif ()

project(boost_channels VERSION "${BOOST_CHANNELS_VERSION}" LANGUAGES CXX)

option(BOOST_CHANNELS_BUILD_TESTS "Build boost::channels tests" ${BUILD_TESTING})
option(BOOST_CHANNELS_BUILD_EXAMPLES "Build boost::channels examples" ${BOOST_CHANNELS_BUILD_TESTS})

find_package(Threads)

file(GLOB_RECURSE BOOST_CHANNELS_HEADERS CONFIGURE_DEPENDS
        include/boost/*.hpp
        include/boost/*.ipp
        include/boost/*.natvis
        )

set(BOOST_CHANNELS_SOURCES)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/include/boost PREFIX "" FILES ${BOOST_CHANNELS_HEADERS})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src PREFIX "" FILES ${BOOST_CHANNELS_SOURCES})

add_library(boost_channels INTERFACE)
add_library(Boost::channels ALIAS boost_channels)

target_include_directories(boost_channels INTERFACE include)

if (BOOST_SUPERPROJECT_VERSION)
    #
    # Building as part of Boost superproject tree, with Boost as dependency.
    #
    target_link_libraries(boost_channels
            INTERFACE
            Boost::asio
            Boost::assert
            Boost::config
            Boost::core
            Boost::static_assert
            Boost::throw_exception
            )

elseif (BOOST_CHANNELS_IN_BOOST_TREE)
    #
    # Building inside Boost tree, out of Boost superproject tree, with Boost as dependency.
    # e.g. on Travis or other CI, or when producing Visual Studio Solution and Projects.
    #
    get_filename_component(BOOST_ROOT ../.. ABSOLUTE)
    set(BOOST_INCLUDEDIR ${BOOST_ROOT})
    set(BOOST_LIBRARYDIR ${BOOST_ROOT}/stage/lib)
    find_package(Boost COMPONENTS serialization REQUIRED)

    target_include_directories(boost_channels INTERFACE ${BOOST_ROOT})
    target_link_directories(boost_channels INTERFACE ${BOOST_ROOT}/stage/lib)

else ()
    #
    # Building out of Boost tree, out of Boost superproject tree, with Boost as dependency.
    # e.g. for packaging or added with add_subdirectory.
    #
    find_package(Boost REQUIRED
            COMPONENTS system thread
            )
    target_link_libraries(boost_channels
            INTERFACE
            Boost::boost
            Boost::system
            Boost::thread
            )
endif ()

if (BOOST_CHANNELS_BUILD_TESTS)
    include(CTest)
    add_subdirectory(test)
endif ()

if (BOOST_CHANNELS_BUILD_EXAMPLES)
    if (BOOST_SUPERPROJECT_VERSION)
        message(STATUS "[channels] superproject build - skipping examples")
    else ()
        add_subdirectory(examples)
    endif ()
endif ()
