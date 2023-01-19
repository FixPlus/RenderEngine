#ifndef RENDERENGINE_WINDOW_H
#define RENDERENGINE_WINDOW_H

#include <vulkan/vulkan.h>

#include <span>
#include <string_view>

struct GLFWwindow;

namespace re::test::utils {

struct WindowCreateInfo {
  unsigned width;
  unsigned height;
  std::string_view title;
};

class Window {
public:
  explicit Window(WindowCreateInfo const &CI);

  Window(Window const &another) = delete;
  Window(Window &&another) noexcept;

  Window &operator=(Window const &another) = delete;
  Window &operator=(Window &&another) noexcept;

  VkSurfaceKHR createSurface(VkInstance instance);

  static std::span<const char *> requiredVulkanExtensions();

  virtual ~Window();

private:
  GLFWwindow *m_window = nullptr;
};
} // namespace re::test::utils

#endif // RENDERENGINE_WINDOW_H
