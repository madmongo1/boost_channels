file(GLOB_RECURSE BOOST_CHANNELS_EXAMPLES_SRCS CONFIGURE_DEPENDS
        *.cpp
        )

foreach (src IN LISTS BOOST_CHANNELS_EXAMPLES_SRCS)
    get_filename_component(example_root ${src} NAME_WE)
    add_executable("${PROJECT_NAME}-example-${example_root}" ${src})
    target_link_libraries("${PROJECT_NAME}-example-${example_root}" PUBLIC Boost::channels Boost::boost Threads::Threads)
endforeach ()
