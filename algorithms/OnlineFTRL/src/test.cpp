#include <iostream>
#include <fstream>
#include <algorithm>
#include <cstdio>
#include <thread>
#include <chrono>
#include <glog/logging.h>
#include <gflags/gflags.h>
#include <boost/filesystem.hpp>
#include <unistd.h>
#include "fast_ftrl_solver.h"
#include "ftrl_train.h"
#include "util.h"

#define SLEEP_MILLISECONDS(x) std::this_thread::sleep_for(std::chrono::milliseconds(x))

#define THROW_RUNTIME_ERROR(x) \
    do { \
        std::stringstream __err_stream; \
        __err_stream << x; \
        __err_stream.flush(); \
        throw std::runtime_error( __err_stream.str() ); \
    } while (0)

#define RUN_COMMAND(x) \
    do { \
        std::stringstream __cmd_stream; \
        __cmd_stream << x; \
        __cmd_stream.flush(); \
        std::string __cmd_str = __cmd_stream.str(); \
        if (system(__cmd_str.c_str())) \
            THROW_RUNTIME_ERROR("Run command \"" << __cmd_str << "\" fail!"); \
    } while (0)

DEFINE_int32(epoch, 1, "Number of iteration, default 1");
DEFINE_string(model, "", "model file");
DEFINE_string(request, "predict", "request should be predict or update");
DEFINE_string(data, "", "Train data for updating model");

namespace {
using namespace std;
static bool validate_model(const char* flagname, const std::string &value)
{
    if (value.empty()) {
        cerr << "-model must be provided!" << endl;
        return false;
    } // if
    return true;
}
static const bool model_dummy = gflags::RegisterFlagValidator(&FLAGS_model, &validate_model);
} // namespace

static
void do_predict_routine()
{
    // check file
    {
        ifstream ifs(FLAGS_model, ios::in);
        if (!ifs)
            THROW_RUNTIME_ERROR("Model file " << FLAGS_model << " doesn't exist!");
    }

    typedef std::pair<size_t, double>    AttrValue;
    typedef std::vector<AttrValue>       AttrArray;

	LRModel<double> model;
	model.Initialize(FLAGS_model.c_str());

    string line, item;
    size_t idx;
    double value;
    AttrArray arr;
    arr.emplace_back(std::make_pair(0, 1));
    while (getline(cin, line)) {
        arr.resize(1);
        stringstream stream(line);
        while (stream >> item) {
            if (sscanf(item.c_str(), "%lu:%lf", &idx, &value) != 2)
                continue;
            arr.emplace_back(std::make_pair(idx, value));
        } // while item
        if (arr.empty()) {
            cout << "null" << endl;
        } else {
            double result = model.Predict(arr);
            result = std::max(std::min(result, 1. - 10e-15), 10e-15);
            cout << result << endl;
        } // if
    } // while
}

static
void do_update_routine()
{
    using namespace std;

    const char *newModel = "model.tmp";
    const char *newModelSave = "model.tmp.save";
    string oldModel = FLAGS_model + ".save";
    // check file
    {
        ifstream ifs(FLAGS_model, ios::in);
        if (!ifs)
            THROW_RUNTIME_ERROR("Model file " << FLAGS_model << " doesn't exist!");
    }
    // check file
    {
        ifstream ifs(oldModel, ios::in);
        if (!ifs)
            THROW_RUNTIME_ERROR("Old model file " << oldModel << " doesn't exist!");
    }
    if (FLAGS_data.empty())
        THROW_RUNTIME_ERROR("-data cannot be empty!");
    // check
    {
        ifstream ifs(FLAGS_data, ios::in);
        if (!ifs)
            THROW_RUNTIME_ERROR("data file " << oldModel << " doesn't exist!");
    }

    // train
    {
        FtrlTrainer<double> trainer;
        trainer.Initialize(FLAGS_epoch, true); // 2nd arg is cache
        trainer.Train(oldModel.c_str(), newModel, FLAGS_data.c_str(), (char*)NULL); // last arg is test
    }

    // RUN_COMMAND("rm -f " << FLAGS_model);
    // RUN_COMMAND("mv " << newModel << " " << FLAGS_model);
    // RUN_COMMAND("rm -f " << oldModel);
    // RUN_COMMAND("mv " << newModelSave << " " << oldModel);

    try {
        boost::filesystem::rename(newModel, FLAGS_model);
        boost::filesystem::rename(newModelSave, oldModel);
    } catch (const boost::filesystem::filesystem_error &ex) {
        cerr << "Renaming fail: " << ex.what() << endl;
    } // try

    // int rc = 0;
    // rc = std::remove(FLAGS_model.c_str());
    // if (rc)
        // std::perror("Error renaming");
    // rc = std::rename(newModel, FLAGS_model.c_str());
    // if (rc)
        // std::perror("Error renaming");
    // rc = std::remove(oldModel.c_str());
    // if (rc)
        // std::perror("Error renaming");
    // rc = std::rename(newModelSave, oldModel.c_str());
    // if (rc)
        // std::perror("Error renaming");
}


void test()
{
    using namespace std;

    try {
        boost::filesystem::rename("model", "model1");
    } catch (const boost::filesystem::filesystem_error &ex) {
        cerr << "Renaming fail: " << ex.what() << endl;
    } // try
}


int main(int argc, char **argv)
{
    using namespace std;

    google::InitGoogleLogging(argv[0]);
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    try {

        if (FLAGS_request == "predict")
            do_predict_routine();
        else if (FLAGS_request == "update")
            do_update_routine();
        else
            THROW_RUNTIME_ERROR("-request can only be predict or update");

    } catch (const std::exception &ex) {
        cerr << "Exception caught by main: " << ex.what() << endl;
        return -1;
    } // try

    return 0;
}
