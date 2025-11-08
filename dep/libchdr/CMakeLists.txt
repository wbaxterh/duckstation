add_library(libchdr
  include/dr_libs/dr_flac.h
  include/libchdr/bitstream.h
  include/libchdr/cdrom.h
  include/libchdr/chd.h
  include/libchdr/chdconfig.h
  include/libchdr/coretypes.h
  include/libchdr/flac.h
  include/libchdr/huffman.h
  src/libchdr_bitstream.c
  src/libchdr_cdrom.c
  src/libchdr_chd.c
  src/libchdr_flac.c
  src/libchdr_huffman.c
)

target_include_directories(libchdr PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

# For Emscripten, we need to compile/link with zlib support and stub out zstd for now
if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  target_compile_options(libchdr PRIVATE "-sUSE_ZLIB=1")
  target_link_libraries(libchdr PRIVATE ZLIB::ZLIB lzma)
  target_link_options(libchdr PRIVATE "-sUSE_ZLIB=1")

  # Create a minimal zstd stub header for Emscripten builds in a directory that will be searched first
  # TODO: Either find/bundle zstd or conditionally compile CHD zstd support
  set(ZSTD_STUB_DIR "${CMAKE_CURRENT_BINARY_DIR}/zstd_stub_include")
  file(MAKE_DIRECTORY "${ZSTD_STUB_DIR}")
  file(WRITE "${ZSTD_STUB_DIR}/zstd.h" "
#ifndef ZSTD_H_STUB
#define ZSTD_H_STUB
/* Stub zstd header for Emscripten - zstd-compressed CHD files will not work */
typedef struct ZSTD_DStream_s ZSTD_DStream;
typedef struct { const void* src; size_t size; size_t pos; } ZSTD_inBuffer;
typedef struct { void* dst; size_t size; size_t pos; } ZSTD_outBuffer;
static inline ZSTD_DStream* ZSTD_createDStream(void) { return NULL; }
static inline size_t ZSTD_freeDStream(ZSTD_DStream* zds) { return 0; }
static inline size_t ZSTD_initDStream(ZSTD_DStream* zds) { return 0; }
static inline size_t ZSTD_decompressStream(ZSTD_DStream* zds, ZSTD_outBuffer* output, ZSTD_inBuffer* input) { return (size_t)(-1); }
static inline unsigned ZSTD_isError(size_t code) { return code == (size_t)(-1); }
#endif
")
  # Add the stub directory BEFORE other includes so it's found first
  target_include_directories(libchdr BEFORE PRIVATE "${ZSTD_STUB_DIR}")
else()
  target_link_libraries(libchdr PRIVATE ZLIB::ZLIB zstd::libzstd_shared lzma)
endif()

