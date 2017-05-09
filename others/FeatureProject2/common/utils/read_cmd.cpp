#include "read_cmd.h"
#include <cstdio>
#include <sstream>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>

namespace Utils {

int read_cmd(const std::string &cmd, std::string &output)
{
    int retval = 0;

    FILE *fp = popen(cmd.c_str(), "r");
    setvbuf(fp, NULL, _IONBF, 0);

    typedef boost::iostreams::stream< boost::iostreams::file_descriptor_source >
        FDRdStream;
    FDRdStream ppStream( fileno(fp), boost::iostreams::never_close_handle );

    std::ostringstream ss;
    ss << ppStream.rdbuf();

    output = ss.str();

    retval = pclose(fp);
    retval = WEXITSTATUS(retval);

    return retval;
}

} // namespace Utils


