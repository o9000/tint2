project( tint2 )
cmake_minimum_required( VERSION 2.8.5 )

option( ENABLE_BATTERY "Enable battery status plugin" ON )
option( ENABLE_TINT2CONF "Enable tint2conf build, a GTK+2 theme configurator for tint2" ON )
option( ENABLE_EXTRA_THEMES "Install additional tint2 themes" ON )
option( ENABLE_RSVG "Rsvg support (launcher only)" ON )
option( ENABLE_SN "Startup notification support" ON )
option( ENABLE_TRACING "Build tint2 with tracing instrumentation" OFF )
option( ENABLE_ASAN "Build tint2 with AddressSanitizer" OFF )
option( ENABLE_BACKTRACE "Dump a backtrace in case of fatal errors (e.g. X11 I/O error)" OFF )
option( ENABLE_BACKTRACE_ON_SIGNAL "Dump a backtrace also when receiving signals such as SIGSEGV" OFF )
if( CMAKE_SYSTEM_NAME STREQUAL "Linux" )
  option( ENABLE_UEVENT "Kernel event handling support" ON )
endif( CMAKE_SYSTEM_NAME STREQUAL "Linux" )

include( GNUInstallDirs )
if(NOT docdir)
  set(docdir ${CMAKE_INSTALL_DOCDIR})
endif()
if(NOT htmldir)
  set(htmldir ${docdir}/html)
endif()

include( FindPkgConfig )
include( CheckLibraryExists )
include( CheckCSourceCompiles )
pkg_check_modules( X11 REQUIRED x11 xcomposite xdamage xinerama xrender xrandr>=1.3 )
pkg_check_modules( PANGOCAIRO REQUIRED pangocairo )
pkg_check_modules( PANGO REQUIRED pango )
pkg_check_modules( CAIRO REQUIRED cairo )
pkg_check_modules( GLIB2 REQUIRED glib-2.0 )
pkg_check_modules( GOBJECT2 REQUIRED gobject-2.0 )
pkg_check_modules( IMLIB2 REQUIRED imlib2>=1.4.2 )

if(ENABLE_BACKTRACE)
	check_c_source_compiles(
		"#include <stdlib.h>\n#include <execinfo.h>\nint main () { backtrace(NULL, 0); }"
		BACKTRACE_LIBC)

	if(BACKTRACE_LIBC)
		set(BACKTRACE_LIBC_FOUND TRUE)
		set(BACKTRACE_L_FLAGS "-rdynamic")
	else()
		pkg_check_modules( UNWIND libunwind )
		find_library(EXECINFO_LIBRARIES NAMES execinfo)
		if(EXECINFO_LIBRARIES OR EXECINFO_LIBRARIES_FOUND)
			set(EXECINFO_FOUND TRUE)
			set(EXECINFO_LIBRARIES "-lexecinfo")
			set(BACKTRACE_L_FLAGS "-rdynamic")
		else()
			set(EXECINFO_LIBRARIES "")
			set(BACKTRACE_L_FLAGS "")
		endif()
	endif()

	if( NOT BACKTRACE_LIBC_FOUND AND NOT UNWIND_FOUND AND NOT EXECINFO_FOUND )
	  message( WARNING "Backtrace support not available. You can enable it by installing libexecinfo or libunwind." )
	endif()
else()
	set(EXECINFO_LIBRARIES "")
	set(BACKTRACE_L_FLAGS "")
endif()

if( ENABLE_RSVG )
pkg_check_modules( RSVG librsvg-2.0>=2.14.0 )
endif( ENABLE_RSVG )

if( ENABLE_SN )
pkg_check_modules( SN libstartup-notification-1.0>=0.12 )
endif(ENABLE_SN)

find_library( RT_LIBRARY rt )

if( NOT X11_FOUND OR NOT PANGOCAIRO_FOUND OR NOT PANGO_FOUND OR NOT CAIRO_FOUND OR NOT GLIB2_FOUND OR NOT GOBJECT2_FOUND OR NOT IMLIB2_FOUND )
  message( FATAL_ERROR "Not all dependencies fulfilled. See https://gitlab.com/o9000/tint2/wikis/Install" )
endif( NOT X11_FOUND OR NOT PANGOCAIRO_FOUND OR NOT PANGO_FOUND OR NOT CAIRO_FOUND OR NOT GLIB2_FOUND OR NOT GOBJECT2_FOUND OR NOT IMLIB2_FOUND )

