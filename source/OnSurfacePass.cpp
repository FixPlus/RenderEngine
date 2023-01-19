#include "RE/OnSurfacePass.hpp"

namespace re {
OnSurfacePass::OnSurfacePass(Engine &engine, std::string_view name,
                             vkw::Surface &surface,
                             SwapChainCreateInfo createInfo)
    : RenderPass(engine, name),
      m_swapChain(std::make_unique<SwapChain>(engine, surface, createInfo)) {}

void OnSurfacePass::onPass(FrameContext &context) {
  // TODO
}
void OnSurfacePass::recreateSwapChain() {
  auto info = m_swapChain->info();
  auto &surface = m_swapChain->surface();
  m_swapChain.reset();
  m_swapChain = std::make_unique<SwapChain>(m_engine, surface, info);
}
void OnSurfacePass::onAddUseHook(const AttachmentUse &use) {
  RenderPass::onAddUseHook(use);
}
void OnSurfacePass::onAddDefHook(const AttachmentDef &def) {
  throw std::runtime_error("On screen pass cannot define any attachments.");
}

} // namespace re