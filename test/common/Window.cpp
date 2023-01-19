#include "Window.h"
#include <stdexcept>

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(__linux__)
#define GLFW_EXPOSE_NATIVE_X11
#endif

#include <vulkan/vulkan.h>

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

namespace re::test::utils {

namespace {

class GlfwLibKeeper {
public:
  GlfwLibKeeper() {
    if (glfwInit() != GLFW_TRUE) {
      throw std::runtime_error("Failed to init GLFW");
    }
  }

  ~GlfwLibKeeper() { glfwTerminate(); }
};

void init() { static GlfwLibKeeper glfwKeeper{}; }

void handleGLFWError() {
  const char *description;
  int code = glfwGetError(&description);

  if (code != GLFW_NO_ERROR)
    return;
  else {
    throw std::runtime_error(description);
  }
}
} // namespace

Window::Window(WindowCreateInfo const &CI) {
  init();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  m_window =
      glfwCreateWindow(CI.width, CI.height, CI.title.data(), nullptr, nullptr);

  if (!m_window)
    handleGLFWError();
}
Window::Window(Window &&another) noexcept : m_window(another.m_window) {
  another.m_window = nullptr;
}
Window &Window::operator=(Window &&another) noexcept {
  std::swap(m_window, another.m_window);
  return *this;
}
Window::~Window() {
  if (!m_window)
    return;
  glfwDestroyWindow(m_window);
}
VkSurfaceKHR Window::createSurface(VkInstance instance) {
  VkSurfaceKHR surface;
  auto err = glfwCreateWindowSurface(instance, m_window, nullptr, &surface);
  if (err)
    throw std::runtime_error("GLFW: Failed to create Vulkan surface");
  return surface;
}
std::span<const char *> Window::requiredVulkanExtensions() {
  uint32_t count;

  const char **extensions = glfwGetRequiredInstanceExtensions(&count);

  return {extensions, extensions + count};
}

} // namespace re::test::utils