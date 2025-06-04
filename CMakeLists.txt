cmake_minimum_required(VERSION 3.10)
project(libtwig VERSION 1.0.0 LANGUAGES C)

include_directories(include)

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

add_library(twig SHARED ${TWIG_SOURCES} ${TWIG_HEADERS})

set_target_properties(twig PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER "include/twig.h"
)

target_link_libraries(twig pthread)

install(TARGETS twig twig_static
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    PUBLIC_HEADER DESTINATION include
)

install(FILES 
    include/twig_regs.h
    DESTINATION include
    OPTIONAL
)