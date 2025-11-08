set(SRCS
  include/imconfig.h
  include/imgui.h
  include/imgui_internal.h
  include/imgui_stdlib.h
  include/imstb_textedit.h
  src/imgui.cpp
  src/imgui_demo.cpp
  src/imgui_draw.cpp
  src/imgui_stdlib.cpp
  src/imgui_tables.cpp
  src/imgui_widgets.cpp
  src/imstb_rectpack.h
  src/imstb_truetype.h
)

# Only add FreeType support if not building for Emscripten
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  list(APPEND SRCS
    include/imgui_freetype.h
    src/imgui_freetype.cpp
  )
else()
  # For Emscripten, add stub implementation
  list(APPEND SRCS
    include/imgui_freetype.h
    src/imgui_freetype_web.cpp
  )
endif()

add_library(imgui ${SRCS})
target_include_directories(imgui PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_include_directories(imgui INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

# For Emscripten, FreeType and PlutoSVG may not be needed (ImGui has embedded font as fallback)
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  target_link_libraries(imgui PUBLIC Freetype::Freetype plutosvg::plutosvg)
endif()
