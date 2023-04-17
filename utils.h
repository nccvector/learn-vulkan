#pragma once

#include "pch.h"
#include <GLFW/glfw3.h>
#include <fstream>

namespace utils {

std::vector<char> readFile( const std::string& filename ) {
  std::ifstream file( filename, std::ios::ate | std::ios::binary );
  if ( !file.is_open() ) {
    std::cerr << "Failed to load \"" << filename << "\"" << std::endl;
  }

  size_t            filesize { static_cast<size_t>( file.tellg() ) };
  std::vector<char> buffer( filesize );
  file.seekg( 0 );
  file.read( buffer.data(), filesize );
  file.close();
  return buffer;
}

vk::ShaderModule createModule( const std::string& filepath, vk::Device device ) {
  std::vector<char>          sourceCode = readFile( filepath );
  vk::ShaderModuleCreateInfo moduleInfo;
  moduleInfo.flags    = vk::ShaderModuleCreateFlags();
  moduleInfo.codeSize = sourceCode.size();
  moduleInfo.pCode    = reinterpret_cast<const uint32_t*>( sourceCode.data() );

  try {
    return device.createShaderModule( moduleInfo );
  } catch ( vk::SystemError& err ) {
    std::cerr << "Failed to create shader module \"" << filepath << "\"" << std::endl;
  }
}

struct QueueFamilyIndices {
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentFamily;

