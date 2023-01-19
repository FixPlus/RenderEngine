#include "TestContext.h"
#include <chrono>
#include <iostream>
#include <thread>

#include "RE/ComputePass.hpp"
#include "RE/RenderGraph.hpp"
#include "RE/RenderPass.hpp"

re::FramebufferImageDef createFramebufferDef(re::ImageAttachment const *image,
                                             unsigned int binding) {
  re::ImageDef::Info info{};
  info.viewType = vkw::V2D;
  info.format = image->info().pixelFormat;
  info.usage = vkw::ImageInterface::isColorFormat(info.format)
                   ? VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
                   : VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  info.layerCount = image->info().layers;
  info.baseLayer = 0;
  return re::FramebufferImageDef{image, info, binding};
}

re::DescriptorImageUse createTextureUse(re::ImageAttachment const *image) {
  re::ImageUse::Info info{};
  info.viewType = vkw::V2D;
  info.format = image->info().pixelFormat;
  info.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
  info.layerCount = image->info().layers;
  info.baseLayer = 0;

  return re::DescriptorImageUse{image, info,
                                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER};
}

re::DescriptorImageDef createStorageImageDef(re::ImageAttachment const *image) {
  re::ImageDef::Info info{};
  info.viewType = vkw::V2D;
  info.format = image->info().pixelFormat;
  info.usage = VK_IMAGE_USAGE_STORAGE_BIT;
  info.layerCount = image->info().layers;
  info.baseLayer = 0;

  return re::DescriptorImageDef{image, info, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE};
}
int main() {
  using namespace std::chrono_literals;
  try {
    re::test::BasicContextCreator cc;
    auto context = cc.create(true, &std::cout);
    std::cout << std::endl;
    auto sleep = 1000ms;

    auto rg = re::RenderGraph::createRenderGraph(context.engine());

    auto pass1 = re::ComputePass(context.engine(), "compute pass 1");
    auto pass2 = re::RenderPass(context.engine(), "render pass");
    auto pass3 = re::ComputePass(context.engine(), "compute pass 2");
    re::ImageAttachmentCreateInfo createInfo{};
    createInfo.pixelFormat = VK_FORMAT_R8G8B8A8_UNORM;
    createInfo.extents = {256, 256, 1};
    createInfo.imageType = vkw::I2D;
    createInfo.layers = 1;
    auto *image1 = rg->createNewImageAttachment("image1", createInfo);
    createInfo.extents = {512, 512, 1};
    auto *image2 = rg->createNewImageAttachment("image2", createInfo);
    auto *image3 = rg->createNewImageAttachment("image3", createInfo);
    auto i1def = createStorageImageDef(image1);
    auto i1use = createTextureUse(image1);

    pass1.addDef(&i1def);
    pass2.addUse(&i1use);

    auto i2def = createFramebufferDef(image2, 0);
    auto i3def = createFramebufferDef(image3, 1);

    pass2.addDef(&i3def);
    pass2.addDef(&i2def);

    auto i2use = createTextureUse(image2);
    auto i3use = createTextureUse(image3);

    pass3.addUse(&i2use);
    pass3.addUse(&i3use);

    rg->addPass(pass1);
    rg->addPass(pass2);
    rg->addPass(pass3);

    std::cout << std::endl;
    std::cout << "Compiling render graph..." << std::endl;
    rg->compile();
    std::cout << "Compilation successful." << std::endl;
    std::cout << std::hex;

    std::cout << "image 1 def: " << rg->getImageAttachmentState(&i1def)
              << std::endl;
    std::cout << "image 1 use: " << rg->getImageAttachmentState(&i1use)
              << std::endl;
    std::cout << "image 2 def: " << rg->getImageAttachmentState(&i2def)
              << std::endl;

  } catch (std::runtime_error &e) {
    std::cout << std::endl << "[ERROR] - " << e.what() << std::endl;
    std::cout << "Test FAIL" << std::endl;
    return -1;
  } catch (std::logic_error &e) {
    std::cout << std::endl << "[ERROR] - " << e.what() << std::endl;
    std::cout << "Test FAIL" << std::endl;
    return -1;
  }
  std::cout << std::endl << "Test OK" << std::endl;
  return 0;
}