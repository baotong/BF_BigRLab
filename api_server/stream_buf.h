#ifndef _STREAM_BUF_H_
#define _STREAM_BUF_H_

#include <cstring>
#include <boost/interprocess/sync/interprocess_mutex.hpp> 
#include <boost/interprocess/sync/interprocess_condition.hpp> 
#include <boost/interprocess/sync/scoped_lock.hpp> 

#define SHM_NAME    "BigRLabShellBuffer"
#define SHARED_BUF_SIZE          (512*1024)
#define SHARED_STREAM_BUF_SIZE   (500*1024)

// 不是stdin那样阻塞，应该看做是文件stream
// 同步是必须的
struct StreamBuf {
    StreamBuf() : reqReady(false), respReady(false)
    { clear(); }

    void clear()
    { memset(buf, 0, sizeof(buf)); }

    char buf[SHARED_STREAM_BUF_SIZE];
    bool reqReady, respReady;
    boost::interprocess::interprocess_mutex      mtx;
    boost::interprocess::interprocess_condition  condReq, condResp;
};



#endif

