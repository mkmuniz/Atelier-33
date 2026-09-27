#pragma once

#include <span>
#include <vector>

#include "Model/Types.hpp"

namespace e33::opt
{
// Descarta itens dominados: mesmo custo (ou menor) e stats que não são
// melhores em nada. É o primeiro passo do M5 porque costuma cortar uma ordem de
// grandeza do espaço de busca antes de qualquer enumeração.
//
// Preserva a ordem original e, entre itens idênticos, mantém exatamente um —
// senão um par de cópias se elimina mutuamente e a melhor build some.
[[nodiscard]] std::vector<const Picto*> prune_dominated(std::span<const Picto> pictos,
                                                        bool include_unowned);
[[nodiscard]] std::vector<const Lumina*> prune_dominated(std::span<const Lumina> luminas,
                                                         bool include_unowned);
} // namespace e33::opt
