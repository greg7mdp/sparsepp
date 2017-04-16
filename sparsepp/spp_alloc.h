#if !defined(spp_alloc_h_guard)
#define spp_alloc_h_guard


/* -----------------------------------------------------------------------------------------------
 *   - use Segment tree with nodes containg info of pages - each page bm_sz objects with bitmap
 *   - _items pointers are aligned mod bm_sz*sizeof(T), with index in segment tree after T items
 *   - when hash_table created with specified size (or resized), prealloc pages for allocator so 
 *     we don't have to sort them one by one.
 *   - use prefix tree structure match pointer 'hint" to page, so we can realloc or allocate in
 *     nearby memory
 * -----------------------------------------------------------------------------------------------
 */

#include <sparsepp/spp_stdint.h> // includes spp_config.h
#include <sparsepp/spp_bitset.h>
#include <sparsepp/spp_smartptr.h>

#define USE_BTREE 1

#if USE_BTREE
    #include <sparsepp/spp_btree.h>
    #define BTREE_SET spp_::btree_set
#elif USE_CPP_BTREE
    #ifdef _WIN32
         typedef int64_t ssize_t; // for cpp-btree
    #endif

    #include <cpp-btree/btree_set.h>
    #define BTREE_SET btree::btree_set
#endif

#include <vector>
#include <algorithm>


namespace spp_
{

#if USE_BTREE

// -----------------------------------------------------------
// btree is almost as fast as the flat map for small sizes, 
// but maintain good insert/delete performance for large 
// sizes.
// -----------------------------------------------------------
template<class T>
class PageContainer : public BTREE_SET<T>
{
public:

};

#else

// -----------------------------------------------------------
// using a sorted vector (a lind of flat map). Cache friendly,
// fast lookups, but slow insert/delete when the size gets 
// large (i.e. when we have a lot of pages).
// -----------------------------------------------------------
template<class T>
class PageContainer : public std::vector<T>
{
public:
    typedef typename std::vector<T> super;

    typename super::iterator upper_bound(const T& value)
    {
        return std::upper_bound(this->begin(), this->end(), value);
    }

    void insert(const T& value)
    {
        // we don't check for unicity - not needed in our case.
        typename super::iterator it = std::upper_bound(this->begin(), this->end(), value);
        this->std::vector<T>::insert(it, value); 
    }

    void erase(const T& value)
    {
        typename super::iterator it = std::lower_bound(this->begin(), this->end(), value);
        if (*it == value)
            this->std::vector<T>::erase(it);
    }
};

#endif


// ----------------------------------------------------------------
// ----------------------------------------------------------------
template<class T, size_t page_size>
class spp_allocator
{
public:
    typedef T          value_type;
    typedef size_t     size_type;
    typedef ptrdiff_t  difference_type;

    typedef T*         pointer;
    typedef const T*   const_pointer;
    typedef T&         reference;
    typedef const T&   const_reference;

private:
    typedef uint32_t   offset_type;

    // ------------------- Page ----------------------------------------
    // pages are not stored in segments, as we want to keep segment tree 
    // compact for better cache hit. Also that way we can double the
    // segment tree size when we need more memory, while still allocating 
    // item blocks as needed.
    // -----------------------------------------------------------------
    template <size_t bm_sz>
    class Page
    {
    public:
        Page() : _num_free(bm_sz), _start_idx(0), _lzs_start(0) {}

        ~Page() { 
            assert(_num_free == bm_sz && _bs.none(0, bm_sz)); 
        }

        T *allocate(size_type n, offset_type &lf, intptr_t &diff) 
        {
            size_t start = _bs.find_next_n(n, _start_idx);

            assert(start != Bitset::npos);
            assert(_bs.none(start, start + n));

            _start_idx = start + n;
            _bs.set(start, start + n);

            if (lf == _num_free && _lzs_start == start)
            {
                lf  -= (offset_type)n;
                diff = - (intptr_t)n;
                _lzs_start += n;
            }
            else 
                _update_longest_free(lf, diff);
                
            _num_free -= n;

            assert(lf >= max_lf || lf <= _num_free);
            assert(_num_free <= bm_sz);
            assert(_lzs_start ==  (size_t)-1 || _bs.none(_lzs_start, _lzs_start + lf));
            
            return (T *)&_items[start]; 
        }

