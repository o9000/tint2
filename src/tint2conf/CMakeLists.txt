project(tint2conf)
cmake_minimum_required(VERSION 2.6)

include( FindPkgConfig )
pkg_check_modules( X11 REQUIRED x11 xrender )
pkg_check_modules( GLIB2 REQUIRED glib-2.0 )
pkg_check_modules( GOBJECT2 REQUIRED gobject-2.0 )
pkg_check_modules( IMLIB2 REQUIRED imlib2 )
pkg_check_modules( GTHREAD2 REQUIRED gthread-2.0 )
pkg_check_modules( GTK2 REQUIRED gtk+-x11-2.0 )

include_directories( ../util
                     ${X11_INCLUDE_DIRS}
                     ${GLIB2_INCLUDE_DIRS}
                     ${GOBJECT2_INCLUDE_DIRS}
                     ${IMLIB2_INCLUDE_DIRS}
                     ${GTHREAD2_INCLUDE_DIRS}
                     ${GTK2_INCLUDE_DIRS} )

set(SOURCES ../util/common.c
            main.c
            theme_view.c )
add_executable( tint2conf ${SOURCES} )
target_link_libraries( tint2conf ${X11_LIBRARIES}
                                 ${GLIB2_LIBRARIES}
                                 ${GOBJECT2_LIBRARIES}
                                 ${IMLIB2_LIBRARIES}
                                 ${GTHREAD2_LIBRARIES}
                                 ${GTK2_LIBRARIES} )

install( TARGETS tint2conf DESTINATION bin )
