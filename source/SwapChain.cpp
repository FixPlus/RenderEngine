#include "RE/SwapChain.hpp"
#include "vkw/CommandBuffer.hpp"
#include "vkw/CommandPool.hpp"
#include "vkw/Fence.hpp"
#include "vkw/Queue.hpp"
#include "vkw/Surface.hpp"

namespace re {
namespace {
VkPresentModeKHR choosePresentMode(vkw::Surface &surface,
                                   vkw::PhysicalDevice const &physicalDevice,
                                   VkPresentModeKHR preferred) {
  // TODO
  auto presentModes = surface.getAvailablePresentModes(physicalDevice);
  auto presentModeCount = presentModes.size();
  VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;

  for (size_t i = 0; i < presentModeCount; i++) {
    if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
      swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
      break;
    }
  }

  if (swapchainPresentMode != VK_PRESENT_MODE_MAILBOX_KHR)
    for (size_t i = 0; i < presentModeCount; i++) {
      if (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR) {
        swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
        break;
      }
    }
#if 0
        //if(createInfo.vsync)
       // swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
#endif
  return swapchainPresentMode;
}

VkFormat chooseDepthFormat(vkw::PhysicalDevice const &physicalDevice,
                           VkFormat preferred) {
  // TODO
  return VK_FORMAT_D32_SFLOAT;
}

vkw::Image<vkw::DEPTH, vkw::I2D, vkw::SINGLE>
createDepthStencilImage(vkw::Device &device, VkExtent2D extents,
                        VkFormat preferredFormat) {
  VmaAllocationCreateInfo createInfo{};
  createInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
  auto depthMap = vkw::Image<vkw::DEPTH, vkw::I2D, vkw::SINGLE>{
      device.getAllocator(),
      createInfo,
      chooseDepthFormat(device.physicalDevice(), preferredFormat),
      extents.width,
      extents.height,
      1,
      1,
      1,
      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT};

  VkImageMemoryBarrier transitLayout{};
  transitLayout.image = depthMap;
  transitLayout.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  transitLayout.pNext = nullptr;
  transitLayout.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  transitLayout.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  transitLayout.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  transitLayout.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  transitLayout.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  transitLayout.subresourceRange.baseArrayLayer = 0;
  transitLayout.subresourceRange.baseMipLevel = 0;
  transitLayout.subresourceRange.layerCount = 1;
  transitLayout.subresourceRange.levelCount = 1;
  transitLayout.dstAccessMask =
      VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
  transitLayout.srcAccessMask = 0;

  auto const &queue = device.anyTransferQueue();
  auto commandPool = vkw::CommandPool{device, 0, queue.family().index()};
  auto transferCommand = vkw::PrimaryCommandBuffer{commandPool};

  transferCommand.begin(0);
  transferCommand.imageMemoryBarrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                     VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                     {&transitLayout, 1});
  transferCommand.end();

  queue.submit(transferCommand);
  queue.waitIdle();

  return depthMap;
}

} // namespace

