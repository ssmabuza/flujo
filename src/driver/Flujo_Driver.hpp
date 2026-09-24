// ==============================================================================
//                Flujo: Copyright Valiant Scientific
//
// Distributed under BSD 3-clause license (See accompanying file Copyright.txt)
// ==============================================================================

#ifndef __Flujo_Driver_HPP__
#define __Flujo_Driver_HPP__

#include <vector>
#include <string>
#include <memory>
#include <stdexcept>

#include <Teuchos_RCP.hpp>
#include <Teuchos_ParameterList.hpp>
#include <Teuchos_Comm.hpp>
#include <Teuchos_DefaultComm.hpp>
#include <Teuchos_DefaultMpiComm.hpp>

#include <Panzer_EquationSet_Factory.hpp>
#include <Panzer_ClosureModel_Factory_TemplateManager.hpp>
#include <Panzer_BCStrategy_Factory.hpp>
#include <Panzer_Traits.hpp>
#include <Panzer_LinearObjFactory.hpp>
#include <Panzer_ResponseLibrary.hpp>

#include <Thyra_ModelEvaluator.hpp>
#include <Thyra_VectorBase.hpp>

namespace panzer {
class ConnManager;
class GlobalIndexer;
class PhysicsBlock;
class WorksetContainer;
class GlobalData;
}  // namespace panzer

namespace panzer_stk {
class STK_Interface;
class STK_MeshFactory;
template <typename ScalarT>
class ModelEvaluatorFactory;
}  // namespace panzer_stk

namespace flujo {

/**
 * @brief Base driver: wires Panzer STK mesh, physics blocks, ConnManager, DOF manager,
 *        workset container, model evaluators (physics & solver), observers, and solution
 *        pipeline from input ParameterLists.
 */
class Driver {
 public:
  Driver(const Teuchos::RCP<const Teuchos::MpiComm<int>>& comm,
         const Teuchos::RCP<const panzer::EquationSetFactory>& eqset_factory,
         const Teuchos::RCP<const panzer::ClosureModelFactory_TemplateManager<panzer::Traits>>& cm_factory,
         const Teuchos::RCP<const panzer::BCStrategyFactory>& bc_factory);

  virtual void setup(const Teuchos::RCP<Teuchos::ParameterList>& input_params);

  virtual void solve() const;

  virtual ~Driver() {}

  Teuchos::RCP<const Teuchos::MpiComm<int>> getComm() const { return comm_; }

  Teuchos::RCP<panzer_stk::STK_Interface> getMesh() const { return mesh_; }

  Teuchos::RCP<panzer_stk::STK_MeshFactory> getMeshFactory() const { return mesh_factory_; }

  Teuchos::RCP<panzer::ConnManager> getConnManager() const { return conn_manager_; }

  Teuchos::RCP<panzer::GlobalIndexer> getGlobalIndexer() const { return global_indexer_; }

  const std::vector<Teuchos::RCP<panzer::PhysicsBlock>>& getPhysicsBlocks() const {
    return physics_blocks_;
  }

  Teuchos::RCP<panzer::WorksetContainer> getWorksetContainer() const { return workset_container_; }

  Teuchos::RCP<panzer::GlobalData> getGlobalData() const { return global_data_; }

  Teuchos::RCP<panzer::LinearObjFactory<panzer::Traits>> getLinearObjFactory() const {
    return lin_obj_factory_;
  }

  Teuchos::RCP<Thyra::ModelEvaluator<double>> getPhysicsModelEvaluator() const {
    return physics_;
  }

  Teuchos::RCP<Thyra::ModelEvaluator<double>> getSolverModelEvaluator() const {
    return solver_;
  }

  Teuchos::RCP<panzer::ResponseLibrary<panzer::Traits>> getResponseLibrary() const {
    return response_library_;
  }

  Teuchos::RCP<panzer::ResponseLibrary<panzer::Traits>> getSTKIOResponseLibrary() const {
    return stk_io_response_library_;
  }

  Teuchos::RCP<Thyra::VectorBase<double>> getSolutionVector() const {
    return gx_;
  }

 protected:
  Teuchos::RCP<const Teuchos::MpiComm<int>> comm_;
  Teuchos::RCP<const panzer::EquationSetFactory> eqset_factory_;
  Teuchos::RCP<const panzer::ClosureModelFactory_TemplateManager<panzer::Traits>> cm_factory_;
  Teuchos::RCP<const panzer::BCStrategyFactory> bc_factory_;

  Teuchos::RCP<Teuchos::ParameterList> input_params_;
  Teuchos::RCP<panzer::GlobalData> global_data_;

  Teuchos::RCP<panzer_stk::STK_MeshFactory> mesh_factory_;
  Teuchos::RCP<panzer_stk::STK_Interface> mesh_;

  std::vector<Teuchos::RCP<panzer::PhysicsBlock>> physics_blocks_;

  Teuchos::RCP<panzer::ConnManager> conn_manager_;
  Teuchos::RCP<panzer::GlobalIndexer> global_indexer_;
  Teuchos::RCP<panzer::LinearObjFactory<panzer::Traits>> lin_obj_factory_;

  Teuchos::RCP<panzer::WorksetContainer> workset_container_;

  Teuchos::RCP<Thyra::ModelEvaluator<double>> physics_;
  Teuchos::RCP<Thyra::ModelEvaluator<double>> solver_;

  Teuchos::RCP<panzer::ResponseLibrary<panzer::Traits>> response_library_;
  Teuchos::RCP<panzer::ResponseLibrary<panzer::Traits>> stk_io_response_library_;

  mutable Teuchos::RCP<Thyra::VectorBase<double>> gx_;
};

}  // namespace flujo

#endif /** __Flujo_Driver_HPP__ */
