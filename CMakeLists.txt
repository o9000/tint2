project( tint2 )
cmake_minimum_required( VERSION 2.6 )

include( FindPkgConfig )
include( CheckLibraryExists )
pkg_check_modules( X11 REQUIRED x11 xcomposite xdamage xinerama xrender xrandr )
pkg_check_modules( PANGOCAIRO REQUIRED pangocairo )
pkg_check_modules( PANGO REQUIRED pango )
pkg_check_modules( CAIRO REQUIRED cairo )
pkg_check_modules( GLIB2 REQUIRED glib-2.0 )
pkg_check_modules( GOBJECT2 REQUIRED gobject-2.0 )
pkg_check_modules( IMLIB2 REQUIRED imlib2 )
find_library( RT_LIBRARY rt )

string( REPLACE ";" " " FLAGS_REPLACED "${IMLIB2_LDFLAGS}" )
set( CMAKE_REQUIRED_FLAGS "${FLAGS_REPLACED}" )
check_library_exists( "${IMLIB2_LIBRARIES}" "imlib_context_set_display" "${IMLIB2_LIBRARY_DIRS}" IMLIB_BUILD_WITH_X )
if( NOT IMLIB_BUILD_WITH_X )
  message( FATAL_ERROR "Imlib is not build with x support" )
endif( NOT IMLIB_BUILD_WITH_X )

include_directories( ${PROJECT_BINARY_DIR}
                     src
                     src/battery
                     src/clock
                     src/systray
                     src/taskbar
                     src/tooltip
                     src/util
                     ${X11_INCLUDE_DIRS}
                     ${PANGOCAIRO_INCLUDE_DIRS}
                     ${PANGO_INCLUDE_DIRS}
                     ${CAIRO_INCLUDE_DIRS}
                     ${GLIB2_INCLUDE_DIRS}
                     ${GOBJECT2_INCLUDE_DIRS}
                     ${IMLIB2_INCLUDE_DIRS} )

set( SOURCES src/config.c
             src/panel.c
             src/server.c
             src/tint.c
             src/clock/clock.c
             src/systray/systraybar.c
             src/taskbar/task.c
             src/taskbar/taskbar.c
             src/tooltip/tooltip.c
             src/util/area.c
             src/util/common.c
             src/util/timer.c
             src/util/window.c )

option( ENABLE_BATTERY "Enable battery status plugin" ON )
option( ENABLE_TINT2CONF "Enable tint2conf build, a GTK+2 theme switcher for tint2" ON )
option( ENABLE_EXAMPLES "Install additional tin2rc examples" OFF )

if( ENABLE_BATTERY )
  set( SOURCES ${SOURCES} src/battery/battery.c )
  add_definitions( -DENABLE_BATTERY )
endif( ENABLE_BATTERY )

if( ENABLE_TINT2CONF )
  add_definitions( -DHAVE_VERSION_H )
  add_subdirectory( src/tint2conf )
  add_dependencies( tint2conf version )
endif( ENABLE_TINT2CONF )

set( MANDIR share/man CACHE PATH "Directory for man pages" )
set( DATADIR share CACHE PATH "Directory for shared data" )
set( SYSCONFDIR /etc CACHE PATH "Directory for configuration files" )
set( DOCDIR share/doc/tint2 CACHE PATH "Directory for documentation files" )

add_custom_target( version ALL "${PROJECT_SOURCE_DIR}/get_svnrev.sh" "\"${PROJECT_SOURCE_DIR}\"" )

link_directories( ${X11_LIBRARY_DIRS}
                  ${PANGOCAIRO_LIBRARY_DIRS}
                  ${PANGO_LIBRARY_DIRS}
                  ${CAIRO_LIBRARY_DIRS}
                  ${GLIB2_LIBRARY_DIRS}
                  ${GOBJECT2_LIBRARY_DIRS}
                  ${IMLIB2_LIBRARY_DIRS} )
add_executable(tint2 ${SOURCES})
target_link_libraries( tint2 ${X11_LIBRARIES}
                             ${PANGOCAIRO_LIBRARIES}
                             ${PANGO_LIBRARIES}
                             ${CAIRO_LIBRARIES}
                             ${GLIB2_LIBRARIES}
                             ${GOBJECT2_LIBRARIES}
                             ${IMLIB2_LIBRARIES} )
if( RT_LIBRARY )
  target_link_libraries( tint2 ${RT_LIBRARY} )
endif( RT_LIBRARY )

add_dependencies( tint2 version )
set_target_properties( tint2 PROPERTIES COMPILE_FLAGS "-Wall -pthread" )
set_target_properties(tint2 PROPERTIES LINK_FLAGS "-pthread" )

install( TARGETS tint2 DESTINATION bin )
install( FILES sample/tint2rc DESTINATION ${SYSCONFDIR}/xdg/tint2 )
install( FILES default_icon.png DESTINATION ${DATADIR}/tint2 )
install( FILES AUTHORS ChangeLog README DESTINATION ${DOCDIR} )
install( FILES doc/tint2.1 DESTINATION ${MANDIR}/man1 )
if( ENABLE_EXAMPLES )
  file( GLOB SAMPLEFILES sample/*.tint2rc )
  install( FILES ${SAMPLEFILES} DESTINATION ${DOCDIR}/examples )
endif( ENABLE_EXAMPLES )
