#ifndef RENDERENGINE_TESTCONTEXT_H
#define RENDERENGINE_TESTCONTEXT_H

#include "RE/Engine.hpp"
#include "Window.h"
#include <vkw/Device.hpp>
#include <vkw/Instance.hpp>
#include <vkw/PhysicalDevice.hpp>
#include <vkw/Surface.hpp>
namespace re::test {

struct RawContext {
  std::unique_ptr<utils::Window> window;
  std::unique_ptr<vkw::Library> vk_lib;
  std::unique_ptr<vkw::Instance> vk_instance;
  std::unique_ptr<vkw::Device> device;
  std::unique_ptr<vkw::Surface> surface;
  std::unique_ptr<Engine> engine;
};

class TestContext final {
public:
  explicit TestContext(RawContext context) : m_context(std::move(context)) {}

  auto &instance() const { return *m_context.vk_instance; }

  auto &device() const { return *m_context.device; }

  auto &window() const { return *m_context.window; }

  auto &surface() const { return *m_context.surface; }

  auto &engine() const { return *m_context.engine; }

  auto &instance() { return *m_context.vk_instance; }

  auto &device() { return *m_context.device; }

  auto &window() { return *m_context.window; }

  auto &surface() { return *m_context.surface; }

  auto &engine() { return *m_context.engine; }

private:
  RawContext m_context;
};

class TestContextCreator {
public:
  TestContext create(bool verbose = false, std::ostream *pstream = nullptr);

  virtual ~TestContextCreator() = default;

protected:
  virtual void fillInstanceCreateInfo(vkw::Library &vkLib,
                                      vkw::InstanceCreateInfo &info) = 0;
  virtual void
  fillPhysicalDeviceProperties(vkw::PhysicalDevice &physicalDevice) = 0;
  virtual void fillWindowCreateInfo(utils::WindowCreateInfo &info) = 0;
};

class BasicContextCreator : public TestContextCreator {
protected:
  void fillInstanceCreateInfo(vkw::Library &vkLib,
                              vkw::InstanceCreateInfo &info) override;
  void
  fillPhysicalDeviceProperties(vkw::PhysicalDevice &physicalDevice) override;
  void fillWindowCreateInfo(utils::WindowCreateInfo &info) override;
};

} // namespace re::test
#endif // RENDERENGINE_TESTCONTEXT_H
