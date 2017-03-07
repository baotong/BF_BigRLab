#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/range/combine.hpp>
#include <json/json.h>
#include "KnnServiceHandler.h"


namespace KNN {

using namespace std;

void KnnServiceHandler::queryByItem(std::vector<Result> & _return, 
            const std::string& item, const int32_t n)
{
    using namespace std;

    // DLOG(INFO) << "Querying item " << item << " n = " << n;

    THROW_INVALID_REQUEST_IF(n <= 0, "Invalid n value " << n);

    vector<string>    result;
    vector<float>    distances;

    result.reserve(n);
    distances.reserve(n);

    g_pWordAnnDB->kNN_By_Word( item, n, result, distances );

    _return.resize( result.size() );
    for (size_t i = 0; i < result.size(); ++i) {
        _return[i].item = std::move(result[i]);
        _return[i].weight = std::move(distances[i]);
    } // for
}

void KnnServiceHandler::queryByItemNoWeight(std::vector<std::string> & _return, 
            const std::string& item, const int32_t n)
{
    std::vector<Result> result;
    queryByItem(result, item, n);
    _return.resize( result.size() );
    
    // 必须用typedef，否则模板里的逗号会被宏解析为参数分割
    typedef boost::tuple<std::string&, Result&> IterType;
    BOOST_FOREACH( IterType v, boost::combine(_return, result) )
        v.get<0>().swap(v.get<1>().item);
}

// 若用double，annoy buildidx会崩溃
void KnnServiceHandler::queryByVector(std::vector<Result> & _return, 
            const std::vector<double> & values, const int32_t n)
{
    using namespace std;

    THROW_INVALID_REQUEST_IF(n <= 0, "Invalid n value " << n);

    vector<string>    result;
    vector<float>     distances;
    vector<float>     fValues( values.begin(), values.end() );

    result.reserve(n);
    distances.reserve(n);

    g_pWordAnnDB->kNN_By_Vector( fValues, n, result, distances );

    _return.resize( result.size() );
    for (size_t i = 0; i < result.size(); ++i) {
        _return[i].item = std::move(result[i]);
        _return[i].weight = std::move(distances[i]);
    } // for
}

void KnnServiceHandler::queryByVectorNoWeight(std::vector<std::string> & _return, 
            const std::vector<double> & values, const int32_t n)
{
    std::vector<Result> result;
    queryByVector(result, values, n);
    _return.resize( result.size() );
    
    // 必须用typedef，否则模板里的逗号会被宏解析为参数分割
    typedef boost::tuple<std::string&, Result&> IterType;
    BOOST_FOREACH( IterType v, boost::combine(_return, result) )
        v.get<0>().swap(v.get<1>().item);
}

void KnnServiceHandler::handleRequest(std::string& _return, const std::string& request)
{
    using namespace std;

    Json::Reader    reader;
    Json::Value     root;
    int             n = 0;
    vector<string>  result;

    // DLOG(INFO) << "KnnService received request: " << request;
    // SLEEP_SECONDS(20);

    if (!reader.parse(request, root))
        THROW_INVALID_REQUEST("Json parse fail!");

    // get n
    try {
        n = root["n"].asInt();
        if (n <= 0)
            THROW_INVALID_REQUEST("Invalid n value");

        if (root.isMember("item")) {
            string item = root["item"].asString();
            queryByItemNoWeight(result, item, n);
        } else if (root.isMember("values")) {
            vector<double> vec;
            vec.reserve(FLAGS_nfields);
            auto& values = root["values"];
            if (values.size() != (size_t)FLAGS_nfields)
                THROW_INVALID_REQUEST("Request vector size " << values.size()
                        << " not equal to specified size " << FLAGS_nfields);
            for (auto it = values.begin(); it != values.end(); ++it)
                vec.push_back(it->asDouble());
            queryByVectorNoWeight(result, vec, n);
        } else {
            THROW_INVALID_REQUEST("Json parse fail! cannot find key item or values");
        } // if
    } catch (const std::exception &ex) {
        THROW_INVALID_REQUEST("Json parse fail! " << ex.what());
    } // try

    Json::Value resp;
    resp["status"] = 0;
    for (auto &v : result)
        resp["result"].append(v);

    Json::FastWriter writer;  
    _return = writer.write(resp);
}

} // namespace KNN



