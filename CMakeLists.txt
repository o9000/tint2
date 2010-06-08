project(tint2)
cmake_minimum_required(VERSION 2.6)

include( FindPkgConfig )
pkg_check_modules( X11 REQUIRED x11 xcomposite xdamage xinerama xrender xrandr )
pkg_check_modules( PANGOCAIRO REQUIRED pangocairo )
pkg_check_modules( PANGO REQUIRED pango )
pkg_check_modules( CAIRO REQUIRED cairo )
pkg_check_modules( GLIB2 REQUIRED glib-2.0 )
pkg_check_modules( GOBJECT2 REQUIRED gobject-2.0 )
pkg_check_modules( IMLIB2 REQUIRED imlib2 )

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

set(SOURCES src/config.c
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

option( ENABLE_BATTERY "Enable battery support in tint2" OFF )
if ( ENABLE_BATTERY )
  set( SOURCES ${SOURCES} src/battery/battery.c )
  add_definitions( -DENABLE_BATTERY )
endif( ENABLE_BATTERY )

option( ENABLE_TINT2CONF "Enable tint2conf. A tint2 theme switcher" ON )
if ( ENABLE_TINT2CONF )
  add_subdirectory( src/tint2conf )
endif( ENABLE_TINT2CONF )


add_executable(tint2 ${SOURCES})
target_link_libraries( tint2 ${X11_LIBRARIES}
                             ${PANGOCAIRO_LIBRARIES}
                             ${PANGO_LIBRARIES}
                             ${CAIRO_LIBRARIES}
                             ${GLIB2_LIBRARIES}
                             ${GOBJECT2_LIBRARIES}
                             ${IMLIB2_LIBRARIES}
                             rt )

install(TARGETS tint2 DESTINATION bin)
