// ==============================================================================
//                Flujo: Copyright Valiant Scientific
//
// Distributed under BSD 3-clause license (See accompanying file Copyright.txt)
// ==============================================================================

#ifndef __Flujo_NOXObserverFactory_HPP__
#define __Flujo_NOXObserverFactory_HPP__

#include <string>
#include <vector>

#include <Teuchos_RCP.hpp>
#include <Teuchos_Assert.hpp>
#include <Teuchos_ParameterList.hpp>

#include <NOX_Abstract_PrePostOperator.H>
#include <NOX_Thyra_Vector.H>
#include <NOX_PrePostOperator_Vector.H>

#include <Panzer_STK_NOXObserverFactory.hpp>
#include <Panzer_STK_Interface.hpp>
#include <Panzer_GlobalIndexer.hpp>
#include <Panzer_LinearObjFactory.hpp>
#include <Panzer_ResponseLibrary.hpp>
#include <Panzer_STK_ResponseEvaluatorFactory_SolutionWriter.hpp>
#include <Panzer_ThyraObjContainer.hpp>
#include <Panzer_AssemblyEngine.hpp>

#include <Thyra_VectorBase.hpp>

namespace flujo {

class NOXObserver_WriteToExodus : public NOX::Abstract::PrePostOperator {
public:
  NOXObserver_WriteToExodus(
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

  void runPreIterate(const NOX::Solver::Generic&) override {}
  void runPostIterate(const NOX::Solver::Generic&) override {}

  void runPreSolve(const NOX::Solver::Generic&) override {
    if (mesh_ != Teuchos::null && mesh_->isExodusInitialized()) {
      mesh_->writeToExodus(0.0);
    }
  }

  void runPostSolve(const NOX::Solver::Generic& solver) override {
    if (mesh_ == Teuchos::null || !mesh_->isExodusInitialized() ||
        lof_ == Teuchos::null || response_library_ == Teuchos::null) {
      return;
    }

    const NOX::Abstract::Vector& x = solver.getSolutionGroup().getX();
    const NOX::Thyra::Vector* n_th_x = dynamic_cast<const NOX::Thyra::Vector*>(&x);
    if (n_th_x == nullptr) {
      return;
    }
    Teuchos::RCP<const Thyra::VectorBase<double>> th_x = n_th_x->getThyraRCPVector();

    panzer::AssemblyEngineInArgs ae_inargs;
    ae_inargs.container_ = lof_->buildLinearObjContainer();
    ae_inargs.ghostedContainer_ = lof_->buildGhostedLinearObjContainer();
    ae_inargs.alpha = 0.0;
    ae_inargs.beta = 1.0;
    ae_inargs.evaluate_transient_terms = false;

    lof_->initializeGhostedContainer(panzer::LinearObjContainer::X, *ae_inargs.ghostedContainer_);

    const Teuchos::RCP<panzer::ThyraObjContainer<double>> thyraContainer =
        Teuchos::rcp_dynamic_cast<panzer::ThyraObjContainer<double>>(ae_inargs.container_, true);
    thyraContainer->set_x_th(Teuchos::rcp_const_cast<Thyra::VectorBase<double>>(th_x));

    response_library_->addResponsesToInArgs<panzer::Traits::Residual>(ae_inargs);
    response_library_->evaluate<panzer::Traits::Residual>(ae_inargs);

    mesh_->writeToExodus(1.0);
  }

private:
  Teuchos::RCP<panzer_stk::STK_Interface> mesh_;
  Teuchos::RCP<const panzer::GlobalIndexer> dof_manager_;
  Teuchos::RCP<const panzer::LinearObjFactory<panzer::Traits>> lof_;
  Teuchos::RCP<panzer::ResponseLibrary<panzer::Traits>> response_library_;
};

class NOXObserverFactory : public panzer_stk::NOXObserverFactory {
public:
  explicit NOXObserverFactory(
      const Teuchos::RCP<panzer::ResponseLibrary<panzer::Traits>>& stkIOResponseLibrary,
      bool write_to_exodus = true)
      : stkIOResponseLibrary_(stkIOResponseLibrary),
        write_to_exodus_(write_to_exodus) {}

  Teuchos::RCP<NOX::Abstract::PrePostOperator> buildNOXObserver(
      const Teuchos::RCP<panzer_stk::STK_Interface>& mesh,
      const Teuchos::RCP<const panzer::GlobalIndexer>& dof_manager,
      const Teuchos::RCP<const panzer::LinearObjFactory<panzer::Traits>>& lof) const override {
    Teuchos::RCP<NOX::PrePostOperatorVector> observer =
        Teuchos::rcp(new NOX::PrePostOperatorVector);
    if (write_to_exodus_) {
      observer->pushBack(
          Teuchos::rcp(new NOXObserver_WriteToExodus(mesh, dof_manager, lof, stkIOResponseLibrary_)));
    }
    return observer;
  }

private:
  Teuchos::RCP<panzer::ResponseLibrary<panzer::Traits>> stkIOResponseLibrary_;
  bool write_to_exodus_;
};

}  // namespace flujo

#endif /** __Flujo_NOXObserverFactory_HPP__ */
