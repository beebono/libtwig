cmake_minimum_required(VERSION 3.10)
project(twig VERSION 1.8.0 LANGUAGES C)

set(TWIG_SOURCES
    src/twig_core.c
    src/mem/twig_ion.c
    src/buffer/twig_fbm.c
    src/buffer/twig_sbm.c
    src/vid/twig_video.c
    src/vid/h264/twig_h264.c
    src/vid/h264/twig_h264_dec.c
    src/vid/h264/twig_h264_parse.c
)

set(TWIG_HEADERS
    include/twig.h
    include/twig_video.h
    include/twig_regs.h
    include/priv/twig_priv.h
    include/priv/twig_mem_priv.h
    include/priv/twig_video_priv.h
    include/allwinner/cedardev_api.h
    include/allwinner/ion.h
)

add_library(twig SHARED ${TWIG_SOURCES})

target_include_directories(twig PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

set_target_properties(twig PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)

target_link_libraries(twig PRIVATE pthread)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/twig.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/twig.pc
    @ONLY
)

install(TARGETS twig
    LIBRARY DESTINATION lib
)

install(FILES 
    include/twig.h
    include/twig_video.h
    DESTINATION include
)

install(FILES 
    ${CMAKE_CURRENT_BINARY_DIR}/twig.pc
    DESTINATION lib/pkgconfig
)