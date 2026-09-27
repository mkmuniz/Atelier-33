#pragma once

#include <string>
#include <vector>

#include "Model/Types.hpp"

namespace e33
{
// Um personagem como ele está no jogo agora.
struct CharacterSnapshot
{
    std::string id{};
    std::string name{};
    Stats base_stats{};        // stats do personagem, sem pictos nem arma
    std::string weapon_id{};
    std::vector<std::string> equipped_picto_ids{};
    std::vector<std::string> active_lumina_ids{};
    int picto_slots{3};
    int lumina_budget{0};
    int lumina_spent{0};
};

struct PartySnapshot
{
    std::vector<CharacterSnapshot> characters{};
    std::vector<std::string> owned_picto_ids{};
    std::vector<std::string> owned_lumina_ids{};
    bool valid{false};          // false = não foi possível ler o jogo
    std::string error{};
};

// Fonte do estado do jogo. A implementação real é a única parte que toca a
// memória do Unreal; tudo acima dela é testável sem o jogo.
class IPartySource
{
public:
    virtual ~IPartySource() = default;

    // Lê o estado atual. Deve ser barato o bastante para rodar algumas vezes
    // por segundo — o overlay precisa refletir a troca de um picto no menu.
    [[nodiscard]] virtual PartySnapshot read() = 0;
    [[nodiscard]] virtual bool available() const = 0;
};

// Fonte de teste e do harness: devolve o que lhe foi dado.
class MockPartySource final : public IPartySource
{
public:
    explicit MockPartySource(PartySnapshot snapshot) : m_snapshot{std::move(snapshot)} {}

    PartySnapshot read() override
    {
        ++m_reads;
        return m_snapshot;
    }
    [[nodiscard]] bool available() const override { return m_snapshot.valid; }

    void set(PartySnapshot snapshot) { m_snapshot = std::move(snapshot); }
    [[nodiscard]] std::size_t reads() const { return m_reads; }

private:
    PartySnapshot m_snapshot{};
    std::size_t m_reads{0};
};
} // namespace e33
