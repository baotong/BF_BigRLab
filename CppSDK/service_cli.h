#ifndef _SERVICE_CLI_H_
#define _SERVICE_CLI_H_

#include <string>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <memory>
#include <curl/curl.h>
#include <glog/logging.h>


#define THROW_RUNTIME_ERROR(x) \
    do { \
        std::stringstream __err_stream; \
        __err_stream << x; \
        __err_stream.flush(); \
        throw std::runtime_error( __err_stream.str() ); \
    } while (0)

namespace BigRLab {

class ServiceCli {
public:
    typedef std::shared_ptr<ServiceCli>     pointer;

public:
    static
    std::shared_ptr<void> globalInit()
    {
        std::shared_ptr<void> ret( (void*)0, [](void*){
            // DLOG(INFO) << "doing global cleanup...";
            curl_global_cleanup();
        } );
        
        curl_global_init(CURL_GLOBAL_ALL);

        return ret;
    }

public:
    explicit ServiceCli( const std::string &url )
                : m_pCurl(NULL)
                , m_strUrl(url)
    {
        m_pCurl = curl_easy_init();
        if (!m_pCurl)
            THROW_RUNTIME_ERROR("ServiceCli constructor curl_easy_init fail!");

        curl_easy_setopt(m_pCurl, CURLOPT_URL, m_strUrl.c_str());
        curl_easy_setopt(m_pCurl, CURLOPT_WRITEFUNCTION, &ServiceCli::requestCallback);
        curl_easy_setopt(m_pCurl, CURLOPT_WRITEDATA, (void*)&m_strCbBuf);
        curl_easy_setopt(m_pCurl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    }

    virtual ~ServiceCli()
    {
        if (m_pCurl)
            curl_easy_cleanup(m_pCurl);
        m_pCurl = NULL;
    }

    int doRequest( const std::string &req )
    {
        m_strCbBuf.clear();
        curl_easy_setopt(m_pCurl, CURLOPT_POSTFIELDS, req.c_str());
        curl_easy_setopt(m_pCurl, CURLOPT_POSTFIELDSIZE, req.length());

        CURLcode res = curl_easy_perform(m_pCurl);
        if (res != CURLE_OK)
            m_strErrMsg = curl_easy_strerror(res);

        return res;
    }

    const std::string& url() const
    { return m_strUrl; }

    const std::string& errmsg() const
    { return m_strErrMsg; }

    std::string& respString()
    { return m_strCbBuf; }

protected:
    static size_t requestCallback(void *contents, size_t size, size_t nmemb, void *userp)
    {
        size_t realsize = size * nmemb;
        std::string *pBuf = (std::string*)userp;
        char *src = (char*)contents;
        pBuf->append( src, realsize );
        return realsize;
    }

protected:
    CURL           *m_pCurl;
    std::string    m_strUrl;
    std::string    m_strCbBuf;
    std::string    m_strErrMsg;
};

} // namespace BigRLab


#endif

