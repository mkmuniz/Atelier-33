#include "Optimizer/Job.hpp"

#include <chrono>
#include <utility>

namespace e33::opt
{
Job::~Job()
{
    cancel();
    join_if_joinable();
}

void Job::start(Request request, Options options)
{
    // Uma busca por vez: começar outra enquanto a anterior roda daria dois
    // resultados competindo pelo mesmo painel.
    cancel();
    join_if_joinable();

    {
        const std::lock_guard lock{m_result_mutex};
        m_result.reset();
    }
    m_cancel.store(false, std::memory_order_release);
    m_progress.store(0.0, std::memory_order_relaxed);
    m_elapsed.store(0.0, std::memory_order_relaxed);
    m_running.store(true, std::memory_order_release);

    m_thread = std::thread{[this, request = std::move(request), options]() mutable {
        const auto started = std::chrono::steady_clock::now();

        auto result = search(request, options, [this, started](double fraction) {
            m_progress.store(fraction, std::memory_order_relaxed);
            const std::chrono::duration<double> elapsed =
                std::chrono::steady_clock::now() - started;
            m_elapsed.store(elapsed.count(), std::memory_order_relaxed);
            return !m_cancel.load(std::memory_order_acquire);
        });

        const std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - started;
        m_elapsed.store(elapsed.count(), std::memory_order_relaxed);
        m_progress.store(1.0, std::memory_order_relaxed);

        {
            const std::lock_guard lock{m_result_mutex};
            m_result = std::move(result);
        }
        // running por último: quem vê running()==false pode ler o resultado.
        m_running.store(false, std::memory_order_release);
    }};
}

void Job::cancel()
{
    m_cancel.store(true, std::memory_order_release);
}

double Job::elapsed_seconds() const
{
    return m_elapsed.load(std::memory_order_relaxed);
}

bool Job::has_result() const
{
    const std::lock_guard lock{m_result_mutex};
    return m_result.has_value();
}

std::optional<Result> Job::take_result()
{
    if (m_running.load(std::memory_order_acquire))
    {
        return std::nullopt;
    }
    join_if_joinable();

    const std::lock_guard lock{m_result_mutex};
    if (!m_result)
    {
        return std::nullopt;
    }
    auto result = std::move(*m_result);
    m_result.reset();
    return result;
}

void Job::join_if_joinable()
{
    if (m_thread.joinable())
    {
        m_thread.join();
    }
}
} // namespace e33::opt