        T *extend(size_type start, size_type old_sz, size_type new_sz, bool request_space_after,
                  offset_type &lf, intptr_t &diff)
        {
            assert(new_sz > old_sz);
            assert(_bs.all(start, start + old_sz));
            assert(_lzs_start != start || lf == 0);

            size_type add = new_sz - old_sz; 

            if (lf < add)
                return 0;
            
            bool have_space_after = (start + new_sz <= page_size) &&
                                    _bs.none(start + old_sz, start + new_sz);
            
            if (request_space_after && have_space_after)
            {
                _bs.set(start + old_sz, start + new_sz);
                _num_free -= add;
                if (_lzs_start == (size_t)-1 || lf >= max_lf || _lzs_start == start + old_sz)
                    _update_longest_free(lf, diff);

                assert(lf >= max_lf || lf <= _num_free);
                assert(_lzs_start ==  (size_t)-1 || _bs.none(_lzs_start, _lzs_start + lf));
                return (T *)&_items[start]; 
            }
            
            bool have_space_before = start >= add && _bs.none(start - add, start);
            if (have_space_before && (!request_space_after || !have_space_after))
            {
                _bs.set(start - add, start);
                _num_free -= add;
                if (_lzs_start == (size_t)-1 || lf >= max_lf || _lzs_start + lf == start)
                    _update_longest_free(lf, diff);
                assert(lf >= max_lf || lf <= _num_free);
                assert(_lzs_start ==  (size_t)-1 || _bs.none(_lzs_start, _lzs_start + lf));
                return (T *)&_items[start - add]; 
            }

            if (have_space_after)
            {
                _bs.set(start + old_sz, start + new_sz);
                _num_free -= add;
                if (_lzs_start == (size_t)-1 || lf >= max_lf || _lzs_start == start + old_sz)
                    _update_longest_free(lf, diff);
                assert(lf >= max_lf || lf <= _num_free);
                assert(_lzs_start ==  (size_t)-1 || _bs.none(_lzs_start, _lzs_start + lf));
                return (T *)&_items[start]; 
            }

            return 0;
        }

        T *shrink(size_type start, size_type old_sz, size_type new_sz, 
                  offset_type &lf, intptr_t &diff)
        {
            assert(new_sz < old_sz);
            assert(_bs.all(start, start + old_sz));

            _bs.reset(start + new_sz, start + old_sz);
            _num_free += old_sz - new_sz;
            if (lf < max_lf && _lzs_start == start + old_sz)
                _update_longest_free(lf, diff);
            assert(lf >= max_lf || lf <= _num_free);
            assert(_lzs_start ==  (size_t)-1 || _bs.none(_lzs_start, _lzs_start + lf));
            return (T *)&_items[start];
        }

        bool free(size_type start, size_type n, offset_type &lf, intptr_t &diff)
        {
            assert(_bs.all(start, start + n));
            _bs.reset(start, start + n);
            _num_free += n;
            if (_num_free == bm_sz)
            {
                _lzs_start = 0;
                lf = bm_sz;
            }
            else if (lf < max_lf)
            {
#if 1
                size_t start_pos;
                offset_type new_lf = (offset_type)_bs.zero_sequence_size_around(start, start+n, start_pos);
                assert(start_pos == (size_t)-1 || start_pos < bm_sz);
                if (new_lf > lf)
                {
                    diff = (intptr_t)new_lf - (intptr_t)lf;
                    lf = new_lf;
                    _lzs_start = start_pos;
                }
#else
                _update_longest_free(lf, diff);
#endif
            }
            else
                diff = 0;
            assert(lf >= max_lf || lf <= _num_free);
            assert(_lzs_start ==  (size_t)-1 || _bs.none(_lzs_start, _lzs_start + lf));
            return true;
        }

        size_t longest_free() const 
        {
            _lzs_start = (size_t)-1;
            if (_num_free <= 1)
                return _num_free;

            // 64 free entries is enough for sparsepp -> don't waste time in longest_zero_sequence()
            return _bs.has_zero_word() ? max_lf : 
                _bs.longest_zero_sequence(max_lf, _lzs_start);
                //_bs.longest_zero_sequence();
        }

