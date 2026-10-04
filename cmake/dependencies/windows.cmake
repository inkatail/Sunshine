# windows specific dependencies

# MinHook setup - use installed minhook for AMD64, otherwise download minhook-detours for ARM64
if(CMAKE_SYSTEM_PROCESSOR MATCHES "AMD64")
    # Make sure MinHook is installed for x86/x64
    find_library(MINHOOK_LIBRARY libMinHook.a)
    find_path(MINHOOK_INCLUDE_DIR MinHook.h PATH_SUFFIXES include)
    if(MINHOOK_LIBRARY AND MINHOOK_INCLUDE_DIR)
        add_library(minhook::minhook STATIC IMPORTED)
        set_property(TARGET minhook::minhook PROPERTY IMPORTED_LOCATION ${MINHOOK_LIBRARY})
        target_include_directories(minhook::minhook INTERFACE ${MINHOOK_INCLUDE_DIR})
    elseif(CMAKE_CROSSCOMPILING)
        # windows-legacy: Linux->Windows cross has no MinHook sysroot package, so
        # compile its plain-C sources (avoids its CMakeLists, which declares a
        # cmake_minimum_required range rejected by CMake 4+).
        message(STATUS "MinHook not found in sysroot, fetching sources for cross build")
        include(FetchContent)
        FetchContent_Declare(
                minhook
                URL https://github.com/TsudaKageyu/minhook/archive/refs/tags/v1.3.4.tar.gz
                URL_HASH SHA256=1aebeae4ca898330c507860acc2fca2eb335fe446a3a2b8444c3bf8b2660a14e
        )
        FetchContent_GetProperties(minhook)
        if(NOT minhook_POPULATED)
            FetchContent_Populate(minhook)
        endif()
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            set(MINHOOK_HDE_SOURCE "${minhook_SOURCE_DIR}/src/hde/hde64.c")
        else()
            set(MINHOOK_HDE_SOURCE "${minhook_SOURCE_DIR}/src/hde/hde32.c")
        endif()
        add_library(minhook_cross STATIC
                "${minhook_SOURCE_DIR}/src/buffer.c"
                "${minhook_SOURCE_DIR}/src/hook.c"
                "${minhook_SOURCE_DIR}/src/trampoline.c"
                "${MINHOOK_HDE_SOURCE}")
        target_include_directories(minhook_cross PUBLIC "${minhook_SOURCE_DIR}/include")
        target_include_directories(minhook_cross PRIVATE
                "${minhook_SOURCE_DIR}/src"
                "${minhook_SOURCE_DIR}/src/hde")
        add_library(minhook::minhook ALIAS minhook_cross)
    else()
        message(FATAL_ERROR "MinHook not found (libMinHook.a). Install mingw-w64 MinHook or equivalent.")
    endif()
else()
    # Download pre-built minhook-detours for ARM64
    message(STATUS "Downloading minhook-detours pre-built binaries for ARM64")
    include(FetchContent)

    FetchContent_Declare(
        minhook-detours
        URL      https://github.com/m417z/minhook-detours/releases/download/v1.0.6/minhook-detours-1.0.6.zip
        URL_HASH SHA256=E719959D824511E27395A82AEDA994CAAD53A67EE5894BA5FC2F4BF1FA41E38E
    )
    FetchContent_MakeAvailable(minhook-detours)

    # Create imported library for the pre-built DLL
    set(_MINHOOK_DLL
        "${minhook-detours_SOURCE_DIR}/Release/minhook-detours.ARM64.Release.dll"
        CACHE INTERNAL "Path to minhook-detours DLL")
    add_library(minhook::minhook SHARED IMPORTED GLOBAL)
    set_property(TARGET minhook::minhook PROPERTY IMPORTED_LOCATION "${_MINHOOK_DLL}")
    set_property(TARGET minhook::minhook PROPERTY IMPORTED_IMPLIB
        "${minhook-detours_SOURCE_DIR}/Release/minhook-detours.ARM64.Release.lib")
    set_target_properties(minhook::minhook PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${minhook-detours_SOURCE_DIR}/src"
    )
endif()
