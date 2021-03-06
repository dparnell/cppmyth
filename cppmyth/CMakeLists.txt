cmake_minimum_required (VERSION 2.6 FATAL_ERROR)
cmake_policy (VERSION 2.6)
set(CMAKE_LEGACY_CYGWIN_WIN32 0) # Remove when CMake >= 2.8.4 is required

project (cppmyth)

set (CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
#include (GNUInstallDirs)

# Options
OPTION (BUILD_SHARED_LIBS "Build shared libraries." OFF)
if (MSVC)
  # This option must match the settings used in your program, in particular if you
  # are linking statically
  OPTION (STATIC_CRT "Link the static CRT libraries" OFF)
endif ()

if (NOT WIN32)
  OPTION (REQUIRE_CXX_98 "Require standard c++98" OFF)
endif ()

###############################################################################
# set lib version here
set (CPPMYTH_LIB_VERSION "2.7.0")
set (CPPMYTH_LIB_SOVERSION "2")

###############################################################################
# add definitions
if (MSVC)
  add_definitions ("/D_USE_32BIT_TIME_T /D_CRT_SECURE_NO_WARNINGS")
  if (STATIC_CRT)
    set (CMAKE_C_FLAGS_RELEASE "/MT")
    set (CMAKE_C_FLAGS_DEBUG "/MTd")
    set (CMAKE_CXX_FLAGS_RELEASE "/MT")
    set (CMAKE_CXX_FLAGS_DEBUG "/MTd")
  endif ()
  set (CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /W3")
  set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /W3")
  set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /W3 /Od /RTC1 /EHsc /nologo")
  set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /W3 /Od /RTC1 /EHsc /nologo")
endif ()

if (NOT WIN32 AND (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX))
  if (NOT CYGWIN)
    add_definitions ("-fPIC")
  endif ()
  add_definitions ("-Wall")
  include (CheckLibraryExists)
  include (CheckAtomic)
  if (HAS_BUILTIN_SYNC_ADD_AND_FETCH)
    add_definitions ("-DHAS_BUILTIN_SYNC_ADD_AND_FETCH")
  endif ()
  if (HAS_BUILTIN_SYNC_SUB_AND_FETCH)
    add_definitions ("-DHAS_BUILTIN_SYNC_SUB_AND_FETCH")
  endif ()
endif ()

###############################################################################
# configure
include (CheckFunctionExists)
include (CheckFunctionKeywords)
find_package (Threads REQUIRED)

check_function_exists (timegm CHK_TIMEGM)
if (CHK_TIMEGM)
    set (HAVE_TIMEGM 1)
else ()
    set (HAVE_TIMEGM 0)
endif ()

check_function_exists (localtime_r CHK_LOCALTIME_R)
if (CHK_LOCALTIME_R)
    set (HAVE_LOCALTIME_R 1)
else ()
    set (HAVE_LOCALTIME_R 0)
endif ()

check_function_exists (gmtime_r CHK_GMTIME_R)
if (CHK_GMTIME_R)
    set (HAVE_GMTIME_R 1)
else ()
    set (HAVE_GMTIME_R 0)
endif ()

# Check what the inline keyword is.
check_function_keywords ("inline")
check_function_keywords ("__inline")
check_function_keywords ("__inline__")
if (HAVE_INLINE)
   set (CC_INLINE inline)
elseif (HAVE___INLINE)
   set (CC_INLINE __inline)
elseif (HAVE___INLINE__)
   set (CC_INLINE __inline__)
else ()
   # no inline on this platform
   set (CC_INLINE)
endif ()

if (REQUIRE_CXX_98)
  set (CXX_STANDARD "199711L")
  message(STATUS "c++98 activated")
else ()
  if (MSVC)
    set (CXX_STANDARD "201103L")
  else ()
    include(CheckCXXCompilerFlag)
    CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
    if(COMPILER_SUPPORTS_CXX11)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
      set (CXX_STANDARD "201103L")
    else()
      set (CXX_STANDARD "199711L")
    endif ()
  endif ()
endif ()

# configure the public config file
configure_file (${CMAKE_CURRENT_SOURCE_DIR}/src/cppmyth_config.h.in
                 ${CMAKE_CURRENT_BINARY_DIR}/include/cppmyth_config.h)

# Copy the header files to the public include folder

file (COPY ${CMAKE_CURRENT_SOURCE_DIR}/src/mythlocked.h
           DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/include/)
file (COPY ${CMAKE_CURRENT_SOURCE_DIR}/src/mythintrinsic.h
           DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/include/)
file (COPY ${CMAKE_CURRENT_SOURCE_DIR}/src/mythsharedptr.h
           DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/include/)
file (COPY ${CMAKE_CURRENT_SOURCE_DIR}/src/mythtypes.h
           DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/include/)
file (COPY ${CMAKE_CURRENT_SOURCE_DIR}/src/mythcontrol.h
           DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/include/)
file (COPY ${CMAKE_CURRENT_SOURCE_DIR}/src/mytheventhandler.h
           DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/include/)
file (COPY ${CMAKE_CURRENT_SOURCE_DIR}/src/mythfileplayback.h
           DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/include/)
file (COPY ${CMAKE_CURRENT_SOURCE_DIR}/src/mythlivetvplayback.h
           DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/include/)
file (COPY ${CMAKE_CURRENT_SOURCE_DIR}/src/mythrecordingplayback.h
           DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/include/)
file (COPY ${CMAKE_CURRENT_SOURCE_DIR}/src/mythwsapi.h
           DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/include/)
file (COPY ${CMAKE_CURRENT_SOURCE_DIR}/src/mythdebug.h
           DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/include/)
file (COPY ${CMAKE_CURRENT_SOURCE_DIR}/src/mythstream.h
           DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/include/)
file (COPY ${CMAKE_CURRENT_SOURCE_DIR}/src/mythwsstream.h
           DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/include/)

file (COPY ${CMAKE_CURRENT_SOURCE_DIR}/src/proto/mythprotobase.h
           DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/include/proto/)
file (COPY ${CMAKE_CURRENT_SOURCE_DIR}/src/proto/mythprotomonitor.h
           DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/include/proto/)
file (COPY ${CMAKE_CURRENT_SOURCE_DIR}/src/proto/mythprotorecorder.h
           DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/include/proto/)
file (COPY ${CMAKE_CURRENT_SOURCE_DIR}/src/proto/mythprotoevent.h
           DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/include/proto/)
file (COPY ${CMAKE_CURRENT_SOURCE_DIR}/src/proto/mythprotoplayback.h
           DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/include/proto/)
file (COPY ${CMAKE_CURRENT_SOURCE_DIR}/src/proto/mythprototransfer.h
           DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/include/proto/)

include_directories (
  ${CMAKE_CURRENT_BINARY_DIR}/include/.)

###############################################################################
# add sources
file (GLOB SRC_FILES
  src/private/mythdto/*.cpp
  src/private/*.c
  src/private/*.cpp
  src/proto/*.cpp
  src/*.cpp)

file (GLOB OS_SRC_FILES
  src/private/os/threads/threadpool.cpp)

if (MSVC)
  list (APPEND OS_SRC_FILES
    src/private/os/windows/winpthreads.c)
endif ()

set (CPPMYTH_SOURCES
  ${SRC_FILES} ${OS_SRC_FILES})

###############################################################################
# add targets
set (cppmyth_LIBRARIES ${CMAKE_THREAD_LIBS_INIT})
if (WIN32)
  list (APPEND cppmyth_LIBRARIES ws2_32)
else ()
  list (APPEND cppmyth_LIBRARIES m)
  find_library (LIBRT rt)
  if (LIBRT)
    list (APPEND cppmyth_LIBRARIES rt)
  endif ()
endif ()

if (BUILD_SHARED_LIBS)
  add_library (cppmyth SHARED ${CPPMYTH_SOURCES})
else ()
  add_library (cppmyth STATIC ${CPPMYTH_SOURCES})
endif ()
target_link_libraries (cppmyth ${cppmyth_LIBRARIES})
set_target_properties (cppmyth PROPERTIES
    VERSION "${CPPMYTH_LIB_VERSION}"
    SOVERSION "${CPPMYTH_LIB_SOVERSION}")

###############################################################################
# install targets
# Create pkg-conf file.
# (We use the same files as ./configure does, so we
#  have to defined the same variables used there).
if (NOT WIN32)
  if (NOT DEFINED CMAKE_INSTALL_BINDIR)
    set (CMAKE_INSTALL_BINDIR bin)
  endif ()
  if (NOT DEFINED CMAKE_INSTALL_LIBDIR)
    set (CMAKE_INSTALL_LIBDIR lib)
  endif ()
  if (NOT DEFINED CMAKE_INSTALL_INCLUDEDIR)
    set (CMAKE_INSTALL_INCLUDEDIR include)
  endif ()
  set (prefix      ${CMAKE_INSTALL_PREFIX})
  set (exec_prefix ${CMAKE_INSTALL_PREFIX})
  set (libdir      ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR})
  set (VERSION     ${CPPMYTH_LIB_VERSION})
  configure_file (cppmyth.pc.in cppmyth.pc @ONLY)

  install (TARGETS cppmyth
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

  install (FILES
    ${CMAKE_CURRENT_BINARY_DIR}/cppmyth.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
endif ()

install (FILES
  ${CMAKE_CURRENT_BINARY_DIR}/include/cppmyth_config.h
  ${CMAKE_CURRENT_BINARY_DIR}/include/mythlocked.h
  ${CMAKE_CURRENT_BINARY_DIR}/include/mythintrinsic.h
  ${CMAKE_CURRENT_BINARY_DIR}/include/mythsharedptr.h
  ${CMAKE_CURRENT_BINARY_DIR}/include/mythtypes.h
  ${CMAKE_CURRENT_BINARY_DIR}/include/mythcontrol.h
  ${CMAKE_CURRENT_BINARY_DIR}/include/mytheventhandler.h
  ${CMAKE_CURRENT_BINARY_DIR}/include/mythfileplayback.h
  ${CMAKE_CURRENT_BINARY_DIR}/include/mythlivetvplayback.h
  ${CMAKE_CURRENT_BINARY_DIR}/include/mythrecordingplayback.h
  ${CMAKE_CURRENT_BINARY_DIR}/include/mythwsapi.h
  ${CMAKE_CURRENT_BINARY_DIR}/include/mythdebug.h
  ${CMAKE_CURRENT_BINARY_DIR}/include/mythstream.h
  ${CMAKE_CURRENT_BINARY_DIR}/include/mythwsstream.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/cppmyth/)

install (FILES
  ${CMAKE_CURRENT_BINARY_DIR}/include/proto/mythprotobase.h
  ${CMAKE_CURRENT_BINARY_DIR}/include/proto/mythprotomonitor.h
  ${CMAKE_CURRENT_BINARY_DIR}/include/proto/mythprotorecorder.h
  ${CMAKE_CURRENT_BINARY_DIR}/include/proto/mythprotoevent.h
  ${CMAKE_CURRENT_BINARY_DIR}/include/proto/mythprotoplayback.h
  ${CMAKE_CURRENT_BINARY_DIR}/include/proto/mythprototransfer.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/cppmyth/proto/)
