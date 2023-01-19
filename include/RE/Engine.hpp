#ifndef RENDERENGINE_ENGINE_HPP
#define RENDERENGINE_ENGINE_HPP

#include "vkw/Device.hpp"

namespace re {

class Engine {
public:
  explicit Engine(vkw::Device &device);

  auto &device() const { return m_device.get(); }

private:
  std::reference_wrapper<vkw::Device> m_device;
};
} // namespace re
#endif // RENDERENGINE_ENGINE_HPP