        size_t num_allocated() const
        {
            size_t res = _bs.count();
            assert(res + _num_free == bm_sz);
            return res;
        }

        size_t num_free() const
        {
            return _num_free;
        }

        const T *base() const
        {
            return (const T *)&_items[0];
        }

    private:
        void _update_longest_free(offset_type &lf, intptr_t &diff)
        {
            offset_type new_lf = (offset_type)longest_free();
            assert(_lzs_start == (size_t)-1 || _lzs_start < bm_sz);
            assert(_lzs_start ==  (size_t)-1 || _bs.none(_lzs_start, _lzs_start + new_lf));
            if (new_lf == lf)
                diff = 0;
            else 
            {
                diff = (intptr_t)new_lf - (intptr_t)lf;
                lf = new_lf;
            }
        }
            
        typedef char TProxy[sizeof(T)];
        typedef spp_bitset<bm_sz> Bitset;

        // 64 free entries is enough for sparsepp -> don't waste time in longest_zero_sequence()
        static const offset_type max_lf = SPP_GROUP_SIZE;

        size_t         _num_free;      // within this page
        size_t         _start_idx;     // within this page
        mutable size_t _lzs_start;     // start index of longest zero sequence - or -1 if not set
        Bitset         _bs;            // 0 == free, 1 == busy
        TProxy         _items[bm_sz];  // memory returned to user  
    };

    // ---------------------- Segment -----------------------------------------------
    template <size_t bm_sz>
    class Segment
    {
    public:
        Segment() : _page(0), _longest_free(bm_sz) {}

        Segment(const Segment &o): 
            _page(0), 
            _longest_free(bm_sz)
        {
            assert(o._page == 0);
            (void)(o._page); // silence warning
        }

        ~Segment() 
        {
            //assert(_longest_free == bm_sz);
            if (_page)
                _free_page();
        }

        Segment& operator=(const Segment &o)
        {
            assert(o._page == 0);
            _page = o._page;
            _longest_free = o._longest_free;
            return *this;
        }

#ifndef SPP_NO_CXX11_RVALUE_REFERENCES
        Segment(Segment &&o) : 
            _page(o._page),
            _longest_free(o._longest_free)
        {
            o._page = 0; 
            o._longest_free = bm_sz;
        }

        Segment& operator=(Segment &&o)
        {
            _page = o._page;
            _longest_free = o._longest_free;
            o._page = 0;
            o._longest_free = bm_sz;
            return *this;
        }
#endif

        void swap(Segment &o)
        {
            using std::swap;

            swap(_page, o._page);
            swap(_longest_free, o._longest_free);
        }

        pointer allocate(size_type n, intptr_t &diff)
        {
            if (n > _longest_free)
                return 0;

            pointer res = 0;

            if (!_page)
                _allocate_page();

            if (_page)
                res = _page->allocate(n, _longest_free, diff);

            return res;
        }

        pointer extend(size_type start, size_type old_sz, size_type new_sz,
                       bool space_after, intptr_t &diff)
        {
            assert(_page);
            return _page->extend(start, old_sz, new_sz, space_after, _longest_free, diff);
        }

        pointer shrink(size_type start, size_type old_sz, size_type new_sz, intptr_t &diff)
        {
            assert(_page);
            return _page->shrink(start, old_sz, new_sz, _longest_free, diff);
        }

        bool free(size_type start, size_type n, intptr_t &diff)
        {
            assert(_page);
            if (_page->free(start, n, _longest_free, diff))
            {
                if (_page->num_free() == bm_sz)
                    _free_page();
                return true;
            }
            return false;
        }

        size_type num_allocated() const
        {
            return _page ? _page->num_allocated() : 0;
        }

        offset_type& longest_free() { return _longest_free; }

        const T *page() const { return _page ? _page->base() : 0; }

    private:
        typedef Page<bm_sz> _Page;

        void _allocate_page()
        {
            _page = (_Page *)malloc(sizeof(_Page));
            new (_page) _Page(); // construct
        }

