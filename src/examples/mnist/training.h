#pragma once

#include "common/options.h"
#include "models/model_task.h"
#include "training/scheduler.h"

#include "examples/mnist/dataset.h"
#include "examples/mnist/validator.h"

namespace marian {

template <class ModelWrapper>
class TrainMNIST : public ModelTask {
private:
  Ptr<Options> options_;

public:
  TrainMNIST(Ptr<Options> options) : options_(options) {}

  void run() override {
    using namespace data;

    // Prepare data set
    auto paths = options_->get<std::vector<std::string>>("train-sets");
    auto dataset = New<data::MNISTData>(paths);
    auto batchGenerator = New<BatchGenerator<data::MNISTData>>(dataset, options_, nullptr);

    // Prepare scheduler with validators
    auto trainState = New<TrainingState>(options_->get<float>("learn-rate"));
    auto scheduler = New<Scheduler>(options_, trainState);
    scheduler->addValidator(New<AccuracyValidator>(options_));

    // Prepare model
    auto model = New<ModelWrapper>(options_);
    model->setScheduler(scheduler);
    model->load();

    // Run training
    while(scheduler->keepGoing()) {
      batchGenerator->prepare(!options_->get<bool>("no-shuffle"));
      for(auto batch : *batchGenerator) {
        if(!scheduler->keepGoing())
           break;
        model->update(batch);
      }
      if(scheduler->keepGoing())
        scheduler->increaseEpoch();
    }
    scheduler->finished();
  }
};
}  // namespace marian
