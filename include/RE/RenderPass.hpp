#ifndef RENDERENGINE_RENDERPASS_HPP
#define RENDERENGINE_RENDERPASS_HPP

#include "RE/Pass.hpp"

namespace re {

class RenderPass : public Pass {
public:
  RenderPass(Engine &engine, std::string_view name) : Pass(engine, name) {}

  void onPass(FrameContext &context) override;

  bool hasDepthAttachment() const;

protected:
  void onAddUseHook(AttachmentUse const &use) override;
  void onAddDefHook(AttachmentDef const &def) override;
  bool m_hasDepthAttachment = false;
  std::set<unsigned> m_takenAttachments;
};

} // namespace re
#endif // RENDERENGINE_RENDERPASS_HPP
