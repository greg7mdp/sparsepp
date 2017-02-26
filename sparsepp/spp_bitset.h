#if !defined(spp_bitset_h_guard)
#define spp_bitset_h_guard

#include <cstddef>
#include <climits>
#include <stdexcept>
#include <cassert>

#include <sparsepp/spp_stdint.h> // includes spp_config.h
#include <sparsepp/spp_utils.h>

namespace spp_
{

static inline uint32_t count_trailing_zeroes_naive(size_t v) SPP_NOEXCEPT
{
    if (v == 0) 
        return sizeof(v) * 8;
    
    uint32_t count = 0;
    while (v % 2 == 0) 
    {
        count++;
        v >>= 1;
    }
    return count;
}

static inline uint32_t  count_leading_zeros(size_t  v) SPP_NOEXCEPT
{
    v = v | (v >> 1);
    v = v | (v >> 2);
    v = v | (v >> 4);
    v = v | (v >> 8);
    v = v | (v >>16);
    if (sizeof(size_t) == 8)
        v = v | (v >> 32);
    return s_popcount(~v);
}

// ----------------------------------------------------------------
// Bitset whose size is always a multiple of 64 bits
// ----------------------------------------------------------------
template <size_t N>
class spp_bitset 
{
public:
    spp_bitset() SPP_NOEXCEPT 
    {
        assert(N > 0 && N % 64 == 0); 
        std::fill_n(_bits, num_words, size_t(0));
    }

    bool operator[](size_t pos) const SPP_NOEXCEPT { return test(pos); }

    bool test(size_t pos) const 
    {
        return !!(_bits[_idx(pos)] & (_mask(pos))); 
    }

    void set(size_t pos)   SPP_NOEXCEPT { _bits[_idx(pos)] |=  _mask(pos); }
    void reset(size_t pos) SPP_NOEXCEPT { _bits[_idx(pos)] &= ~_mask(pos); }
    void flip(size_t pos)  SPP_NOEXCEPT { _bits[_idx(pos)] ^= ~_mask(pos); }

    void set(size_t from, size_t to)  SPP_NOEXCEPT
    {
        size_t first  = _num_words(from); // first full word after from
        size_t last   = _idx(to);         // word containing to

        if (first <= last)
        {
            if (from % bits_per_word)
                _bits[first-1] |= ~(_mask(from) - 1);
            
            size_t i;
            for (i=first; i<last; ++i)
                _bits[i] = size_t(-1);

            if (to % bits_per_word)
                _bits[i] |= _mask(to) - 1;
        }
        else
        {
            // bits to set are within one word
            assert(last == first - 1);

            size_t val = (_mask(from) - 1) ^ (_mask(to) - 1);
            _bits[last] |= val;
        }
    }

    void reset(size_t from, size_t to) SPP_NOEXCEPT
    {
        size_t first  = _num_words(from); // first full word after from
        size_t last   = _idx(to);         // word containing to

        if (first <= last)
        {
            if (from % bits_per_word)
                _bits[first-1] &= _mask(from) - 1;
            
            size_t i;
            for (i=first; i<last; ++i)
                _bits[i] = size_t(0);

            if (to % bits_per_word)
                _bits[i] &= ~(_mask(to) - 1);
        }
        else
        {
            // bits to set are within one word
            assert(last == first - 1);

            size_t val = (_mask(from) - 1) ^ (_mask(to) - 1);
            _bits[last] &= ~val;
        }
    }

    bool all(size_t from, size_t to) const SPP_NOEXCEPT
    {
        size_t first  = _num_words(from); // first full word after from
        size_t last   = _idx(to);         // word containing to

        if (first <= last)
        {
            if (from % bits_per_word)
            {
                size_t val = ~(_mask(from) - 1);
                if ((_bits[first-1] & val) != val)
                    return false;
            }
                
            size_t i;
            for (i=first; i<last; ++i)
                if (_bits[i] != size_t(-1))
                    return false;

            if (to % bits_per_word)
            {
                size_t val = _mask(to) - 1;
                if ((_bits[i] & val) != val)
                    return false;
            }
        }
        else
        {
            // bits to test are within one word
            assert(last == first - 1);

            size_t val = (_mask(from) - 1) ^ (_mask(to) - 1);
            return (_bits[last] & val) == val;
        }
        return true;
    } 

