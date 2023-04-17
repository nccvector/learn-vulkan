#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <string>

struct WindowData
{
  WindowData( GLFWwindow * wnd, std::string const & name, vk::Extent2D const & extent );
  WindowData( const WindowData & ) = delete;
  WindowData( WindowData && other );
  ~WindowData() noexcept;

  GLFWwindow* handle;
  std::string  name;
  vk::Extent2D extent;
};
