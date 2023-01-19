#include "TestContext.h"

#include <vkw/Layers.hpp>

namespace re::test {

namespace {
class TCCMessagePoster {
public:
  TCCMessagePoster(std::ostream &stream) : m_stream(stream){};

  std::ostream &message() {
    m_stream.get() << "\n[VERBOSE] - ";
    return m_stream.get();
  }

private:
  std::reference_wrapper<std::ostream> m_stream;
};
} // namespace
TestContext TestContextCreator::create(bool verbose, std::ostream *pstream) {
  assert((!verbose || pstream) &&
         "Stream must be specified if verbose is enabled");
  std::optional<TCCMessagePoster> msgPoster;
  if (verbose)
    msgPoster.emplace(*pstream);

  RawContext context;
  utils::WindowCreateInfo windowCI{};

  fillWindowCreateInfo(windowCI);
  if (verbose) {
    msgPoster.value().message() << "Context creation started...";
    msgPoster.value().message() << "Creating window...";
  }
  context.window = std::make_unique<utils::Window>(windowCI);
  if (verbose) {
    msgPoster.value().message() << "Window created OK";
    msgPoster.value().message() << "Loading vulkan library...";
  }
  context.vk_lib = std::make_unique<vkw::Library>();
  if (verbose) {
    msgPoster.value().message() << "Vulkan library loaded OK";
    msgPoster.value().message() << "Creating vulkan instance...";
  }
  vkw::InstanceCreateInfo instanceCI;
  auto surfaceExts = utils::Window::requiredVulkanExtensions();

  for (auto &ext : surfaceExts) {
    instanceCI.requestExtension(vkw::Library::ExtensionId(ext));
  }

  fillInstanceCreateInfo(*context.vk_lib, instanceCI);
  context.vk_instance =
      std::make_unique<vkw::Instance>(*context.vk_lib, instanceCI);

  if (verbose) {
    msgPoster.value().message() << "Vulkan instance created OK";
    msgPoster.value().message() << "Creating surface...";
  }

  context.surface = std::make_unique<vkw::Surface>(
      *context.vk_instance,
      context.window->createSurface(*context.vk_instance));
  auto &surface = *context.surface;

  if (verbose) {
    msgPoster.value().message() << "Surface created OK";
    msgPoster.value().message() << "Creating device...";
  }

  // TODO: make it possible to pick specific device
  auto devs = context.vk_instance->enumerateAvailableDevices();

  auto found = std::find_if(devs.begin(), devs.end(), [&surface](auto &dev) {
    auto presentQueues =
        surface.getQueueFamilyIndicesThatSupportPresenting(dev);
    if (presentQueues.empty())
      return false;
    dev.queueFamilies().at(0).requestQueue();
    return true;
  });

  if (found == devs.end())
    throw std::runtime_error(
        "[Test]: no vulkan device with present support available");

  auto dev = *found;

  dev.enableExtension(vkw::ext::KHR_swapchain);

  fillPhysicalDeviceProperties(dev);
  context.device = std::make_unique<vkw::Device>(*context.vk_instance, dev);

  if (verbose) {
    msgPoster.value().message() << "Device created OK";
    msgPoster.value().message() << "Creating engine...";
  }

  context.engine = std::make_unique<Engine>(*context.device);

  if (verbose) {
    msgPoster.value().message() << "Engine created OK";
    msgPoster.value().message() << "Context creation finished OK";
  }

  return TestContext{std::move(context)};
}

void BasicContextCreator::fillInstanceCreateInfo(
    vkw::Library &vkLib, vkw::InstanceCreateInfo &info) {
  info.requestApiVersion(vkLib.instanceAPIVersion());
  if (vkLib.hasLayer(vkw::layer::KHRONOS_validation)) {
    info.requestLayer(vkw::layer::KHRONOS_validation);
    info.requestExtension(vkw::ext::EXT_debug_utils);
  }
}
void BasicContextCreator::fillWindowCreateInfo(utils::WindowCreateInfo &info) {
  info.title = "test";
  info.width = 800;
  info.height = 600;
}
void BasicContextCreator::fillPhysicalDeviceProperties(
    vkw::PhysicalDevice &physicalDevice) {}
} // namespace re::test