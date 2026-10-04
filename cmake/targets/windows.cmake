# windows specific target definitions
set_target_properties(sunshine PROPERTIES LINK_SEARCH_START_STATIC 1)
set(CMAKE_FIND_LIBRARY_SUFFIXES ".dll")
find_library(ZLIB ZLIB1)
list(APPEND SUNSHINE_EXTERNAL_LIBRARIES
        $<TARGET_OBJECTS:sunshine_rc_object>
        Wtsapi32.lib
        version.lib)
# windows-legacy: Windowsapp.lib (UWP/WinRT umbrella, Win10+) is only needed for WGC.
# Omitting it on Win7/8.x builds avoids loader failures from missing api-ms-win imports.
if(SUNSHINE_ENABLE_WGC)
    list(APPEND SUNSHINE_EXTERNAL_LIBRARIES Windowsapp.lib)
endif()
