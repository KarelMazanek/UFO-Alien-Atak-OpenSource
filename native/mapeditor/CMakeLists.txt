# Integration adapter; editor licensing and credits are integrated without changing map logic or graphics.
set(UAA_MAPEDITOR_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/../external/mapeditor-0.1.4/uaa-native-source")
if(NOT EXISTS "${UAA_MAPEDITOR_SOURCE}/src/main.cpp")
 message(FATAL_ERROR "UAA Mapeditor 0.1.4 source archive is missing")
endif()
add_executable(uaa-mapeditor "${UAA_MAPEDITOR_SOURCE}/src/main.cpp")
target_include_directories(uaa-mapeditor PRIVATE "${UAA_MAPEDITOR_SOURCE}/vendor")
target_link_libraries(uaa-mapeditor PRIVATE SDL2::SDL2 Threads::Threads)
set_target_properties(uaa-mapeditor PROPERTIES
 RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/mapeditor"
 RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/mapeditor"
 RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/mapeditor")
if(WIN32)
 target_link_libraries(uaa-mapeditor PRIVATE comdlg32)
 if(MINGW)
  target_link_options(uaa-mapeditor PRIVATE -static -static-libgcc -static-libstdc++ -Wl,--subsystem,windows)
 elseif(MSVC)
  target_link_options(uaa-mapeditor PRIVATE /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup)
 endif()
 add_custom_command(TARGET uaa-mapeditor POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:SDL2::SDL2>" "$<TARGET_FILE_DIR:uaa-mapeditor>")
endif()
add_custom_command(TARGET uaa-mapeditor POST_BUILD
 COMMAND "${CMAKE_COMMAND}" -E copy_directory "${UAA_MAPEDITOR_SOURCE}/LICENSES" "$<TARGET_FILE_DIR:uaa-mapeditor>/LICENSES"
 COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${UAA_MAPEDITOR_SOURCE}/README.md" "$<TARGET_FILE_DIR:uaa-mapeditor>/README.md")
set(UAA_MAPEDITOR_DATA "" CACHE FILEPATH "Original uaa-data.pak for UAA Mapeditor 0.1.4")
if(UAA_MAPEDITOR_DATA)
 if(NOT EXISTS "${UAA_MAPEDITOR_DATA}")
  message(FATAL_ERROR "UAA_MAPEDITOR_DATA does not exist: ${UAA_MAPEDITOR_DATA}")
 endif()
 add_custom_command(TARGET uaa-mapeditor POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${UAA_MAPEDITOR_DATA}" "$<TARGET_FILE_DIR:uaa-mapeditor>/uaa-data.pak")
endif()
add_executable(uaa-mapeditor-core-tests "${UAA_MAPEDITOR_SOURCE}/tests/core_tests.cpp")
target_include_directories(uaa-mapeditor-core-tests PRIVATE "${UAA_MAPEDITOR_SOURCE}/src" "${UAA_MAPEDITOR_SOURCE}/vendor")
# Upstream tests use assert; they must execute also in Release builds.
if(MSVC)
 target_compile_options(uaa-mapeditor-core-tests PRIVATE /UNDEBUG)
else()
 target_compile_options(uaa-mapeditor-core-tests PRIVATE -UNDEBUG)
endif()
add_test(NAME mapeditor-core COMMAND uaa-mapeditor-core-tests)

add_custom_command(TARGET uaa-mapeditor POST_BUILD
 COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${UAA_MAPEDITOR_SOURCE}/LICENSE" "${UAA_MAPEDITOR_SOURCE}/LICENSING.md" "${UAA_MAPEDITOR_SOURCE}/MUSIC-LICENSE.txt" "$<TARGET_FILE_DIR:uaa-mapeditor>")
if(UAA_INSTALLERS)
 find_package(Python3 REQUIRED COMPONENTS Interpreter)
 if(WIN32)
  set(UAA_EDITOR_TARGET win32)
 else()
  set(UAA_EDITOR_TARGET linux32)
 endif()
 add_custom_command(TARGET uaa-mapeditor POST_BUILD
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../tools/validate_mapeditor.py" "$<TARGET_FILE_DIR:uaa-mapeditor>" "${UAA_EDITOR_TARGET}" --output "${CMAKE_BINARY_DIR}/mapeditor-manifest.json")
endif()
