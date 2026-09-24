// ==============================================================================
//                Flujo: Copyright Valiant Scientific
//
// Distributed under BSD 3-clause license (See accompanying file Copyright.txt)
// ==============================================================================

#include "Flujo_Driver.hpp"

#include <map>
#include <string>
#include <vector>
#include <iostream>

#include <Shards_CellTopology.hpp>

#include <Teuchos_TestForException.hpp>
#include <Teuchos_ParameterList.hpp>
#include <Teuchos_TimeMonitor.hpp>

#include "Panzer_GlobalData.hpp"
#include "Panzer_PhysicsBlock.hpp"
#include "Panzer_ElementBlockIdToPhysicsIdMap.hpp"
#include "Panzer_BlockedDOFManagerFactory.hpp"
#include "Panzer_DOFManagerFactory.hpp"
#include "Panzer_WorksetContainer.hpp"

#include "Panzer_STK_ModelEvaluatorFactory.hpp"
#include "Panzer_STKConnManager.hpp"
#include "Panzer_STK_WorksetFactory.hpp"
#include "Panzer_ThyraObjContainer.hpp"
#include "Panzer_AssemblyEngine.hpp"

#include <Thyra_VectorStdOps.hpp>

#include "Flujo_NOXObserverFactory.hpp"
#ifdef PANZER_HAVE_TEMPUS
#include "Flujo_TempusObserverFactory.hpp"
#endif

