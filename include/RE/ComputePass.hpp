#ifndef RENDERENGINE_COMPUTEPASS_HPP
#define RENDERENGINE_COMPUTEPASS_HPP

#include "RE/Pass.hpp"

namespace re {

class ComputePass : public Pass {
public:
  ComputePass(Engine &engine, std::string_view name) : Pass(engine, name) {}

  void onPass(FrameContext &context) override;

protected:
  void onAddUseHook(AttachmentUse const &use) override;
  void onAddDefHook(AttachmentDef const &def) override;
};
} // namespace re
#endif // RENDERENGINE_COMPUTEPASS_HPP
