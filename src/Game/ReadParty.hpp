#pragma once

#include "Game/PartyState.hpp"

namespace e33
{
// Leitura real da memória do jogo, pelos offsets do SDK gerado com o
// Dumper-7. Quando um patch quebrar o mod, é aqui.
class UnrealPartySource final : public IPartySource
{
public:
    PartySnapshot read() override;
    [[nodiscard]] bool available() const override;
};
} // namespace e33
