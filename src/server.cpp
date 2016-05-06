#include "service_factory.h"
#include <glog/logging.h>

using namespace BigRLab;
using namespace std;

namespace Test {

    void print_PropertyTable( const PropertyTable &ppt )
    {
        for (const auto &v : ppt) {
            cout << v.first << ": ";
            for (const auto &vv : v.second)
                cout << vv << "; ";
            cout << endl;
        } // for
    }

    void test1(int argc, char **argv)
    {
        ServiceFactory::pointer pServiceFactory = ServiceFactory::getInstance();

        PropertyTable ppt;
        parse_config_file( argv[1], ppt );
        print_PropertyTable( ppt );

        exit(0);
    }

} // namespace Test


int main( int argc, char **argv )
{
    try {
        google::InitGoogleLogging(argv[0]);

        Test::test1(argc, argv);

    } catch ( const std::exception &ex ) {
        cerr << "Exception caught by main: " << ex.what() << endl;
        exit(-1);
    } // try

    return 0;
}

