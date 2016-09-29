#include <boost/filesystem.hpp>
#include "common.hpp"
#include "FtrlModel.h"

using namespace std;

void FtrlModel::init(const std::string &modelFile)
{
    boost::unique_lock<FtrlModel> lock(*this);
    m_pModel.reset(new LRModel<double>());
    m_pModel->Initialize(modelFile.c_str());
}

double FtrlModel::predict(const AttrArray &arr)
{
    boost::shared_lock<FtrlModel> lock(*this);
    double result = m_pModel->Predict(arr);
    lock.unlock();
    result = std::max(std::min(result, 1. - 10e-15), 10e-15);
    return result;
}

void FtrlModel::updateModel(const std::string &oldModel, 
                            const std::string &trainData, int epoch)
{
    string oldModelSave = oldModel + ".save";
    string newModel = oldModel + ".tmp";
    string newModelSave = newModel + ".save";

    // check oldModel
    {
        ifstream ifs(oldModel, ios::in);
        if (!ifs)
            THROW_RUNTIME_ERROR("Cannot open " << oldModel << " for reading!");
    }
    // check oldModelSave
    {
        ifstream ifs(oldModelSave, ios::in);
        if (!ifs)
            THROW_RUNTIME_ERROR("Cannot open " << oldModelSave << " for reading!");
    }
    
    // train
    {
        FtrlTrainer<double> trainer;
        trainer.Initialize(epoch, true); // 2nd arg is cache
        trainer.Train(oldModelSave.c_str(), newModel.c_str(), trainData.c_str(), (char*)NULL); // last arg is test
    }

    boost::unique_lock<FtrlModel> lock(*this);
    m_pModel.reset();
    boost::filesystem::rename(newModel, oldModel);
    boost::filesystem::rename(newModelSave, oldModelSave);
    m_pModel.reset(new LRModel<double>());
    m_pModel->Initialize(oldModel.c_str());
}
