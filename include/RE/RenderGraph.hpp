#ifndef RENDERENGINE_RENDERGRAPH_HPP
#define RENDERENGINE_RENDERGRAPH_HPP

#include "RE/AttachmentPool.hpp"
#include "RE/Engine.hpp"
#include "RE/OnSurfacePass.hpp"

namespace re {

class Command;

class RenderGraph {
public:
  explicit RenderGraph(Engine &engine) : m_engine(engine){};

  void addPass(Pass &pass) { m_passes.emplace_back(pass); }

  ImageAttachment const *
  createNewImageAttachment(std::string_view name,
                           ImageAttachmentCreateInfo const &info);
  BufferAttachment const *
  createNewBufferAttachment(std::string_view name,
                            BufferAttachmentCreateInfo const &info);

  virtual bool compile() = 0;

  virtual vkw::ImageViewBase const *
  getImageAttachmentState(AttachmentUse const *use) = 0;
  virtual vkw::BufferBase const *
  getBufferAttachmentState(AttachmentUse const *use) = 0;

  virtual vkw::ImageViewBase const *
  getImageAttachmentState(AttachmentDef const *def) = 0;
  virtual vkw::BufferBase const *
  getBufferAttachmentState(AttachmentDef const *def) = 0;

  virtual std::span<Command const> record() = 0;

  static std::unique_ptr<RenderGraph> createRenderGraph(Engine &engine);

  virtual ~RenderGraph() = default;

protected:
  std::reference_wrapper<Engine> m_engine;
  std::set<std::unique_ptr<Attachment>> m_attachments;
  boost::container::small_vector<std::reference_wrapper<Pass>, 3> m_passes;
};

} // namespace re
#endif // RENDERENGINE_RENDERGRAPH_HPP
