/*
 * c++ -o /tmp/join join.cpp -std=c++11 -g
 */
#include <iostream>
#include <fstream>
#include <string>
#include <boost/format.hpp>


using namespace std;

string sep = "\t";

int main(int argc, char **argv)
{
    const char *fname1 = argv[1];
    const char *fname2 = argv[2];

    string line1, line2;

    ifstream ifs1(fname1, ios::in);
    ifstream ifs2(fname2, ios::in);

    while (true) {
        if (!getline(ifs1, line1)) break;    
        if (!getline(ifs2, line2)) break;    
        cout << boost::format("%s%s%s") % line1 % sep % line2 << endl;
    } // while

    return 0;
}

