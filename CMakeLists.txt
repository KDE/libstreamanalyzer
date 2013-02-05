project (libstreamanalyzer)

##### cmake settings #####

cmake_minimum_required(VERSION 2.8.9 FATAL_ERROR)

cmake_policy(VERSION 2.6.0)
if(POLICY CMP0017)
  cmake_policy(SET CMP0017 NEW)
endif()

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(MacroCheckGccVisibility)
include(MacroFindOptionalDep)
enable_testing()

# Used to report found packages
include(FeatureSummary)

# Used to create config files
include(CMakePackageConfigHelpers)

# Used to set some generic paths
include(GNUInstallDirs)

##### global variables #####

# due to a bug in libxml2, we only get thread support if _REENTRANT is defined
add_definitions(-D_REENTRANT)

set(LIBSTREAMANALYZER_VERSION_MAJOR 0)
set(LIBSTREAMANALYZER_VERSION_MINOR 7)
set(LIBSTREAMANALYZER_VERSION_PATCH 8)
set(LIBSTREAMANALYZER_VERSION ${LIBSTREAMANALYZER_VERSION_MAJOR}.${LIBSTREAMANALYZER_VERSION_MINOR}.${LIBSTREAMANALYZER_VERSION_PATCH})
set(LIBSTREAMANALYZER_SOVERSION ${LIBSTREAMANALYZER_VERSION_MAJOR})

option(BUILD_UTILS "build luceneindexer, xmlindexer, rdfindexer, ontoprint utilities" ON)
option(BUILD_DEEPTOOLS "build deep find and deepgrep tools" ON)
option(ENABLE_XINE
       "enable Xine support. This adds support for a larger number of media formats(highly unstable)." OFF)
option(ENABLE_FFMPEG
       "enable FFMPEG support. This adds support for a larger number of media formats(testing)." ON)
option(ENABLE_EXIV2
       "enable exiv2 support. This allows you to index EXIF/IPTC metadata." ON)
set(CLUCENE_MIN_VERSION "0.9.21")
option(ENABLE_CLUCENE "enable CLucene support (recommended)" ON)
option(ENABLE_CLUCENE_NG "enable clucene-ng (version 1+) support (testing)" ON)

# (Absolute) paths definitions
set(LIB_DESTINATION     "${CMAKE_INSTALL_FULL_LIBDIR}")
set(INCLUDE_DESTINATION "${CMAKE_INSTALL_FULL_INCLUDEDIR}")
set(DATA_DESTINATION    "${CMAKE_INSTALL_FULL_DATAROOTDIR}")

# Set up RPATH handling, so the libs are found if they are installed to a non-standard location.
# By default cmake builds the targets with full RPATH to everything in the build directory,
# but then removes the RPATH when installing.
# These two options below make it set the RPATH of the installed targets to all
# RPATH directories outside the current CMAKE_BINARY_DIR and also the library
# install directory. Alex
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH  TRUE)
set(CMAKE_INSTALL_RPATH                ${LIB_DESTINATION} )

if(NOT WIN32 AND NOT CMAKE_SYSTEM MATCHES "SunOS-5*.")
  add_definitions(-fPIC)
endif()

##### environment inspection #####

# check for visibility support
macro_check_gcc_visibility(__STRIGI_HAVE_GCC_VISIBILITY)

# check for required packages
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
  find_package(LibStreams REQUIRED)
else()
  set(LIBSTREAMS_INCLUDE_DIRS
    ${libstreams_SOURCE_DIR}/include
    ${libstreams_BINARY_DIR}/include
    )
  set(LIBSTREAMS_LIBRARIES streams)
endif()


find_package(Threads)
set_package_properties(Threads PROPERTIES
                       DESCRIPTION "The thread library of the system"
                       TYPE REQUIRED
                      )

if(NOT CMAKE_USE_PTHREADS_INIT AND NOT CMAKE_USE_WIN32_THREADS_INIT)
  message(FATAL_ERROR "Strigi needs either pthreads or win32 threads, none was found.")
endif()

find_package(LibXml2)
set_package_properties(LibXml2 PROPERTIES
                       URL "http://xmlsoft.org"
                       TYPE REQUIRED
                      )
