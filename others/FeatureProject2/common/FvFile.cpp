#include "FvFile.h"
#include "CommDef.h"
#include <fstream>
#include <thrift/transport/TFileTransport.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/transport/TZlibTransport.h>


using namespace FeatureProject;


IFvFile::IFvFile(const std::string &fname) : FvFile(fname)
{
    using namespace std;
    using namespace apache::thrift;
    using namespace apache::thrift::protocol;
    using namespace apache::thrift::transport;

    auto _transport1 = boost::make_shared<TFileTransport>(fname, true);
    auto _transport2 = boost::make_shared<TBufferedTransport>(_transport1);
    auto transport = boost::make_shared<TZlibTransport>(_transport2);
    m_pProtocol = boost::make_shared<TBinaryProtocol>(transport);
}


bool IFvFile::readOne(FeatureVector &out) const
{
    try {
        FeatureVector fv;
        fv.read(m_pProtocol.get());
        FeatureProject::swap(fv, out);
    } catch (const apache::thrift::transport::TTransportException&) {
        return false;
    } // try

    return true;
}


OFvFile::OFvFile(const std::string &fname) : FvFile(fname), m_bFinish(false)
{
    using namespace std;
    using namespace apache::thrift;
    using namespace apache::thrift::protocol;
    using namespace apache::thrift::transport;

    // trunc file
    {
        ofstream ofs(fname, ios::out | ios::trunc);
        THROW_RUNTIME_ERROR_IF(!ofs, "OFvFile cannot open " << fname << " for writting!");
    } // trunc

    auto _transport1 = boost::make_shared<TFileTransport>(fname);
    auto _transport2 = boost::make_shared<TBufferedTransport>(_transport1);
    auto transport = boost::make_shared<TZlibTransport>(_transport2,
            128, 1024,
            128, 1024,
            Z_BEST_COMPRESSION);
    m_pProtocol = boost::make_shared<TBinaryProtocol>(transport);
}


OFvFile::~OFvFile()
{ try { finish(); } catch (...) {} }


void OFvFile::finish()
{
    using namespace apache::thrift::transport;

    if (!m_bFinish) {
        auto pTransport = boost::dynamic_pointer_cast<TZlibTransport>(m_pProtocol->getTransport());
        assert(pTransport);
        if (pTransport) pTransport->finish();
        m_bFinish = true;
    } // if
}


void OFvFile::writeOne(const FeatureVector &fv) const
{ fv.write(m_pProtocol.get()); }

