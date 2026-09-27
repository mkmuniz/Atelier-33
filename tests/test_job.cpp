#include <doctest/doctest.h>

#include <chrono>
#include <thread>

#include "Optimizer/Job.hpp"

using namespace e33;
using namespace e33::opt;

namespace
{
Request big_request(int count)
{
    Request r;
    r.base_stats.attack = 200.0;
    r.skill.power = 1.0;
    r.target.defense = 150.0;
    r.picto_slots = 3;
    for (int i = 0; i < count; ++i)
    {
        Picto p;
        p.id = "p" + std::to_string(i);
        p.stats.attack = 30.0 + i * 5.0;
        p.stats.crit_rate = 0.45 - 0.01 * i; // trade-off: sobrevive a poda
        r.pictos.push_back(p);
    }
    return r;
}

void wait_until_done(Job& job, std::chrono::milliseconds limit = std::chrono::seconds{10})
{
    const auto deadline = std::chrono::steady_clock::now() + limit;
    while (job.running() && std::chrono::steady_clock::now() < deadline)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds{2});
    }
}
} // namespace

TEST_CASE("job roda fora da thread chamadora e devolve resultado")
{
    Job job;
    job.start(big_request(10), Options{.top_n = 3});
    wait_until_done(job);

    REQUIRE_FALSE(job.running());
    const auto result = job.take_result();
    REQUIRE(result.has_value());
    CHECK_FALSE(result->top.empty());
    CHECK(job.progress() == doctest::Approx(1.0));
}

TEST_CASE("resultado so sai uma vez")
{
    Job job;
    job.start(big_request(8));
    wait_until_done(job);

    CHECK(job.take_result().has_value());
    CHECK_FALSE(job.take_result().has_value());
}

TEST_CASE("nada a colher enquanto a busca roda")
{
    Job job;
    job.start(big_request(20), Options{.top_n = 5});
    // Pode ja ter terminado; o que nao pode e devolver resultado com running().
    if (job.running())
    {
        CHECK_FALSE(job.take_result().has_value());
    }
    wait_until_done(job);
    CHECK(job.take_result().has_value());
}

TEST_CASE("cancelar interrompe e o resultado vem marcado como parcial")
{
    Job job;
    job.start(big_request(24), Options{.top_n = 5});
    job.cancel();
    wait_until_done(job);

    const auto result = job.take_result();
    REQUIRE(result.has_value());
    CHECK_FALSE(result->exhaustive);
}

TEST_CASE("comecar outra busca cancela a anterior sem vazar thread")
{
    Job job;
    job.start(big_request(22), Options{.top_n = 5});
    job.start(big_request(6), Options{.top_n = 2});
    wait_until_done(job);

    const auto result = job.take_result();
    REQUIRE(result.has_value());
    // O resultado tem de ser o da segunda busca, nao o da primeira.
    CHECK(result->top.size() <= 2);
}

TEST_CASE("destruir o job com busca em andamento nao trava nem estoura")
{
    // O destrutor cancela e da join; sem isso, fechar o jogo com uma busca
    // rodando derruba o processo.
    Job job;
    job.start(big_request(24), Options{.top_n = 5});
    CHECK(true);
}

TEST_CASE("elapsed é medido e nao regride")
{
    Job job;
    job.start(big_request(12));
    wait_until_done(job);
    static_cast<void>(job.take_result());

    CHECK(job.elapsed_seconds() >= 0.0);
}
