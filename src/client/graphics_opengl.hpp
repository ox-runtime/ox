#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "../logging.h"

namespace ox {
namespace client {
namespace opengl {

void CreateTextures(std::vector<uint32_t>& glTextureIds, uint32_t width, uint32_t height, uint32_t numImages) {
    if (glTextureIds.empty()) {
        glTextureIds.resize(numImages);
        glGenTextures(numImages, glTextureIds.data());

        // Initialize each texture with minimal settings
        for (uint32_t i = 0; i < numImages; i++) {
            glBindTexture(GL_TEXTURE_2D, glTextureIds[i]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void DestroyTextures(std::vector<uint32_t>& glTextureIds) {
    if (!glTextureIds.empty()) {
        glDeleteTextures(static_cast<GLsizei>(glTextureIds.size()), glTextureIds.data());
        glTextureIds.clear();
    }
}

bool CopyTextureToMemory(uint32_t textureId, uint32_t width, uint32_t height, std::byte* dest, size_t destSize) {
    // Verify we have enough space (RGBA8)
    size_t requiredSize = width * height * 4;
    if (destSize < requiredSize) {
        LOG_ERROR("Destination buffer too small for texture data");
        return false;
    }

    // Clear any previous errors
    while (glGetError() != GL_NO_ERROR);

    // Bind the texture and read pixels directly as RGBA
    glBindTexture(GL_TEXTURE_2D, textureId);

    GLenum bindError = glGetError();
    if (bindError != GL_NO_ERROR) {
        LOG_ERROR(
            ("OpenGL error binding texture " + std::to_string(textureId) + ": " + std::to_string(bindError)).c_str());
        return false;
    }

    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, dest);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Check for GL errors
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        LOG_ERROR(("OpenGL error reading texture " + std::to_string(textureId) + ": " + std::to_string(error)).c_str());
        return false;
    }

    return true;
}

std::vector<int64_t> GetSupportedFormats() {
    return {
        static_cast<int64_t>(GL_RGBA),
        static_cast<int64_t>(GL_RGBA8),
    };
}

// Detect OpenGL graphics binding from session create info
bool DetectGraphicsBinding(const void* next, void** outBinding) {
    while (next) {
        const XrBaseInStructure* header = reinterpret_cast<const XrBaseInStructure*>(next);
        if (header->type == XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR ||
            header->type == XR_TYPE_GRAPHICS_BINDING_OPENGL_XLIB_KHR ||
            header->type == XR_TYPE_GRAPHICS_BINDING_OPENGL_XCB_KHR ||
            header->type == XR_TYPE_GRAPHICS_BINDING_OPENGL_WAYLAND_KHR) {
            if (outBinding) {
                *outBinding = nullptr;  // OpenGL doesn't need binding data
            }
            LOG_DEBUG("DetectGraphicsBinding: OpenGL graphics binding detected");
            return true;
        }
        next = header->next;
    }
    return false;
}

// Populate swapchain images for OpenGL
void PopulateSwapchainImages(const std::vector<uint32_t>& glTextureIds, uint32_t numImages, XrStructureType imageType,
                             XrSwapchainImageBaseHeader* images) {
    for (uint32_t i = 0; i < numImages; ++i) {
        XrSwapchainImageOpenGLKHR* glImages = reinterpret_cast<XrSwapchainImageOpenGLKHR*>(images);
        glImages[i].type = imageType;
        glImages[i].next = nullptr;
        glImages[i].image = (i < glTextureIds.size()) ? glTextureIds[i] : 0;
    }
}

// OpenXR extension function
XRAPI_ATTR XrResult XRAPI_CALL xrGetOpenGLGraphicsRequirementsKHR(
    XrInstance instance, XrSystemId systemId, XrGraphicsRequirementsOpenGLKHR* graphicsRequirements) {
    LOG_DEBUG("xrGetOpenGLGraphicsRequirementsKHR called");
    if (!graphicsRequirements) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    graphicsRequirements->minApiVersionSupported = XR_MAKE_VERSION(1, 1, 0);
    graphicsRequirements->maxApiVersionSupported = XR_MAKE_VERSION(4, 6, 0);
    return XR_SUCCESS;
}

}  // namespace opengl
}  // namespace client
}  // namespace ox
