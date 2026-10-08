# Windows CI uses embedded compiler debug data so independently cached objects
# do not share a compiler PDB. The linker still produces the app's normal PDB.
set(CMAKE_C_COMPILER_LAUNCHER sccache CACHE STRING "C compiler cache")
set(CMAKE_CXX_COMPILER_LAUNCHER sccache CACHE STRING "C++ compiler cache")
set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT Embedded CACHE STRING "Cacheable MSVC debug information")
# MSVC precompiled headers bypass sccache. Cache complete translation units on
# ephemeral CI runners instead; the first cold build populates the cache.
set(MUSE_COMPILE_USE_PCH OFF CACHE BOOL "Use persistent compiler cache instead of runner-local PCH")