        void _free_page()
        {
            if (_page)
            {
                _page->~_Page(); // destruct
                ::free(_page);
                _longest_free = bm_sz;
                _page = 0;
            }
        }

        _Page       *_page;     // either actual page ptr
        offset_type  _longest_free;
    };


    // ------------------- Segment Tree ---------------------------------------------
    // http://codeforces.com/blog/entry/18051?mobile=true
    // ------------------------------------------------------------------------------
    template <size_t bm_sz>
    class SegTree : public spp_rc
    {
    public:
        SegTree() : 
            _num_allocated(0), 
            _num_seg(2),
            _num_extend_tries(0),
            _num_extend_successes(0)
        {
        }

        pointer allocate(size_type n, const_pointer hint = 0) 
        {
            assert(n <= bm_sz);
            pointer res = 0;

            if (_seg.empty())
                _seg.resize(_num_seg * 2); // default value _longest_free OK

            // check the hint first
            if (hint)
            {
                const_pageiter it = _seg_pages.upper_bound(_PageIndex(hint));
                if (it != _seg_pages.end())
                {
                    --it;
                    if (hint - it->_page_ptr < (intptr_t)bm_sz && ((res = _alloc(it->_page_idx, n, hint))))
                        return res;
                }
            }

            // allocate using segment tree
            // ---------------------------

            // first check whether we need to allocate new pages
            if (_seg[1].longest_free() < n)
            {
                // need to grow the segment tree
                std::vector<_Segment>  new_seg(4 * _num_seg);
                for (size_t i=_num_seg; i>0; i /= 2)   
                {
                    for (size_t j=0; j<i; ++j)   
                    {
                        size_t idx = 4 * i - j - 1;
                        new_seg[idx].swap(_seg[idx - 2 * i]);
                    }
                }
                _num_seg *= 2;
                _seg.swap(new_seg);
                assert(_seg[1].longest_free() == bm_sz);

                // recreate _seg_pages with correct indices
                // ----------------------------------------
                _seg_pages.clear();
#if USE_BTREE
                for (size_t i=_num_seg; i < 2*_num_seg; ++i)
                    if (_seg[i].page())
                        _seg_pages.insert(_PageIndex(_seg[i].page(), i));
#else
                _seg_pages.reserve(_num_seg);
                for (size_t i=_num_seg; i < 2*_num_seg; ++i)
                    if (_seg[i].page())
                        _seg_pages.push_back(_PageIndex(_seg[i].page(), i));
                std::sort(_seg_pages.begin(), _seg_pages.end());
#endif
            }

            // then find the first page which has a buffer big enough
            size_t i;
            for (i=2; i<_num_seg; i *= 2)
            {
                if (_seg[i].longest_free() < n)
                    ++i;
            }
            if (_seg[i].longest_free() < n)
                ++i;
            assert(_seg[i].longest_free() >= n);

            // and allocate from that page
            res = _alloc(i, n, 0);
            return res;
        }

        void deallocate(pointer p, size_type n) 
        {
            const_pageiter it = _find_page(p);
            _free(it->_page_idx, (size_type)(p - it->_page_ptr), n);
        }

        // tries to extend the current buffer if possible *without* moving the content. If 
        // space_after == true, tries to add space after preferably, o/w before.
        // returns null if buffer couldn't be extended.
        // --------------------------------------------------------------------------------
        pointer extend(pointer p, size_type old_size, size_type new_size, bool space_after) 
        {
            assert(new_size > old_size);
            if (new_size <= old_size)
            {
                if (new_size == old_size)
                    return p;
                return 0;
            }

            const_pageiter it = _find_page(p);
            return _extend(it->_page_idx, (size_type)(p - it->_page_ptr), 
                           old_size, new_size, space_after);
        }

        pointer shrink(pointer p, size_type old_size, size_type new_size) 
        {
            assert(new_size && new_size < old_size);
            const_pageiter it = _find_page(p);
            return _shrink(it->_page_idx, (size_type)(p - it->_page_ptr), 
                           old_size, new_size);
        }

