#ifdef __APPLE__
#ifdef __OBJC__

#import "metal_swapchain.h"
#import <Metal/Metal.h>
#import <Foundation/Foundation.h>
#include "../logging.h"

namespace ox {
namespace client {

// Map OpenXR format (as int64_t) to MTLPixelFormat
static MTLPixelFormat MapFormatToMetal(int64_t format) {
    switch (format) {
        case 80:  // MTLPixelFormatRGBA8Unorm
            return MTLPixelFormatRGBA8Unorm;
        case 81:  // MTLPixelFormatRGBA8Unorm_sRGB
            return MTLPixelFormatRGBA8Unorm_sRGB;
        case 70:  // MTLPixelFormatBGRA8Unorm
            return MTLPixelFormatBGRA8Unorm;
        case 71:  // MTLPixelFormatBGRA8Unorm_sRGB
            return MTLPixelFormatBGRA8Unorm_sRGB;
        default:
            LOG_ERROR(("Unsupported Metal format: " + std::to_string(format)).c_str());
            return MTLPixelFormatInvalid;
    }
}

bool CreateMetalSwapchainTextures(
    void* metalCommandQueue,
    uint32_t width,
    uint32_t height,
    int64_t format,
    uint32_t numImages,
    void** outTextures)
{
    if (!metalCommandQueue || !outTextures || numImages == 0) {
        LOG_ERROR("CreateMetalSwapchainTextures: Invalid parameters");
        return false;
    }

    id<MTLCommandQueue> commandQueue = (__bridge id<MTLCommandQueue>)metalCommandQueue;
    id<MTLDevice> device = commandQueue.device;
    if (!device) {
        LOG_ERROR("CreateMetalSwapchainTextures: Invalid Metal command queue");
        return false;
    }

    MTLPixelFormat mtlFormat = MapFormatToMetal(format);
    
    LOG_INFO(("Creating " + std::to_string(numImages) + " Metal textures: " + 
              std::to_string(width) + "x" + std::to_string(height) + 
              " format=" + std::to_string(static_cast<int>(mtlFormat))).c_str());

    // Create texture descriptor
    MTLTextureDescriptor* descriptor = [MTLTextureDescriptor 
        texture2DDescriptorWithPixelFormat:mtlFormat
        width:width
        height:height
        mipmapped:NO];
    
    if (!descriptor) {
        LOG_ERROR("CreateMetalSwapchainTextures: Failed to create texture descriptor");
        return false;
    }

    // Configure descriptor with version-appropriate API
    if (@available(macOS 10.12, iOS 10.0, *)) {
        // Metal 1.2+: Use modern storageMode API
        descriptor.storageMode = MTLStorageModePrivate;
        LOG_DEBUG("Using Metal 1.2+ storageMode API");
    } else {
        // Metal 1.0/1.1: Use legacy resourceOptions
        descriptor.resourceOptions = MTLResourceStorageModePrivate;
        LOG_DEBUG("Using Metal 1.0 resourceOptions API");
    }
    
    if (@available(macOS 10.11.4, iOS 9.0, *)) {
        // Metal 1.1+: Set texture usage hints
        descriptor.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
        LOG_DEBUG("Set texture usage flags (Metal 1.1+)");
    } else {
        // Metal 1.0: Usage flags don't exist, skip them
        LOG_DEBUG("Skipping usage flags (Metal 1.0)");
    }

    // Create textures (same for all Metal versions)
    for (uint32_t i = 0; i < numImages; i++) {
        id<MTLTexture> texture = [device newTextureWithDescriptor:descriptor];
        
        if (!texture) {
            LOG_ERROR(("CreateMetalSwapchainTextures: Failed to create texture " + 
                      std::to_string(i)).c_str());
            
            // Clean up previously created textures
            for (uint32_t j = 0; j < i; j++) {
                if (outTextures[j]) {
                    CFRelease(outTextures[j]);
                    outTextures[j] = nullptr;
                }
            }
            return false;
        }

        // Retain the texture and store as void*
        outTextures[i] = (void*)CFBridgingRetain(texture);
        
        LOG_DEBUG(("Created Metal texture " + std::to_string(i) + 
                  " successfully").c_str());
    }
        
        LOG_DEBUG(("Created Metal texture " + std::to_string(i) + " successfully").c_str());
    }

    LOG_INFO(("Successfully created " + std::to_string(numImages) + " Metal textures").c_str());
    return true;
}

void ReleaseMetalSwapchainTextures(void** textures, uint32_t numTextures) {
    if (!textures) {
        return;
    }

    LOG_DEBUG(("Releasing " + std::to_string(numTextures) + " Metal textures").c_str());

    for (uint32_t i = 0; i < numTextures; i++) {
        if (textures[i]) {
            // Release the retained texture
            CFRelease(textures[i]);
            textures[i] = nullptr;
        }
    }

    LOG_INFO(("Released " + std::to_string(numTextures) + " Metal textures").c_str());
}

std::vector<int64_t> GetSupportedMetalFormats() {
    return {
        static_cast<int64_t>(MTLPixelFormatRGBA8Unorm),
        static_cast<int64_t>(MTLPixelFormatRGBA8Unorm_sRGB),
        static_cast<int64_t>(MTLPixelFormatBGRA8Unorm),
        static_cast<int64_t>(MTLPixelFormatBGRA8Unorm_sRGB),
    };
}

void* GetMetalDefaultDevice() {
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    return (__bridge void*)device;
}

}  // namespace client
}  // namespace ox

#endif  // __OBJC__
#endif  // __APPLE__
