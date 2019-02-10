#pragma once

#include <stdexcept>

#define NO_COPY_OR_ASSIGNMENT(Class)                    \
  Class(Class&&) = delete;                              \
  Class(const Class&) = delete;                         \
  Class & operator=(Class&&) = delete;                  \
  Class & operator=(const Class&) = delete;             \

class VkException : public std::exception {};
class VkTimeoutException : public VkException {};
class VkNotReadyException : public VkException {};
class VkIncompleteException : public VkException {};
class VkSuboptimalException : public VkException {};
class VkErrorOutOfDateException : public VkException {};
class VkErrorDeviceLostException : public VkException {};
class VkErrorSurfaceLostException : public VkException {};
class VkErrorInvalidShaderException : public VkException {};
class VkErrorFragmentedPoolException : public VkException {};
class VkErrorTooManyObjectsException : public VkException {};
class VkErrorLayerNotPresentException : public VkException {};
class VkErrorMemoryMapFailedException : public VkException {};
class VkErrorOutOfHostMemoryException : public VkException {};
class VkErrorOutOfPoolMemoryException : public VkException {};
class VkErrorValidationFailedException : public VkException {};
class VkErrorNativeWindowInUseException : public VkException {};
class VkErrorFeatureNotPresentException : public VkException {};
class VkErrorOutOfDeviceMemoryException : public VkException {};
class VkErrorFormatNotSupportedException : public VkException {};
class VkErrorIncompatibleDriverException : public VkException {};
class VkErrorExtensionNotPresentException : public VkException {};
class VkErrorIncompatibleDisplayException : public VkException {};
class VkErrorInitializationFailedException : public VkException {};
class VkErrorInvalidExternalHandleException : public VkException {};

#define THROW_ON_ERROR(__function__) {                                                        \
	VkResult __result__ = (__function__);                                                       \
  switch (__result__) {                                                                       \
    case VK_TIMEOUT: throw VkTimeoutException();                                              \
    case VK_NOT_READY: throw VkNotReadyException();                                           \
    case VK_INCOMPLETE: throw VkIncompleteException();                                        \
    case VK_SUBOPTIMAL_KHR: throw VkSuboptimalException();                                    \
    case VK_ERROR_DEVICE_LOST: throw VkErrorDeviceLostException();                            \
    case VK_ERROR_OUT_OF_DATE_KHR: throw VkErrorOutOfDateException();                         \
    case VK_ERROR_SURFACE_LOST_KHR: throw VkErrorSurfaceLostException();                      \
    case VK_ERROR_FRAGMENTED_POOL: throw VkErrorFragmentedPoolException();                    \
    case VK_ERROR_INVALID_SHADER_NV: throw VkErrorInvalidShaderException();                   \
    case VK_ERROR_TOO_MANY_OBJECTS: throw VkErrorTooManyObjectsException();                   \
    case VK_ERROR_MEMORY_MAP_FAILED: throw VkErrorMemoryMapFailedException();                 \
    case VK_ERROR_LAYER_NOT_PRESENT: throw VkErrorLayerNotPresentException();                 \
    case VK_ERROR_OUT_OF_HOST_MEMORY: throw VkErrorOutOfHostMemoryException();                \
    case VK_ERROR_FEATURE_NOT_PRESENT: throw VkErrorFeatureNotPresentException();             \
    case VK_ERROR_INCOMPATIBLE_DRIVER: throw VkErrorIncompatibleDriverException();            \
    case VK_ERROR_OUT_OF_DEVICE_MEMORY: throw VkErrorOutOfDeviceMemoryException();            \
    case VK_ERROR_VALIDATION_FAILED_EXT: throw VkErrorValidationFailedException();            \
    case VK_ERROR_OUT_OF_POOL_MEMORY_KHR: throw VkErrorOutOfPoolMemoryException();            \
    case VK_ERROR_FORMAT_NOT_SUPPORTED: throw VkErrorFormatNotSupportedException();           \
    case VK_ERROR_EXTENSION_NOT_PRESENT: throw VkErrorExtensionNotPresentException();         \
    case VK_ERROR_INITIALIZATION_FAILED: throw VkErrorInitializationFailedException();        \
    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: throw VkErrorNativeWindowInUseException();        \
    case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: throw VkErrorIncompatibleDisplayException();      \
    case VK_ERROR_INVALID_EXTERNAL_HANDLE_KHR: throw VkErrorInvalidExternalHandleException(); \
    default: break;                                                                           \
  }                                                                                           \
}                                                                                             \

#define PI 3.14159265358979323846