#include <sstream>
#include <json/json.h>
#include "FtrlServiceHandler.h"
#include "common.hpp"
#include "alg_common.hpp"

namespace FTRL {

using namespace std;

double FtrlServiceHandler::lrPredict(const std::string& id, const std::string& data)
{
    if (id.empty())
        THROW_INVALID_REQUEST("id cannot be empty!");

    const char *pId = id.c_str();
    if (pId[0] < 0 || pId[0] > 127 || pId[1] < 0 || pId[1] > 127)
        THROW_INVALID_REQUEST("id can only contains alphabet or digit!");

    FtrlModel::AttrArray    arr;
    string                  item;
    size_t                  idx;
    double                  value;

    arr.reserve(256);
    arr.emplace_back(std::make_pair(0, 1));

    stringstream stream(data);
    while (stream >> item) {
        if (sscanf(item.c_str(), "%lu:%lf", &idx, &value) != 2)
            continue;
        arr.emplace_back(std::make_pair(idx, value));
    } // while item

    if (arr.size() == 1)
        THROW_INVALID_REQUEST("Invalid data string!");

    // TODO put into db

    return g_pFtrlModel->predict(arr);
}

void FtrlServiceHandler::correct(const std::string& id, const double value)
{

}

void FtrlServiceHandler::handleRequest(std::string& _return, const std::string& request)
{
}

} // namespace FTRL