SwapChain::SwapChain(::re::Engine &engine, vkw::Surface &surface,
                     SwapChainCreateInfo createInfo)
    : vkw::SwapChain(engine.device(),
                     compileInfo(engine.device(), surface, createInfo)),
      m_surface(surface), m_images(retrieveImages()), m_info(createInfo),
      m_acquireSemaphore(engine.device()) {

  auto &device = engine.device();

  if (createInfo.createDepthBuffer) {
    m_depth = std::make_unique<vkw::Image<vkw::DEPTH, vkw::I2D, vkw::SINGLE>>(
        createDepthStencilImage(
            device,
            surface.getSurfaceCapabilities(device.physicalDevice())
                .currentExtent,
            createInfo.preferredDepthFormat));
    m_info.preferredDepthFormat = m_depth->format();
    m_depth_view = std::make_unique<vkw::ImageView<vkw::DEPTH, vkw::V2DA>>(
        device, *m_depth, m_depth->format());
  } else {
    m_info.preferredDepthFormat = VK_FORMAT_UNDEFINED;
  }

  boost::container::small_vector<VkImageMemoryBarrier, 3> transitLayouts;

  for (auto &image : m_images) {
    VkImageMemoryBarrier transitLayout{};
    transitLayout.image = image;
    transitLayout.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    transitLayout.pNext = nullptr;
    transitLayout.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    transitLayout.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    transitLayout.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    transitLayout.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    transitLayout.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    transitLayout.subresourceRange.baseArrayLayer = 0;
    transitLayout.subresourceRange.baseMipLevel = 0;
    transitLayout.subresourceRange.layerCount = 1;
    transitLayout.subresourceRange.levelCount = 1;
    transitLayout.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    transitLayout.srcAccessMask = 0;

    transitLayouts.push_back(transitLayout);
  }

  auto queue = device.anyGraphicsQueue();
  auto commandPool = vkw::CommandPool{device, 0, queue.family().index()};
  auto commandBuffer = vkw::PrimaryCommandBuffer{commandPool};

  commandBuffer.begin(0);

  commandBuffer.imageMemoryBarrier(
      VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
      std::span<const VkImageMemoryBarrier>(transitLayouts.data(),
                                            transitLayouts.size()));

  commandBuffer.end();

  auto fence = vkw::Fence{device};

  auto submitInfo = vkw::SubmitInfo(commandBuffer);

  queue.submit(submitInfo, fence);
  fence.wait();

  VkComponentMapping mapping;
  mapping.r = VK_COMPONENT_SWIZZLE_IDENTITY;
  mapping.g = VK_COMPONENT_SWIZZLE_IDENTITY;
  mapping.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  mapping.a = VK_COMPONENT_SWIZZLE_IDENTITY;

  for (auto &image : m_images) {
    m_image_views.emplace_back(device, image, image.format(), 0u, 1u, 0u, 1u,
                               mapping);
  }
}

VkSwapchainCreateInfoKHR
SwapChain::compileInfo(vkw::Device &device, vkw::Surface &surface,
                       SwapChainCreateInfo &createInfo) {
  VkSwapchainCreateInfoKHR ret{};

  auto &physicalDevice = device.physicalDevice();
  auto availablePresentQueues =
      surface.getQueueFamilyIndicesThatSupportPresenting(physicalDevice);

  // TODO: rewrite to this library's exceptions
  if (!device.physicalDevice().extensionSupported(vkw::ext::KHR_swapchain) ||
      availablePresentQueues.empty())
    throw vkw::Error(
        "Cannot create SwapChain on device that does not support presenting");

  auto surfCaps = surface.getSurfaceCapabilities(physicalDevice);

  ret.surface = surface;
  ret.clipped = VK_TRUE;
  ret.imageExtent = surfCaps.currentExtent;
  ret.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  ret.imageArrayLayers = 1;
  ret.presentMode = surface.getAvailablePresentModes(physicalDevice).front();
  ret.queueFamilyIndexCount = 0;
  // TODO: actually use passed options to choose format
  ret.imageColorSpace =
      surface.getAvailableFormats(physicalDevice).front().colorSpace;
  createInfo.preferredPixelFormat = ret.imageFormat =
      surface.getAvailableFormats(physicalDevice).front().format;
  ret.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  createInfo.imageCount = ret.minImageCount = surfCaps.minImageCount;
  ret.preTransform = surfCaps.currentTransform;
  ret.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  createInfo.presentMode =
      choosePresentMode(surface, physicalDevice, createInfo.presentMode);

  // Find a supported composite alpha format (not all devices support alpha
  // opaque)
  VkCompositeAlphaFlagBitsKHR compositeAlpha =
      VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  // Simply select the first composite alpha format available
  std::array<VkCompositeAlphaFlagBitsKHR, 4> compositeAlphaFlags = {
      VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
      VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
      VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
      VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
  };
  for (auto &compositeAlphaFlag : compositeAlphaFlags) {
    if (surfCaps.supportedCompositeAlpha & compositeAlphaFlag) {
      compositeAlpha = compositeAlphaFlag;
      break;
    };
  }

  ret.compositeAlpha = compositeAlpha;
  ret.presentMode = createInfo.presentMode;

  return ret;
}
void SwapChain::acquireNextImage() {
  vkw::SwapChain::acquireNextImage(m_acquireSemaphore);
}
} // namespace re