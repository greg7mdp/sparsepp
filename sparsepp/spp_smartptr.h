#if !defined(spp_smartptr_h_guard)
#define spp_smartptr_h_guard


/* -----------------------------------------------------------------------------------------------
 * quick version of intrusive_ptr
 * -----------------------------------------------------------------------------------------------
 */

#include <cassert>

// ------------------------------------------------------------------------
class spp_rc
{
public:
    spp_rc() : _cnt(0) {}
    spp_rc(const spp_rc &) : _cnt(0) {}
    void increment() const { ++_cnt; }
    void decrement() const { assert(_cnt); if (--_cnt == 0) delete this; }
    unsigned count() const { assert(_cnt); return _cnt; }

protected:
    virtual ~spp_rc() {}

private:
    mutable unsigned _cnt;
};

// ------------------------------------------------------------------------
template <class T>
class spp_sptr
{
public:
    spp_sptr() : _p(0) {}
    spp_sptr(T *p) : _p(p)                  { if (_p) _p->increment(); }
    spp_sptr(const spp_sptr &o) : _p(o._p)  { if (_p) _p->increment(); }
    ~spp_sptr()                             { if (_p) _p->decrement(); }
    spp_sptr& operator=(const spp_sptr &o)  { reset(o._p); return *this; }
    T* get() const                          { return _p; }
    void swap(spp_sptr &o)                  { T *tmp = _p; _p = o._p; o._p = tmp; }
    void reset(const T *p = 0)             
    { 
        if (p == _p) 
            return; 
        if (_p) _p->decrement(); 
        _p = (T *)p; 
        if (_p) _p->increment();
    }
    T* operator->() const { return const_cast<T *>(_p); }

private:
    T *_p;
};    


#endif // spp_smartptr_h_guard
