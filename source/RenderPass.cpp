#include "RE/RenderPass.hpp"
#include <exception>

namespace re {

void RenderPass::onAddUseHook(AttachmentUse const &use) {}
void RenderPass::onAddDefHook(AttachmentDef const &def) {
  if (def.attachment()->type() == AttachmentType::BUFFER) {
    throw std::runtime_error("RenderPass cannot define a buffer");
  }
  auto *imageDef = dynamic_cast<FramebufferImageDef const *>(&def);
  if (!imageDef) {
    throw std::runtime_error("RenderPass can only define framebuffer images");
  }

  auto &imageDefInfo = imageDef->info();

  if (vkw::ImageInterface::isDepthFormat(imageDefInfo.format)) {
    if (m_hasDepthAttachment)
      throw std::runtime_error(
          "RenderPass cannot have more than one depth attachment");
    if (imageDefInfo.usage != VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
      throw std::runtime_error("Depth attachment has invalid usage info");

    m_hasDepthAttachment = true;
  } else {
    if (imageDefInfo.usage != VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
      throw std::runtime_error("Color attachment has invalid usage info");
  }

  if (m_takenAttachments.contains(imageDef->binding()))
    throw std::runtime_error("Attachment index has already been registered");

  m_takenAttachments.emplace(imageDef->binding());
}
bool RenderPass::hasDepthAttachment() const { return m_hasDepthAttachment; }
void RenderPass::onPass(FrameContext &context) {
  // TODO: implement
}
} // namespace re