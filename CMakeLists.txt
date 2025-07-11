cmake_minimum_required(VERSION 3.10)
project(twig VERSION 0.0.2 LANGUAGES C)

set(TWIG_SOURCES
    src/twig.c
    src/twig_dec.c
    src/twig_frame.c
    src/twig_ion.c
)

set(TWIG_HEADERS
    include/twig.h
    include/twig_bits.h
    include/twig_dec.h
    include/twig_regs.h
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
    SOVERSION 0
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
    DESTINATION include
)

install(FILES 
    ${CMAKE_CURRENT_BINARY_DIR}/twig.pc
    DESTINATION lib/pkgconfig
)