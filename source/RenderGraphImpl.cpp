#include "RenderGraphImpl.hpp"
#include "Command.hpp"
#include "RE/ComputePass.hpp"
#include "RE/Pass.hpp"
#include "RE/TransferPass.hpp"
#include "SyncPass.hpp"
#include <cassert>
#include <list>
#include <numeric>
#include <vkw/CommandPool.hpp>
#include <vkw/FrameBuffer.hpp>

#ifndef NDEBUG
#include <iostream>
#endif

namespace re {

std::unique_ptr<RenderGraph> RenderGraph::createRenderGraph(Engine &engine) {
  return std::make_unique<RenderGraphImpl>(engine);
}

RenderGraphImpl::RenderGraphImpl(Engine &engine) : RenderGraph(engine) {}

vkw::ImageViewBase const *
RenderGraphImpl::getImageAttachmentState(AttachmentUse const *use) {
  return m_imageUses.at(use);
}
vkw::BufferBase const *
RenderGraphImpl::getBufferAttachmentState(AttachmentUse const *use) {
  return m_bufferUses.at(use);
}

vkw::ImageViewBase const *
RenderGraphImpl::getImageAttachmentState(AttachmentDef const *def) {
  return m_imageDefs.at(def);
}
vkw::BufferBase const *
RenderGraphImpl::getBufferAttachmentState(AttachmentDef const *def) {
  return m_bufferDefs.at(def);
}
namespace {
using PassList = std::list<Pass *>;

struct AttachmentLiveRange {
  PassList::iterator begin, end;
  AttachmentDef const *def;
  boost::container::small_vector<
      std::pair<PassList::iterator, AttachmentUse const *>, 3>
      uses;
};

struct PassSequenceAnalysis {
  std::map<Attachment const *,
           boost::container::small_vector<AttachmentLiveRange, 2>>
      liveRanges;
  PassList passList;
};

struct ImageAttachmentUseInfo {
  VkImageUsageFlags overallFlags = 0;
  std::set<VkFormat> writeFormats;
  std::set<VkFormat> readFormats;
};
struct BufferAttachmentUseInfo {
  VkBufferUsageFlags overallFlags = 0;
};
template <vkw::ImagePixelType ptype, vkw::ImageType itype,
          vkw::ImageViewType vtype>
std::unique_ptr<vkw::ImageViewBase> createSpecificImageView(
    vkw::Device &device, vkw::Image<ptype, itype, vkw::ARRAY> const &image,
    VkFormat format, unsigned baseLayer, unsigned layerCount) {
  if constexpr (vkw::CompatibleViewTypeC<itype, vtype>) {
    return std::make_unique<vkw::ImageView<ptype, vtype>>(
        device, image, format, baseLayer, layerCount);
  } else {
    assert(0 && "Broken image view");
  }
  return std::unique_ptr<vkw::ImageViewBase>{};
}
template <vkw::ImagePixelType ptype, vkw::ImageType itype, typename T>
std::unique_ptr<vkw::ImageViewBase>
createImageView(vkw::Device &device,
                vkw::Image<ptype, itype, vkw::ARRAY> const &image,
                T const &info) {
  switch (info.viewType) {
  case vkw::V1D:
    return createSpecificImageView<ptype, itype, vkw::V1D>(
        device, image, info.format, info.baseLayer, info.layerCount);
  case vkw::V1DA:
    return createSpecificImageView<ptype, itype, vkw::V1DA>(
        device, image, info.format, info.baseLayer, info.layerCount);
  case vkw::V2D:
    return createSpecificImageView<ptype, itype, vkw::V2D>(
        device, image, info.format, info.baseLayer, info.layerCount);
  case vkw::V2DA:
    return createSpecificImageView<ptype, itype, vkw::V2DA>(
        device, image, info.format, info.baseLayer, info.layerCount);
  case vkw::V3D:
    return createSpecificImageView<ptype, itype, vkw::V3D>(
        device, image, info.format, info.baseLayer, info.layerCount);
  case vkw::VCUBE:
    return createSpecificImageView<ptype, itype, vkw::VCUBE>(
        device, image, info.format, info.baseLayer, info.layerCount);
  case vkw::VCUBEA:
    return createSpecificImageView<ptype, itype, vkw::VCUBEA>(
        device, image, info.format, info.baseLayer, info.layerCount);
  default:
    assert(0 && "Broken image");
  }

  return std::unique_ptr<vkw::ImageViewBase>{};
}
template <vkw::ImagePixelType ptype, vkw::ImageType itype>
void createImageArtifacts(
    vkw::Device &device, ImageAttachmentCreateInfo const &info,
    ImageAttachmentUseInfo const &useInfo, std::span<AttachmentLiveRange> lrs,
    std::function<void(std::unique_ptr<vkw::ImageInterface>)> imageInserter,
    std::function<void(std::unique_ptr<vkw::ImageViewBase>,
                       AttachmentUse const *, AttachmentDef const *)>
        imageViewInserter) {
  VmaAllocationCreateInfo ACI{};
  auto image = std::make_unique<vkw::Image<ptype, itype, vkw::ARRAY>>(
      device.getAllocator(), ACI, info.pixelFormat, info.extents.width,
      info.extents.height, info.extents.depth, info.layers, 1,
      useInfo.overallFlags);

  auto viewFormat = *useInfo.writeFormats.begin();
  for (auto &lr : lrs) {
    auto *def = dynamic_cast<ImageDef const *>(lr.def);
    assert(def && "Broken attachment def");
    auto &defInfo = def->info();
    std::unique_ptr<vkw::ImageViewBase> defView =
        createImageView(device, *image, defInfo);
    std::invoke(imageViewInserter, std::move(defView), nullptr, lr.def);

    for (auto &puse : lr.uses) {
      auto *use = dynamic_cast<ImageUse const *>(puse.second);
      assert(use && "Broken attachment use");
      auto &useInf = use->info();
      std::unique_ptr<vkw::ImageViewBase> useView =
          createImageView(device, *image, useInf);
      std::invoke(imageViewInserter, std::move(useView), puse.second, nullptr);
    }
  }

  std::invoke(imageInserter, std::move(image));
}
template <vkw::ImageType itype>
bool imageAndViewCompatible(vkw::ImageViewType vtype) {
  switch (vtype) {
  case vkw::V1D:
    return vkw::CompatibleViewTypeC<itype, vkw::V1D>;
  case vkw::V1DA:
    return vkw::CompatibleViewTypeC<itype, vkw::V1DA>;
  case vkw::V2D:
    return vkw::CompatibleViewTypeC<itype, vkw::V2D>;
  case vkw::V2DA:
    return vkw::CompatibleViewTypeC<itype, vkw::V2DA>;
  case vkw::V3D:
    return vkw::CompatibleViewTypeC<itype, vkw::V3D>;
  case vkw::VCUBE:
    return vkw::CompatibleViewTypeC<itype, vkw::VCUBE>;
  case vkw::VCUBEA:
    return vkw::CompatibleViewTypeC<itype, vkw::VCUBEA>;
  default:
    assert(0 && "unhandled case");
  }
  return false;
}
bool imageAndViewCompatible(vkw::ImageType itype, vkw::ImageViewType vtype) {
  switch (itype) {
  case vkw::I1D:
    return imageAndViewCompatible<vkw::I1D>(vtype);
  case vkw::I2D:
    return imageAndViewCompatible<vkw::I2D>(vtype);
  case vkw::I3D:
    return imageAndViewCompatible<vkw::I3D>(vtype);
  case vkw::ICUBE:
    return imageAndViewCompatible<vkw::ICUBE>(vtype);
  default:
    assert(0 && "unhandled case");
  }
  return false;
}

unsigned getAvailableQueueFamily(vkw::Device &device, Pass const *pass) {
  if (auto *renderPass = dynamic_cast<RenderPass const *>(pass)) {
    return device.anyGraphicsQueue().family().index();
  } else if (auto *computePass = dynamic_cast<ComputePass const *>(pass)) {
    return device.anyComputeQueue().family().index();
  } else if (auto *transferPass = dynamic_cast<TransferPass const *>(pass)) {
    return device.anyTransferQueue().family().index();
  } else {
    throw std::runtime_error("Broken pass");
  }
  return 0;
}

} // namespace

bool RenderGraphImpl::compile() {
  // TODO this

  PassSequenceAnalysis passesAnalysis;

  // 1. Scan all passes and build live range analysis of attachments.

  for (auto &passRef : m_passes) {
    auto &pass = passRef.get();
    passesAnalysis.passList.emplace_back(&pass);
    auto passIt = --passesAnalysis.passList.end();
    std::set<Attachment const *> definedByPass;
    // First, scan through all defs.
    for (auto &def : pass.defs()) {
      auto *attach = def->attachment();
      if (std::none_of(m_attachments.begin(), m_attachments.end(),
                       [attach](auto &a) { return attach == a.get(); })) {
        // TODO: normal error here
        throw std::runtime_error("Referenced attachment is unknown.");
      }

      auto inserted = definedByPass.emplace(attach);
      if (!inserted.second) {
        // TODO: normal error here
        throw std::runtime_error("Attachment is defined twice by same pass.");
      }
      auto found = passesAnalysis.liveRanges.find(attach);
      if (found == passesAnalysis.liveRanges.end()) {
        // First live range start.
        auto &range =
            passesAnalysis.liveRanges.emplace(attach, 1).first->second.front();
        range.def = def;
        range.begin = passIt;
        range.end = range.begin;
        ++range.end;
      } else {
        // Secondary live range start, close up previous live range.
        assert(!found->second.empty() &&
               "at least one live range expected here.");
        auto &prevRange = found->second.back();
        auto &prevUses = prevRange.uses;
        if (prevUses.empty()) {
          // TODO: warning here - unused attachment definition
          prevRange.end = ++prevRange.begin;
        } else {
          prevRange.end = ++prevUses.back().first;
        }
        auto &range = found->second.emplace_back();
        range.def = def;
        range.begin = passIt;
        range.end = range.begin;
        ++range.end;
      }
    }
    // second, scan all uses.
    for (auto &use : pass.uses()) {
      auto *attach = use->attachment();
      if (std::none_of(m_attachments.begin(), m_attachments.end(),
                       [attach](auto &a) { return attach == a.get(); })) {
        // TODO: normal error here
        throw std::runtime_error("Referenced attachment is unknown.");
      }
      if (definedByPass.contains(attach)) {
        // TODO - normal error here
        throw std::runtime_error(
            "Attachment cannot be used in pass that defines it.");
      }
      auto found = passesAnalysis.liveRanges.find(attach);
      if (found == passesAnalysis.liveRanges.end()) {
        // TODO - normal error here
        throw std::runtime_error("Attachment used before definition.");
      }
      assert(!found->second.empty() &&
             "expected at least one live range here.");

      auto &currentRange = found->second.back();
      currentRange.uses.emplace_back(passIt, use);
    }
  }

  // Diagnostics for unused attachments.
  for (auto &attachment : m_attachments) {
    if (!passesAnalysis.liveRanges.contains(attachment.get())) {
      // TODO: warning here - unused attachment
    }
    // Check if last live range has no uses. Rest ranges were handled during
    // analysis.
    auto found = passesAnalysis.liveRanges.find(attachment.get());
    assert(!found->second.empty() && "Expected at least one live range.");
    auto &lastRange = found->second.back();

    if (lastRange.uses.empty()) {
      // TODO: warning here - unused attachment
    }
  }

  // 2. Analyse each use and def to verify that attachments could be created.

  struct AttachmentUsageAnalysis {
    std::map<ImageAttachment const *, ImageAttachmentUseInfo> images;
    std::map<BufferAttachment const *, BufferAttachmentUseInfo> buffers;
  } usageAnalysis;

  // run usage analysis
  for (auto &attachment : passesAnalysis.liveRanges) {
    if (auto *image = dynamic_cast<ImageAttachment const *>(attachment.first)) {
      auto &imageUsage =
          usageAnalysis.images.emplace(image, ImageAttachmentUseInfo{})
              .first->second;
      auto &imageInfo = image->info();
      for (auto &liveRange : attachment.second) {
        auto *pDef = dynamic_cast<ImageDef const *>(liveRange.def);
        if (!pDef) {
          throw std::runtime_error("Attachment def type mismatch.");
        }
        auto &defInfo = pDef->info();
        // check image and view compatibility
        if (!imageAndViewCompatible(imageInfo.imageType, defInfo.viewType)) {
          throw std::runtime_error("Incompatible image and view types found");
        }
        // check layers sanity
        if (defInfo.layerCount == 0)
          throw std::runtime_error("image attachment ref has 0 layers");
        if (defInfo.baseLayer + defInfo.layerCount > imageInfo.layers)
          throw std::runtime_error(
              "image attachment reference uses more layers than image has.");
        imageUsage.overallFlags |= defInfo.usage;
        imageUsage.writeFormats.emplace(defInfo.format);

        for (auto &use : liveRange.uses) {
          auto *pUse = dynamic_cast<ImageUse const *>(use.second);
          if (!pUse) {
            throw std::runtime_error("Attachment use type mismatch.");
          }
          auto &useInfo = pUse->info();
          // check image and view compatibility
          if (!imageAndViewCompatible(imageInfo.imageType, useInfo.viewType)) {
            throw std::runtime_error("Incompatible image and view types found");
          }
          // check layers sanity
          if (useInfo.layerCount == 0)
            throw std::runtime_error("image attachment ref has 0 layers");
          if (useInfo.baseLayer + useInfo.layerCount > imageInfo.layers)
            throw std::runtime_error(
                "image attachment reference uses more layers than image has.");
          imageUsage.overallFlags |= useInfo.usage;
          imageUsage.readFormats.emplace(useInfo.format);
        }
      }
      auto imageFormat = imageInfo.pixelFormat;
      // check format match
      // TODO: for now it checked for exact match but it can be less strict
      if (std::any_of(
              imageUsage.readFormats.begin(), imageUsage.readFormats.begin(),
              [imageFormat](auto format) { return format == imageFormat; }) ||
          std::any_of(
              imageUsage.writeFormats.begin(), imageUsage.writeFormats.begin(),
              [imageFormat](auto format) { return format == imageFormat; })) {
        throw std::runtime_error("Image attachment format mismatch.");
      }
    } else if (auto *buffer =
                   dynamic_cast<BufferAttachment const *>(attachment.first)) {
      auto &bufferUsage =
          usageAnalysis.buffers.emplace(buffer, BufferAttachmentUseInfo{})
              .first->second;
      auto &bufferInfo = buffer->info();
      for (auto &liveRange : attachment.second) {
        auto *pDef = dynamic_cast<BufferDef const *>(liveRange.def);
        if (!pDef) {
          throw std::runtime_error("Attachment def type mismatch.");
        }
        auto &defInfo = pDef->info();
        // range sanity check
        if (defInfo.offset >= bufferInfo.size ||
            defInfo.offset + defInfo.size >= bufferInfo.size)
          throw std::runtime_error(
              "Buffer def is out of range of buffer size.");

        for (auto &use : liveRange.uses) {
          auto *pUse = dynamic_cast<BufferUse const *>(use.second);
          if (!pUse) {
            throw std::runtime_error("Attachment use type mismatch.");
          }
          auto &useInfo = pUse->info();
          // range sanity check
          if (defInfo.offset >= bufferInfo.size ||
              defInfo.offset + defInfo.size >= bufferInfo.size)
            throw std::runtime_error(
                "Buffer use is out of range of buffer size.");
        }
      }
    } else {
      assert(0 && "Broken attachment");
    }
  }

  // create attachments(and image view)

  for (auto &att : passesAnalysis.liveRanges) {
    if (att.first->type() == AttachmentType::BUFFER) {
      auto *pBuff = dynamic_cast<BufferAttachment const *>(att.first);
      assert(pBuff && "broken attachment");
      auto &buff = *pBuff;
      VkBufferCreateInfo createInfo{};

      createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
      createInfo.pNext = nullptr;
      createInfo.size = buff.info().size;
      createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
      createInfo.usage = usageAnalysis.buffers.at(pBuff).overallFlags;

      VmaAllocationCreateInfo allocationCreateInfo{};
      allocationCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

      auto &bufHandle =
          m_buffers.emplace_back(std::make_unique<vkw::BufferBase>(
              m_engine.get().device().getAllocator(), createInfo,
              allocationCreateInfo));

      for (auto &lr : att.second) {
        m_bufferDefs.emplace(lr.def, bufHandle.get());
        for (auto &use : lr.uses) {
          m_bufferUses.emplace(use.second, bufHandle.get());
        }
      }
    } else {
      auto *pImage = dynamic_cast<ImageAttachment const *>(att.first);
      assert(pImage && "broken attachment");
      auto &image = *pImage;
      // TODO
      auto imageInserter = [this](std::unique_ptr<vkw::ImageInterface> image) {
        m_images.template emplace_back(std::move(image));
      };
      auto imageViewInserter =
          [this](std::unique_ptr<vkw::ImageViewBase> imageView,
                 AttachmentUse const *use, AttachmentDef const *def) {
            assert((use || def) && !(use && def) &&
                   "use or def must be provided");
            auto *view = m_imageViews.emplace_back(std::move(imageView)).get();
            if (use) {
              m_imageUses.emplace(use, view);
            } else {
              m_imageDefs.emplace(def, view);
            }
          };
      switch (image.info().imageType) {
      case vkw::I1D:
        if (vkw::ImageInterface::isColorFormat(image.info().pixelFormat))
          createImageArtifacts<vkw::COLOR, vkw::I1D>(
              m_engine.get().device(), image.info(),
              usageAnalysis.images.at(pImage),
              {att.second.begin(), att.second.end()}, imageInserter,
              imageViewInserter);
        else
          createImageArtifacts<vkw::DEPTH, vkw::I1D>(
              m_engine.get().device(), image.info(),
              usageAnalysis.images.at(pImage),
              {att.second.begin(), att.second.end()}, imageInserter,
              imageViewInserter);
        break;
      case vkw::I2D:
        if (vkw::ImageInterface::isColorFormat(image.info().pixelFormat))
          createImageArtifacts<vkw::COLOR, vkw::I2D>(
              m_engine.get().device(), image.info(),
              usageAnalysis.images.at(pImage),
              {att.second.begin(), att.second.end()}, imageInserter,
              imageViewInserter);
        else
          createImageArtifacts<vkw::DEPTH, vkw::I2D>(
              m_engine.get().device(), image.info(),
              usageAnalysis.images.at(pImage),
              {att.second.begin(), att.second.end()}, imageInserter,
              imageViewInserter);
        break;
      case vkw::I3D:
        if (vkw::ImageInterface::isColorFormat(image.info().pixelFormat))
          createImageArtifacts<vkw::COLOR, vkw::I3D>(
              m_engine.get().device(), image.info(),
              usageAnalysis.images.at(pImage),
              {att.second.begin(), att.second.end()}, imageInserter,
              imageViewInserter);
        else
          createImageArtifacts<vkw::DEPTH, vkw::I3D>(
              m_engine.get().device(), image.info(),
              usageAnalysis.images.at(pImage),
              {att.second.begin(), att.second.end()}, imageInserter,
              imageViewInserter);
        break;
      default:
        assert(0 && "Broken image");
      }
    }
  }

  // split pass list to batches

  struct CommandBatchesSplit {
    boost::container::small_vector<std::pair<PassList::iterator, unsigned>, 3>
        batches;
    unsigned getPassFamilyGroup(PassList::iterator pass) {
      auto passIt = batches.begin();
      for (auto It = passIt->first; It != passEnd; ++It) {
        if (passIt + 1 != batches.end() && It == (passIt + 1)->first)
          ++passIt;
        if (pass == It)
          return passIt->second;
      }
      assert(0 && "Broken pass");
    }
    PassList::iterator passEnd;
  } commandSplitAnalysis;
  unsigned lastQueueFamily = std::numeric_limits<unsigned>::max();
  for (auto passIt = passesAnalysis.passList.begin(),
            passEnd = passesAnalysis.passList.end();
       passIt != passEnd; ++passIt) {
    auto queueFamily =
        getAvailableQueueFamily(m_engine.get().device(), *passIt);
    if (queueFamily != lastQueueFamily) {
      lastQueueFamily = queueFamily;
      commandSplitAnalysis.batches.emplace_back(passIt, lastQueueFamily);
    }
  }
  commandSplitAnalysis.passEnd = passesAnalysis.passList.end();

  // insert synchronization passes for every def-use pair

  for (auto &att : passesAnalysis.liveRanges) {
    // TODO: allow def-use pair intersect pass list edge
    auto lastUseIt = std::find_if(att.second.rbegin(), att.second.rend(),
                                  [](auto &lr) { return !lr.uses.empty(); });

    if (lastUseIt == att.second.rend())
      throw std::runtime_error("For now unused attachments are not allowed");
    auto lastUse = lastUseIt->uses.back();
    bool lastUseLater = true;
    for (auto &lr : att.second) {
      if (lr.uses.empty()) {
        if (att.second.size() > 1)
          throw std::runtime_error(
              "For now multiple def-def hazards are not allowed");
        // Special case, when some attachment has only one def and no uses
        // TODO
        throw std::runtime_error("For now def-def hazards are not allowed");
      }
      auto *defPass = *lr.begin;
      if (auto *renderPass = dynamic_cast<RenderPass *>(defPass)) {
        // Render passes are implicitly synchronized
        lastUse = lr.uses.back();
        lastUseLater = false;
        continue;
      }
      auto *def = lr.def;
      auto *use = lr.uses.begin()->second;
      auto found =
          std::find_if(lr.begin, lr.uses.front().first, [](auto *pass) {
            return dynamic_cast<SyncPass *>(pass);
          });
      if (found == lr.uses.front().first) {
        m_internalPasses.emplace_back(
            std::make_unique<SyncPass>(m_engine.get(), "sync pass"));
        found = passesAnalysis.passList.emplace(found,
                                                m_internalPasses.back().get());
      }
      auto *syncPass = dynamic_cast<SyncPass *>(*found);
      assert(syncPass && "It must be sync pass");
      if (att.first->type() == AttachmentType::IMAGE) {
        auto *imageAttachment =
            dynamic_cast<ImageAttachment const *>(att.first);
        auto *iDef = dynamic_cast<ImageDef const *>(def);
        auto *iUse = dynamic_cast<ImageUse const *>(use);

        assert(imageAttachment && iDef && iUse && "must be image attachment");

        syncPass->addImageBarrier(
            m_imageDefs.at(iDef)->image(), iDef, iUse,
            commandSplitAnalysis.getPassFamilyGroup(lr.begin),
            commandSplitAnalysis.getPassFamilyGroup(lr.uses.front().first));
      } else {
        auto *bufferAttachment =
            dynamic_cast<BufferAttachment const *>(att.first);
        auto *bDef = dynamic_cast<BufferDef const *>(def);
        auto *bUse = dynamic_cast<BufferUse const *>(use);
        assert(bufferAttachment && bDef && bUse && "must be buffer attachment");
        syncPass->addBufferBarrier(
            m_bufferDefs.at(bDef), bDef, bUse,
            commandSplitAnalysis.getPassFamilyGroup(lastUse.first),
            commandSplitAnalysis.getPassFamilyGroup(lr.begin));
      }

      // insert pre def sync for R->W hazard
      found = std::find_if(
          lastUseLater ? passesAnalysis.passList.begin() : lastUse.first,
          lr.begin, [](auto *pass) { return dynamic_cast<SyncPass *>(pass); });

      if (found == lr.begin) {
        m_internalPasses.emplace_back(
            std::make_unique<SyncPass>(m_engine.get(), "sync pass"));
        found = passesAnalysis.passList.emplace(found,
                                                m_internalPasses.back().get());
      }
      syncPass = dynamic_cast<SyncPass *>(*found);
      assert(syncPass && "It must be sync pass");
      // TODO: we should insert release/acquire pairs if family indices are not
      // equal
      if (att.first->type() == AttachmentType::IMAGE) {
        auto *imageAttachment =
            dynamic_cast<ImageAttachment const *>(att.first);
        auto *iDef = dynamic_cast<ImageDef const *>(def);
        auto *iUse = dynamic_cast<ImageUse const *>(lastUse.second);

        assert(imageAttachment && iDef && iUse && "must be image attachment");

        syncPass->addImageBarrier(m_imageDefs.at(iDef)->image(), iDef, iUse);
      } else {
        auto *bufferAttachment =
            dynamic_cast<BufferAttachment const *>(att.first);
        auto *bDef = dynamic_cast<BufferDef const *>(def);
        auto *bUse = dynamic_cast<BufferUse const *>(lastUse.second);
        assert(bufferAttachment && bDef && bUse && "must be buffer attachment");
        syncPass->addBufferBarrier(m_bufferDefs.at(bDef), bDef, bUse);
      }
      lastUse = lr.uses.back();
      lastUseLater = false;
    }
  }

  // create command buffers and semaphores
  // TODO

  // create render passes and insert sync passes
  // TODO

  // debug dump
#ifndef NDEBUG
  std::cout << "Live ranges" << std::endl;
  std::map<Attachment const *, unsigned> gaps;
  unsigned passNameGap = 4;
  passNameGap = std::accumulate(passesAnalysis.passList.begin(),
                                passesAnalysis.passList.end(), passNameGap,
                                [](auto gap, auto &pass) {
                                  auto nameLen = pass->name().size();
                                  return gap < nameLen ? nameLen : gap;
                                });
  std::for_each(passesAnalysis.liveRanges.begin(),
                passesAnalysis.liveRanges.end(), [&gaps](auto &att) {
                  unsigned nameLen = att.first->name().size();
                  auto len = std::max(nameLen, 4u);
                  gaps.emplace(att.first, len);
                });

  std::cout << "pass" << std::string(passNameGap - 4, ' ') << "|";
  for (auto &att : gaps) {
    std::cout << att.first->name()
              << std::string(att.second - att.first->name().size(), ' ') << "|";
  }
  std::cout << std::endl;
  for (auto passIt = passesAnalysis.passList.begin();
       passIt != passesAnalysis.passList.end(); ++passIt) {
    auto &pass = *passIt;
    std::cout << pass->name()
              << std::string(passNameGap - pass->name().size(), ' ') << "|";
    for (auto &att : gaps) {
      auto &lrs = passesAnalysis.liveRanges.at(att.first);
      auto foundDef = std::find_if(lrs.begin(), lrs.end(), [&passIt](auto &lr) {
        return lr.begin == passIt;
      });
      if (foundDef != lrs.end()) {
        std::cout << "Def" << std::string(att.second - 3, ' ') << "|";
      } else {
        auto foundUse =
            std::find_if(lrs.begin(), lrs.end(), [&passIt](auto &lr) {
              return std::any_of(
                  lr.uses.begin(), lr.uses.end(),
                  [passIt](auto &use) { return use.first == passIt; });
            });
        if (foundUse != lrs.end()) {
          std::cout << "Use" << std::string(att.second - 3, ' ') << "|";
        } else {
          std::cout << std::string(att.second, ' ') << "|";
        }
      }
    }
    std::cout << std::endl;
  }

  // Command batch dump

  std::cout << std::endl << "Formed command batches:" << std::endl << std::endl;
  for (auto i = 0u; i < commandSplitAnalysis.batches.size(); ++i) {
    std::cout << "Command batch #" << i << " : family group #"
              << commandSplitAnalysis.batches.at(i).second << std::endl;
    std::for_each(commandSplitAnalysis.batches.at(i).first,
                  i == commandSplitAnalysis.batches.size() - 1
                      ? commandSplitAnalysis.passEnd
                      : commandSplitAnalysis.batches.at(i + 1).first,
                  [](auto *pass) { std::cout << pass->name() << ", "; });
    std::cout << std::endl;
  }
  std::for_each(passesAnalysis.passList.begin(), passesAnalysis.passList.end(),
                [](auto *pass) { std::cout << pass->name() << ", "; });
#endif
  /*
   *
   * 1. scan passes and build attachment table
   *
   * 2. create attachment objects and render passes
   *
   * 3. insert needed synchronization passes
   *
   *
   */
  return true;
}

std::span<Command const> RenderGraphImpl::record() {
  FrameContext context;
  context.renderGraph = this;
  context.currentPass = nullptr;
  auto BatchIt = m_commandBatch.begin();
  auto CommandInfoIt = m_command_info.begin();
  for (; BatchIt != m_commandBatch.end(); ++BatchIt, ++CommandInfoIt) {
    auto &command = *BatchIt;
    switch (command.type()) {
    case CommandType::EXECUTE: {
      auto &ecommand = static_cast<ExecuteCommand &>(command);
      context.commandBuffer = (*CommandInfoIt).value().buffer.get();
      context.commandBuffer->begin(0);
      // Why is that was needed?
      // assert(m_command_pass_map.count(&command));
      for (auto &pass : (*CommandInfoIt).value().sequence) {
        if (m_pass_render_pass_map.contains(pass)) {
          // This is render pass then
          auto &rp = m_pass_render_pass_map.at(pass);
          context.currentPass = rp.first;
          // TODO add clear values and render area staff
          context.commandBuffer->beginRenderPass(
              *rp.first, *rp.second, rp.second->getFullRenderArea());
          pass->onPass(context);
          context.commandBuffer->endRenderPass();
          context.currentPass = nullptr;
        } else {
          // Regular pass
          pass->onPass(context);
        }
      }
      context.commandBuffer->end();
      break;
    }
    case CommandType::PRESENT: {
      auto &pcommand = static_cast<PresentCommand &>(command);
      pcommand.presentInfo().updateImages();
      break;
    }
    default:
      assert(0);
    }
  }
  return {m_commandBatch.data(), m_commandBatch.size()};
}
RenderGraphImpl::~RenderGraphImpl() = default;

} // namespace re