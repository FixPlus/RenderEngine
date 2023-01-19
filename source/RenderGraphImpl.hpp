#ifndef RENDERENGINE_RENDERGRAPHIMPL_HPP
#define RENDERENGINE_RENDERGRAPHIMPL_HPP

#include "Command.hpp"
#include "RE/RenderGraph.hpp"
#include <map>
#include <vkw/RenderPass.hpp>
namespace re {

class RenderGraphImpl final : public RenderGraph {
public:
  explicit RenderGraphImpl(Engine &engine);

  vkw::ImageViewBase const *
  getImageAttachmentState(AttachmentUse const *use) override;
  vkw::BufferBase const *
  getBufferAttachmentState(AttachmentUse const *use) override;

  vkw::ImageViewBase const *
  getImageAttachmentState(AttachmentDef const *def) override;
  vkw::BufferBase const *
  getBufferAttachmentState(AttachmentDef const *def) override;

  bool compile() override;

  std::span<Command const> record() override;

  ~RenderGraphImpl() override;

private:
  using PassSequence = boost::container::small_vector<Pass *, 5>;

  struct ExecuteCommandInfo {
    PassSequence sequence;
    std::unique_ptr<vkw::PrimaryCommandBuffer> buffer;
  };

  boost::container::small_vector<Command, 5> m_commandBatch;
  boost::container::small_vector<std::optional<ExecuteCommandInfo>, 5>
      m_command_info;
  std::map<Pass *, std::pair<vkw::RenderPass *, vkw::FrameBuffer *>>
      m_pass_render_pass_map;

  boost::container::small_vector<std::unique_ptr<vkw::RenderPass>, 5>
      m_render_passes;
  boost::container::small_vector<std::unique_ptr<vkw::FrameBuffer>, 5>
      m_framebuffers;
  boost::container::small_vector<std::unique_ptr<vkw::Semaphore>, 5>
      m_semaphore;

  boost::container::small_vector<std::unique_ptr<Pass>, 5> m_internalPasses;
  std::set<unsigned, std::unique_ptr<vkw::CommandPool>> m_commandPools;

  // Attachment states
  std::map<AttachmentUse const *, vkw::ImageViewBase const *> m_imageUses;
  std::map<AttachmentUse const *, vkw::BufferBase const *> m_bufferUses;

  std::map<AttachmentDef const *, vkw::ImageViewBase const *> m_imageDefs;
  std::map<AttachmentDef const *, vkw::BufferBase const *> m_bufferDefs;

  boost::container::small_vector<std::unique_ptr<vkw::ImageInterface>, 5>
      m_images;
  boost::container::small_vector<std::unique_ptr<vkw::ImageViewBase>, 5>
      m_imageViews;
  boost::container::small_vector<std::unique_ptr<vkw::BufferBase>, 5> m_buffers;
};

} // namespace re
#endif // RENDERENGINE_RENDERGRAPHIMPL_HPP
