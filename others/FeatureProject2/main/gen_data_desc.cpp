#include <glog/logging.h>
#include <gflags/gflags.h>
#include <fstream>
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <json/json.h>
#include "CommDef.h"
#include "main_fun.h"

DEFINE_string(data, "", "data file to generate description");
DEFINE_string(desc, "", "description file");
DEFINE_string(head, "", "file contains table head");
DEFINE_bool(hasid, false, "gen_data_desc whether data has id column");
DEFINE_int32(nsample, 100, "number of samples for gen_data_desc");
DEFINE_string(sep, "", "seperator chars for data file");


typedef std::vector<std::vector<std::string> >         StringMatrix;

static
void detect_type(const StringMatrix &samples, const uint32_t col, std::string &type)
{
    using namespace std;

    type = "string";

    bool isDouble = true;
    double fVal = 0.0;
    for (uint32_t i = 0; i < samples.size(); ++i) {
        const string& val = samples[i][col];
        if (!boost::conversion::try_lexical_convert(val, fVal)) {
            isDouble = false;
            break;
        } // if
    } // for i

    if (isDouble)
        type = "double";
}


void gen_data_desc()
{
    using namespace std;

    THROW_RUNTIME_ERROR_IF(FLAGS_data.empty(), "-data arg must be specified!");

    if (FLAGS_desc.empty()) {
        FLAGS_desc = FLAGS_data + ".json";
        LOG(WARNING) << "description file not specified, use auto " << FLAGS_desc;
    } // if

    DLOG(INFO) << "gen_data_desc() data = " << FLAGS_data << ", desc = " << FLAGS_desc;
    DLOG(INFO) << "FLAGS_sep = " << FLAGS_sep;
    DLOG(INFO) << "FLAGS_head = " << FLAGS_head;
    DLOG(INFO) << "FLAGS_nsample = " << FLAGS_nsample;
    DLOG(INFO) << "FLAGS_hasid = " << FLAGS_hasid;

    // read head
    vector<string>  arrFeature;
    if (!FLAGS_head.empty()) {
        ifstream ifs(FLAGS_head, ios::in);
        THROW_RUNTIME_ERROR_IF(!ifs, "Cannot open specified -head file " << FLAGS_head);
        ostringstream oss;
        oss << ifs.rdbuf() << flush;
        string strHead = oss.str();
        boost::split(arrFeature, strHead, boost::is_any_of(SPACES), boost::token_compress_on);
    } // if

    // for debug
    for (auto &v : arrFeature)
        DLOG(INFO) << v;

    uint32_t nFeatures = arrFeature.size() ? (uint32_t)(arrFeature.size()) : 0;
    LOG_IF(INFO, nFeatures) << "Detected " << nFeatures << " features from head file.";

    // read samples
    StringMatrix    samples;
    {
        ifstream ifs(FLAGS_data, ios::in);
        THROW_RUNTIME_ERROR_IF(!ifs, "gen_data_desc() cannot open -data file!");
        string line;
        for (int lineno = 0; lineno < FLAGS_nsample && getline(ifs, line); ++lineno) {
            boost::trim_if(line, boost::is_any_of(FLAGS_sep + SPACES));
            if (line.empty()) {
                LOG(WARNING) << "Line " << (lineno + 1) << " is empty, skip.";
                continue;
            } // if

            vector<string>      record;
            record.reserve(nFeatures + 1);
            boost::split(record, line, boost::is_any_of(FLAGS_sep), boost::token_compress_on);
            
            // remove id
            if (FLAGS_hasid) record.erase(record.begin());

            std::for_each(record.begin(), record.end(), [](string &s){
                boost::trim(s);
            });

            if (!nFeatures) {
                nFeatures = record.size();
                LOG(INFO) << "Detected " << nFeatures << " features from data.";
            } else {
                THROW_RUNTIME_ERROR_IF(record.size() != nFeatures, 
                        "gen_data_desc() error record in line " << (lineno + 1)
                        << " nFields " << record.size() << " not equal to nFeatures " << nFeatures);
            } // if

            samples.emplace_back();
            samples.back().swap(record);
        } // for lineno
    } // read samples

    // gen head names
    if (arrFeature.empty()) {
        arrFeature.reserve(nFeatures);
        char buf[40];
        for (uint32_t i = 1; i <= nFeatures; ++i) {
            snprintf(buf, 40, "Feature%u", i);
            arrFeature.emplace_back(buf);
        } // for i
    } // if

    vector<string> arrType(nFeatures);
    for (uint32_t i = 0; i < nFeatures; ++i)
        detect_type(samples, i, arrType[i]);

    // write to json file
    Json::Value     root;
    root["nfeatures"] = nFeatures;
    if (!FLAGS_sep.empty()) 
        root["sep"] = FLAGS_sep;
    for (uint32_t i = 0; i < nFeatures; ++i) {
        auto &back = root["features"].append(Json::Value());
        back["name"] = arrFeature[i];
        back["type"] = arrType[i];
    } // for i

    Json::StyledWriter writer;
    string outStr = writer.write(root);

    ofstream ofs(FLAGS_desc, ios::out);
    THROW_RUNTIME_ERROR_IF(!ofs, "gen_data_desc() cannot open desc file for writting!");
    ofs << outStr << flush;

    LOG(INFO) << "Data description file has written into " << FLAGS_desc;
    cout << "NOTE!!! We cannot guarantee the data description file generated by this program"
        << " is corrent, please check it by yourself and make changes if necessary." << endl;
}