    bool any(size_t from, size_t to) const SPP_NOEXCEPT
    {
        size_t first  = _num_words(from); // first full word after from
        size_t last   = _idx(to);         // word containing to

        if (first <= last)
        {
            if ((from % bits_per_word) && (_bits[first-1] & ~(_mask(from) - 1)))
                return true;
                
            size_t i;
            for (i=first; i<last; ++i)
                if (_bits[i])
                    return true;

            if ((to % bits_per_word) && (_bits[i] & (_mask(to) - 1)))
                return true;
        }
        else
        {
            // bits to test are within one word
            assert(last == first - 1);

            size_t val = (_mask(from) - 1) ^ (_mask(to) - 1);
            return !!(_bits[last] & val);
        }

        return false;
    } 

    bool none(size_t from, size_t to) const SPP_NOEXCEPT
    {
        return !this->any(from, to);
    }

    bool all() const SPP_NOEXCEPT
    {
        for (size_t i=0; i<num_words; ++i)
            if (_bits[i] != size_t(-1))
                return false;
        return true; 
    }

    // if there is one, returns index of first zero bit in start_pos
    // does not change start_pos if all bits set.
    // -------------------------------------------------------------
    bool all(size_t &start_pos) const SPP_NOEXCEPT
    {
        for (size_t i=0; i<num_words; ++i)
        {
            if (_bits[i] != size_t(-1))
            {
                start_pos = i * bits_per_word + count_trailing_zeroes(~_bits[i]);
                return false;
            }
        }
        return true; 
    }

    bool any() const SPP_NOEXCEPT
    {
        for (size_t i=0; i<num_words; ++i)
            if (_bits[i])
                return true;
        return false; 
    }

    bool none() const { return !this->any(); }

    bool operator==(const spp_bitset &rhs) const SPP_NOEXCEPT
    { 
        return std::equal(_bits, _bits + num_words, rhs._bits); 
    }

    bool operator!=(const spp_bitset &rhs) const SPP_NOEXCEPT
    { 
        return !(*this == rhs); 
    }
        
    spp_bitset& operator&=(const spp_bitset& rhs) SPP_NOEXCEPT
    {
        for (size_t i=0; i<num_words; ++i)
            _bits[i] &= rhs._bits[i];
        return *this;
    }

    spp_bitset& operator|=(const spp_bitset& rhs) SPP_NOEXCEPT
    {
        for (size_t i=0; i<num_words; ++i)
            _bits[i] |= rhs._bits[i];
        return *this;
    }

    spp_bitset& operator^=(const spp_bitset& rhs) SPP_NOEXCEPT
    {
        for (size_t i=0; i<num_words; ++i)
            _bits[i] ^= rhs._bits[i];
        return *this;
    }

    spp_bitset& operator<<=(size_t n) SPP_NOEXCEPT
    {
        if (n >= N)
        {
            reset();
            return *this;
        }

        if (n > 0) 
        {
            size_t last = num_words - 1;  
            size_t div  = n / bits_per_word; 
            size_t r    = n % bits_per_word; 

            if (r) 
            {
                size_t c = bits_per_word - r;

                for (size_t i = last-div; i > 0; --i) 
                    _bits[i + div] = (_bits[i] << r) | (_bits[i-1] >> c);
                _bits[div] = _bits[0] << r;
            }
            else 
            {
                for (size_t i = last-div; i > 0; --i)
                    _bits[i + div] = _bits[i];
                _bits[div] = _bits[0];
            }

            std::fill_n(&_bits[0], div, size_t(0));
        }

        return *this;
    }

    spp_bitset& operator>>=(size_t n) SPP_NOEXCEPT
    {
        if (n >= N)
        {
            reset();
            return *this;
        }

        if (n > 0) 
        {
            size_t last = num_words - 1;  
            size_t div  = n / bits_per_word; 
            size_t r    = n % bits_per_word; 

            if (r) 
            {
                size_t c = bits_per_word - r;

                for (size_t i = div; i < last; ++i) 
                    _bits[i-div] = (_bits[i] >> r) | (_bits[i+1] << c);
                _bits[last - div] = _bits[last] >> r;
            }
            else 
            {
                for (size_t i = div; i <= last; ++i) 
                    _bits[i - div] = _bits[i];
            }

            std::fill_n(&_bits[num_words - div], div, size_t(0));
        }

        return *this;
    }

    spp_bitset& set() SPP_NOEXCEPT
    {
        for (size_t i=0; i<num_words; ++i)
            _bits[i] = size_t(-1);
        return *this;
    }

