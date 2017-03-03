#ifndef _KNN_SERVICE_HANDLER_H_
#define _KNN_SERVICE_HANDLER_H_

#include <gflags/gflags.h>
#include "WordAnnDB.h"


namespace KNN {

class KnnServiceHandler : virtual public KnnServiceIf {
public:
    virtual void queryByItem(std::vector<Result> & _return, const std::string& item, const int32_t n);
    virtual void queryByVector(std::vector<Result> & _return, const std::vector<double> & values, const int32_t n);
    virtual void queryByVectorNoWeight(std::vector<std::string> & _return, const std::vector<double> & values, const int32_t n);
    virtual void queryByItemNoWeight(std::vector<std::string> & _return, const std::string& item, const int32_t n);
    virtual void handleRequest(std::string& _return, const std::string& request);
};

} // namespace KNN

DECLARE_int32(nfields);
extern boost::shared_ptr<KNN::WordAnnDB> g_pWordAnnDB;


#endif


