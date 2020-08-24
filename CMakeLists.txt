CMAKE_MINIMUM_REQUIRED(VERSION 3.1.0)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
# First, check is the compiler is new enough, most version of Clang are fine
# until problems arise, checking for GCC is enough
IF (CMAKE_COMPILER_IS_GNUCC)
   EXECUTE_PROCESS(COMMAND ${CMAKE_C_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
   IF (GCC_VERSION VERSION_GREATER 4.8 OR GCC_VERSION VERSION_EQUAL 4.8)
      MESSAGE(STATUS "Found GCC version >= 4.8: " ${GCC_VERSION})
   ELSE()
      MESSAGE(FATAL_ERROR "Your version of GCC is too old, please install GCC 4.8 or later")
   ENDIF()
ENDIF()

set (CMAKE_CXX_STANDARD 14)

# set project name and version
PROJECT(jamictl)

IF(EXISTS "${PROJECT_SOURCE_DIR}/version.txt")
   FILE (STRINGS "version.txt" PROJECT_VERSION)
ELSE()
   SET (PROJECT_VERSION "development")
ENDIF()

# set build date
IF( NOT ("$ENV{SOURCE_DATE_EPOCH}" STREQUAL ""))
  EXECUTE_PROCESS(
    COMMAND date -u -d "1970-01-01 UTC $ENV{SOURCE_DATE_EPOCH} seconds" "+%Y-%m-%d %H:%M:%S UTC"
    OUTPUT_VARIABLE CLIENT_BUILD_DATE
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )
ELSEIF( "${CLIENT_BUILD_DATE}" STREQUAL "")
  EXECUTE_PROCESS(
    COMMAND date -u "+%Y-%m-%d %H:%M:%S UTC"
    OUTPUT_VARIABLE CLIENT_BUILD_DATE
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )
ENDIF()

# add the binary tree to the search path for include files
# so that we will find config.h
INCLUDE_DIRECTORIES("${PROJECT_BINARY_DIR}")

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# make sure we're using Qt5
SET(ENABLE_QT5 true)

# for now, set debug as default
SET(CMAKE_BUILD_TYPE Debug)

# during packaging the CMAKE_INSTALL_PREFIX is used to specify where to install the files, but it
# may not be the same as where the package will put them, use JAMI_INSTALL_PREFIX to let the client
# know at runtime where to expect installed files to be located
SET(JAMI_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}
  CACHE PATH "prefix where the package will be installed on the user's system (eg: /usr/local); defaults to the CMAKE_INSTALL_PREFIX"
)

# Check if LRC's location is manually specified with -DLibRingClient_PROJECT_DIR
IF(LibRingClient_PROJECT_DIR)
   SET(LIB_RING_CLIENT_INCLUDE_DIR ${LibRingClient_PROJECT_DIR}/src)
   # TODO: maybe allow a way to specify a different build dir
   SET(LIB_RING_LIBRARY_INCLUDE_DIR ${LibRingClient_PROJECT_DIR}/build-local)

   # check for the existence of the directories;
   # if the dirs have missing or outdated files then we will just get build/link errors
   IF(EXISTS "${LIB_RING_CLIENT_INCLUDE_DIR}/")
      MESSAGE("will expect lrc headers in ${LIB_RING_CLIENT_INCLUDE_DIR}")
      IF(EXISTS "${LIB_RING_LIBRARY_INCLUDE_DIR}/")
         MESSAGE("will expect lrc libraries in " ${LIB_RING_LIBRARY_INCLUDE_DIR})
         SET(LibRingClient_FOUND true)
         SET(LIB_RING_CLIENT_LIBRARY ringclient)
      ELSE()
         MESSAGE( FATAL_ERROR "${LIB_RING_LIBRARY_INCLUDE_DIR} does not exist")
      ENDIF(EXISTS "${LIB_RING_LIBRARY_INCLUDE_DIR}/")
   ELSE()
      MESSAGE( FATAL_ERROR "${LIB_RING_CLIENT_INCLUDE_DIR} does not exist")
   ENDIF(EXISTS "${LIB_RING_CLIENT_INCLUDE_DIR}/")
ELSE(LibRingClient_PROJECT_DIR)
   FIND_PACKAGE(LibRingClient REQUIRED)
ENDIF(LibRingClient_PROJECT_DIR)

# find packages
FIND_PACKAGE(PkgConfig REQUIRED)
FIND_PACKAGE(Qt5Core REQUIRED)
FIND_PACKAGE(Readline 6 REQUIRED)
IF( LIBNOTIFY_FOUND )
    pkg_check_modules(CANBERRA REQUIRED libcanberra-gtk3>=0.25)
ENDIF()
FIND_PACKAGE(Gettext) #optional for translations
PKG_CHECK_MODULES(LIBNM libnm>=1.0) #optional to detect changes in the network

# include libs
INCLUDE_DIRECTORIES(${LIB_RING_CLIENT_INCLUDE_DIR})
INCLUDE_DIRECTORIES(SYSTEM ${Qt5Core_INCLUDE_DIRS} )
INCLUDE_DIRECTORIES(tabulate)

# link libs
LINK_DIRECTORIES(${LIB_RING_LIBRARY_INCLUDE_DIR})
LINK_DIRECTORIES(${Qt5Core_LIBRARY_DIRS} )
LINK_DIRECTORIES()

# lib compiler flags
ADD_DEFINITIONS(${LIB_RING_LIBRARY_CFLAGS})
ADD_DEFINITIONS(${Qt5Core_CFLAGS})
ADD_DEFINITIONS(${LIBNM_CFLAGS})

IF(NOT ${ENABLE_STATIC} MATCHES false)
  SET(QT5_MODULE_PATH ${QT5_PATH}/lib/cmake)
  FIND_PACKAGE(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED Core DBus)
  FIND_PACKAGE(Qt5DBus REQUIRED)
  INCLUDE_DIRECTORIES(SYSTEM ${Qt5DBus_INCLUDE_DIRS} )
  LINK_DIRECTORIES(${Qt5DBus_LIBRARY_DIRS} )
  ADD_DEFINITIONS(${Qt5DBus_CFLAGS})
ENDIF()

# warnings
ADD_DEFINITIONS(
   -Wall
   -Wextra
   -Wno-cpp
   -Wmissing-noreturn
   -Wpointer-arith
   -Wcast-align
   -Wwrite-strings
   -Wformat-nonliteral
   -Wformat-security
   -Wswitch-enum
   -Winit-self
   -Wmissing-include-dirs
   -Wundef
   -Wmissing-format-attribute
   -Wno-reorder
   -Wunused
   -Wuninitialized
   -Woverloaded-virtual
   -Wsuggest-attribute=const
   -Wunused-value
   -Wdouble-promotion
   -Wnonnull
   -Wsequence-point
   -Wunused-but-set-parameter
   -Wsizeof-pointer-memaccess
   -Wvarargs
)

# more warnings
IF (CMAKE_COMPILER_IS_GNUCC)
   IF (GCC_VERSION VERSION_GREATER 4.9 OR GCC_VERSION VERSION_EQUAL 4.9)
      ADD_DEFINITIONS(
         -Wconditionally-supported
         -Wdate-time
         -Wdelete-incomplete
         -Wfloat-conversion
      )
   ENDIF()

   IF (GCC_VERSION VERSION_GREATER 5.0 OR GCC_VERSION VERSION_EQUAL 5.0)
      ADD_DEFINITIONS(
         -Wsuggest-override
         -Wsuggest-final-types
         -Wsuggest-final-methods
         -Wbool-compare
         -Wformat-signedness
         -Wlogical-not-parentheses
         -Wnormalized
         -Wshift-count-negative
         -Wshift-count-overflow
         -Wsized-deallocation
         -Wsizeof-array-argument
      )
   ENDIF()

ENDIF()

# source files to compile
SET( SRC_FILES
   src/main.cpp
   src/jamictl.cpp
   src/dringctrl.cpp
   src/dringctrl.h
)



# IF(${ENABLE_TEST} MATCHES true)
#    ADD_SUBDIRECTORY( src/test)
# ENDIF()

# for translations
IF (GETTEXT_FOUND)

  # get all translation files; they're expected to be named using the following convention:
  # '<lang code>[_<country code].po'
  # eg: fr_CA.po (the country code is optional)
  FILE(GLOB PO_FILES ${CMAKE_SOURCE_DIR}/po/*.po)

  # Install each of the translation files
  FOREACH(PO_FILE IN LISTS PO_FILES)
    # extract the language (and optional country code) from the filename
    GET_FILENAME_COMPONENT(LANG ${PO_FILE} NAME_WE)
    GETTEXT_PROCESS_PO_FILES(${LANG} ALL PO_FILES ${PO_FILE})

    SET(TRANSLATIONS_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/share/locale/${LANG}/LC_MESSAGES)

    # install to share/locale and rename each to ring-client-gnome.mo
    INSTALL(
      FILES ${PROJECT_BINARY_DIR}/${LANG}.gmo
      DESTINATION ${TRANSLATIONS_INSTALL_DIR}
      RENAME ${PROJECT_NAME}.mo
    )
  ENDFOREACH(PO_FILE)

ENDIF(GETTEXT_FOUND)

ADD_EXECUTABLE(jamictl ${GLIB_RESOURCES_RING} ${GSCHEMA_RING} ${SRC_FILES})

IF(NOT ${ENABLE_STATIC} MATCHES false)
TARGET_LINK_LIBRARIES(jamictl
   ${LIB_RING_CLIENT_LIBRARY}
   ${Qt5Core_LIBRARIES}
   ${Qt5DBus_LIBRARIES}
   ${READLINE_LIBRARIES}
   -lpthread
   -lrt
   )
ELSE()
TARGET_LINK_LIBRARIES(jamictl
   ${LIB_RING_CLIENT_LIBRARY}
   ${READLINE_LIBRARIES}
   ${Qt5Core_LIBRARIES}
   )
ENDIF()

# configure libnm variable for config.h file
IF( LIBNM_FOUND )
   SET(USE_LIBNM 1)
ELSE()
   SET(USE_LIBNM 0)
ENDIF()

INSTALL(TARGETS jamictl
   RUNTIME DESTINATION bin
)

# install .desktop in XDG desktop dir so that it is recognized by the system
INSTALL(FILES ${PROJECT_SOURCE_DIR}/jamictl.desktop
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications
)

# install audio files
INSTALL(FILES ${PROJECT_SOURCE_DIR}/sounds/ringtone_notify.wav
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/sounds/jamictl
)

# install .desktop in the jamictl data dir, so that it can be copied to the
# autostart dir by the client
INSTALL(FILES ${PROJECT_SOURCE_DIR}/jamictl.desktop.autostart
   DESTINATION
      ${CMAKE_INSTALL_PREFIX}/share/jamictl
   PERMISSIONS
      WORLD_READ
      OWNER_WRITE
      OWNER_READ
      GROUP_READ
   RENAME
      jamictl.desktop
)

INSTALL(FILES pixmaps/jami.svg
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/scalable/apps
)

INSTALL(FILES jamictl.appdata.xml
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/metainfo/
)

# install shell script which will launch Jami
INSTALL(
   FILES
      "${PROJECT_SOURCE_DIR}/src/jami"
   DESTINATION
      ${CMAKE_INSTALL_PREFIX}/bin
   PERMISSIONS
      WORLD_READ
      OWNER_WRITE
      OWNER_READ
      GROUP_READ
      OWNER_EXECUTE
      GROUP_EXECUTE
      WORLD_EXECUTE
)

INSTALL(CODE "
    EXECUTE_PROCESS(COMMAND rm ${CMAKE_INSTALL_PREFIX}/bin/ring.cx -f)
    EXECUTE_PROCESS(COMMAND ln -s ${CMAKE_INSTALL_PREFIX}/bin/jami ${CMAKE_INSTALL_PREFIX}/bin/ring.cx)
    ")