    spp_bitset& reset() SPP_NOEXCEPT
    {
        for (size_t i=0; i<num_words; ++i)
            _bits[i] = size_t(0);
        return *this;
    }
    
    spp_bitset& flip() SPP_NOEXCEPT
    {
        for (size_t i=0; i<num_words; ++i)
            _bits[i] = ~_bits[i];
        return *this;
    }
    
    size_t count() const SPP_NOEXCEPT
    {
        size_t cnt = 0;

        for (size_t i=0; i<num_words; ++i)
            cnt += s_popcount(_bits[i]);

        return cnt;
    }

    size_t size() const SPP_NOEXCEPT
    {
        return size_t(N);
    }

    spp_bitset operator<<(size_t n) const SPP_NOEXCEPT
    {
        spp_bitset res(*this);
        res <<= n;
        return res;
    }

    spp_bitset operator>>(size_t n) const SPP_NOEXCEPT
    {
        spp_bitset res(*this);
        res >>= n;
        return res;
    }
   
    spp_bitset operator|(const spp_bitset &o) const SPP_NOEXCEPT
    {
        spp_bitset res(*this);
        res |= o;
        return res;
    }
   
    spp_bitset operator&(const spp_bitset &o) const SPP_NOEXCEPT
    {
        spp_bitset res(*this);
        res &= o;
        return res;
    }
   
    spp_bitset operator~() const SPP_NOEXCEPT
    {
        spp_bitset res(*this);
        res.flip();
        return res;
    }

    // returns length of longuest sequence of consecutive zeros
    // thanks to Michal Forisek for the algorithm
    // --------------------------------------------------------
    size_t longest_zero_sequence() const SPP_NOEXCEPT
    {
        if (none())
            return size();

        if (all())
            return size_t(0);

        spp_bitset state = (*this) | (*this) << 1;
        state.set(0);

        if (state.all()) 
            return 1;

        size_t steps = 1;
        while (true)
        {
            spp_bitset new_state = state | state << steps;
            if (new_state.all()) 
                break;
            state = new_state;
            steps *= 2;
        }

        size_t lo = steps, hi = 2 * steps;
        while (hi - lo > 1) 
        {
            size_t med = lo + (hi - lo) / 2;
            spp_bitset med_state = state | state << (med - steps);
            if (med_state.all())
                hi = med; 
            else 
                lo = med;
        }
        return hi;
    }

    // returns length of longuest sequence of consecutive zeros
    // thanks to Michal Forisek for the algorithm
    // --------------------------------------------------------
    size_t longest_zero_sequence(size_t ceiling,    // max value needed
                                 size_t &start_pos) // start of sequence
        const SPP_NOEXCEPT
    {
        if (none())
            return size();

        if (all())
            return size_t(0);

        spp_bitset state = (*this) | (*this) << 1;
        state.set(0);

        if (state.all()) 
            return 1;

        size_t steps = 1;
        while (true)
        {
            spp_bitset new_state = state | state << steps;
            if (new_state.all(start_pos)) 
                break;

            if (steps >= ceiling)
            {
                start_pos = (size_t)-1;
                return ceiling; // must be ceiling, not steps
            }

            state = new_state;
            steps *= 2;
        }

        size_t lo = steps, hi = 2*steps;
        while (hi-lo > 1) 
        {
            size_t med = lo+(hi-lo)/2;
            spp_bitset med_state = state | state << (med-steps);
            if (med_state.all(start_pos))
                hi = med; 
            else 
                lo = med;
        }

        if (hi >= ceiling)
        {
            start_pos = (size_t)-1;
            return ceiling; // must be ceiling, not hi
        }

        if (start_pos != (size_t)-1)
            start_pos -= hi - 1;
        return hi;
    }

    // returns the length of the zero sequence around [start, end]
    size_t zero_sequence_size_around(size_t start, size_t end, size_t &start_pos)
    {
        size_t lg = end - start;

        size_t cur = start;
        while (cur > 0 && !test(--cur))
            ++lg;

        start_pos = end - lg;

        cur = end;
        while (cur < N && !test(cur++))
            ++lg;

        return lg;
    }
        
        
#ifdef SPP_TEST
    // slow implementation - just for testing
    // --------------------------------------
    size_t longest_zero_sequence_naive() const SPP_NOEXCEPT
    {
        size_t longest = 0;
        size_t lg = 0;
        for (size_t cur = 0; cur < N; ++cur)
        {
            if (!test(cur))
            {
                if (++lg > longest) 
                    longest = lg;
            }
            else
                lg = 0;
        }
        return longest;
    }

