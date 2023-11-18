cmake_minimum_required(VERSION 3.22...3.25)

# Prohibit in-source builds
if("${CMAKE_BINARY_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}")
  message(FATAL_ERROR "OBS: You cannot build in a source directory (or any directory with CMakeLists.txt file). "
                      "Please make a build subdirectory. Feel free to remove CMakeCache.txt and CMakeFiles.")
endif()

set(PACKAGE_NAME            "qt-example")

# Defer enabling C and CXX languages.
project(${PACKAGE_NAME} NONE)

## Export build compilation database if possible
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/common")
# Set OS-specific constants in non-deprecated way
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  set(CMAKE_C_EXTENSIONS FALSE)
  set(CMAKE_CXX_EXTENSIONS FALSE)
  set(OS_MACOS ON)
  set(OS_POSIX ON)
  list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/macos")
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux|FreeBSD|OpenBSD")
  set(CMAKE_CXX_EXTENSIONS FALSE)
  list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/linux")
  string(TOUPPER "${CMAKE_SYSTEM_NAME}" _SYSTEM_NAME_U)
  set(OS_${_SYSTEM_NAME_U} ON)
  set(OS_POSIX ON)
elseif(WIN32)
  set(CMAKE_C_EXTENSIONS FALSE)
  set(CMAKE_CXX_EXTENSIONS FALSE)
  list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/windows")
  set(OS_WINDOWS ON)
  set(OS_POSIX OFF)
endif()

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
enable_language(C)
enable_language(CXX)

include(buildspec)

# find_qt: Macro to find best possible Qt version for use with the project:
macro(find_qt)
  set(multiValueArgs COMPONENTS COMPONENTS_WIN COMPONENTS_MAC COMPONENTS_LINUX)
  cmake_parse_arguments(find_qt "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  # Do not use versionless targets in the first step to avoid Qt::Core being clobbered by later opportunistic
  # find_package runs
  set(QT_NO_CREATE_VERSIONLESS_TARGETS TRUE)

  message(DEBUG "Attempting to find Qt 6")
  find_package(
    Qt6
    COMPONENTS Core
    REQUIRED)

  # Enable versionless targets for the remaining Qt components
  set(QT_NO_CREATE_VERSIONLESS_TARGETS FALSE)

  set(qt_components ${find_qt_COMPONENTS})
  if(OS_WINDOWS)
    list(APPEND qt_components ${find_qt_COMPONENTS_WIN})
  elseif(OS_MACOS)
    list(APPEND qt_components ${find_qt_COMPONENTS_MAC})
  else()
    list(APPEND qt_components ${find_qt_COMPONENTS_LINUX})
  endif()
  message(DEBUG "Trying to find Qt components ${qt_components}...")

  find_package(Qt6 REQUIRED ${qt_components})

  list(APPEND qt_components Core)

  if("Gui" IN_LIST find_qt_COMPONENTS_LINUX)
    list(APPEND qt_components "GuiPrivate")
  endif()

  # Check for versionless targets of each requested component and create if necessary
  foreach(component IN LISTS qt_components)
    message(DEBUG "Checking for target Qt::${component}")
    if(NOT TARGET Qt::${component} AND TARGET Qt6::${component})
      add_library(Qt::${component} INTERFACE IMPORTED)
      set_target_properties(Qt::${component} PROPERTIES INTERFACE_LINK_LIBRARIES Qt6::${component})
    endif()
  endforeach()
endmacro()

find_qt(COMPONENTS Core Gui Widgets)

if (WIN32)
  add_definitions(-D_WIN32)
  add_definitions(-DWIN32)
  add_definitions(-D_WINDOWS)
  add_definitions(-D_ATL_NO_OPENGL)
  add_definitions(-DNOMINMAX)
  add_definitions(-DUNICODE)
  add_definitions(-DWIN32_LEAN_AND_MEAN)
  add_definitions(-D_UNICODE)
endif()

add_executable(example
  WIN32
  example.cpp
  )

target_link_libraries(example PRIVATE Qt::Gui Qt::Widgets)
