#ifndef RENDERENGINE_COMMAND_HPP
#define RENDERENGINE_COMMAND_HPP
#include <boost/container/small_vector.hpp>
#include <set>
#include <vkw/CommandBuffer.hpp>
#include <vkw/Queue.hpp>
#include <vkw/SwapChain.hpp>

namespace re {

enum class CommandType { EXECUTE, PRESENT };

struct QueueDescription {
  unsigned queueFamilyIndex;
  unsigned id;
};

class Command {
public:
  explicit Command(CommandType type, QueueDescription desc)
      : m_type(type), m_desc(desc) {}

  auto &queueDescription() const { return m_desc; }

  auto type() const { return m_type; }

  virtual ~Command() = default;

private:
  CommandType m_type;
  QueueDescription m_desc;
};

class ExecuteCommand final : public Command {
public:
  explicit ExecuteCommand(std::unique_ptr<vkw::SubmitInfo> &&submitInfo,
                          QueueDescription desc)
      : Command(CommandType::EXECUTE, desc),
        m_submitInfo(std::move(submitInfo)) {}

  auto &submitInfo() const { return *m_submitInfo; }

private:
  std::unique_ptr<vkw::SubmitInfo const> m_submitInfo;
};

class PresentCommand final : public Command {
public:
  explicit PresentCommand(std::unique_ptr<vkw::PresentInfo> &&presentInfo,
                          QueueDescription desc)
      : Command(CommandType::PRESENT, desc),
        m_presentInfo(std::move(presentInfo)) {}

  vkw::PresentInfo const &presentInfo() const { return *m_presentInfo; }

  vkw::PresentInfo &presentInfo() { return *m_presentInfo; }

private:
  std::unique_ptr<vkw::PresentInfo> m_presentInfo;
};

} // namespace re
#endif // RENDERENGINE_COMMAND_HPP
