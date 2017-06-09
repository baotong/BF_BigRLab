/*
 * 执行任务
 * GLOG_logtostderr=1 ./feature.bin -conf tasks.json
 * 查看fv文件
 * GLOG_logtostderr=1 ./feature.bin -dump xx.fv [-top 100]
 * 
 * 自动生成data描述
 * GLOG_logtostderr=1 ./feature.bin -gen_data_desc -data $datafile -desc $descFile [-head $headFile] [-hasid true] [-sep "$sepchar"] [-nsample [100]]
 * -nsample 取多少样本分析，默认100个
 *  
 * example:
 * GLOG_logtostderr=1 ./feature.bin -gen_data_desc -data ../data/adult.txt -desc ../data/adult.json -hasid true -sep ","
 * GLOG_logtostderr=1 ./feature.bin -gen_data_desc -data ../data/adult.txt -desc ../data/adult.json -head ../data/head.txt -hasid true -sep "," -nsample 500
 */
#include <glog/logging.h>
#include <gflags/gflags.h>
#include <climits>
#include <fstream>
#include <boost/algorithm/string.hpp>
// #include <boost/format.hpp>
#include "CommDef.h"
#include "FeatureTask.h"
#include "FvFile.h"
#include "main_fun.h"
#include "utils/read_sep.hpp"


using namespace FeatureProject;


DEFINE_string(conf, "", "Info about raw data in json format");
DEFINE_string(dump, "", "Print fv file");
DEFINE_int32(top, 0, "dump top k lines");
DEFINE_bool(gen_data_desc, false, "Run as generating data description");


namespace Test {
using namespace std;

// void test_gen_tmp_output()
// {
    // cout << FeatureTask::gen_tmp_output("format output") << endl;
    // cout << FeatureTask::gen_tmp_output("format output concur") << endl;
    // cout << FeatureTask::gen_tmp_output("formatOutput") << endl;
// }

// void test()
// {
    // ifstream ifs("../data/adult.fi", ios::in);
    // FeatureIndex fidx;
    // ifs >> fidx;
// }

#if 0
void test1(const std::string &fname)
{
    ifstream ifs(fname, ios::in);
    string line;
    while (getline(ifs, line)) {
        vector<string> record;
        boost::split(record, line, boost::is_any_of("\t"));
        cout << record.size() << endl;
    } // while
}
#endif

#if 0
void test_read_sep()
{
    string sep = ",|blanks";
    Utils::read_sep(sep);
    cout << sep.length() << endl;
    for (char ch : sep)
        cout << ch << " ";
        // printf("%2u ", (uint8_t)ch);
        // cout << boost::format("%02u") % (uint8_t)ch << " ";
    cout << endl;
}
#endif

void test1()
{
    ifstream ifs("../data/adult.txt", ios::in);
    string line;
    vector<string> strValues;

    string sep = ",";
    Utils::read_sep(sep);
    cout << sep.length() << endl;
    cout << sep << endl;
    getline(ifs, line);
    boost::split(strValues, line, boost::is_any_of(sep));
    cout << strValues.size() << endl;
}

} // namespace Test


static
void do_dump(const std::string &fname, int _TopK)
{
    using namespace std;

    uint32_t topk = _TopK ? (uint32_t)_TopK : std::numeric_limits<uint32_t>::max();

    IFvFile ifv(fname);
    for (uint32_t i = 0; i < topk; ++i) {
        FeatureVector fv;
        if (!ifv.readOne(fv))
            break;
        cout << fv << endl;
    } // for i
}


int main(int argc, char **argv)
try {
    using namespace std;

    // Test::test_gen_tmp_output();
    // Test::test();
    // Test::test1("../wm_data/singlevalue.dat");
    // Test::test_read_sep();
    // Test::test1();
    // exit(0);

    google::InitGoogleLogging(argv[0]);
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    if (!FLAGS_conf.empty()) {
        auto pTaskMgr = std::make_shared<FeatureTaskMgr>();
        pTaskMgr->loadConf(FLAGS_conf);
        pTaskMgr->start();

    } else if (!FLAGS_dump.empty()) {
        do_dump(FLAGS_dump, FLAGS_top);

    } else if (FLAGS_gen_data_desc) {
        gen_data_desc();
    
    } else {
        cerr << "Wrong command!" << endl;
        return -1;
    } // if

    return 0;

} catch (const std::exception &ex) {
    std::cerr << "Exception caught by main: " << ex.what() << std::endl;
    return -1;
}