        bool validate() const
        {
            size_t actual = 0;
            for (size_t i=0; i<_seg.size(); ++i)
                actual += _seg[i].num_allocated();
            assert(actual == _num_allocated);
            return actual == _num_allocated;
        }
            
        void swap(SegTree &o)
        {
            using std::swap;

            swap(_num_allocated, o._num_allocated);
            swap(_num_seg, o._num_seg);
            swap(_num_extend_tries, o._num_extend_tries);
            swap(_num_extend_successes, o._num_extend_successes);
            swap(_seg, o._seg);
            _seg_pages.swap(o._seg_pages);
        }

    private:
        // ----------------------------------------------------------------------
        class _PageIndex
        {
        public:
            _PageIndex(const T *ptr = 0, size_t idx = 0) : _page_ptr(ptr), _page_idx(idx) {}
            bool operator<(const _PageIndex &o) const { return _page_ptr < o._page_ptr; }
            bool operator==(const _PageIndex &o) const { return _page_ptr == o._page_ptr; }

            const T *_page_ptr;
            size_t   _page_idx;
        };

        typedef Segment<bm_sz>                     _Segment;
        typedef PageContainer<_PageIndex> SegPages;  // page_ptr -> segment index
        typedef typename SegPages::const_iterator const_pageiter;

        const_pageiter _find_page(pointer p) const
        {
            const_pageiter it = _seg_pages.upper_bound(_PageIndex(p));
            --it;
            assert(p - it->_page_ptr < (intptr_t)bm_sz);
            return it;
        }

        pointer _alloc(size_t seg_idx, size_type n, const_pointer /* hint */ = 0)
        {
            intptr_t diff = 0;
            _Segment &segment = _seg[seg_idx];
            const T *page = segment.page();
            pointer res = segment.allocate(n, diff);
            if (res)
            {
                _num_allocated += n;
                _update_segment_tree(seg_idx, diff);
                if (!page && segment.page())
                {
                    _PageIndex pi(segment.page(), seg_idx);
                    _seg_pages.insert(pi);
                }
            }
            return res;
        }

        pointer _extend(size_t seg_idx, size_type n, size_type old_size, size_type new_size, 
                        bool space_after)
        {
            intptr_t diff = 0;
            _num_extend_tries++;
            _Segment &segment = _seg[seg_idx];
            pointer res = segment.extend(n, old_size, new_size, space_after, diff);
            if (res)
            {
                _num_allocated += new_size - old_size;
                _update_segment_tree(seg_idx, diff);
                _num_extend_successes++;
            }
            return res;
        }

        pointer _shrink(size_t seg_idx, size_type n, size_type old_size, size_type new_size)
        {
            intptr_t diff = 0;
            _Segment &segment = _seg[seg_idx];
            pointer res = segment.shrink(n, old_size, new_size, diff);
            if (res)
            {
                _num_allocated -= old_size - new_size;
                _update_segment_tree(seg_idx, diff);
            }
            return res;
        }
        
        void _free(size_t seg_idx, size_type start, size_type n)
        {
            intptr_t diff = 0;
            _Segment &segment = _seg[seg_idx];
            const T *page = segment.page();

            if (segment.free(start, n, diff))
            {
                _num_allocated -= n;
                _update_segment_tree(seg_idx, diff);
                if (page && !segment.page())
                {
                    _seg_pages.erase(_PageIndex(page));
                    if (_seg_pages.empty())
                    {
                        SegPages().swap(_seg_pages);
                        std::vector<_Segment>().swap(_seg);
                    }
                }
            }
        }

        template <class Ti> Ti _mymax(Ti a, Ti b) { return a > b ? a : b; }

        void _update_segment_tree(size_t seg_idx, intptr_t diff)
        {
            if (diff == 0)
                return;
            assert(seg_idx >= _num_seg);
            if (diff > 0)
            {
                // longest_free for seg_idx is larger - propagate up if needed
                while (seg_idx > 1 && _seg[seg_idx/2].longest_free() < _seg[seg_idx].longest_free())
                {
                    _seg[seg_idx/2].longest_free() = _seg[seg_idx].longest_free();
                    seg_idx /= 2;
                }
            }
            else
            {
                // longest_free for seg_idx is smaller - propagate up if needed
                while (seg_idx > 1)
                {
                    seg_idx &= ~1;
                    offset_type cur_max = _mymax(_seg[seg_idx].longest_free(), 
                                                 _seg[seg_idx + 1].longest_free());
                    seg_idx /= 2;
                    if (cur_max < _seg[seg_idx].longest_free())
                        _seg[seg_idx].longest_free() = cur_max;
                    else
                        break;
                }
            }
        }
        
