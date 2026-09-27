#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "Calc/Formula.hpp"
#include "Data/GameData.hpp"

namespace e33::calc
{
// Uma medição real: build exata, alvo, skill e o dano observado em jogo.
//
// Vinte fixtures bem escolhidas (com e sem break, com e sem fraqueza, crítico,
// buff empilhado) valem mais que duzentas aleatórias — o que se quer cobrir são
// os multiplicadores, não repetições do mesmo caso.
struct Fixture
{
    std::string id{};
    std::string note{};
    Stats base_stats{};
    std::vector<std::string> picto_ids{};
    std::vector<std::string> lumina_ids{};
    std::string skill_id{};
    std::string enemy_id{};
    bool broken{false};
    double observed_damage{0.0};
    // Dano observado é um número medido em jogo; como o crítico é aleatório, a
    // fixture diz qual dos três foi medido.
    enum class Kind
    {
        NonCrit,
        Crit,
        Expected,
    } kind{Kind::NonCrit};
};

struct FixtureError
{
    std::string id{};
    double observed{0.0};
    double predicted{0.0};
    [[nodiscard]] double relative() const;
};

struct FixtureReport
{
    std::vector<FixtureError> errors{};
    std::vector<std::string> skipped{};  // fixtures cujos ids não existem nos dados
    [[nodiscard]] double mean_absolute_relative_error() const;
    [[nodiscard]] double worst_absolute_relative_error() const;
    [[nodiscard]] std::size_t count() const { return errors.size(); }
};

// Carrega todas as fixtures de um diretório (arquivos *.json).
[[nodiscard]] std::vector<Fixture> load_fixtures(const std::filesystem::path& dir);

// Compara previsão e medição. É o número que o README publica.
[[nodiscard]] FixtureReport evaluate_fixtures(const std::vector<Fixture>& fixtures,
                                              const GameData& data,
                                              const FormulaCoefficients& coefficients);
} // namespace e33::calc