  bool isComplete() {
    return graphicsFamily.has_value() && presentFamily.has_value();
  }
};

struct SwapchainSupportDetails {
  vk::SurfaceCapabilitiesKHR        capabilities;
  std::vector<vk::SurfaceFormatKHR> formats;
  std::vector<vk::PresentModeKHR>   presentModes;
};

struct SwapchainFrame {
  vk::Image     image;
  vk::ImageView imageView;
};

struct SwapchainBundle {
  vk::SwapchainKHR            swapchain;
  std::vector<SwapchainFrame> frames;
  vk::Format                  format;
  vk::Extent2D                extent;
};

bool supported( std::vector<const char*>& extensions, std::vector<const char*>& layers ) {
  // Show supported extensions
  std::vector<vk::ExtensionProperties> supportedExtensions = vk::enumerateInstanceExtensionProperties();
  std::cout << "Instance can support following extensions: \n";
  for ( vk::ExtensionProperties supportedExtension : supportedExtensions ) {
    std::cout << "\t\"" << supportedExtension.extensionName << "\"\n";
  }

  // Check extension support
  bool found;
  for ( const char* extension : extensions ) {
    found = false;
    for ( vk::ExtensionProperties supportedExtension : supportedExtensions ) {
      if ( strcmp( extension, supportedExtension.extensionName ) == 0 ) {
        std::cout << "Extension \"" << extension << "\" is supported!\n";
        found = true;
        break;
      }
    }

    if ( !found ) {
      std::cout << "Extension \"" << extension << "\" is not supported!\n";
      return false;
    }
  }

  // Show supported layers
  std::vector<vk::LayerProperties> supportedLayers = vk::enumerateInstanceLayerProperties();
  std::cout << "Device can support following layers: \n";
  for ( vk::LayerProperties supportedLayer : supportedLayers ) {
    std::cout << "\t\"" << supportedLayer.layerName << "\"\n";
  }

  // Check extension support
  for ( const char* layer : layers ) {
    found = false;
    for ( vk::LayerProperties supportedLayer : supportedLayers ) {
      if ( strcmp( layer, supportedLayer.layerName ) == 0 ) {
        std::cout << "Layer \"" << layer << "\" is supported!\n";
        found = true;
        break;
      }
    }

    if ( !found ) {
      std::cout << "Layer \"" << layer << "\" is not supported!\n";
      return false;
    }
  }

  return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback( VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
                                              VkDebugUtilsMessageTypeFlagsEXT             messageType,
                                              const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                              void*                                       pUserData ) {
  std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;
  return VK_FALSE;
}

vk::DebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT( vk::Instance& instance, vk::DispatchLoaderDynamic& dldi ) {
  vk::DebugUtilsMessengerCreateInfoEXT createInfo =
      vk::DebugUtilsMessengerCreateInfoEXT( vk::DebugUtilsMessengerCreateFlagsEXT(),                 // flags
                                            vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |     // severity
                                                vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | // severity
                                                vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,    // severity
                                            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |         // type
                                                vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |  // type
                                                vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,  // type
                                            debugCallback,                                           // callback
                                            nullptr );                                               // userdata

  return instance.createDebugUtilsMessengerEXT( createInfo, nullptr, dldi );
}

vk::Instance vkCreateInstance( const char* applicationName, std::vector<const char*> extensions,
                               std::vector<const char*> layers ) {
  // Query vulkan version
  uint32_t version { 0 };
  vkEnumerateInstanceVersion( &version );

  std::cout << "System can support vulkan variant: " << VK_API_VERSION_VARIANT( version )
            << ", Major: " << VK_API_VERSION_MAJOR( version ) << ", Minor: " << VK_API_VERSION_MINOR( version )
            << ", Patch: " << VK_API_VERSION_PATCH( version ) << "\n";

  // Request a lower version
  version = VK_MAKE_API_VERSION( 0, 1, 0, 0 );

  // Create appinfo
  vk::ApplicationInfo appInfo = vk::ApplicationInfo( applicationName, version, "Venom Engine", version, version );

  // Query extensions (glfw)
  // glfwinit is needed for glfw based vulkan extensions loading
  if ( !glfwInit() ) {
    throw std::runtime_error(
        "glfw: Could not initialize glfw. glfwInit() is needed for glfw based vulkan extensions loading" );
  }

  std::cout << "Extensions to be required:\n";
  for ( const char* extensionName : extensions ) {
    std::cout << "\t\"" << extensionName << "\"\n";
  }

  // If not supported then no point in creating instance
  if ( !supported( extensions, layers ) ) {
    std::cerr << "Extensions or layers not supported.\n";
    return nullptr;
  }

  // Create instance info with required extensions
  vk::InstanceCreateInfo createInfo =
      vk::InstanceCreateInfo( vk::InstanceCreateFlags(), &appInfo,                          // App info
                              static_cast<uint32_t>( layers.size() ), layers.data(),        // Enabled layers
                              static_cast<uint32_t>( extensions.size() ), extensions.data() // Enabled extensions
      );

  // Create vulkan instance
  vk::Instance instance = vk::createInstance( createInfo );

  return instance;
}

int getDevicePriority( vk::PhysicalDevice& physicalDevice ) {
  // Get properties
  vk::PhysicalDeviceProperties properties = physicalDevice.getProperties();

  switch ( properties.deviceType ) {
  case ( vk::PhysicalDeviceType::eCpu ):
    return -1;

  case ( vk::PhysicalDeviceType::eDiscreteGpu ):
    return 2;

  case ( vk::PhysicalDeviceType::eIntegratedGpu ):
    return 1;

  case ( vk::PhysicalDeviceType::eVirtualGpu ):
    return 0;

  case ( vk::PhysicalDeviceType::eOther ):
    return -1;
  }

  return -1;
}

void logDeviceProperties( vk::PhysicalDevice& device ) {
  // Get properties
  vk::PhysicalDeviceProperties properties = device.getProperties();

  // Log properties
  std::cout << "================================================================================\n";
  std::cout << "Device name: " << properties.deviceName << std::endl;
  std::cout << "Device type: ";
  switch ( properties.deviceType ) {
  case ( vk::PhysicalDeviceType::eCpu ):
    std::cout << "CPU";
    break;

  case ( vk::PhysicalDeviceType::eDiscreteGpu ):
    std::cout << "Discrete GPU";
    break;

  case ( vk::PhysicalDeviceType::eIntegratedGpu ):
    std::cout << "Integrated GPU";
    break;

  case ( vk::PhysicalDeviceType::eVirtualGpu ):
    std::cout << "Virtual GPU";
    break;

  case ( vk::PhysicalDeviceType::eOther ):
    std::cout << "Other";
    break;
  }
  std::cout << std::endl;
  std::cout << "Priority: " << getDevicePriority( device );
  std::cout << std::endl;
  std::cout << "================================================================================\n";
}


bool isDeviceSuitable( vk::PhysicalDevice& device, std::vector<const char*> requiredExensions ) {

  //  // converting vector<const char*> to vector<string>
  //  std::vector<std::string> requiredExensionsStrings( requiredExensions.begin(),
  //                                                     requiredExensions.begin() + requiredExensions.size() );

  for ( const char* requiredExension : requiredExensions ) {
    bool found = false;
    for ( vk::ExtensionProperties& extension : device.enumerateDeviceExtensionProperties() ) {
      if ( std::strcmp( requiredExension, extension.extensionName ) == 0 ) {
        std::cout << requiredExension << " is supported.\n";
        found = true;
        break;
      }
    }
    if ( !found ) {
      std::cout << "ERROR: Required extension \"" << requiredExension << "\" is not supported.\n";
      std::cout << "ERROR: Device \"" << device.getProperties().deviceName << "\" is not suitable.\n";
      return false;
    }
  }

  // If survived this long then all good
  std::cout << "All extensions supported. [OK]\n";
  std::cout << "Device \"" << device.getProperties().deviceName << "\" is suitable. [OK]\n";
  return true;
}

vk::PhysicalDevice vkChoosePhysicalDevice( vk::Instance& instance ) {
  std::cout << "Choosing Physical device\n";

  // Query the system for available devices
  std::vector<vk::PhysicalDevice> availableDevices = instance.enumeratePhysicalDevices();

  std::cout << "There are " << availableDevices.size() << " physical devices available on this system.\n";

  // Required extensions (swapchain should be a must have for our case)
  const std::vector<const char*> requiredExensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

  // Log required extensions
  std::cout << "Following extensions will be requested:\n";
  for ( const char* extension : requiredExensions ) {
    std::cout << "\t\"" << extension << "\"\n";
  }

  // Choose a suitable device
  vk::PhysicalDevice selectedDevice = nullptr;
  int                maxPriority    = -1;
  for ( vk::PhysicalDevice device : availableDevices ) {
    // Log device properties
    logDeviceProperties( device );

    if ( isDeviceSuitable( device, requiredExensions ) && getDevicePriority( device ) > maxPriority ) {
      selectedDevice = device;
      maxPriority    = getDevicePriority( device );
    }
  }

  std::cout << "================================================================================\n";
  std::cout << "Selected device: " << selectedDevice.getProperties().deviceName << std::endl;
  std::cout << "================================================================================\n";

  return selectedDevice;
}

QueueFamilyIndices vkFindQueueFamilies( vk::PhysicalDevice device, vk::SurfaceKHR surface ) {
  QueueFamilyIndices indices;

  std::vector<vk::QueueFamilyProperties> queueFamilies = device.getQueueFamilyProperties();

  uint32_t i = 0;
  for ( vk::QueueFamilyProperties queueFamily : queueFamilies ) {
    if ( queueFamily.queueFlags & vk::QueueFlagBits::eGraphics && !indices.graphicsFamily.has_value() ) {
      indices.graphicsFamily = i;

      std::cout << "Selected graphics family: " << i << std::endl;
    }

    if ( device.getSurfaceSupportKHR( i, surface ) && !indices.presentFamily.has_value() ) {
      indices.presentFamily = i;

      std::cout << "Selected present family: " << i << std::endl;
    }

    i++;
  }

  return indices;
}

void logTransformBits( vk::SurfaceTransformFlagsKHR bits ) {
  if ( bits & vk::SurfaceTransformFlagBitsKHR::eIdentity ) {
    std::cout << "Identity" << std::endl;
  }

  if ( bits & vk::SurfaceTransformFlagBitsKHR::eHorizontalMirror ) {
    std::cout << "Horizontal Mirror" << std::endl;
  }

  if ( bits & vk::SurfaceTransformFlagBitsKHR::eHorizontalMirrorRotate90 ) {
    std::cout << "Horizontal Mirror Rotate 90" << std::endl;
  }

  if ( bits & vk::SurfaceTransformFlagBitsKHR::eHorizontalMirrorRotate180 ) {
    std::cout << "Horizontal Mirror Rotate 180" << std::endl;
  }

  if ( bits & vk::SurfaceTransformFlagBitsKHR::eHorizontalMirrorRotate270 ) {
    std::cout << "Horizontal Mirror Rotate 270" << std::endl;
  }

  if ( bits & vk::SurfaceTransformFlagBitsKHR::eRotate90 ) {
    std::cout << "Rotate 90" << std::endl;
  }

  if ( bits & vk::SurfaceTransformFlagBitsKHR::eRotate180 ) {
    std::cout << "Rotate 180" << std::endl;
  }

  if ( bits & vk::SurfaceTransformFlagBitsKHR::eRotate270 ) {
    std::cout << "Rotate 270" << std::endl;
  }

  if ( bits & vk::SurfaceTransformFlagBitsKHR::eInherit ) {
    std::cout << "Inherit" << std::endl;
  }
}

void logAlphaCompositeBits( vk::CompositeAlphaFlagsKHR bits ) {
  if ( bits & vk::CompositeAlphaFlagBitsKHR::eOpaque ) {
    std::cout << "Opaque" << std::endl;
  }

  if ( bits & vk::CompositeAlphaFlagBitsKHR::eInherit ) {
    std::cout << "Inherit" << std::endl;
  }

  if ( bits & vk::CompositeAlphaFlagBitsKHR::ePreMultiplied ) {
    std::cout << "Pre multiplied" << std::endl;
  }

  if ( bits & vk::CompositeAlphaFlagBitsKHR::ePostMultiplied ) {
    std::cout << "Post multiplied" << std::endl;
  }
}

void logImageUsageBits( vk::ImageUsageFlags bits ) {
  if ( bits & vk::ImageUsageFlagBits::eShadingRateImageNV ) {
    std::cout << "eShadingRateImageNV" << std::endl;
  }

  if ( bits & vk::ImageUsageFlagBits::eAttachmentFeedbackLoopEXT ) {
    std::cout << "eAttachmentFeedbackLoopEXT" << std::endl;
  }

  if ( bits & vk::ImageUsageFlagBits::eColorAttachment ) {
    std::cout << "eColorAttachment" << std::endl;
  }

  if ( bits & vk::ImageUsageFlagBits::eColorAttachment ) {
    std::cout << "eColorAttachment" << std::endl;
  }

  if ( bits & vk::ImageUsageFlagBits::eDepthStencilAttachment ) {
    std::cout << "eDepthStencilAttachment" << std::endl;
  }

  if ( bits & vk::ImageUsageFlagBits::eFragmentDensityMapEXT ) {
    std::cout << "eFragmentDensityMapEXT" << std::endl;
  }

  if ( bits & vk::ImageUsageFlagBits::eFragmentShadingRateAttachmentKHR ) {
    std::cout << "eFragmentShadingRateAttachmentKHR" << std::endl;
  }

  if ( bits & vk::ImageUsageFlagBits::eInputAttachment ) {
    std::cout << "eInputAttachment" << std::endl;
  }

  if ( bits & vk::ImageUsageFlagBits::eSampled ) {
    std::cout << "eSampled" << std::endl;
  }

  if ( bits & vk::ImageUsageFlagBits::eStorage ) {
    std::cout << "eStorage" << std::endl;
  }

  if ( bits & vk::ImageUsageFlagBits::eTransferSrc ) {
    std::cout << "eTransferSrc" << std::endl;
  }

  if ( bits & vk::ImageUsageFlagBits::eTransferDst ) {
    std::cout << "eTransferDst" << std::endl;
  }
}

SwapchainSupportDetails vkQuerySwapchainSupport( vk::PhysicalDevice& device, vk::SurfaceKHR& surface ) {
  SwapchainSupportDetails support;

  // CAPABILITIES
  support.capabilities = device.getSurfaceCapabilitiesKHR( surface );

  std::cout << "Swapchain can support the following surface capabilites:\n";
  std::cout << "\tMinimum image count: " << support.capabilities.minImageCount << std::endl;
  std::cout << "\tMaximum image count: " << support.capabilities.maxImageCount << std::endl;
  std::cout << "\tCurrent extent:\n";
  std::cout << "\t\tWidth: " << support.capabilities.currentExtent.width << std::endl;
  std::cout << "\t\tHeight: " << support.capabilities.currentExtent.height << std::endl;
  std::cout << "\t\tMinimum width: " << support.capabilities.minImageExtent.width << std::endl;
  std::cout << "\t\tMinimum height: " << support.capabilities.minImageExtent.height << std::endl;
  std::cout << "\t\tMaximum width: " << support.capabilities.maxImageExtent.width << std::endl;
  std::cout << "\t\tMaximum height: " << support.capabilities.maxImageExtent.height << std::endl;

  std::cout << "\tMaximum image array layers: " << support.capabilities.maxImageArrayLayers << std::endl;

  std::cout << "\tCurrent transform:\n";
  logTransformBits( support.capabilities.currentTransform );

  std::cout << "\tSupported alpha composite bits:\n";
  logAlphaCompositeBits( support.capabilities.supportedCompositeAlpha );

  std::cout << "\tSupported image usage bits:\n";
  logImageUsageBits( support.capabilities.supportedUsageFlags );

  // FORMATS
  support.formats = device.getSurfaceFormatsKHR( surface );
  for ( vk::SurfaceFormatKHR supportedFormat : support.formats ) {
    std::cout << "Supported pixel format: " << vk::to_string( supportedFormat.format ) << std::endl;
    std::cout << "Supported color space: " << vk::to_string( supportedFormat.colorSpace ) << std::endl;
  }

  // PRESENT MODES
  // TODO: some similar logging shit

  return support;
}

SwapchainBundle vkCreateSwapchain( vk::Device logicalDevice, vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface,
                                   uint32_t width, uint32_t height ) {
  SwapchainSupportDetails support = vkQuerySwapchainSupport( physicalDevice, surface );

  // Choose swapchain surface format
  vk::SurfaceFormatKHR chosenFormat = support.formats[0]; // First one is the default
  for ( vk::SurfaceFormatKHR format : support.formats ) {
    if ( format.format == vk::Format::eB8G8R8A8Unorm && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear ) {
      chosenFormat = format;
    }
  }

  // Choose swapchain present mode
  vk::PresentModeKHR chosenPresentMode = vk::PresentModeKHR::eFifo; // Fifo is the default
  for ( vk::PresentModeKHR presentMode : support.presentModes ) {
    if ( presentMode == vk::PresentModeKHR::eMailbox ) {
      chosenPresentMode = presentMode;
    }
  }

  // Choose swapchain extent
  vk::Extent2D chosenExtent;
  if ( support.capabilities.currentExtent.width != UINT32_MAX ) {
    chosenExtent = support.capabilities.currentExtent;
  } else {
    vk::Extent2D extent = { width, height };
  }

  uint32_t imageCount = std::min( support.capabilities.maxImageCount, support.capabilities.minImageCount + 1 );

  // Create swapchain createinfo
  vk::SwapchainCreateInfoKHR createInfo =
      vk::SwapchainCreateInfoKHR( vk::SwapchainCreateFlagsKHR(), surface, imageCount, chosenFormat.format,
                                  chosenFormat.colorSpace, chosenExtent, 1, vk::ImageUsageFlagBits::eColorAttachment );

  QueueFamilyIndices indices              = vkFindQueueFamilies( physicalDevice, surface );
  uint32_t           queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

  if ( indices.graphicsFamily.value() != indices.presentFamily.value() ) {
    createInfo.imageSharingMode      = vk::SharingMode::eConcurrent;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices   = queueFamilyIndices;
  } else {
    createInfo.imageSharingMode = vk::SharingMode::eExclusive;
  }

  createInfo.preTransform   = support.capabilities.currentTransform;
  createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
  createInfo.presentMode    = chosenPresentMode;
  createInfo.clipped        = VK_TRUE;

  createInfo.oldSwapchain = vk::SwapchainKHR( nullptr ); // Inheritance

  SwapchainBundle bundle {};
  try {
    bundle.swapchain = logicalDevice.createSwapchainKHR( createInfo );
  } catch ( vk::SystemError err ) {
    throw std::runtime_error( "Failed to create swapchain." );
  }

  std::vector<vk::Image> images = logicalDevice.getSwapchainImagesKHR( bundle.swapchain );
  bundle.frames.resize( images.size() );
  for ( size_t i = 0; i < images.size(); i++ ) {
    vk::ImageViewCreateInfo createInfo         = {};
    createInfo.image                           = images[i];
    createInfo.viewType                        = vk::ImageViewType::e2D;
    createInfo.components.r                    = vk::ComponentSwizzle::eIdentity;
    createInfo.components.g                    = vk::ComponentSwizzle::eIdentity;
    createInfo.components.b                    = vk::ComponentSwizzle::eIdentity;
    createInfo.components.a                    = vk::ComponentSwizzle::eIdentity;
    createInfo.subresourceRange.aspectMask     = vk::ImageAspectFlagBits::eColor;
    createInfo.subresourceRange.baseMipLevel   = 0;
    createInfo.subresourceRange.levelCount     = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount     = 1;
    createInfo.format                          = chosenFormat.format;

    bundle.frames[i].image     = images[i];
    bundle.frames[i].imageView = logicalDevice.createImageView( createInfo );
  }

  bundle.format = chosenFormat.format;
  bundle.extent = chosenExtent;

  return bundle;
}
} // namespace utils

// Pipeline create stuff
namespace utils {
struct GraphicsPipelineInBundle {
  vk::Device   device;
  std::string  vertexFilepath;
  std::string  fragmentFilepath;
  vk::Extent2D swapchainExtent;
  vk::Format   swapchainImageFormat;
};

struct GraphicsPipelineOutBundle {
  vk::PipelineLayout layout;
  vk::RenderPass     renderPass;
  vk::Pipeline       pipeline;
};

vk::RenderPass makeRenderPass( vk::Device device, vk::Format swapchainImageFormat ) {
  vk::AttachmentDescription colorAttachment = {};
  colorAttachment.flags                     = vk::AttachmentDescriptionFlags();
  colorAttachment.format                    = swapchainImageFormat;
  colorAttachment.samples                   = vk::SampleCountFlagBits::e1;
  colorAttachment.loadOp                    = vk::AttachmentLoadOp::eClear;
  colorAttachment.storeOp                   = vk::AttachmentStoreOp::eStore;
  colorAttachment.stencilLoadOp             = vk::AttachmentLoadOp::eDontCare;
  colorAttachment.stencilStoreOp            = vk::AttachmentStoreOp::eDontCare;
  colorAttachment.initialLayout             = vk::ImageLayout::eUndefined;
  colorAttachment.finalLayout               = vk::ImageLayout::ePresentSrcKHR;

  vk::AttachmentReference colorAttachmentRef = {};
  colorAttachmentRef.attachment              = 0;
  colorAttachmentRef.layout                  = vk::ImageLayout::eColorAttachmentOptimal;

  // We always have atleast one subpass
  vk::SubpassDescription subpass;
  subpass.flags                = vk::SubpassDescriptionFlags();
  subpass.pipelineBindPoint    = vk::PipelineBindPoint::eGraphics;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments    = &colorAttachmentRef;

  vk::RenderPassCreateInfo renderPassInfo = {};
  renderPassInfo.flags                    = vk::RenderPassCreateFlags();
  renderPassInfo.attachmentCount          = 1;
  renderPassInfo.pAttachments             = &colorAttachment;
  renderPassInfo.subpassCount             = 1;
  renderPassInfo.pSubpasses               = &subpass;

  try {
    return device.createRenderPass( renderPassInfo );
  } catch ( vk::SystemError err ) {
    std::cerr << "Could not create render pass." << std::endl;
  }
}

vk::PipelineLayout makePipelineLayout( vk::Device device ) {
  vk::PipelineLayoutCreateInfo layoutInfo;
  layoutInfo.flags                  = vk::PipelineLayoutCreateFlags();
  layoutInfo.setLayoutCount         = 0;
  layoutInfo.pushConstantRangeCount = 0;
  try {
    return device.createPipelineLayout( layoutInfo );
  } catch ( vk::SystemError err ) {
    std::cerr << "Could not create pipeline layout." << std::endl;
  }
}


GraphicsPipelineOutBundle makeGraphicsPipeline( GraphicsPipelineInBundle specification ) {
  vk::GraphicsPipelineCreateInfo pipelineInfo;
  pipelineInfo.flags = vk::PipelineCreateFlags();

  std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;

  // Vertex input
  vk::PipelineVertexInputStateCreateInfo vertexInputInfo = {};
  vertexInputInfo.flags                                  = vk::PipelineVertexInputStateCreateFlags();
  vertexInputInfo.vertexBindingDescriptionCount          = 0;
  vertexInputInfo.vertexAttributeDescriptionCount        = 0;

  pipelineInfo.pVertexInputState = &vertexInputInfo;

  // Input assembly
  vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
  inputAssemblyInfo.flags    = vk::PipelineInputAssemblyStateCreateFlags();
  inputAssemblyInfo.topology = vk::PrimitiveTopology::eTriangleList;

  pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;

  // Vertex shader
  std::cout << "Creating vertex shader module" << std::endl;
  vk::ShaderModule vertexShader = utils::createModule( specification.vertexFilepath, specification.device );

  vk::PipelineShaderStageCreateInfo vertexShaderInfo = {};
  vertexShaderInfo.flags                             = vk::PipelineShaderStageCreateFlags();
  vertexShaderInfo.stage                             = vk::ShaderStageFlagBits::eVertex;
  vertexShaderInfo.module                            = vertexShader;
  vertexShaderInfo.pName                             = "main";

  shaderStages.push_back( vertexShaderInfo );

  // Viewport and scissor
  // a: viewport
  vk::Viewport viewport;
  viewport.x        = 0.0f;
  viewport.y        = 0.0f;
  viewport.width    = specification.swapchainExtent.width;
  viewport.height   = specification.swapchainExtent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  // b: scissor
  vk::Rect2D scissor;
  scissor.offset.x = 0.0f;
  scissor.offset.y = 0.0f;
  scissor.extent   = specification.swapchainExtent;

  vk::PipelineViewportStateCreateInfo viewportState = {};
  viewportState.flags                               = vk::PipelineViewportStateCreateFlags();
  viewportState.viewportCount                       = 1;
  viewportState.pViewports                          = &viewport;
  viewportState.scissorCount                        = 1;
  viewportState.pScissors                           = &scissor;

  pipelineInfo.pViewportState = &viewportState;

  // Rasterizer
  vk::PipelineRasterizationStateCreateInfo rasterizer = {};
  rasterizer.flags                                    = vk::PipelineRasterizationStateCreateFlags();
  rasterizer.depthClampEnable                         = VK_FALSE;
  rasterizer.rasterizerDiscardEnable                  = VK_FALSE;
  rasterizer.polygonMode                              = vk::PolygonMode::eFill;
  rasterizer.lineWidth                                = 1;
  rasterizer.cullMode                                 = vk::CullModeFlagBits::eBack;
  rasterizer.frontFace                                = vk::FrontFace::eCounterClockwise;
  rasterizer.depthBiasEnable                          = VK_FALSE;

  pipelineInfo.pRasterizationState = &rasterizer;

  // Fragment shader
  std::cout << "Creating vertex shader module" << std::endl;
  vk::ShaderModule fragmentShader = utils::createModule( specification.fragmentFilepath, specification.device );

  vk::PipelineShaderStageCreateInfo fragmentShaderInfo = {};
  fragmentShaderInfo.flags                             = vk::PipelineShaderStageCreateFlags();
  fragmentShaderInfo.stage                             = vk::ShaderStageFlagBits::eFragment;
  fragmentShaderInfo.module                            = fragmentShader;
  fragmentShaderInfo.pName                             = "main";

  shaderStages.push_back( fragmentShaderInfo );

  pipelineInfo.stageCount = shaderStages.size();
  pipelineInfo.pStages    = shaderStages.data();

  // Multisampling
  vk::PipelineMultisampleStateCreateInfo multisampling = {};
  multisampling.flags                                  = vk::PipelineMultisampleStateCreateFlags();
  multisampling.sampleShadingEnable                    = VK_FALSE;
  multisampling.rasterizationSamples                   = vk::SampleCountFlagBits::e1;

  pipelineInfo.pMultisampleState = &multisampling;

  // Color blend
  vk::PipelineColorBlendAttachmentState colorBlendAttachment = {};
  colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
                                      | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
  colorBlendAttachment.blendEnable                    = VK_FALSE;
  vk::PipelineColorBlendStateCreateInfo colorBlending = {};
  colorBlending.flags                                 = vk::PipelineColorBlendStateCreateFlags();
  colorBlending.logicOpEnable                         = VK_FALSE;
  colorBlending.logicOp                               = vk::LogicOp::eCopy;
  colorBlending.attachmentCount                       = 1;
  colorBlending.pAttachments                          = &colorBlendAttachment;
  colorBlending.blendConstants[0]                     = 0.0f;
  colorBlending.blendConstants[1]                     = 0.0f;
  colorBlending.blendConstants[2]                     = 0.0f;
  colorBlending.blendConstants[3]                     = 0.0f;

  pipelineInfo.pColorBlendState = &colorBlending;

  // Create pipeline layout
  std::cout << "Creating pipeline layout" << std::endl;
  vk::PipelineLayout layout = makePipelineLayout( specification.device );

  pipelineInfo.layout = layout;

  // Create renderpass
  std::cout << "Creating renderpass" << std::endl;
  vk::RenderPass renderPass = makeRenderPass( specification.device, specification.swapchainImageFormat );

  pipelineInfo.renderPass = renderPass;

  // Extra stuff (basePipelineHandle could be used to inherit from another pipeline)
  pipelineInfo.basePipelineHandle = nullptr;

  // Create pipeline
  std::cout << "Creating pipeline" << std::endl;
  vk::Pipeline graphicsPipeline;
  try {
    graphicsPipeline = ( specification.device.createGraphicsPipeline( nullptr, pipelineInfo ) ).value;
  } catch ( vk::SystemError err ) {
    std::cerr << "could not create pipeline" << std::endl;
  }

  GraphicsPipelineOutBundle output = {};
  output.layout                    = layout;
  output.renderPass                = renderPass;
  output.pipeline                  = graphicsPipeline;

  return output;
}
} // namespace utils
