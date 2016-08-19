#include "common.hpp"
#include "xgboost_learner.h"

using namespace std;
using namespace xgboost;

DMLC_REGISTER_PARAMETER(CLIParam);


DMatrix* XgBoostLearner::DMatrixFromStr( const std::string &line )
{
    string                       item;
    // unsigned long                indp[2];
    vector<unsigned long>        indp;
    vector<uint32_t>             indices;
    vector<float>                values;
    uint32_t         index = 0;
    float            value = 0.0;

    stringstream stream(line);
    while (stream >> item) {
        if (sscanf(item.c_str(), "%u:%f", &index, &value) != 2)
            continue;
        indices.push_back(index);
        values.push_back(value);
    } // while
    // indp[0] = 0;
    // indp[1] = indices.size()+1;
    indp.push_back(0);
    indp.push_back(indices.size()+1);

    DMatrix *mat = NULL;

    if (!values.empty())
        XGDMatrixCreateFromCSR(&indp[0], &indices[0], &values[0],
                        indp.size(), indices.size(), (DMatrixHandle*)&mat);

    return mat;
}

XgBoostLearner::XgBoostLearner( CLIParam *param, CLIParam *param2 ) 
            : m_pParam(param), m_pParam2(param2)
{
    using namespace std;

    m_pLearner.reset(Learner::Create({}));
    // 加载model和配置参数
    {
        std::unique_ptr<dmlc::Stream> fi(
                dmlc::Stream::Create( param->model_in.c_str(), "r" ) );
        m_pLearner->Configure( param->cfg );
        m_pLearner->Load(fi.get());
    }

    m_pfnPredict2 = std::bind( &XgBoostLearner::noPredict2, this, placeholders::_1, placeholders::_2 );

    if (param2) {
        m_pLearner2.reset(Learner::Create({}));
        std::unique_ptr<dmlc::Stream> fi(
                dmlc::Stream::Create( param2->model_in.c_str(), "r" ) );
        m_pLearner2->Configure( param2->cfg );
        m_pLearner2->Load(fi.get());
        m_pfnPredict2 = std::bind( &XgBoostLearner::doPredict2, this, placeholders::_1, placeholders::_2 );
    } // if
}

void XgBoostLearner::predict( DMatrix *inMat, std::vector<float> &result, bool pred_leaf )
{
    result.clear();
    m_pLearner->Predict(inMat, m_pParam->pred_margin,
            &result, m_pParam->ntree_limit, pred_leaf);
}

void XgBoostLearner::doPredict2( xgboost::DMatrix *inMat, std::vector<float> &result )
{
    result.clear();
    m_pLearner2->Predict(inMat, m_pParam2->pred_margin,
            &result, m_pParam2->ntree_limit, false);
}

void XgBoostLearner::noPredict2( xgboost::DMatrix *inMat, std::vector<float> &result )
{
    THROW_RUNTIME_ERROR("Not supported!");
}
