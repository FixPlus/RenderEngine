#ifndef RENDERENGINE_SYNCPASS_HPP
#define RENDERENGINE_SYNCPASS_HPP

#include "RE/Pass.hpp"

namespace re {

class SyncPass : public Pass {
public:
  SyncPass(Engine &engine, std::string_view name);

  void onPass(FrameContext &context) override;

  void addImageBarrier(vkw::ImageInterface const *image, ImageDef const *def,
                       ImageUse const *use,
                       unsigned srcFamily = VK_QUEUE_FAMILY_IGNORED,
                       unsigned dstFamily = VK_QUEUE_FAMILY_IGNORED);

  void addBufferBarrier(vkw::BufferBase const *buffer, BufferDef const *def,
                        BufferUse const *use,
                        unsigned srcFamily = VK_QUEUE_FAMILY_IGNORED,
                        unsigned dstFamily = VK_QUEUE_FAMILY_IGNORED);

private:
  boost::container::small_vector<VkImageMemoryBarrier, 3> m_imageBarriers;
  boost::container::small_vector<VkBufferMemoryBarrier, 3> m_bufferBarriers;
};

} // namespace re
#endif // RENDERENGINE_SYNCPASS_HPP
