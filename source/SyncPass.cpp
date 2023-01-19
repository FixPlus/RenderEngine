#include "SyncPass.hpp"

namespace re {

void SyncPass::onPass(FrameContext &context) {
  context.commandBuffer->pipelineBarrier(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                         VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, {},
                                         m_imageBarriers, m_bufferBarriers);
}
SyncPass::SyncPass(Engine &engine, std::string_view name)
    : Pass(engine, name) {}
void SyncPass::addImageBarrier(const vkw::ImageInterface *image,
                               const ImageDef *def, const ImageUse *use,
                               unsigned int srcFamily, unsigned int dstFamily) {
  assert(def && "Def must be set");
  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.pNext = nullptr;
  barrier.image = *image;
  barrier.oldLayout = def->info().layout;
  barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
  if (use) {
    barrier.newLayout = def->info().layout;
    // TODO - further specify memory access scope
    barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
  } else {
    barrier.newLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.dstAccessMask = VK_ACCESS_NONE;
  }
  barrier.subresourceRange = image->completeSubresourceRange();
  barrier.srcQueueFamilyIndex = srcFamily;
  barrier.dstQueueFamilyIndex = dstFamily;

  m_imageBarriers.emplace_back(barrier);
}
void SyncPass::addBufferBarrier(vkw::BufferBase const *buffer,
                                BufferDef const *def, BufferUse const *use,
                                unsigned srcFamily, unsigned dstFamily) {
  VkBufferMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  barrier.pNext = nullptr;
  barrier.buffer = *buffer;
  barrier.offset = 0;
  barrier.size = buffer->size();
  barrier.srcQueueFamilyIndex = srcFamily;
  barrier.dstQueueFamilyIndex = dstFamily;
  barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
  barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;

  m_bufferBarriers.emplace_back(barrier);
}

} // namespace re