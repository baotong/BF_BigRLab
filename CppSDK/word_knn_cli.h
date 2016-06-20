#ifndef _WORD_KNN_CLI_H_
#define _WORD_KNN_CLI_H_

#include "service_cli.h"
#include <json/json.h>
#include <vector>

class WordKnnCli : public BigRLab::ServiceCli {
    typedef BigRLab::ServiceCli     BaseType;
public:
    enum { INVALID_JSON = 301 };
public:
    WordKnnCli( const std::string &url ) : BaseType(url) {}

    int queryItem(const std::string &item, int k, std::vector<std::string> &result)
    {
        result.clear();

        Json::Value query;
        query["item"] = item;
        query["n"] = k;

        Json::FastWriter writer;  
        std::string queryStr = writer.write(query);

        int ret = BaseType::doRequest(queryStr);
        if (ret) return ret;

        return parseResp(result);
    }

    int queryVector( const std::vector<double> &values, int k, std::vector<std::string> &result )
    {
        result.clear();

        Json::Value query;

        query["n"] = k;
        for (const auto &v : values)
            query["values"].append(v);

        Json::FastWriter writer;  
        std::string queryStr = writer.write(query);

        int ret = BaseType::doRequest(queryStr);
        if (ret) return ret;

        return parseResp(result);
    }

private:
    int parseResp( std::vector<std::string> &result )
    {
        Json::Value  response;
        Json::Reader reader;
        if (!reader.parse(respString(), response)) {
            m_strErrMsg = "Parse response json error.";
            return INVALID_JSON;
        } // if

        try {
            int status = response["status"].asInt();
            if (status) {
                m_strErrMsg = response["errmsg"].asString();
                return status;
            } // if
            Json::Value &itemArray = response["result"];
            result.reserve(itemArray.size());
            for (Json::Value::iterator it = itemArray.begin(); it != itemArray.end(); ++it)
                result.emplace_back(it->asString());

        } catch (const std::exception &ex) {
            m_strErrMsg = "Parse response json error: ";
            m_strErrMsg.append( ex.what() );
            return INVALID_JSON;
        } // try

        return 0;
    }

};


#endif

