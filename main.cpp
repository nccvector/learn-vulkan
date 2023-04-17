#include "utils.h"
#include <GLFW/glfw3.h> #include <asm-generic/errno.h>
class Application {
  public:
  Application() {
    this->initWindow();
    this->initVulkan();
  }

  ~Application() {
    mVkDevice.destroyPipeline( mVkPipeline );
    mVkDevice.destroyPipelineLayout( mVkLayout );
    mVkDevice.destroyRenderPass( mVkRenderPass );

    for ( utils::SwapchainFrame frame : mVkSwapchainFrames ) {
      mVkDevice.destroyImageView( frame.imageView );
    }

    mVkDevice.destroySwapchainKHR( mVkSwapchain );
    mVkDevice.destroy();
    mVkInstance.destroySurfaceKHR( mVkSurface );
    mVkInstance.destroyDebugUtilsMessengerEXT( mVkDebugMessenger, nullptr, mVkDldi );
    mVkInstance.destroy();
    glfwTerminate();
  }

  private:
  void initVulkan() {
    // CREATE INSTANCE (with extensions and debug layers)
    // Required extensions and layers
    uint32_t                 glfwExtensionCount = 0;
    const char**             glfwExtensions     = glfwGetRequiredInstanceExtensions( &glfwExtensionCount );
    std::vector<const char*> requiredExtensions( glfwExtensions, glfwExtensions + glfwExtensionCount );
    requiredExtensions.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );
    std::vector<const char*> requiredLayers = { "VK_LAYER_KHRONOS_validation" };
    mVkInstance       = utils::vkCreateInstance( "My Application", requiredExtensions, requiredLayers );
    mVkDldi           = vk::DispatchLoaderDynamic( mVkInstance, vkGetInstanceProcAddr );
    mVkDebugMessenger = utils::vkCreateDebugUtilsMessengerEXT( mVkInstance, mVkDldi );
    VkSurfaceKHR c_style_surface;
    if ( glfwCreateWindowSurface( mVkInstance, mWindow, nullptr, &c_style_surface ) != VK_SUCCESS ) {
      std::cout << "Could not create window surface.\n";
    }
    mVkSurface = c_style_surface;

    // CHOOSE PHYSICAL DEVICE
    mVkPhysicalDevice = utils::vkChoosePhysicalDevice( mVkInstance );

    // CREATE LOGICAL DEVICE
    utils::QueueFamilyIndices indices            = utils::vkFindQueueFamilies( mVkPhysicalDevice, mVkSurface );
    std::vector<uint32_t>     uniqueQueueIndices = { indices.graphicsFamily.value() };
    if ( indices.graphicsFamily.value() != indices.presentFamily.value() ) {
      uniqueQueueIndices.push_back( indices.presentFamily.value() );
    }

    float                                  queuePriority = 1.0f;
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfo;
    for ( uint32_t queueFamilyIndex : uniqueQueueIndices ) {
      queueCreateInfo.push_back(
          vk::DeviceQueueCreateInfo( vk::DeviceQueueCreateFlags(), queueFamilyIndex, 1, &queuePriority ) );
    }

    // Specify deviceExtensions (Swapchain)
    std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    vk::PhysicalDeviceFeatures deviceFeatures = vk::PhysicalDeviceFeatures();
    // deviceFeatures.samplerAnisotropy          = true;
    vk::DeviceCreateInfo deviceInfo =
        vk::DeviceCreateInfo( vk::DeviceCreateFlags(),                          // Flags
                              queueCreateInfo.size(), queueCreateInfo.data(),   // QueueInfo
                              requiredLayers.size(), requiredLayers.data(),     // Layers
                              deviceExtensions.size(), deviceExtensions.data(), // Device extensions
                              &deviceFeatures );
    try {
      mVkDevice = mVkPhysicalDevice.createDevice( deviceInfo );
    } catch ( vk::SystemError err ) {
      std::cout << "Device create failed.\n";
    }
    mVkGraphicsQueue = mVkDevice.getQueue( indices.graphicsFamily.value(), 0 );
    mVkPresentQueue  = mVkDevice.getQueue( indices.presentFamily.value(), 0 );

    // Creating swapchain
    utils::SwapchainBundle bundle =
        utils::vkCreateSwapchain( mVkDevice, mVkPhysicalDevice, mVkSurface, mWidth, mHeight );
    mVkSwapchain       = bundle.swapchain;
    mVkSwapchainFrames = bundle.frames;
    mVkSwapchainFormat = bundle.format;
    mVkSwapchainExtent = bundle.extent;

    // CREATE PIPELINE
    utils::GraphicsPipelineInBundle specification = {};
    specification.device                          = mVkDevice;
    specification.vertexFilepath                  = "shaders/vert.spv";
    specification.fragmentFilepath                = "shaders/frag.spv";
    specification.swapchainExtent                 = mVkSwapchainExtent;
    specification.swapchainImageFormat            = mVkSwapchainFormat;
    utils::GraphicsPipelineOutBundle output       = utils::makeGraphicsPipeline( specification );

    mVkLayout     = output.layout;
    mVkRenderPass = output.renderPass;
    mVkPipeline   = output.pipeline;
  }

  void initWindow() {
    // glfwinit is needed for glfw based vulkan extensions loading
    if ( !glfwInit() ) {
      throw std::runtime_error( "glfw: Could not initialize glfw." );
    }
    // No default rendering client, we will hook vulkan later...
    glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
    // Support resizing in swapchain before allowing here...
    glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE );

    mWindow = glfwCreateWindow( mWidth, mHeight, "Vulkan Application", nullptr, nullptr );
    if ( !mWindow ) {
      throw std::runtime_error( "glfw: Could not create window." );
    }
  }

  private:
  uint32_t    mWidth { 800 };
  uint32_t    mHeight { 600 };
  GLFWwindow* mWindow { nullptr };

  // Vulkan vars
  // Instance related vars
  vk::Instance               mVkInstance { nullptr };
  vk::DebugUtilsMessengerEXT mVkDebugMessenger { nullptr };
  vk::DispatchLoaderDynamic  mVkDldi;
  vk::SurfaceKHR             mVkSurface;
  // Device related vars
  vk::PhysicalDevice mVkPhysicalDevice { nullptr };
  vk::Device         mVkDevice { nullptr };
  vk::Queue          mVkGraphicsQueue { nullptr };
  vk::Queue          mVkPresentQueue { nullptr };
  // Swapchain related vars
  vk::SwapchainKHR                   mVkSwapchain;
  std::vector<utils::SwapchainFrame> mVkSwapchainFrames;
  vk::Format                         mVkSwapchainFormat;
  vk::Extent2D                       mVkSwapchainExtent;
  // Pipeline related vars
  vk::PipelineLayout mVkLayout;
  vk::RenderPass     mVkRenderPass;
  vk::Pipeline       mVkPipeline;
};

int main() {
  Application app;

  return 0;
}
