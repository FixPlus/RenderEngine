#include "RE/RenderGraph.hpp"

namespace re {

ImageAttachment const *
RenderGraph::createNewImageAttachment(std::string_view name,
                                      const ImageAttachmentCreateInfo &info) {
  auto attachment = std::make_unique<ImageAttachment>(name, info);
  auto *ret = attachment.get();
  m_attachments.emplace(std::move(attachment));
  return ret;
}
BufferAttachment const *
RenderGraph::createNewBufferAttachment(std::string_view name,
                                       const BufferAttachmentCreateInfo &info) {
  auto attachment = std::make_unique<BufferAttachment>(name, info);
  auto *ret = attachment.get();
  m_attachments.emplace(std::move(attachment));
  return ret;
}

} // namespace re