namespace flujo {

Driver::Driver(const Teuchos::RCP<const Teuchos::MpiComm<int>>& comm,
               const Teuchos::RCP<const panzer::EquationSetFactory>& eqset_factory,
               const Teuchos::RCP<const panzer::ClosureModelFactory_TemplateManager<panzer::Traits>>& cm_factory,
               const Teuchos::RCP<const panzer::BCStrategyFactory>& bc_factory)
    : comm_(comm),
      eqset_factory_(eqset_factory),
      cm_factory_(cm_factory),
      bc_factory_(bc_factory) {}

void Driver::setup(const Teuchos::RCP<Teuchos::ParameterList>& input_params) {
  TEUCHOS_TEST_FOR_EXCEPTION(input_params == Teuchos::null, std::logic_error,
                             "Flujo::Driver::setup: input_params is null.");
  TEUCHOS_TEST_FOR_EXCEPTION(comm_ == Teuchos::null, std::logic_error,
                             "Flujo::Driver::setup: MPI communicator is null.");

  input_params_ = input_params;
  global_data_ = panzer::createGlobalData();

  // Set communicator in user data for closures that require it
  input_params_->sublist("User Data").set("Comm", comm_);

  // 1. Build STK IO Response Library
  stk_io_response_library_ = Teuchos::rcp(new panzer::ResponseLibrary<panzer::Traits>());

  // 2. Check if output to Exodus is requested
  bool write_to_exodus = true;
  if (input_params_->isSublist("Output") && input_params_->sublist("Output").isParameter("Write to Exodus")) {
    write_to_exodus = input_params_->sublist("Output").get<bool>("Write to Exodus");
  }

  // 3. Build panzer_stk::ModelEvaluatorFactory
  panzer_stk::ModelEvaluatorFactory<double> me_factory;
  me_factory.setParameterList(input_params_);

  // 4. Set observers for NOX and Tempus
  Teuchos::RCP<const panzer_stk::NOXObserverFactory> nox_obs_factory =
      Teuchos::rcp(new NOXObserverFactory(stk_io_response_library_, write_to_exodus));
  me_factory.setNOXObserverFactory(nox_obs_factory);

#ifdef PANZER_HAVE_TEMPUS
  Teuchos::RCP<const panzer_stk::TempusObserverFactory> tempus_obs_factory =
      Teuchos::rcp(new TempusObserverFactory(stk_io_response_library_, write_to_exodus));
  me_factory.setTempusObserverFactory(tempus_obs_factory);
#endif

  // 5. Build core Panzer STK objects (Mesh, PhysicsBlocks, DOFManager, LinearObjFactory, Worksets, Physics ME)
  me_factory.buildObjects(comm_, global_data_, eqset_factory_, *bc_factory_, *cm_factory_);

  // 6. Cache references
  mesh_ = me_factory.getMesh();
  physics_blocks_ = me_factory.getPhysicsBlocks();
  conn_manager_ = me_factory.getConnManager();
  global_indexer_ = me_factory.getGlobalIndexer();
  lin_obj_factory_ = me_factory.getLinearObjFactory();
  workset_container_ = me_factory.getWorksetContainer();
  physics_ = me_factory.getPhysicsModelEvaluator();
  response_library_ = me_factory.getResponseLibrary();

  // Cache mesh factory if mesh sublist exists
  if (input_params_->isSublist("Mesh")) {
    mesh_factory_ = me_factory.buildSTKMeshFactory(input_params_->sublist("Mesh"));
  }

  // 7. Build solver ModelEvaluator (Piro wrapping NOX or Tempus)
  solver_ = me_factory.getResponseOnlyModelEvaluator();

  // 8. Initialize and compile STK IO response library
  if (response_library_ != Teuchos::null && stk_io_response_library_ != Teuchos::null) {
    stk_io_response_library_->initialize(*response_library_);

    Teuchos::ParameterList user_data;
    if (input_params_->isSublist("User Data")) {
      user_data = input_params_->sublist("User Data");
    }
    if (input_params_->isSublist("Assembly") && input_params_->sublist("Assembly").isParameter("Workset Size")) {
      user_data.set<int>("Workset Size", input_params_->sublist("Assembly").get<int>("Workset Size"));
    }

    Teuchos::ParameterList closure_models;
    if (input_params_->isSublist("Closure Models")) {
      closure_models = input_params_->sublist("Closure Models");
    }

    stk_io_response_library_->buildResponseEvaluators(physics_blocks_,
                                                     *cm_factory_,
                                                     closure_models,
                                                     user_data);
  }
}

void Driver::solve() const {
  TEUCHOS_TEST_FOR_EXCEPTION(solver_ == Teuchos::null, std::runtime_error,
                             "Flujo::Driver::solve: solver ModelEvaluator is null. Call setup() first.");
  TEUCHOS_TEST_FOR_EXCEPTION(physics_ == Teuchos::null, std::runtime_error,
                             "Flujo::Driver::solve: physics ModelEvaluator is null. Call setup() first.");

  Thyra::ModelEvaluatorBase::InArgs<double> inArgs = solver_->createInArgs();
  Thyra::ModelEvaluatorBase::OutArgs<double> outArgs = solver_->createOutArgs();

  // The global solution vector is returned as the last response slot
  Teuchos::RCP<Thyra::VectorBase<double>> gx = Thyra::createMember(*physics_->get_x_space());
  if (outArgs.Ng() > 0) {
    for (int i = 0; i < outArgs.Ng() - 1; ++i) {
      outArgs.set_g(i, Teuchos::null);
    }
    outArgs.set_g(outArgs.Ng() - 1, gx);
  }

  // Execute solve (NOX / Tempus)
  solver_->evalModel(inArgs, outArgs);
  gx_ = gx;

  // Evaluate and print diagnostic responses if any
  if (physics_->Ng() > 0) {
    Thyra::ModelEvaluatorBase::InArgs<double> respInArgs = physics_->createInArgs();
    Thyra::ModelEvaluatorBase::OutArgs<double> respOutArgs = physics_->createOutArgs();

    respInArgs.set_x(gx);

    for (int i = 0; i < respOutArgs.Ng(); ++i) {
      Teuchos::RCP<Thyra::VectorBase<double>> response = Thyra::createMember(*physics_->get_g_space(i));
      respOutArgs.set_g(i, response);
    }

    physics_->evalModel(respInArgs, respOutArgs);

    if (comm_ != Teuchos::null && comm_->getRank() == 0) {
      for (int i = 0; i < respOutArgs.Ng(); ++i) {
        Teuchos::RCP<Thyra::VectorBase<double>> resp = respOutArgs.get_g(i);
        if (resp != Teuchos::null) {
          std::cout << "Response " << i << ": " << Thyra::get_ele(*resp, 0) << std::endl;
        }
      }
    }
  }
}

}  // namespace flujo
