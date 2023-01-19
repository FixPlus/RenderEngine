#ifndef RENDERENGINE_PASS_HPP
#define RENDERENGINE_PASS_HPP

#include "RE/Attachment.hpp"
#include "RE/Engine.hpp"
#include "RE/FrameContext.hpp"
#include <variant>

namespace re {

class AttachmentUse {
public:
  AttachmentUse() = default;

  virtual Attachment const *attachment() const = 0;

  virtual ~AttachmentUse() = default;

private:
  Attachment const *m_attachment;
};

class AttachmentDef {
public:
  AttachmentDef() = default;

  virtual Attachment const *attachment() const = 0;

  virtual ~AttachmentDef() = default;

private:
  Attachment const *m_attachment;
};

class DescriptorAttachmentRef {
public:
  explicit DescriptorAttachmentRef(VkDescriptorType type) : m_type(type) {}

  struct DescriptorWrite {
    std::reference_wrapper<vkw::DescriptorSet> descriptor;

    unsigned binding;
  };

  void addDescriptorWriteJob(DescriptorWrite write) {
    m_writes.emplace_back(write);
  }

  std::span<DescriptorWrite const> writes() const { return m_writes; }
  auto descriptortype() const { return m_type; }

  virtual ~DescriptorAttachmentRef() = default;

private:
  VkDescriptorType m_type;
  boost::container::small_vector<DescriptorWrite, 5> m_writes;
};

class FramebufferAttachmentRef {
public:
  explicit FramebufferAttachmentRef(unsigned binding) : m_binding(binding) {}

  unsigned binding() const { return m_binding; }

  virtual ~FramebufferAttachmentRef() = default;

private:
  unsigned m_binding;
};

class ImageUse : public AttachmentUse {
public:
  struct Info {
    VkImageLayout layout;
    VkImageUsageFlags usage;
    vkw::ImageViewType viewType;
    unsigned baseLayer;
    unsigned layerCount;
    VkFormat format;
  };

  ImageUse(const ImageAttachment *image, Info info)
      : m_info(info), m_image(image) {}

  Attachment const *attachment() const override { return m_image; }

  Info const &info() const { return m_info; }

private:
  Info m_info;
  const ImageAttachment *m_image;
};

class FramebufferImageUse : public ImageUse, public FramebufferAttachmentRef {
public:
  FramebufferImageUse(const ImageAttachment *image, Info info, unsigned binding)
      : ImageUse(image, info), FramebufferAttachmentRef(binding) {}
};

class InputAttachmentImageUse : public FramebufferImageUse,
                                public DescriptorAttachmentRef {
public:
  InputAttachmentImageUse(const ImageAttachment *image, unsigned binding,
                          unsigned baseLayer, unsigned layerCount,
                          VkFormat format)
      : FramebufferImageUse(image, m_fill_info(baseLayer, layerCount, format),
                            binding),
        DescriptorAttachmentRef(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT) {}

private:
  static Info m_fill_info(unsigned baseLayer, unsigned layerCount,
                          VkFormat format) {
    Info ret{};
    ret.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    ret.usage = VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
    ret.baseLayer = baseLayer;
    ret.layerCount = layerCount;
    ret.format = format;
    return ret;
  }
};

class DescriptorImageUse : public ImageUse, public DescriptorAttachmentRef {
public:
  DescriptorImageUse(const ImageAttachment *image, Info info,
                     VkDescriptorType descriptorType)
      : ImageUse(image, info), DescriptorAttachmentRef(descriptorType) {}
};

class BufferUse : public AttachmentUse, public DescriptorAttachmentRef {
public:
  struct Info {
    VkBufferUsageFlags usage;
    VkDeviceSize offset;
    VkDeviceSize size;
  };
  BufferUse(const BufferAttachment *buffer, Info info, VkDescriptorType type)
      : DescriptorAttachmentRef(type), m_buffer(buffer), m_info(info) {}

  auto &info() const { return m_info; }
  Attachment const *attachment() const override { return m_buffer; }

private:
  Info m_info;
  const BufferAttachment *m_buffer;
};

class ImageDef : public AttachmentDef {
public:
  struct Info {
    VkImageLayout layout;
    VkImageUsageFlags usage;
    vkw::ImageViewType viewType;
    unsigned baseLayer;
    unsigned layerCount;
    VkFormat format;
  };

  ImageDef(const ImageAttachment *image, Info info)
      : m_info(info), m_image(image) {}

  Attachment const *attachment() const override { return m_image; }

  Info const &info() const { return m_info; }

private:
  Info m_info;
  const ImageAttachment *m_image;
};

class FramebufferImageDef : public ImageDef, public FramebufferAttachmentRef {
public:
  FramebufferImageDef(const ImageAttachment *image, Info info, unsigned binding)
      : ImageDef(image, info), FramebufferAttachmentRef(binding) {}
};

class DescriptorImageDef : public ImageDef, public DescriptorAttachmentRef {
public:
  DescriptorImageDef(const ImageAttachment *image, Info info,
                     VkDescriptorType descriptorType)
      : ImageDef(image, info), DescriptorAttachmentRef(descriptorType) {}
};

class BufferDef : public AttachmentDef, public DescriptorAttachmentRef {
public:
  struct Info {
    VkBufferUsageFlags usage;
    VkDeviceSize offset;
    VkDeviceSize size;
  };
  BufferDef(const BufferAttachment *buffer, Info info, VkDescriptorType type)
      : DescriptorAttachmentRef(type), m_buffer(buffer), m_info(info) {}

  auto &info() const { return m_info; }
  Attachment const *attachment() const override { return m_buffer; }

private:
  Info m_info;
  const BufferAttachment *m_buffer;
};

class Pass {
public:
  Pass(Engine &engine, std::string_view name)
      : m_engine(engine), m_name(name){};

  virtual void onPass(FrameContext &context) = 0;

  void addUse(AttachmentUse const *use) {
    onAddUseHook(*use);
    auto Inserted = m_uses.emplace(use);
    if (!Inserted.second)
      throw std::runtime_error("Use already registered");
  }

  void addDef(AttachmentDef const *def) {
    onAddDefHook(*def);
    auto Inserted = m_defs.emplace(def);
    if (!Inserted.second)
      throw std::runtime_error("Def already registered");
  }

  auto &uses() const { return m_uses; }

  auto &defs() const { return m_defs; }

  auto uses_begin() const { return m_uses.begin(); }

  auto uses_end() const { return m_uses.end(); }

  auto defs_begin() const { return m_defs.begin(); }

  auto defs_end() const { return m_defs.end(); }

  std::string_view name() const { return m_name; }

  virtual ~Pass() = default;

protected:
  // Hooks can be used by child classes to check
  // validity of attachment being added
  virtual void onAddUseHook(AttachmentUse const &use){};
  virtual void onAddDefHook(AttachmentDef const &def){};

  std::reference_wrapper<Engine> m_engine;

private:
  std::set<AttachmentUse const *> m_uses;
  std::set<AttachmentDef const *> m_defs;
  std::string m_name;
};

} // namespace re
#endif // RENDERENGINE_PASS_HPP
