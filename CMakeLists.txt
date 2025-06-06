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
    PUBLIC_HEADER "include/twig.h"
)

target_link_libraries(twig PRIVATE pthread)

install(TARGETS twig
    LIBRARY DESTINATION lib
    PUBLIC_HEADER DESTINATION include
)

install(FILES 
    include/twig_regs.h
    DESTINATION include
    OPTIONAL
)