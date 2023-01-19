#include "RE/ComputePass.hpp"

namespace re {

void ComputePass::onPass(FrameContext &context) {
  // TODO
}
void ComputePass::onAddUseHook(const AttachmentUse &use) {
  Pass::onAddUseHook(use);
  // TODO
}
void ComputePass::onAddDefHook(const AttachmentDef &def) {
  Pass::onAddDefHook(def);
  // TODO
}

} // namespace re