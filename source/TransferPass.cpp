#include "RE/TransferPass.hpp"

namespace re {

void TransferPass::onPass(FrameContext &context) {
  // TODO
}
void TransferPass::onAddUseHook(const AttachmentUse &use) {
  Pass::onAddUseHook(use);
  // TODO
}
void TransferPass::onAddDefHook(const AttachmentDef &def) {
  Pass::onAddDefHook(def);
  // TODO
}

} // namespace re