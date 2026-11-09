# CPack creates component roots before cmake --install. Normalize them too,
# including usr/lib/uaa-native-dev, even when the packaging process uses umask 077.
if(NOT DEFINED CPACK_TEMPORARY_DIRECTORY OR NOT IS_DIRECTORY "${CPACK_TEMPORARY_DIRECTORY}")
 message(FATAL_ERROR "Missing CPack staging directory")
endif()
file(GLOB_RECURSE UAA_STAGED_ENTRIES LIST_DIRECTORIES TRUE "${CPACK_TEMPORARY_DIRECTORY}/*")
foreach(UAA_STAGED_ENTRY IN LISTS UAA_STAGED_ENTRIES)
 if(IS_DIRECTORY "${UAA_STAGED_ENTRY}")
  file(CHMOD "${UAA_STAGED_ENTRY}" PERMISSIONS
   OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
 endif()
endforeach()
