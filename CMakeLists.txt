cmake_minimum_required(VERSION 3.10)
project(twig VERSION 1.0.0 LANGUAGES C)

set(TWIG_SOURCES
    src/twig_core.c
    src/twig_mem_ion.c
    src/twig_mem_ve.c
)

set(TWIG_HEADERS
    include/twig.h
    include/twig_priv.h
    include/twig_regs.h
    include/allwinner/cedardev_api.h
    include/allwinner/ion.h
)

add_library(twig SHARED ${TWIG_SOURCES})
target_compile_options(twig PRIVATE -std=gnu99)

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
    ${CMAKE_CURRENT_SOURCE_DIR}/twig.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/twig.pc
    @ONLY
)

install(TARGETS twig
    LIBRARY DESTINATION lib
)

install(FILES 
    include/twig.h
    include/twig_regs.h
    DESTINATION include
)

install(FILES 
    ${CMAKE_CURRENT_BINARY_DIR}/twig.pc
    DESTINATION lib/pkgconfig
)