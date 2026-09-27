#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>

#include "Optimizer/Search.hpp"

namespace e33::opt
{
// Busca em thread separada.
//
// Obrigatório, não otimização posterior: se a busca rodar dentro do frame do
// jogo, o mod é inutilizável. A UI lê progresso e resultado sem bloquear, e
// pode cancelar a qualquer momento.
class Job
{
public:
    Job() = default;
    ~Job();

    Job(const Job&) = delete;
    Job& operator=(const Job&) = delete;

    // Começa uma busca. Se já havia uma rodando, cancela e espera antes.
    void start(Request request, Options options = {});

    void cancel();

    [[nodiscard]] bool running() const { return m_running.load(std::memory_order_acquire); }
    [[nodiscard]] double progress() const { return m_progress.load(std::memory_order_relaxed); }
    [[nodiscard]] double elapsed_seconds() const;

    // Resultado pronto, se houver. Só devolve depois que a thread terminou.
    [[nodiscard]] std::optional<Result> take_result();
    [[nodiscard]] bool has_result() const;

private:
    void join_if_joinable();

    std::thread m_thread{};
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_cancel{false};
    std::atomic<double> m_progress{0.0};
    std::atomic<double> m_elapsed{0.0};

    mutable std::mutex m_result_mutex{};
    std::optional<Result> m_result{};
};
} // namespace e33::opt
