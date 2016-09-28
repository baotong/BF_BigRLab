#ifndef _FTRL_MODEL_H_
#define _FTRL_MODEL_H_

#include <memory>
#include <boost/thread.hpp>
#include <boost/thread/lockable_adapter.hpp>
#include "fast_ftrl_solver.h"
#include "ftrl_train.h"
#include "util.h"


class FtrlModel 
        : public boost::upgrade_lockable_adapter<boost::shared_mutex> {
public:
    typedef std::pair<size_t, double>    AttrValue;
    typedef std::vector<AttrValue>       AttrArray;

public:
    void init(const std::string &modelFile);
    double predict(const AttrArray &arr);
    void updateModel(const std::string &oldModel, 
                     const std::string &trainData, int epoch);

private:
    std::unique_ptr< LRModel<double> >      m_pModel;
};


#endif

