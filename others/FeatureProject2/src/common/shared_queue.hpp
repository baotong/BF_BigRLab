#ifndef _SHARED_QUEUE_HPP_
#define _SHARED_QUEUE_HPP_ 

#include <thread>
#include <condition_variable>
#include <chrono>


template < typename T >
class SharedQueue : public std::deque<T> {
    typedef typename std::deque<T>   BaseType;
public:
    SharedQueue( std::size_t _MaxSize = UINT_MAX ) 
                : maxSize(_MaxSize) {}

    bool full() const { return this->size() >= maxSize; }

    void push( const T& elem )
    {
        std::unique_lock<std::mutex> lk(lock);

        while ( this->full() )
            condWr.wait( lk );

        this->push_back( elem );

        lk.unlock();
        condRd.notify_one();
    }

    void push( T&& elem )
    {
        std::unique_lock<std::mutex> lk(lock);

        while ( this->full() )
            condWr.wait( lk );

        this->push_back( std::move(elem) );

        lk.unlock();
        condRd.notify_one();
    }

    void pop( T& retval )
    {
        std::unique_lock<std::mutex> lk(lock);
        
        while ( this->empty() )
            condRd.wait( lk );

        retval = std::move(this->front());
        this->pop_front();

        lk.unlock();
        condWr.notify_one();
    }

    T pop()
    {
        T retval;
        this->pop( retval );
        return retval;
    }

    bool timed_push( const T& elem, std::size_t timeout )
    {
        std::unique_lock<std::mutex> lk(lock);

        if (!condWr.wait_for(lk, std::chrono::milliseconds(timeout),
                    [this]()->bool {return !this->full();}))
            return false;

        this->push_back( elem );

        lk.unlock();
        condRd.notify_one();

        return true;
    }

    bool timed_push( T&& elem, std::size_t timeout )
    {
        std::unique_lock<std::mutex> lk(lock);

        if (!condWr.wait_for(lk, std::chrono::milliseconds(timeout),
                    [this]()->bool {return !this->full();}))
            return false;

        this->push_back( std::move(elem) );

        lk.unlock();
        condRd.notify_one();

        return true;
    }

    bool timed_pop( T& retval, std::size_t timeout )
    {
        std::unique_lock<std::mutex> lk(lock);
        
        //!! 这里的pred条件和while正好相反，相当于until某条件
        if (!condRd.wait_for(lk, std::chrono::milliseconds(timeout),
                  [this]()->bool {return !this->empty();}))
            return false;

        retval = std::move(this->front());
        this->pop_front();

        lk.unlock();
        condWr.notify_one();

        return true;
    }

    void clear()
    {
        std::unique_lock<std::mutex> lk(lock);
        BaseType::clear();
    }

    std::mutex& mutex()
    { return std::ref(lock); }

protected:
    const std::size_t           maxSize;
    std::mutex                  lock;
    std::condition_variable     condRd;
    std::condition_variable     condWr;
};


#endif /* _SHARED_QUEUE_HPP_ */

