#include <sstream>
#include <json/json.h>
#include "FtrlServiceHandler.h"
#include "common.hpp"
#include "alg_common.hpp"

namespace FTRL {

using namespace std;

double FtrlServiceHandler::lrPredict(const std::string& input)
{
    FtrlModel::AttrArray    arr;
    string                  item;
    size_t                  idx;
    double                  value;

    arr.reserve(256);
    arr.emplace_back(std::make_pair(0, 1));

    stringstream stream(input);
    while (stream >> item) {
        if (sscanf(item.c_str(), "%lu:%lf", &idx, &value) != 2)
            continue;
        arr.emplace_back(std::make_pair(idx, value));
    } // while item

    if (arr.size() == 1)
        THROW_INVALID_REQUEST("Invalid input string!");

    return g_pFtrlModel->predict(arr);
}

void FtrlServiceHandler::handleRequest(std::string& _return, const std::string& request)
{
}

} // namespace FTRL

