#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace e33
{
// Preferências do mod. A mesma classe existe no Boss Music Swapper: as duas
// devem convergir para o e33-modkit quando ele existir.
struct Settings
{
    std::string hotkey{"F8"};     // padrão do plano; J é do Gramophone Everywhere
    float font_scale{1.0f};       // essencial em 4K
    bool verbose_log{false};
    bool start_open{false};
    bool compact_mode{false};     // só o número de dano, overlay sempre aberto
    bool include_unowned{false};  // considerar pictos ainda não obtidos
    std::string data_version{"1.5.0"};

    static constexpr float kMinFontScale = 0.5f;
    static constexpr float kMaxFontScale = 3.0f;

    void load(std::filesystem::path path);
    [[nodiscard]] bool save() const;

    [[nodiscard]] std::string to_json_string() const;
    bool apply_json(std::string_view text);
    [[nodiscard]] std::optional<int> hotkey_virtual_key() const;
    [[nodiscard]] const std::filesystem::path& path() const { return m_path; }

private:
    void clamp_values();
    std::filesystem::path m_path{};
};

[[nodiscard]] std::optional<int> virtual_key_from_name(std::string_view name);
[[nodiscard]] std::string_view name_from_virtual_key(int vk);
} // namespace e33
