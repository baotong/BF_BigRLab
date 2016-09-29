#include <sstream>
#include <json/json.h>
#include <glog/logging.h>
#include <gflags/gflags.h>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include "FtrlServiceHandler.h"
#include "common.hpp"
#include "alg_common.hpp"

namespace FTRL {

using namespace std;

bool FtrlServiceHandler::checkId(const std::string &id)
{
    const char *pId = id.c_str();
    if (pId[0] < 0 || pId[0] > CHAR_MAX || pId[1] < 0 || pId[1] > CHAR_MAX)
        return false;
    return true;
}

double FtrlServiceHandler::lrPredict(const std::string& id, const std::string& data)
{
    if (id.empty())
        THROW_INVALID_REQUEST("id cannot be empty!");

    if (!checkId(id))
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

    g_pDb->add(id, data);

    return g_pFtrlModel->predict(arr);
}

bool FtrlServiceHandler::setValue(const std::string& id, const double value)
{
    const char *updateModelData = "update_model.data";
    bool ret = g_pDb->setValue(id, value);
    if (ret) {
        auto& arr = g_pDb->hasValues();
        if (arr.size() >= FLAGS_update_cnt) {
            ofstream ofs(updateModelData, ios::out);
            if (!ofs)
                THROW_INVALID_REQUEST("Cannot open " << updateModelData << " for writting!");
            boost::unique_lock<DB::ValueArray> lock(arr);
            for (auto& pItem : arr)
                ofs << pItem->value << " " << pItem->data << endl;
            arr.clear();
            lock.unlock();
            ofs.close();

            // update model
            try {
                g_pFtrlModel->updateModel(FLAGS_model, updateModelData, FLAGS_epoch);
            } catch (const std::exception &ex) {
                THROW_INVALID_REQUEST(ex.what());
            } // try

            try {
                boost::filesystem::remove(updateModelData);
            } catch (...) {}
        } // if
    } // if
    return ret;
}

void FtrlServiceHandler::handleRequest(std::string& _return, const std::string& request)
{
    Json::Reader    reader;
    Json::Value     root;
    Json::Value     resp;

    // DLOG(INFO) << "KnnService received request: " << request;

    if (!reader.parse(request, root))
        THROW_INVALID_REQUEST("Json parse fail!");

    try {
        string req = root["req"].asString();
        string id = root["id"].asString();
        string data = root["data"].asString();

        if ("predict" == req) {
            double result = lrPredict(id, data);
            resp["result"] = result;
            resp["status"] = 0;
        } else if ("update" == req) {
            double value = boost::lexical_cast<double>(data);
            bool ret = setValue(id, value);
            resp["status"] = (ret ? 0 : 1);
        } // if

        Json::FastWriter writer;  
        _return = writer.write(resp);

    } catch (const InvalidRequest &err) {
        throw err;
    } catch (const std::exception &ex) {
        LOG(ERROR) << "handleRequest fail: " << ex.what();
        THROW_INVALID_REQUEST("handleRequest fail: " << ex.what());
    } // try
}

} // namespace FTRL