find_package(Iconv)
set_package_properties(Iconv PROPERTIES
                       TYPE REQUIRED
                      )


find_optional_dep(CLucene ENABLE_CLUCENE CLucene_FOUND "CLucene backend")
find_optional_dep(CLucene1 ENABLE_CLUCENE_NG CLUCENE1_FOUND "CLucene Version 1+ backend")
find_optional_dep(Exiv2 ENABLE_EXIV2 EXIV2_FOUND "indexing of EXIF/IPTC metadata")
find_optional_dep(FFmpeg ENABLE_FFMPEG FFMPEG_FOUND "indexing FFMPEG" COMPONENTS AVCODEC AVFORMAT AVUTIL SWSCALE)
find_package(XAttr)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)


include(ConfigureChecks.cmake)

##### building and testing #####

configure_file(${libstreamanalyzer_SOURCE_DIR}/strigi_thread.h.cmake
               ${libstreamanalyzer_BINARY_DIR}/include/strigi/strigi_thread.h)

# every directory needs the headers that will be installed
include_directories(${LIBSTREAMS_INCLUDE_DIRS}
                    ${libstreamanalyzer_SOURCE_DIR}/include
                    ${libstreamanalyzer_SOURCE_DIR}/include/strigi
                    ${libstreamanalyzer_BINARY_DIR}/include
                    ${libstreamanalyzer_BINARY_DIR}/include/strigi
                    ${libstreamanalyzer_SOURCE_DIR}/lib
                    ${libstreamanalyzer_BINARY_DIR}/lib
                    ${ICONV_INCLUDE_DIR}
                    ${LIBSTREAMS_LIBRARY_DIRS}
                    ${EXIV2_INCLUDEDIR}
                    ${XINE_INCLUDE_DIR}
                    ${FFMPEG_INCLUDE_DIRS}
                    ${MAGIC_INCLUDE_DIR}
                    ${LIBXML2_INCLUDE_DIR}
)
if(XATTR_FOUND)
  include_directories(${XATTR_INCLUDE_DIR})
endif()

add_subdirectory(lib)
add_subdirectory(plugins)
#add_subdirectory(tests)

##### installing #####

# all installed files are listed here
# headers
file(GLOB STRIGI_HEADERS include/strigi/*.h)
install(FILES
        ${STRIGI_HEADERS}
        ${libstreamanalyzer_BINARY_DIR}/include/strigi/strigi_thread.h
        DESTINATION ${INCLUDE_DESTINATION}/strigi
)

configure_package_config_file(LibStreamAnalyzerConfig.cmake.in
                              ${CMAKE_CURRENT_BINARY_DIR}/LibStreamAnalyzerConfig.cmake
                              INSTALL_DESTINATION ${LIB_DESTINATION}/cmake/LibStreamAnalyzer
                              PATH_VARS INCLUDE_DESTINATION LIB_DESTINATION
)

write_basic_package_version_file(${libstreamanalyzer_BINARY_DIR}/LibStreamAnalyzerConfigVersion.cmake
                                 VERSION ${LIBSTREAMANALYZER_VERSION}
                                 COMPATIBILITY SameMajorVersion
                                )

install(FILES ${libstreamanalyzer_BINARY_DIR}/LibStreamAnalyzerConfig.cmake
              ${libstreamanalyzer_BINARY_DIR}/LibStreamAnalyzerConfigVersion.cmake
        DESTINATION ${LIB_DESTINATION}/cmake/LibStreamAnalyzer)

# ontologies
file (GLOB STRIGI_ONTOLOGIES share/strigi/fieldproperties/*.rdfs)
install(FILES ${STRIGI_ONTOLOGIES}
        DESTINATION ${DATA_DESTINATION}/strigi/fieldproperties/
)

# library
if(NOT WIN32)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/lib/libstreamanalyzer.pc.cmake
                  ${CMAKE_CURRENT_BINARY_DIR}/lib/libstreamanalyzer.pc)

  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/lib/libstreamanalyzer.pc
          DESTINATION ${LIB_DESTINATION}/pkgconfig)
endif()
