// ==============================================================================
//                Flujo: Copyright Valiant Scientific
//
// Distributed under BSD 3-clause license (See accompanying file Copyright.txt)
// ==============================================================================

#ifndef __Flujo_EquationSetFactory_HPP__
#define __Flujo_EquationSetFactory_HPP__

#include "Panzer_CellData.hpp"
#include "Panzer_EquationSet_Factory.hpp"
#include "Panzer_EquationSet_Factory_Defines.hpp"
#include "Panzer_EquationSet_TemplateManager.hpp"
#include "Panzer_GlobalData.hpp"
#include "Teuchos_RCP.hpp"
#include "Teuchos_TestForException.hpp"

#include "Flujo_EquationSet_Biot.hpp"
#include "Flujo_EquationSet_ConvectionDiffusionReaction.hpp"
#include "Flujo_EquationSet_EnergyTransport.hpp"
#include "Flujo_EquationSet_Maxwell.hpp"
#include "Flujo_EquationSet_NavierStokes.hpp"
#include "Flujo_EquationSet_Poisson.hpp"

namespace flujo {

PANZER_DECLARE_EQSET_TEMPLATE_BUILDER(EquationSet_Biot, EquationSet_Biot)
PANZER_DECLARE_EQSET_TEMPLATE_BUILDER(EquationSet_ConvectionDiffusionReaction,
                                      EquationSet_ConvectionDiffusionReaction)
PANZER_DECLARE_EQSET_TEMPLATE_BUILDER(EquationSet_EnergyTransport, EquationSet_EnergyTransport)
PANZER_DECLARE_EQSET_TEMPLATE_BUILDER(EquationSet_Maxwell, EquationSet_Maxwell)
PANZER_DECLARE_EQSET_TEMPLATE_BUILDER(EquationSet_NavierStokes, EquationSet_NavierStokes)
PANZER_DECLARE_EQSET_TEMPLATE_BUILDER(EquationSet_Poisson, EquationSet_Poisson)

class EquationSetFactory : public panzer::EquationSetFactory {
public:
  explicit EquationSetFactory(const Teuchos::RCP<panzer::GlobalData>& global_data = Teuchos::null)
      : global_data_(global_data) {}

  Teuchos::RCP<panzer::EquationSet_TemplateManager<panzer::Traits>> buildEquationSet(
      const Teuchos::RCP<Teuchos::ParameterList>& params, const int& default_integration_order,
      const panzer::CellData& cell_data, const Teuchos::RCP<panzer::GlobalData>& global_data,
      const bool build_transient_support) const override {
    auto eq_set = Teuchos::rcp(new panzer::EquationSet_TemplateManager<panzer::Traits>);
    bool found = false;

    PANZER_BUILD_EQSET_OBJECTS("Biot", EquationSet_Biot)
    PANZER_BUILD_EQSET_OBJECTS("CDR", EquationSet_ConvectionDiffusionReaction)
    PANZER_BUILD_EQSET_OBJECTS("Convection-Diffusion-Reaction",
                               EquationSet_ConvectionDiffusionReaction)
    PANZER_BUILD_EQSET_OBJECTS("Energy-Transport", EquationSet_EnergyTransport)
    PANZER_BUILD_EQSET_OBJECTS("Maxwell", EquationSet_Maxwell)
    PANZER_BUILD_EQSET_OBJECTS("Navier-Stokes", EquationSet_NavierStokes)
    PANZER_BUILD_EQSET_OBJECTS("Poisson", EquationSet_Poisson)

    if (!found) {
      const std::string msg =
          "Error - equation set type \"" + params->get<std::string>("Type") +
          "\" is not supported by flujo::EquationSetFactory.";
      TEUCHOS_TEST_FOR_EXCEPTION(true, std::logic_error, msg);
    }

    return eq_set;
  }

private:
  Teuchos::RCP<panzer::GlobalData> global_data_;
};

}  // namespace flujo

#endif
