#ifndef _THRIFT_FILE_HPP_
#define _THRIFT_FILE_HPP_

#include "CommDef.h"
#include <fstream>
#include <boost/smart_ptr.hpp>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TFileTransport.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/transport/TZlibTransport.h>

namespace FeatureProject {

class ThriftFile {
public:
    ThriftFile(const std::string &fname) : m_strName(fname) {}
    const std::string& name() const { return m_strName; }
protected:
    std::string     m_strName;
    boost::shared_ptr<apache::thrift::protocol::TProtocol> m_pProtocol;
};


template <typename FeatureType>
class IThriftFile : public ThriftFile {
public:
    IThriftFile(const std::string &fname) : ThriftFile(fname)
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

    bool readOne(FeatureType &out) const
    {
        try {
            FeatureType fv;
            fv.read(m_pProtocol.get());
            FeatureProject::swap(fv, out);
        } catch (const apache::thrift::transport::TTransportException&) {
            return false;
        } // try

        return true;
    }
};


template <typename FeatureType>
class OThriftFile : public ThriftFile {
public:
    OThriftFile(const std::string &fname) : ThriftFile(fname), m_bFinish(false)
    {
        using namespace std;
        using namespace apache::thrift;
        using namespace apache::thrift::protocol;
        using namespace apache::thrift::transport;

        // trunc file
        {
            ofstream ofs(fname, ios::out | ios::trunc);
            THROW_RUNTIME_ERROR_IF(!ofs, "OThriftFile cannot open " << fname << " for writting!");
        } // trunc

        auto _transport1 = boost::make_shared<TFileTransport>(fname);
        auto _transport2 = boost::make_shared<TBufferedTransport>(_transport1);
        auto transport = boost::make_shared<TZlibTransport>(_transport2,
                128, 1024,
                128, 1024,
                Z_BEST_COMPRESSION);
        m_pProtocol = boost::make_shared<TBinaryProtocol>(transport);
    }

    ~OThriftFile()
    { try { finish(); } catch (...) {} }

    void writeOne(const FeatureType &fv) const
    { fv.write(m_pProtocol.get()); }

    void finish()
    {
        using namespace apache::thrift::transport;

        if (!m_bFinish) {
            auto pTransport = boost::dynamic_pointer_cast<TZlibTransport>(m_pProtocol->getTransport());
            assert(pTransport);
            if (pTransport) pTransport->finish();
            m_bFinish = true;
        } // if
    }
private:
    bool    m_bFinish;
};


} // namespace FeatureProject


#endif /* _THRIFT_FILE_HPP_ */