string( REPLACE ";" " " FLAGS_REPLACED "${IMLIB2_LDFLAGS}" )
set( CMAKE_REQUIRED_FLAGS "${FLAGS_REPLACED}" )
check_library_exists( "${IMLIB2_LIBRARIES}" "imlib_context_set_display" "${IMLIB2_LIBRARY_DIRS}" IMLIB_BUILD_WITH_X )
if( NOT IMLIB_BUILD_WITH_X )
  message( FATAL_ERROR "Imlib is not built with X support" )
endif( NOT IMLIB_BUILD_WITH_X )


if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  add_definitions( -D_POSIX_C_SOURCE=200809L -D_BSD_SOURCE -D_DEFAULT_SOURCE )
else(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  add_definitions( -D_WITH_GETLINE )
endif(CMAKE_SYSTEM_NAME STREQUAL "Linux")


include_directories( ${PROJECT_BINARY_DIR}
                     src
                     src/battery
                     src/clock
                     src/systray
                     src/taskbar
                     src/launcher
                     src/tooltip
                     src/util
                     src/execplugin
                     src/button
                     src/freespace
                     src/separator
                     ${X11_INCLUDE_DIRS}
                     ${PANGOCAIRO_INCLUDE_DIRS}
                     ${PANGO_INCLUDE_DIRS}
                     ${CAIRO_INCLUDE_DIRS}
                     ${GLIB2_INCLUDE_DIRS}
                     ${GOBJECT2_INCLUDE_DIRS}
                     ${IMLIB2_INCLUDE_DIRS}
                     ${RSVG_INCLUDE_DIRS}
                     ${SN_INCLUDE_DIRS} )

set( SOURCES src/config.c
             src/panel.c
             src/server.c
             src/main.c
             src/init.c
             src/signals.c
             src/tracing.c
             src/mouse_actions.c
             src/drag_and_drop.c
             src/clock/clock.c
             src/systray/systraybar.c
             src/launcher/launcher.c
             src/launcher/apps-common.c
             src/launcher/icon-theme-common.c
             src/launcher/xsettings-client.c
             src/launcher/xsettings-common.c
             src/taskbar/task.c
             src/taskbar/taskbar.c
             src/taskbar/taskbarname.c
             src/tooltip/tooltip.c
             src/execplugin/execplugin.c
             src/button/button.c
             src/freespace/freespace.c
             src/separator/separator.c
             src/tint2rc.c
             src/util/area.c
             src/util/common.c
             src/util/fps_distribution.c
             src/util/strnatcmp.c
             src/util/timer.c
             src/util/cache.c
             src/util/color.c
             src/util/gradient.c
             src/util/window.c )

if( ENABLE_BATTERY )
  set( SOURCES ${SOURCES} src/battery/battery.c)

  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set( SOURCES ${SOURCES} src/battery/linux.c)
  elseif(CMAKE_SYSTEM_NAME STREQUAL "DragonFly")
    set( SOURCES ${SOURCES} src/battery/freebsd.c)
  elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    set( SOURCES ${SOURCES} src/battery/freebsd.c)
  elseif(CMAKE_SYSTEM_NAME STREQUAL "OpenBSD")
    set( SOURCES ${SOURCES} src/battery/openbsd.c)
  elseif(CMAKE_SYSTEM_NAME STREQUAL "NetBSD")
    set( SOURCES ${SOURCES} src/battery/openbsd.c)
  else(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set( SOURCES ${SOURCES} src/battery/dummy.c)
  endif(CMAKE_SYSTEM_NAME STREQUAL "Linux")

  add_definitions( -DENABLE_BATTERY )
endif( ENABLE_BATTERY )

if( ENABLE_RSVG )
  if( RSVG_FOUND )
	add_definitions( -DHAVE_RSVG )
  else()
    message( FATAL_ERROR "SVG support enabled yet dependency not fulfilled: librsvg-2.0" )
  endif( RSVG_FOUND )
endif( ENABLE_RSVG )

if( ENABLE_SN )
  if( SN_FOUND )
    add_definitions( -DHAVE_SN -DSN_API_NOT_YET_FROZEN )
  else()
    message( FATAL_ERROR "Startup notification support enabled yet dependency not fulfilled: libstartup-notification-1.0" )
  endif( SN_FOUND )
endif( ENABLE_SN)

if( ENABLE_UEVENT )
  add_definitions( -DENABLE_UEVENT )
  set( SOURCES ${SOURCES} src/util/uevent.c)
endif( ENABLE_UEVENT )

if(ENABLE_BACKTRACE)
	if(BACKTRACE_LIBC_FOUND)
	  add_definitions( -DENABLE_EXECINFO )
	endif()

	if( UNWIND_FOUND )
	  add_definitions( -DENABLE_LIBUNWIND )
	endif( UNWIND_FOUND )

	if( EXECINFO_FOUND )
	  add_definitions( -DENABLE_EXECINFO )
	endif( EXECINFO_FOUND )

	if(ENABLE_BACKTRACE_ON_SIGNAL)
		add_definitions( -DBACKTRACE_ON_SIGNAL )
	endif()
else()
	add_definitions( -DDISABLE_BACKTRACE )
endif()

if( ENABLE_TINT2CONF )
  add_definitions( -DHAVE_VERSION_H )
  add_subdirectory( src/tint2conf )
  add_dependencies( tint2conf version )
endif( ENABLE_TINT2CONF )

if( ENABLE_ASAN )
  SET(ASAN_C_FLAGS " -O0 -g3 -fsanitize=address -fno-common -fno-omit-frame-pointer -rdynamic -Wshadow")
  SET(ASAN_L_FLAGS " -O0 -g3 -fsanitize=address -fno-common -fno-omit-frame-pointer -rdynamic -fuse-ld=gold ")
else()
  SET(ASAN_C_FLAGS "")
  SET(ASAN_L_FLAGS "")
endif()

if( ENABLE_TRACING )
  add_definitions( -DHAVE_TRACING )
  SET(TRACING_C_FLAGS " -finstrument-functions -finstrument-functions-exclude-file-list=tracing.c -finstrument-functions-exclude-function-list=get_time,gettime -O0 -g3 -fno-common -fno-omit-frame-pointer -rdynamic")
  SET(TRACING_L_FLAGS " -O0 -g3 -fno-common -fno-omit-frame-pointer -rdynamic")
else()
  SET(TRACING_C_FLAGS "")
  SET(TRACING_L_FLAGS "")
endif()

add_custom_target( version ALL "${PROJECT_SOURCE_DIR}/get_version.sh" -- "\"${PROJECT_SOURCE_DIR}/\"" )

link_directories( ${X11_LIBRARY_DIRS}
                  ${PANGOCAIRO_LIBRARY_DIRS}
                  ${PANGO_LIBRARY_DIRS}
                  ${CAIRO_LIBRARY_DIRS}
                  ${GLIB2_LIBRARY_DIRS}
                  ${GOBJECT2_LIBRARY_DIRS}
                  ${IMLIB2_LIBRARY_DIRS}
                  ${RSVG_LIBRARY_DIRS}
                  ${SN_LIBRARY_DIRS} )
add_executable(tint2 ${SOURCES})
target_link_libraries( tint2 ${X11_LIBRARIES}
                             ${PANGOCAIRO_LIBRARIES}
                             ${PANGO_LIBRARIES}
                             ${CAIRO_LIBRARIES}
                             ${GLIB2_LIBRARIES}
                             ${GOBJECT2_LIBRARIES}
							 ${IMLIB2_LIBRARIES}
							 ${UNWIND_LIBRARIES}
							 ${EXECINFO_LIBRARIES} )
if( ENABLE_RSVG )
  target_link_libraries( tint2 ${RSVG_LIBRARIES} )
endif( ENABLE_RSVG )
if( ENABLE_SN )
  target_link_libraries( tint2 ${SN_LIBRARIES} )
endif( ENABLE_SN )
if( RT_LIBRARY )
  target_link_libraries( tint2 ${RT_LIBRARY} )
endif( RT_LIBRARY )

target_link_libraries( tint2 m )

add_dependencies( tint2 version )
set_target_properties( tint2 PROPERTIES COMPILE_FLAGS "-Wall -Wpointer-arith -fno-strict-aliasing -pthread -std=c99 ${ASAN_C_FLAGS} ${TRACING_C_FLAGS}" )
set_target_properties( tint2 PROPERTIES LINK_FLAGS "-pthread -fno-strict-aliasing ${ASAN_L_FLAGS} ${BACKTRACE_L_FLAGS}  ${TRACING_L_FLAGS}" )

install( TARGETS tint2 DESTINATION bin )
install( FILES tint2.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps )
install( FILES tint2.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications )
install( FILES themes/tint2rc DESTINATION /etc/xdg/tint2 )
install( FILES default_icon.png DESTINATION ${CMAKE_INSTALL_DATADIR}/tint2 )
install( FILES AUTHORS ChangeLog README.md doc/tint2.md DESTINATION ${docdir} )
install( FILES doc/manual.html doc/readme.html DESTINATION ${htmldir} )
install( DIRECTORY doc/images DESTINATION ${htmldir} )
install( FILES doc/tint2.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1 )
if( ENABLE_EXTRA_THEMES )
  add_subdirectory(themes)
endif( ENABLE_EXTRA_THEMES )
