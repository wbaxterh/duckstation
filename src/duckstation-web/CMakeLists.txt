# DuckStation Web Frontend (Emscripten/WASM Build)

# This target is only built when using Emscripten
if(NOT EMSCRIPTEN)
  message(STATUS "Skipping duckstation-web (not an Emscripten build)")
  return()
endif()

message(STATUS "Building duckstation-web frontend for WebAssembly")

add_executable(duckstation-web
  web_host.cpp
  web_settings.cpp
  web_settings.h
)

# Link against DuckStation core libraries
target_link_libraries(duckstation-web PRIVATE
  core
  util
  common
  scmversion
)

# Emscripten-specific compiler flags for dependencies
target_compile_options(duckstation-web PRIVATE
  "-sUSE_ZLIB=1"
)

# Emscripten-specific linker flags
set(EMSCRIPTEN_LINK_FLAGS
  # Module settings
  "-s MODULARIZE=1"
  "-s EXPORT_ES6=1"
  "-s EXPORT_NAME='createDuckStationModule'"

  # Environment
  "-s ENVIRONMENT=web"
  "-s ALLOW_MEMORY_GROWTH=1"

  # Filesystem
  "-s FORCE_FILESYSTEM=1"
  "-s EXPORTED_RUNTIME_METHODS=['FS','cwrap','ccall']"

  # WebGL
  "-s USE_WEBGL2=1"
  "-s OFFSCREENCANVAS_SUPPORT=0"  # Start without offscreen canvas
  "-s FULL_ES3=1"

  # Dependencies - use Emscripten ports
  "-sUSE_ZLIB=1"

  # Memory
  "-s INITIAL_MEMORY=1GB"
  "-s MAXIMUM_MEMORY=4GB"
  "-s ALLOW_MEMORY_GROWTH=1"

  # Exported functions (C functions callable from JavaScript)
  "-s EXPORTED_FUNCTIONS=['_main','_duckstation_init','_duckstation_load_bios','_duckstation_boot_game','_duckstation_frame','_duckstation_shutdown']"

  # Optimization & Debugging
  "-s ASSERTIONS=1"  # Enable assertions for debugging
  "-s GL_ASSERTIONS=1"
  "-g3"  # Full debug symbols for readable stack traces

  # Function table settings
  "-s ALLOW_TABLE_GROWTH=1"  # Allow function table to grow dynamically
  "-s RESERVED_FUNCTION_POINTERS=2000"  # Reserve space for virtual function pointers

  # File system pre-loading (we'll load files via JS instead)
  # "--preload-file ${CMAKE_SOURCE_DIR}/data/resources@/app/resources"

  # Debugging
  # "-s DEMANGLE_SUPPORT=1"  # Removed - no longer supported in modern Emscripten
)

# Join all flags
string(REPLACE ";" " " EMSCRIPTEN_LINK_FLAGS_STR "${EMSCRIPTEN_LINK_FLAGS}")
set_target_properties(duckstation-web PROPERTIES
  LINK_FLAGS "${EMSCRIPTEN_LINK_FLAGS_STR}"
  SUFFIX ".js"
)

# Install rules - copy output to web-dist/duckstation/
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/duckstation-web.js
  ${CMAKE_CURRENT_BINARY_DIR}/duckstation-web.wasm
  DESTINATION ${CMAKE_SOURCE_DIR}/web-dist/duckstation
)

# Copy any additional worker files if generated
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/duckstation-web.worker.js
  DESTINATION ${CMAKE_SOURCE_DIR}/web-dist/duckstation
  OPTIONAL
)
