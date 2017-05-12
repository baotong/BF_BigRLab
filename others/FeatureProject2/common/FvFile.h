#ifndef _FV_FILE_H_
#define _FV_FILE_H_

#include "FeatureVector_types.h"
#include <boost/smart_ptr.hpp>
#include <thrift/protocol/TBinaryProtocol.h>


class FvFile {
public:
    FvFile(const std::string &fname) : m_strName(fname) {}
    const std::string& name() const { return m_strName; }
protected:
    std::string     m_strName;
    boost::shared_ptr<apache::thrift::protocol::TProtocol> m_pProtocol;
};


class IFvFile : public FvFile {
public:
    IFvFile(const std::string &fname);
    bool readOne(FeatureProject::FeatureVector &out) const;
};


class OFvFile : public FvFile {
public:
    OFvFile(const std::string &fname);
    ~OFvFile();
    void writeOne(const FeatureProject::FeatureVector &fv) const;
    void finish();
private:
    bool    m_bFinish;
};


#endif /* _FV_FILE_H_ */

