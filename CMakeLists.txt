####################################################################################################
# Crossings - SuperCollider server plugins that synthesize and shape sound at the waveset level.
# Built with SuperCollider's canonical cmake_gen helper modules (vendored in cmake_modules/).
####################################################################################################

cmake_minimum_required(VERSION 3.12)
set(project_name "Crossings")

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake_modules ${CMAKE_MODULE_PATH})
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

####################################################################################################
# load helper modules (vendored from <SC_PATH>/tools/cmake_gen)

include(SuperColliderServerPlugin RESULT_VARIABLE server_plugin_found)
if(NOT server_plugin_found)
    message(FATAL_ERROR "Could not find SuperColliderServerPlugin module in cmake_modules/")
endif()

include(SuperColliderCompilerConfig RESULT_VARIABLE compiler_config_found)
if(NOT compiler_config_found)
    message(FATAL_ERROR "Could not find SuperColliderCompilerConfig module in cmake_modules/")
endif()

####################################################################################################
# build options (scsynth-only by default; supernova off so nova-tt is not required)

option(SCSYNTH "Build plugins for scsynth" ON)
option(SUPERNOVA "Build plugins for supernova" OFF)
option(NATIVE "Optimize for native architecture" OFF)
option(STRICT "Use strict warning flags" OFF)
option(BUILD_UNIVERSAL "Build a universal binary (arm64 + x86_64) on macOS" ON)

####################################################################################################
# locate the SuperCollider source checkout

# Build against a source tree whose plugin ABI (sc_api_version) matches the scsynth you run.
# The 3.14.1 worktree is API v3 -> loadable in the installed 3.14.x / 3.12.x apps.
# (The develop checkout is API v6 and only loads in a scsynth built from develop.)
set(SC_PATH "/Users/bjarni/Dev/supercollider-3.14.1" CACHE PATH "Path to SuperCollider source")
sc_check_sc_path("${SC_PATH}")
message(STATUS "Found SuperCollider: ${SC_PATH}")
set(SC_PATH "${SC_PATH}" CACHE PATH "Path to SuperCollider source" FORCE)

include("${SC_PATH}/SCVersion.txt")
message(STATUS "Building plugins for SuperCollider version: ${SC_VERSION}")

# set project after SCVersion.txt so it doesn't clobber our config
project(${project_name})
sc_do_initial_compiler_config() # after project() so the compiler ID is known

if(APPLE AND BUILD_UNIVERSAL)
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "Build architectures" FORCE)
endif()

####################################################################################################
# install location: the user SuperCollider Extensions folder (per-user, no sudo)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT OR NOT CMAKE_INSTALL_PREFIX)
    if(APPLE)
        set(CMAKE_INSTALL_PREFIX "$ENV{HOME}/Library/Application Support/SuperCollider/Extensions"
            CACHE PATH "Install prefix" FORCE)
    elseif(WIN32)
        set(CMAKE_INSTALL_PREFIX "$ENV{LOCALAPPDATA}/SuperCollider/Extensions"
            CACHE PATH "Install prefix" FORCE)
    else()
        set(CMAKE_INSTALL_PREFIX "$ENV{HOME}/.local/share/SuperCollider/Extensions"
            CACHE PATH "Install prefix" FORCE)
    endif()
endif()
message(STATUS "Install directory set to: ${CMAKE_INSTALL_PREFIX}")

####################################################################################################
# plugin targets
# sc_add_server_plugin(dest_dir name cpp_files sc_files schelp_files)
#   -> creates <name>_scsynth (and <name>_supernova if SUPERNOVA), forces .scx / no prefix,
#      sets include dirs + compiler flags, installs .scx/.sc/.schelp under <prefix>/<dest_dir>.


foreach(ugen WsGen)
    sc_add_server_plugin(
        "Crossings"                                  # destination dir under install prefix
        "${ugen}"                                    # target base name
        "plugins/${ugen}.cpp"                        # C++ sources
        "classes/${ugen}.sc"                         # SC class files
        "HelpSource/Classes/${ugen}.schelp"          # help files
    )
endforeach()

message(STATUS "Generating plugin targets done")
