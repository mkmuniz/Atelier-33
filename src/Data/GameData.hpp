#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "Model/Types.hpp"

namespace e33
{
// Tabelas extraídas do jogo por tools/extract.ts, uma pasta por versão.
//
// Carregar é tolerante por item e intolerante por arquivo: uma linha torta é
// pulada com aviso, mas um arquivo ausente é reportado, porque otimizar sem a
// tabela de pictos não é uma degradação, é um resultado errado.
class GameData
{
public:
    struct LoadReport
    {
        std::vector<std::string> errors{};
        std::size_t pictos{0};
        std::size_t luminas{0};
        std::size_t weapons{0};
        std::size_t skills{0};
        std::size_t enemies{0};

        [[nodiscard]] bool ok() const { return errors.empty(); }
    };

    LoadReport load(const std::filesystem::path& version_dir);

    [[nodiscard]] const std::vector<Picto>& pictos() const { return m_pictos; }
    [[nodiscard]] const std::vector<Lumina>& luminas() const { return m_luminas; }
    [[nodiscard]] const std::vector<Weapon>& weapons() const { return m_weapons; }
    [[nodiscard]] const std::vector<Skill>& skills() const { return m_skills; }
    [[nodiscard]] const std::vector<Target>& enemies() const { return m_enemies; }

    [[nodiscard]] const Picto* find_picto(std::string_view id) const;
    [[nodiscard]] const Lumina* find_lumina(std::string_view id) const;
    [[nodiscard]] const Skill* find_skill(std::string_view id) const;
    [[nodiscard]] const Target* find_enemy(std::string_view id) const;

    [[nodiscard]] const std::string& version() const { return m_version; }

private:
    std::vector<Picto> m_pictos{};
    std::vector<Lumina> m_luminas{};
    std::vector<Weapon> m_weapons{};
    std::vector<Skill> m_skills{};
    std::vector<Target> m_enemies{};
    std::string m_version{};
};
} // namespace e33
