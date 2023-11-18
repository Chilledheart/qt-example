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

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/Modules")
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

if (OS_WINDOWS)
  if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX
        "${CMAKE_BINARY_DIR}/rundir"
        CACHE STRING "Directory to install OBS after building" FORCE)
  endif()
endif()

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

# find_qt_plugins: Find and add Qt plugin libraries associated with Qt component to target
function(find_qt_plugins)
  set(oneValueArgs COMPONENT TARGET FOUND_VAR)
  cmake_parse_arguments(var "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  string(REPLACE "::" ";" library_tuple "${var_COMPONENT}")
  list(GET library_tuple 0 library_namespace)
  list(GET library_tuple 1 library_name)

  if(NOT ${library_namespace} MATCHES "Qt[56]?")
    message(FATAL_ERROR "'find_qt_plugins' has to be called with a valid target from the Qt, Qt5, or Qt6 namespace.")
  endif()

  list(
    APPEND
    qt_plugins_Core
    platforms
    printsupport
    styles
    imageformats
    iconengines)
  list(APPEND qt_plugins_Gui platforminputcontexts virtualkeyboard)
  list(APPEND qt_plugins_Network bearer)
  list(APPEND qt_plugins_Sql sqldrivers)
  list(APPEND qt_plugins_Multimedia mediaservice audio)
  list(APPEND qt_plugins_3dRender sceneparsers geometryloaders)
  list(APPEND qt_plugins_3dQuickRender renderplugins)
  list(APPEND qt_plugins_Positioning position)
  list(APPEND qt_plugins_Location geoservices)
  list(APPEND qt_plugins_TextToSpeech texttospeech)
  list(APPEND qt_plugins_WebView webview)

  if(qt_plugins_${library_name})
    get_target_property(library_location ${var_COMPONENT} IMPORTED_LOCATION)
    get_target_property(is_framework ${var_COMPONENT} FRAMEWORK)

    if(is_framework)
      # Resolve Qt plugin location relative to framework binary location on macOS
      set(plugins_location "../../../../../plugins")
      cmake_path(ABSOLUTE_PATH plugins_location BASE_DIRECTORY "${library_location}" NORMALIZE)
    else()
      # Resolve Qt plugin location relative to dynamic library location
      set(plugins_location "../../plugins")
      cmake_path(ABSOLUTE_PATH plugins_location BASE_DIRECTORY "${library_location}" NORMALIZE)
    endif()

    foreach(plugin IN ITEMS ${qt_plugins_${library_name}})
      if(NOT plugin IN_LIST plugins_list)
        if(EXISTS "${plugins_location}/${plugin}")
          # Gather all .dll or .dylib files in given plugin subdirectory
          file(
            GLOB plugin_libraries
            RELATIVE "${plugins_location}/${plugin}"
            "${plugins_location}/${plugin}/*.dylib" "${plugins_location}/${plugin}/*.dll")
          message(DEBUG "Found Qt plugin ${plugin} libraries: ${plugin_libraries}")
          foreach(plugin_library IN ITEMS ${plugin_libraries})
            set(plugin_full_path "${plugins_location}/${plugin}/${plugin_library}")
            list(APPEND plugins_list ${plugin_full_path})
          endforeach()
        endif()
      endif()
    endforeach()
  endif()

  set(${var_FOUND_VAR}
      ${plugins_list}
      PARENT_SCOPE)
endfunction()

find_qt(COMPONENTS Core Gui Widgets Network Svg Xml)
find_package(ZLIB)
find_package(CURL REQUIRED)

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

include(CopyMSVCBins)
add_executable(example
  WIN32
  example.cpp
  )

target_link_libraries(example PRIVATE Qt::Gui Qt::Widgets)

function(copy_directory src_dir dst_dir)
  file(GLOB additionalFiles RELATIVE ${src_dir} "${src_dir}/*")
  foreach(FILENAME ${additionalFiles})
    if(IS_DIRECTORY "${src_dir}/${FILENAME}")
      set(_cmake_command "copy_directory")
    else()
      set(_cmake_command "copy")
    endif()
    add_custom_command(
      TARGET example
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E ${_cmake_command} "${src_dir}/${FILENAME}" "${dst_dir}/${FILENAME}"
      COMMENT "Copy run ${FILENAME} to rundir"
      VERBATIM)
  endforeach()
endfunction()

if (MSVC)
  if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(ADDITIONAL_FILES_DIR ${CMAKE_SOURCE_DIR}/additional_install_files/exec64d)
  else()
    set(ADDITIONAL_FILES_DIR ${CMAKE_SOURCE_DIR}/additional_install_files/exec64r)
  endif()
  copy_directory(${CMAKE_SOURCE_DIR}/additional_install_files/exec64 ${CMAKE_BINARY_DIR})
  copy_directory(${ADDITIONAL_FILES_DIR} ${CMAKE_BINARY_DIR})
endif()
