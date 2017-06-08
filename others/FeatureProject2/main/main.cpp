/*
 * 执行任务
 * GLOG_logtostderr=1 ./feature.bin -conf tasks.json
 * 查看fv文件
 * GLOG_logtostderr=1 ./feature.bin -dump xx.fv [-top 100]
 * 
 * 自动生成data描述
 * GLOG_logtostderr=1 ./feature.bin -gen_data_desc -data $datafile -desc $descFile [-head $headFile] [-hasid true] [-sep "$sepchar"]
 * example:
 * GLOG_logtostderr=1 ./feature.bin -gen_data_desc -data ../data/adult.txt -desc ../data/adult.json -hasid true -sep ","
 * GLOG_logtostderr=1 ./feature.bin -gen_data_desc -data ../data/adult.txt -desc ../data/adult.json -head ../data/head.txt -hasid true -sep ","
 */
#include <glog/logging.h>
#include <gflags/gflags.h>
#include <climits>
#include <fstream>
#include "CommDef.h"
#include "FeatureTask.h"
#include "FvFile.h"
#include "main_fun.h"

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

