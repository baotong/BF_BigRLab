#ifndef _TEST_HPP_
#define _TEST_HPP_

namespace Test {

using namespace std;

void test_db(const string &dataFile, const string &valueFile)
{
    ifstream ifs(dataFile, ios::in);
    if (!ifs)
        THROW_RUNTIME_ERROR("Cannot open " << dataFile << " for reading!");

    size_t id = 0;
    string line;
    while (getline(ifs, line)) {
        ++id;
        g_pDb->add(std::to_string(id), line);
        // SLEEP_MILLISECONDS(30);
    } // while

    cout << g_pDb->size() << endl;
}


void test1()
{
    // time_t start = std::time(0);
    // SLEEP_SECONDS(1);
    // cout << "Wall time passed: " << std::difftime(std::time(0), start) << endl;
    // time_t maxTime = std::numeric_limits<time_t>::max();
    // cout << maxTime << endl;
    // cout << std::numeric_limits<double>::max() << endl;
    cout << sizeof(std::shared_ptr<string>) << endl;
    cout << sizeof(string::iterator) << endl;
    cout << sizeof(std::map<string,int>::iterator) << endl;
}

} // namespace Test

#endif

