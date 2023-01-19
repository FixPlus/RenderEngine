#ifndef RENDERENGINE_FRAMECONTEXT_HPP
#define RENDERENGINE_FRAMECONTEXT_HPP
#include <vkw/CommandBuffer.hpp>
#include <vkw/RenderPass.hpp>

namespace re {
class RenderGraph;

struct FrameContext {
  RenderGraph const *renderGraph;
  vkw::PrimaryCommandBuffer *commandBuffer;
  vkw::RenderPass const *currentPass;
};

} // namespace re
#endif // RENDERENGINE_FRAMECONTEXT_HPP
