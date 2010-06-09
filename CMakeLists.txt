project( tint2 )
cmake_minimum_required( VERSION 2.6 )

include( FindPkgConfig )
include( CheckFunctionExists )
pkg_check_modules( X11 REQUIRED x11 xcomposite xdamage xinerama xrender xrandr )
pkg_check_modules( PANGOCAIRO REQUIRED pangocairo )
pkg_check_modules( PANGO REQUIRED pango )
pkg_check_modules( CAIRO REQUIRED cairo )
pkg_check_modules( GLIB2 REQUIRED glib-2.0 )
pkg_check_modules( GOBJECT2 REQUIRED gobject-2.0 )
pkg_check_modules( IMLIB2 REQUIRED imlib2 )

set( CMAKE_REQUIRED_FLAGS ${IMLIB2_LDFLAGS} ${IMLIB2_CFLAGS} )
check_function_exists( imlib_context_set_display IMLIB_BUILD_WITH_X )
if( NOT IMLIB_BUILD_WITH_X )
  message( FATAL_ERROR "Imlib is not build with x support" )
endif( NOT IMLIB_BUILD_WITH_X )

include_directories( src
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

option( DISABLE_BATTERY "Disable battery status plugin" OFF )
option( DISABLE_TINT2CONF "Disable tint2conf build, a GTK+2 theme switcher for tint2" OFF )
option( ENABLE_EXAMPLES "Install additional tin2rc examples" OFF )

if( NOT DISABLE_BATTERY )
  set( SOURCES ${SOURCES} src/battery/battery.c )
  add_definitions( -DENABLE_BATTERY )
endif( NOT DISABLE_BATTERY )

if( NOT DISABLE_TINT2CONF )
  add_subdirectory( src/tint2conf )
endif( NOT DISABLE_TINT2CONF )


set( MANDIR ${CMAKE_INSTALL_PREFIX}/share/man CACHE PATH "Directory for man pages" )
set( DATADIR ${CMAKE_INSTALL_PREFIX}/share CACHE PATH "Directory for shared data" )
set( SYSCONFDIR /etc CACHE PATH "Directory for configuration files" )
set( DOCDIR ${CMAKE_INSTALL_PREFIX}/share/doc/tint2 CACHE PATH "Directory for documentation files" )

add_executable(tint2 ${SOURCES})
link_directories( ${X11_LIBRARY_DIRS}
                  ${X11_LIBRARY_DIRS}
                  ${PANGOCAIRO_LIBRARY_DIRS}
                  ${PANGO_LIBRARY_DIRS}
                  ${CAIRO_LIBRARY_DIRS}
                  ${GLIB2_LIBRARY_DIRS}
                  ${GOBJECT2_LIBRARY_DIRS}
                  ${IMLIB2_LIBRARY_DIRS} )
target_link_libraries( tint2 ${X11_LIBRARIES}
                             ${PANGOCAIRO_LIBRARIES}
                             ${PANGO_LIBRARIES}
                             ${CAIRO_LIBRARIES}
                             ${GLIB2_LIBRARIES}
                             ${GOBJECT2_LIBRARIES}
                             ${IMLIB2_LIBRARIES}
                             rt )

set_target_properties(tint2 PROPERTIES COMPILE_FLAGS -Wall)
set_target_properties(tint2 PROPERTIES LINK_FLAGS -Wl,--as-needed)

install( TARGETS tint2 DESTINATION bin )
install( FILES sample/tint2rc DESTINATION ${SYSCONFDIR}/xdg/tint2 )
install( FILES AUTHORS ChangeLog README DESTINATION ${DOCDIR} )
if( ENABLE_EXAMPLES )
  file( GLOB SAMPLEFILES sample/*.tint2rc )
  install( FILES ${SAMPLEFILES} DESTINATION ${DOCDIR}/examples )
endif( ENABLE_EXAMPLES )
