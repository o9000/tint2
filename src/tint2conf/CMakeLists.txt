project(tint2conf)
cmake_minimum_required(VERSION 2.6)

include( FindPkgConfig )
pkg_check_modules( X11_T2C REQUIRED x11 xrender )
pkg_check_modules( GLIB2 REQUIRED glib-2.0 )
pkg_check_modules( GOBJECT2 REQUIRED gobject-2.0 )
pkg_check_modules( IMLIB2 REQUIRED imlib2 )
pkg_check_modules( GTHREAD2 REQUIRED gthread-2.0 )
pkg_check_modules( GTK2 REQUIRED gtk+-x11-2.0 )
pkg_check_modules( RSVG librsvg-2.0>=2.36.0 )

include_directories( ../util
                     ${X11_T2C_INCLUDE_DIRS}
                     ${GLIB2_INCLUDE_DIRS}
                     ${GOBJECT2_INCLUDE_DIRS}
                     ${IMLIB2_INCLUDE_DIRS}
                     ${GTHREAD2_INCLUDE_DIRS}
                     ${GTK2_INCLUDE_DIRS}
                     ${RSVG_INCLUDE_DIRS} )

set(SOURCES ../util/common.c
			../config.c
            ../server.c
            ../launcher/apps-common.c
            ../launcher/icon-theme-common.c
            main.c
            properties.c
            properties_rw.c
            theme_view.c )

add_definitions( -DTINT2CONF )

option( ENABLE_RSVG "Rsvg support (launcher only)" ON )

if( ENABLE_RSVG )
  if( RSVG_FOUND )
    add_definitions( -DHAVE_RSVG )
  endif( RSVG_FOUND )
endif( ENABLE_RSVG )

link_directories( ${X11_T2C_LIBRARY_DIRS}
                  ${GLIB2_LIBRARY_DIRS}
                  ${GOBJECT2_LIBRARY_DIRS}
                  ${IMLIB2_LIBRARY_DIRS}
                  ${GTHREAD2_LIBRARY_DIRS}
                  ${GTK2_LIBRARY_DIRS}
                  ${RSVG_LIBRARY_DIRS} )
add_executable( tint2conf ${SOURCES} )
target_link_libraries( tint2conf ${X11_T2C_LIBRARIES}
                                 ${GLIB2_LIBRARIES}
                                 ${GOBJECT2_LIBRARIES}
                                 ${IMLIB2_LIBRARIES}
                                 ${GTHREAD2_LIBRARIES}
                                 ${GTK2_LIBRARIES}
                                 ${RSVG_LIBRARIES} )

if ( NOT DATADIR )
  set( DATADIR share )
endif( NOT DATADIR )

add_definitions( -DINSTALL_PREFIX=\"${CMAKE_INSTALL_PREFIX}\" )
set_target_properties( tint2conf PROPERTIES COMPILE_FLAGS "-Wall -pthread" )
set_target_properties( tint2conf PROPERTIES LINK_FLAGS "-pthread" )

install( TARGETS tint2conf DESTINATION bin )
install( PROGRAMS tintwizard.py DESTINATION bin )
install( FILES taskbar.svg DESTINATION ${DATADIR}/icons/hicolor/scalable/apps )
install( FILES tint2conf.desktop DESTINATION ${DATADIR}/applications )
install( CODE "execute_process(COMMAND gtk-update-icon-cache -f -t ${DATADIR}/icons/hicolor WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX})" )
