#ifndef RENDERENGINE_TRANSFERPASS_HPP
#define RENDERENGINE_TRANSFERPASS_HPP

#include "RE/Pass.hpp"

namespace re {

class TransferPass : public Pass {
public:
  TransferPass(Engine &engine, std::string_view name) : Pass(engine, name) {}

  void onPass(FrameContext &context) override;

protected:
  void onAddUseHook(AttachmentUse const &use) override;
  void onAddDefHook(AttachmentDef const &def) override;
};
} // namespace re
#endif // RENDERENGINE_TRANSFERPASS_HPP
