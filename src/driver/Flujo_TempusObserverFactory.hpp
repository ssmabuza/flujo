// ==============================================================================
//                Flujo: Copyright Valiant Scientific
//
// Distributed under BSD 3-clause license (See accompanying file Copyright.txt)
// ==============================================================================

#ifndef __Flujo_TempusObserverFactory_HPP__
#define __Flujo_TempusObserverFactory_HPP__

#include "PanzerAdaptersSTK_config.hpp"

#ifdef PANZER_HAVE_TEMPUS

#include <string>
#include <vector>

#include <Teuchos_RCP.hpp>
#include <Teuchos_Assert.hpp>

#include <Tempus_Integrator.hpp>
#include <Tempus_IntegratorObserver.hpp>
#include <Tempus_IntegratorObserverComposite.hpp>

#include <Panzer_STK_TempusObserverFactory.hpp>
#include <Panzer_STK_Interface.hpp>
#include <Panzer_GlobalIndexer.hpp>
#include <Panzer_LinearObjFactory.hpp>
#include <Panzer_ResponseLibrary.hpp>
#include <Panzer_STK_ResponseEvaluatorFactory_SolutionWriter.hpp>
#include <Panzer_ThyraObjContainer.hpp>
#include <Panzer_AssemblyEngine.hpp>

#include <Thyra_VectorBase.hpp>
#include <Thyra_DefaultMultiVectorProductVector.hpp>

namespace flujo {

class TempusObserver_WriteToExodus : public Tempus::IntegratorObserver<double> {
public:
  TempusObserver_WriteToExodus(
      const Teuchos::RCP<panzer_stk::STK_Interface>& mesh,
      const Teuchos::RCP<const panzer::GlobalIndexer>& dof_manager,
      const Teuchos::RCP<const panzer::LinearObjFactory<panzer::Traits>>& lof,
      const Teuchos::RCP<panzer::ResponseLibrary<panzer::Traits>>& response_library)
      : mesh_(mesh),
        dof_manager_(dof_manager),
        lof_(lof),
        response_library_(response_library) {
    if (mesh_ != Teuchos::null && response_library_ != Teuchos::null) {
      std::vector<std::string> eBlocks;
      mesh_->getElementBlockNames(eBlocks);

      panzer_stk::RespFactorySolnWriter_Builder builder;
      builder.mesh = mesh_;
      response_library_->addResponse("Main Field Output", eBlocks, builder);
    }
  }

  void observeStartIntegrator(const Tempus::Integrator<double>& integrator) override {
    writeToExodus(integrator);
  }
  void observeStartTimeStep(const Tempus::Integrator<double>&) override {}
  void observeNextTimeStep(const Tempus::Integrator<double>&) override {}
  void observeBeforeTakeStep(const Tempus::Integrator<double>&) override {}
  void observeAfterTakeStep(const Tempus::Integrator<double>&) override {}
  void observeAfterCheckTimeStep(const Tempus::Integrator<double>&) override {}
  void observeEndTimeStep(const Tempus::Integrator<double>& integrator) override {
    writeToExodus(integrator);
  }
  void observeEndIntegrator(const Tempus::Integrator<double>&) override {}

  void writeToExodus(const Tempus::Integrator<double>& integrator) {
    if (mesh_ == Teuchos::null || !mesh_->isExodusInitialized() ||
        lof_ == Teuchos::null || response_library_ == Teuchos::null) {
      return;
    }

    Teuchos::RCP<const Thyra::VectorBase<double>> solution =
        integrator.getSolutionHistory()->getStateTimeIndexN()->getX();

    auto augmented =
        Teuchos::rcp_dynamic_cast<const Thyra::DefaultMultiVectorProductVector<double>>(solution);
    if (augmented != Teuchos::null) {
      solution = augmented->getMultiVector()->col(0);
    }

    panzer::AssemblyEngineInArgs ae_inargs;
    ae_inargs.container_ = lof_->buildLinearObjContainer();
    ae_inargs.ghostedContainer_ = lof_->buildGhostedLinearObjContainer();
    ae_inargs.alpha = 0.0;
    ae_inargs.beta = 1.0;
    ae_inargs.evaluate_transient_terms = false;

    lof_->initializeGhostedContainer(panzer::LinearObjContainer::X, *ae_inargs.ghostedContainer_);

    const Teuchos::RCP<panzer::ThyraObjContainer<double>> thyraContainer =
        Teuchos::rcp_dynamic_cast<panzer::ThyraObjContainer<double>>(ae_inargs.container_, true);
    thyraContainer->set_x_th(Teuchos::rcp_const_cast<Thyra::VectorBase<double>>(solution));

    response_library_->addResponsesToInArgs<panzer::Traits::Residual>(ae_inargs);
    response_library_->evaluate<panzer::Traits::Residual>(ae_inargs);

    mesh_->writeToExodus(integrator.getSolutionHistory()->getCurrentTime());
  }

private:
  Teuchos::RCP<panzer_stk::STK_Interface> mesh_;
  Teuchos::RCP<const panzer::GlobalIndexer> dof_manager_;
  Teuchos::RCP<const panzer::LinearObjFactory<panzer::Traits>> lof_;
  Teuchos::RCP<panzer::ResponseLibrary<panzer::Traits>> response_library_;
};

class TempusObserverFactory : public panzer_stk::TempusObserverFactory {
public:
  explicit TempusObserverFactory(
      const Teuchos::RCP<panzer::ResponseLibrary<panzer::Traits>>& stkIOResponseLibrary,
      bool write_to_exodus = true)
      : stkIOResponseLibrary_(stkIOResponseLibrary),
        write_to_exodus_(write_to_exodus) {}

  bool useNOXObserver() const override { return false; }

  Teuchos::RCP<Tempus::IntegratorObserver<double>> buildTempusObserver(
      const Teuchos::RCP<panzer_stk::STK_Interface>& mesh,
      const Teuchos::RCP<const panzer::GlobalIndexer>& dof_manager,
      const Teuchos::RCP<const panzer::LinearObjFactory<panzer::Traits>>& lof) const override {
    Teuchos::RCP<Tempus::IntegratorObserverComposite<double>> composite =
        Teuchos::rcp(new Tempus::IntegratorObserverComposite<double>);
    if (write_to_exodus_) {
      composite->addObserver(
          Teuchos::rcp(new TempusObserver_WriteToExodus(mesh, dof_manager, lof, stkIOResponseLibrary_)));
    }
    return composite;
  }

private:
  Teuchos::RCP<panzer::ResponseLibrary<panzer::Traits>> stkIOResponseLibrary_;
  bool write_to_exodus_;
};

}  // namespace flujo

#endif  // PANZER_HAVE_TEMPUS
#endif  /** __Flujo_TempusObserverFactory_HPP__ */
