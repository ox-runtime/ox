#pragma once

#ifdef __APPLE__

#include <cstdint>
#include <vector>

// Forward declarations to avoid including Metal headers in C++ files
#ifdef __OBJC__
@protocol MTLDevice;
@protocol MTLTexture;
#else
typedef void* id;
#endif

namespace ox {
namespace client {

// Create Metal textures for swapchain
// Returns true on success, false on failure
bool CreateMetalSwapchainTextures(void* metalDevice,  // id<MTLDevice>
                                  uint32_t width, uint32_t height,
                                  int64_t format,  // MTLPixelFormat as int64_t
                                  uint32_t numImages,
                                  void** outTextures);  // Array of id<MTLTexture> to fill

// Release Metal textures
void ReleaseMetalSwapchainTextures(void** textures, uint32_t numTextures);

// Get supported Metal pixel formats as int64_t values
std::vector<int64_t> GetSupportedMetalFormats();

}  // namespace client
}  // namespace ox

#endif  // __APPLE__