        // ----------------------------------------------------------------------
        size_t                 _num_allocated;
        size_t                 _num_seg;   // number of segments used for allocation
        size_t                 _num_extend_tries;
        size_t                 _num_extend_successes;                 
        std::vector<_Segment>  _seg;
        SegPages               _seg_pages; // find segments when deallocating/realloc
    };

public:

    spp_allocator() : _st(new SegTree<page_size>) {}
    ~spp_allocator() {}
    spp_allocator(const spp_allocator& o) : _st(o._st) { }
    spp_allocator& operator=(const spp_allocator &o) { _st = o._st; return *this; }

#if !defined(SPP_NO_CXX11_RVALUE_REFERENCES)
    spp_allocator(spp_allocator&& o) 
    {
        o.swap(*this);
    }
#endif

    void swap(spp_allocator &o)
    {
        _st.swap(o._st);
    }

    pointer address(reference r) const  { return &r; }

    const_pointer address(const_reference r) const  { return &r; }

    pointer allocate(size_type n, const_pointer hint = 0) 
    {
        return _st->allocate(n, hint);
    }

    void deallocate(pointer p, size_type n) 
    {
        _st->deallocate(p, n);
    }

    // tries to extend the current buffer if possible *without* moving the content. If 
    // space_after == true, tries to add space after preferably, o/w before.
    // returns null if buffer couldn't be extended.
    // -------------------------------------------------------------------------------
    pointer extend(pointer p, size_type old_size, size_type new_size, bool space_after) 
    {
        return _st->extend(p, old_size, new_size, space_after);
    }

    pointer shrink(pointer p, size_type old_size, size_type new_size) 
    {
        assert(new_size <= old_size);

        if (new_size == 0)
        {
            _st->deallocate(p, old_size);
            return 0;
        }

        if (new_size == old_size)
            return p;

        return _st->shrink(p, old_size, new_size);
    }

    pointer reallocate(pointer p, size_type old_size, size_type new_size) 
    {
        if (!p)
            return allocate(new_size);

        if (new_size <= old_size)
            return shrink(p, old_size, new_size); // we assume that shrink returns p 

        pointer res = extend(p, old_size, new_size, true);
        if (res)
        {
            if (res < p)
                memmove(res, p, old_size * sizeof(T));
            return res; 
        }

        res = allocate(new_size, p);
        if (res)
            memcpy(res, p, old_size * sizeof(T));
        deallocate(p, old_size);
        return res;
    }

    size_type max_size() const  
    {
        return static_cast<size_type>(-1);
    }

    void construct(pointer p, const value_type& val) { new(p) value_type(val);  }

    void destroy(pointer p) { p->~value_type(); }

    // sparsepp uses the rebind type for the group_allocator. Make sure this allocator 
    // uses malloc/free, as the spp_allocator has a max_size() of page_size.
    // -------------------------------------------------------------------------------
    template<class U>
    struct rebind 
    {
        typedef spp_::libc_allocator<U> other;
    };

private:
    spp_sptr<SegTree<page_size> >  _st;
};

}  // spp_ namespace


template<class T>
inline bool operator==(const spp_::spp_allocator<T> &a, const spp_::spp_allocator<T> &b)
{
    return &a == &b;
}

template<class T>
inline bool operator!=(const spp_::spp_allocator<T> &a, const spp_::spp_allocator<T> &b)
{
    return &a != &b;
}

namespace std
{
    template <class T, size_t page_size>
    inline void swap(spp_::spp_allocator<T, page_size> &a, spp_::spp_allocator<T, page_size> &b)
    {
        a.swap(b);
    }
}

#endif // spp_alloc_h_guard
