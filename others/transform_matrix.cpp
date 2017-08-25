/*
 * c++ -o /tmp/test transform_matrix.cpp -lglog -lgflags -std=c++11 -O3
 */
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <iterator>
#include <ctime>
#include <boost/format.hpp>
#include <glog/logging.h>
#include <gflags/gflags.h>


DEFINE_double(rate, -1.0, "click rate");
static bool rate_dummy = gflags::RegisterFlagValidator(&FLAGS_rate, 
        [](const char *flagname, double value)->bool{
    return (value >= 0.0 && value <= 1.0);
});


static
void do_work()
{
    using namespace std;

    srand(time(0));
    int rateCount = (int)(FLAGS_rate * 100);

    string line;
    uint32_t skipNum = 0;
    while (getline(cin, line)) {
        istringstream iss(line);
        iss >> skipNum;
        if (!iss) continue;
        vector<double>  arr;
        arr.reserve(1024);
        std::copy(istream_iterator<double>(iss), istream_iterator<double>(), back_inserter(arr));
        // output
        cout << (rand() % 100 < rateCount ? "1 " : "0 ");
        for (size_t i = 0; i < arr.size(); ++i) {
            if (arr[i] != 0.0)
                cout << (i + 1) << ":" << arr[i] << " ";
        } // for
        cout << endl;
    } // while
}


int main(int argc, char **argv)
try {
    using namespace std;

    google::InitGoogleLogging(argv[0]);
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    LOG(INFO) << "Start transforming...";
    do_work();
    LOG(INFO) << "Job done!";

    return 0;

} catch (const std::exception &ex) {
    LOG(ERROR) << "Exception caught by main: " << ex.what();
    return -1;
}



