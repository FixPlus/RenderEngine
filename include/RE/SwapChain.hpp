#ifndef RENDERENGINE_SWAPCHAIN_HPP
#define RENDERENGINE_SWAPCHAIN_HPP

#include "RE/Engine.hpp"
#include "vkw/Image.hpp"
#include "vkw/Semaphore.hpp"
#include "vkw/SwapChain.hpp"

namespace re {

struct SwapChainCreateInfo {
  unsigned imageCount = 2;
  bool createDepthBuffer = true;
  VkFormat preferredPixelFormat = VK_FORMAT_UNDEFINED;
  VkFormat preferredDepthFormat = VK_FORMAT_UNDEFINED;
  VkPresentModeKHR presentMode = VK_PRESENT_MODE_MAX_ENUM_KHR;
};

class SwapChain : public vkw::SwapChain {
public:
  using ImageViewsContainerT =
      boost::container::small_vector<vkw::ImageView<vkw::COLOR, vkw::V2DA>, 3>;

  ImageViewsContainerT const &attachments() const { return m_image_views; };

  vkw::ImageView<vkw::DEPTH, vkw::V2DA> const *depthAttachment() const {
    return m_depth_view.get();
  }

  bool hasDepthAttachment() const { return *m_depth_view; }

  SwapChain(::re::Engine &engine, vkw::Surface &surface,
            SwapChainCreateInfo createInfo);

  auto presentMode() const { return m_info.presentMode; }

  auto pixelFormat() const { return m_info.preferredPixelFormat; }

  auto depthFormat() const { return m_info.preferredDepthFormat; }

  auto &info() const { return m_info; }

  auto &surface() const { return m_surface; }

  void acquireNextImage();

  auto &semaphore() const { return m_acquireSemaphore; }

private:
  ImageViewsContainerT m_image_views;
  boost::container::small_vector<vkw::SwapChainImage, 3> m_images;
  std::unique_ptr<vkw::Image<vkw::DEPTH, vkw::I2D, vkw::SINGLE>> m_depth;
  std::unique_ptr<vkw::ImageView<vkw::DEPTH, vkw::V2DA>> m_depth_view;
  static VkSwapchainCreateInfoKHR compileInfo(vkw::Device &device,
                                              vkw::Surface &surface,
                                              SwapChainCreateInfo &createInfo);

  SwapChainCreateInfo m_info;
  std::reference_wrapper<vkw::Surface> m_surface;
  vkw::Semaphore m_acquireSemaphore;
};
} // namespace re
#endif // RENDERENGINE_SWAPCHAIN_HPP
