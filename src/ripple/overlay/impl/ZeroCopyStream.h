

#ifndef RIPPLE_OVERLAY_ZEROCOPYSTREAM_H_INCLUDED
#define RIPPLE_OVERLAY_ZEROCOPYSTREAM_H_INCLUDED

#include <google/protobuf/io/zero_copy_stream.h>
#include <boost/asio/buffer.hpp>
#include <cstdint>

namespace ripple {


template <class Buffers>
class ZeroCopyInputStream
    : public ::google::protobuf::io::ZeroCopyInputStream
{
private:
    using iterator = typename Buffers::const_iterator;
    using const_buffer = boost::asio::const_buffer;

    google::protobuf::int64 count_ = 0;
    iterator last_;
    iterator first_;    
    const_buffer pos_;  

public:
    explicit
    ZeroCopyInputStream (Buffers const& buffers);

    bool
    Next (const void** data, int* size) override;

    void
    BackUp (int count) override;

    bool
    Skip (int count) override;

    google::protobuf::int64
    ByteCount() const override
    {
        return count_;
    }
};


template <class Buffers>
ZeroCopyInputStream<Buffers>::ZeroCopyInputStream (
        Buffers const& buffers)
    : last_ (buffers.end())
    , first_ (buffers.begin())
    , pos_ ((first_ != last_) ?
        *first_ : const_buffer(nullptr, 0))
{
}

template <class Buffers>
bool
ZeroCopyInputStream<Buffers>::Next (
    const void** data, int* size)
{
    *data = boost::asio::buffer_cast<void const*>(pos_);
    *size = boost::asio::buffer_size(pos_);
    if (first_ == last_)
        return false;
    count_ += *size;
    pos_ = (++first_ != last_) ? *first_ :
        const_buffer(nullptr, 0);
    return true;
}

template <class Buffers>
void
ZeroCopyInputStream<Buffers>::BackUp (int count)
{
    --first_;
    pos_ = *first_ +
        (boost::asio::buffer_size(*first_) - count);
    count_ -= count;
}

template <class Buffers>
bool
ZeroCopyInputStream<Buffers>::Skip (int count)
{
    if (first_ == last_)
        return false;
    while (count > 0)
    {
        auto const size =
            boost::asio::buffer_size(pos_);
        if (count < size)
        {
            pos_ = pos_ + count;
            count_ += count;
            return true;
        }
        count_ += size;
        if (++first_ == last_)
            return false;
        count -= size;
        pos_ = *first_;
    }
    return true;
}



template <class Streambuf>
class ZeroCopyOutputStream
    : public ::google::protobuf::io::ZeroCopyOutputStream
{
private:
    using buffers_type = typename Streambuf::mutable_buffers_type;
    using iterator = typename buffers_type::const_iterator;
    using mutable_buffer = boost::asio::mutable_buffer;

    Streambuf& streambuf_;
    std::size_t blockSize_;
    google::protobuf::int64 count_ = 0;
    std::size_t commit_ = 0;
    buffers_type buffers_;
    iterator pos_;

public:
    explicit
    ZeroCopyOutputStream (Streambuf& streambuf,
        std::size_t blockSize);

    ~ZeroCopyOutputStream();

    bool
    Next (void** data, int* size) override;

    void
    BackUp (int count) override;

    google::protobuf::int64
    ByteCount() const override
    {
        return count_;
    }
};


template <class Streambuf>
ZeroCopyOutputStream<Streambuf>::ZeroCopyOutputStream(
        Streambuf& streambuf, std::size_t blockSize)
    : streambuf_ (streambuf)
    , blockSize_ (blockSize)
    , buffers_ (streambuf_.prepare(blockSize_))
    , pos_ (buffers_.begin())
{
}

template <class Streambuf>
ZeroCopyOutputStream<Streambuf>::~ZeroCopyOutputStream()
{
    if (commit_ != 0)
        streambuf_.commit(commit_);
}

template <class Streambuf>
bool
ZeroCopyOutputStream<Streambuf>::Next(
    void** data, int* size)
{
    if (commit_ != 0)
    {
        streambuf_.commit(commit_);
        count_ += commit_;
    }

    if (pos_ == buffers_.end())
    {
        buffers_ = streambuf_.prepare (blockSize_);
        pos_ = buffers_.begin();
    }

    *data = boost::asio::buffer_cast<void*>(*pos_);
    *size = boost::asio::buffer_size(*pos_);
    commit_ = *size;
    ++pos_;
    return true;
}

template <class Streambuf>
void
ZeroCopyOutputStream<Streambuf>::BackUp (int count)
{
    assert(count <= commit_);
    auto const n = commit_ - count;
    streambuf_.commit(n);
    count_ += n;
    commit_ = 0;
}

} 

#endif








