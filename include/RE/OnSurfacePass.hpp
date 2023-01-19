#ifndef RENDERENGINE_ONSURFACEPASS_HPP
#define RENDERENGINE_ONSURFACEPASS_HPP
#include "RE/RenderPass.hpp"
#include "RE/SwapChain.hpp"
#include "vkw/Surface.hpp"

namespace re {

class OnSurfacePass : public RenderPass {
public:
  OnSurfacePass(Engine &engine, std::string_view name, vkw::Surface &surface,
                SwapChainCreateInfo createInfo);

  void onPass(FrameContext &context) override;

  auto &swapChain() const { return *m_swapChain; }

  void recreateSwapChain();

protected:
  void onAddUseHook(AttachmentUse const &use) override;
  void onAddDefHook(AttachmentDef const &def) override;

private:
  std::unique_ptr<SwapChain> m_swapChain;
};

} // namespace re
#endif // RENDERENGINE_ONSURFACEPASS_HPP
