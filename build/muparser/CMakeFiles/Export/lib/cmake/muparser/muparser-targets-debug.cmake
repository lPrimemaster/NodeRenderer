#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "muparser::muparser" for configuration "Debug"
set_property(TARGET muparser::muparser APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(muparser::muparser PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/muparser.lib"
  )

list(APPEND _IMPORT_CHECK_TARGETS muparser::muparser )
list(APPEND _IMPORT_CHECK_FILES_FOR_muparser::muparser "${_IMPORT_PREFIX}/lib/muparser.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