    size_t longest_zero_sequence_naive(size_t ceiling,    // max value needed
                                       size_t &start_pos) const SPP_NOEXCEPT
    {
        size_t longest = 0;
        size_t lg = 0;
        size_t end_pos = 0;
        for (size_t cur = 0; cur < N; ++cur)
        {
            if (!test(cur))
            {
                if (++lg > longest) 
                {
                    longest = lg;
                    end_pos = cur;
                    if (longest >= ceiling)
                    {
                        start_pos = (size_t)-1;
                        return ceiling; // must be ceiling, not steps
                    }
                }
            }
            else
                lg = 0;
        }                                       
        start_pos = end_pos - (longest - 1);
        return longest;
    }

    void set_naive(size_t from, size_t to) 
    {
        for (size_t cur = from; cur < to; ++cur)
            set(cur);
    }

    void reset_naive(size_t from, size_t to) 
    {
        for (size_t cur = from; cur < to; ++cur)
            reset(cur);
    }

    bool all_naive(size_t from, size_t to) const SPP_NOEXCEPT
    {
        for (size_t cur = from; cur < to; ++cur)
            if (!test(cur))
                return false;
        return true;
    }
        
    bool all_naive(size_t &start_idx)  const SPP_NOEXCEPT
    {
        for (size_t cur = 0; cur < N; ++cur)
            if (!test(cur))
            {
                start_idx = cur;
                return false;
            }
        return true;
    }

    bool any_naive(size_t from, size_t to) const SPP_NOEXCEPT
    {
        for (size_t cur = from; cur < to; ++cur)
            if (test(cur))
                return true;
        return false;
    }
        
#endif

    
    size_t find_first_n(size_t num_zeros)
    {
        if (num_zeros == 0)
            return npos;
        return _find_next_n(num_zeros, 0, N);
    }

    size_t find_next_n(size_t num_zeros, size_t start_pos = 0)
    {
        if (start_pos > N || num_zeros == 0)
            return npos;

        size_t res = _find_next_n(num_zeros, start_pos, N);

        if (res == npos && start_pos)
            res = _find_next_n(num_zeros, 0, start_pos + num_zeros); // + num_zeros needed!

        return res;
    }

    bool has_zero_word() const
    {
        for (int i=(int)num_words-1; i>0;--i)
            if (!_bits[i])
                return true;
        return false;
    }
        
    
    static const size_t npos            = size_t(-1);
    static const unsigned bits_per_word = sizeof(size_t) * 8;
    static const unsigned num_words     = N / bits_per_word;


private:
    // find first sequence of num_zeros zeroes starting at position start_pos
    // slow implementation!
    // see http://www.perlmonks.org/?node_id=1037467
    // ----------------------------------------------------------------------
    size_t _find_next_n(size_t num_zeros, size_t start_pos, size_t end_pos) const SPP_NOEXCEPT
    {
        assert(num_zeros <= N && start_pos <= end_pos);
        if (end_pos < start_pos + num_zeros)
            return npos;

        if (this->none(start_pos, start_pos + num_zeros))
            return start_pos;

        if (end_pos > N)
            end_pos = N;
        
        size_t lg = 0;
        for (size_t cur = start_pos; cur < end_pos; ++cur)
        {
            if (!test(cur))
            {
                if (++lg == num_zeros)
                {
                    assert(this->none(cur - num_zeros + 1, cur + 1));
                    return cur - num_zeros + 1;
                }
            }
            else
            {
                if (cur % bits_per_word == 0)
                {
                    // can we skip the whole word?
                    size_t x = _bits[_idx(cur)];
                    if (x == (size_t)-1)
                    {
                        cur += bits_per_word - 1;
                        if (cur >= end_pos)
                            return npos;
                    }
                    else if (bits_per_word - s_popcount(x) < num_zeros)
                    {
                        cur += bits_per_word - count_leading_zeros(x) - 1;
                        if (cur >= end_pos)
                            return npos;
                    }
                }
                lg = 0;
            }
        }
        return npos;
    }

    static size_t _num_words(size_t num_bits) { return (num_bits + bits_per_word - 1) / bits_per_word; }
    static size_t _idx(size_t pos)            { return pos / bits_per_word; }
    static size_t _mask(size_t pos)           { return size_t(1) << (pos % bits_per_word); }

    size_t _bits[num_words];
};

}


#endif // spp_bitset_h_guard
