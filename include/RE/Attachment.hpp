#ifndef RENDERENGINE_ATTACHMENT_HPP
#define RENDERENGINE_ATTACHMENT_HPP
#include <string>
#include <string_view>
#include <vkw/Image.hpp>
#include <vulkan/vulkan.h>

namespace re {

enum class AttachmentType { IMAGE, BUFFER };

class Attachment {
public:
  Attachment(std::string_view name, AttachmentType type)
      : m_name(name), m_type(type){};

  std::string_view name() const { return m_name; }

  auto type() const { return m_type; }

  virtual ~Attachment() = default;

private:
  AttachmentType m_type;
  std::string m_name;
};

struct ImageAttachmentCreateInfo {
  vkw::ImageType imageType;
  VkFormat pixelFormat;
  VkExtent3D extents;
  unsigned layers;
};

class ImageAttachment final : public Attachment {
public:
  ImageAttachment(std::string_view name, ImageAttachmentCreateInfo const &info)
      : Attachment(name, AttachmentType::IMAGE), m_info(info) {}

  auto &info() const { return m_info; }

private:
  ImageAttachmentCreateInfo m_info;
};

struct BufferAttachmentCreateInfo {
  VkDeviceSize size;
};

class BufferAttachment final : public Attachment {
public:
  BufferAttachment(std::string_view name,
                   BufferAttachmentCreateInfo const &info)
      : Attachment(name, AttachmentType::BUFFER), m_info(info) {}

  auto &info() const { return m_info; }

private:
  BufferAttachmentCreateInfo m_info;
};

} // namespace re
#endif // RENDERENGINE_ATTACHMENT_HPP
