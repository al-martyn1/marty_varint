/* \file
   \brief Protobuf varint encoding an decoding for integral types

 */

#pragma once

#include <algorithm>
#include <cstdint>
#include <climits>
#include <cstring>
#include <iterator>
#include <stdexcept>


// marty::varint::
namespace marty {
namespace varint {

//----------------------------------------------------------------------------



//----------------------------------------------------------------------------
// ZigZag convertation signed -> unsigned for further encoding
// https://protobuf.dev/programming-guides/encoding/#signed-ints

// Integer types
// https://learn.microsoft.com/en-us/cpp/cpp/fundamental-types-cpp?view=msvc-170#integer-types

//----------------------------------------------------------------------------
inline
unsigned char zigzagEncode(signed char val)
{
    return (unsigned char)((val << 1) ^ (val >> ((sizeof(val)*CHAR_BIT) - 1)));
}

//----------------------------------------------------------------------------
inline
unsigned short int zigzagEncode(signed short int val)
{
    return (unsigned short int)((val << 1) ^ (val >> ((sizeof(val)*CHAR_BIT) - 1)));
}

//----------------------------------------------------------------------------
inline
unsigned int zigzagEncode(signed int val)
{
    return (unsigned int)((val << 1) ^ (val >> ((sizeof(val)*CHAR_BIT) - 1)));
}

//----------------------------------------------------------------------------
inline
unsigned long int zigzagEncode(signed long int val)
{
    return (unsigned long int)((val << 1) ^ (val >> ((sizeof(val)*CHAR_BIT) - 1)));
}

//----------------------------------------------------------------------------
inline
unsigned long long int zigzagEncode(signed long long int val)
{
    return (unsigned long long int)((val << 1) ^ (val >> ((sizeof(val)*CHAR_BIT) - 1)));
}

//----------------------------------------------------------------------------



//----------------------------------------------------------------------------
inline
signed char zigzagDecode(unsigned char val)
{
    return (signed char)(((val & 1u) * (unsigned char)(-1)) ^ (val >> 1u));
}

//----------------------------------------------------------------------------
inline
signed short int zigzagDecode(unsigned short int val)
{
    return (signed short int)(((val & 1u) * (unsigned short int)(-1)) ^ (val >> 1u));
}

//----------------------------------------------------------------------------
inline
signed int zigzagDecode(unsigned int val)
{
    return (signed int)(((val & 1u) * (unsigned int)(-1)) ^ (val >> 1u));
}

//----------------------------------------------------------------------------
inline
signed long int zigzagDecode(unsigned long int val)
{
    return (signed long int)(((val & 1u) * (unsigned long int)(-1)) ^ (val >> 1u));
}

//----------------------------------------------------------------------------
inline
signed long long int zigzagDecode(unsigned long long int val)
{
    return (signed long long int)(((val & 1u) * (unsigned long long int)(-1)) ^ (val >> 1u));
}

//----------------------------------------------------------------------------



//----------------------------------------------------------------------------
namespace impl {

//----------------------------------------------------------------------------



//----------------------------------------------------------------------------
// https://protobuf.dev/programming-guides/encoding/#varints
// pBuf must me enough
inline
size_t encode(std::uint8_t *pBuf, unsigned long long int val)
{
    size_t totalWritten = 0;

    const std::uint8_t msb    = 0x80; // std::uint8_t(1u<<(CHAR_BIT-1));
    const std::uint8_t msbClr = std::uint8_t(~msb);

    while(val!=0)
    {
        std::uint8_t curByte = (std::uint8_t)(val & msbClr);
        val >>= 7; // (CHAR_BIT-1);
        pBuf[totalWritten++] = curByte;
    }

    if (!totalWritten) // Если у нас на входе был ноль, то ничего не было записано вообще
        pBuf[totalWritten++] = 0;
    
    // Устанавливаем бит продолжения для всех байт, кроме последнего
    for(std::size_t i=0; i!=(totalWritten-1); ++i)
        pBuf[i] |= msb;

    return totalWritten;
}

//----------------------------------------------------------------------------
// returns number of bytes processed from input
template<typename IteratorType>
std::size_t decode( IteratorType b, IteratorType e
                  , std::size_t *pNumberNumBytes // optional pointer to variable which receives number of integer number bytes readed (less oq equal to return value)
                  , unsigned long long int *pVal // optional pointer to variable which receives parsing result
                  )
{
    std::uint8_t buf[16];
    std::size_t  i = 0;

    for(; i!=sizeof(buf) && b!=e; ++i, ++b)
    {
        std::uint8_t byte = std::uint8_t(*b);
        buf[i] = std::uint8_t(byte&0x7Fu);

        if (!(byte&0x80u)) // continuation bit not found
            break;
    }

    if (i==sizeof(buf) || b==e) // не найден байт без бита продолжения - какая-то фигня во входных данных, 16 байт ни один целочисленный тип не занимает
        return 0u;


    std::size_t totalReaded = i + 1u;
    std::size_t nBitsReaded = 0;

    std::reverse(&buf[0], &buf[totalReaded]);

    unsigned long long int res = 0;

    for(i=0; i!=totalReaded; ++i)
    {
        res <<= 7;
        res |= (unsigned long long int)buf[i];
        nBitsReaded += 7;
    }

    // А вот тут уже меряем вCHAR_BIT-ах, так как нас интересуют размер локального типа
    //std::size_t nBytes = (nBitsReaded%8)!=0 ? 1+nBitsReaded/8 : nBitsReaded/8;
    // if (pNumberNumBytes)
    //     *pNumberNumBytes = (nBitsReaded%CHAR_BIT)!=0 ? 1+nBitsReaded/CHAR_BIT : nBitsReaded/CHAR_BIT;
    if (pNumberNumBytes)
        *pNumberNumBytes = nBitsReaded/CHAR_BIT;

    if (pVal)
        *pVal = res;

    return totalReaded;
}

//----------------------------------------------------------------------------



//----------------------------------------------------------------------------
} // namespace impl

//----------------------------------------------------------------------------



//----------------------------------------------------------------------------
inline
size_t encode(std::uint8_t *pBuf, std::size_t bufSize, unsigned long long int val)
{
    std::uint8_t buf[16];
    size_t totalWritten = impl::encode(&buf[0], val);

    if (totalWritten>bufSize) // буфера не хватает, сигналим об ошибке нулём
        return 0u;

    if (pBuf)
        memmove( (void*)pBuf, (const void*)&buf[0], totalWritten);

    return totalWritten;
}

//----------------------------------------------------------------------------
template<typename BackInsertIteratorType>
BackInsertIteratorType encode(BackInsertIteratorType iter, unsigned long long int val)
{
    uint8_t buf[16];
    size_t totalWritten = impl::encode(&buf[0], val);

    for(std::size_t i=0; i!=totalWritten; ++i)
        *iter++ = buf[i];

    return iter;
}

//----------------------------------------------------------------------------
inline
size_t encode(std::uint8_t *pBuf, std::size_t bufSize, signed long long int val)
{
    // Сначала ZigZag
    unsigned long long zigzagedVal = zigzagEncode(val);
    return encode(pBuf, bufSize, zigzagedVal);
}

//----------------------------------------------------------------------------
template<typename BackInsertIteratorType>
BackInsertIteratorType encode(BackInsertIteratorType iter, signed long long int val)
{
    // Сначала ZigZag
    unsigned long long zigzagedVal = zigzagEncode(val);
    return encode(iter, zigzagedVal);
}

//----------------------------------------------------------------------------
#define MARTY_VARINT_IMPL_ENCODE(TYPENAME)                                   \
                                                                             \
             inline                                                          \
             size_t encode(std::uint8_t *pBuf, std::size_t bufSize, unsigned TYPENAME val) \
             {                                                               \
                 return encode(pBuf, bufSize, (unsigned long long)val);      \
             }                                                               \
                                                                             \
             template<typename BackInsertIteratorType>                       \
             BackInsertIteratorType encode(BackInsertIteratorType iter, unsigned TYPENAME val) \
             {                                                               \
                 return encode(iter, (unsigned long long)val);               \
             }                                                               \
                                                                             \
                                                                             \
             inline                                                          \
             size_t encode(std::uint8_t *pBuf, std::size_t bufSize, signed TYPENAME val) \
             {                                                               \
                 return encode(pBuf, bufSize, (signed long long)val);        \
             }                                                               \
                                                                             \
             template<typename BackInsertIteratorType>                       \
             BackInsertIteratorType encode(BackInsertIteratorType iter, signed TYPENAME val) \
             {                                                               \
                 return encode(iter, (signed long long)val);                 \
             }

MARTY_VARINT_IMPL_ENCODE(long int)
MARTY_VARINT_IMPL_ENCODE(int)
MARTY_VARINT_IMPL_ENCODE(short int)
MARTY_VARINT_IMPL_ENCODE(char)

inline
size_t encode(std::uint8_t *pBuf, std::size_t bufSize, char val)
{
    return encode(pBuf, bufSize, (signed char)val);
}

template<typename BackInsertIteratorType>
BackInsertIteratorType encode(BackInsertIteratorType iter, char val)
{
    return encode(iter, (signed char)val);
}

//----------------------------------------------------------------------------



//----------------------------------------------------------------------------
// returns number of bytes processed from input
inline
std::size_t decode(const std::uint8_t *pBuf, std::size_t bufSize, unsigned long long int *pVal=0)
{
    std::size_t numBytes = 0;
    auto res = impl::decode(pBuf, pBuf+bufSize, &numBytes, pVal);

    if (!res)
        throw std::invalid_argument("too long serialized varint");

    if (numBytes>sizeof(*pVal))
        throw std::out_of_range("varint value is out of range");

    return res;
}

//----------------------------------------------------------------------------
template<typename IteratorType>
IteratorType decode( IteratorType b, IteratorType e, unsigned long long int *pVal=0)
{
    std::size_t numBytes = 0;
    auto res = impl::decode(b, e, &numBytes, pVal);

    if (!res)
        throw std::invalid_argument("too long serialized varint");

    if (numBytes>sizeof(*pVal))
        throw std::out_of_range("varint value is out of range");

    return std::next(b, std::ptrdiff_t(res));
}

//----------------------------------------------------------------------------
inline
std::size_t decode(const std::uint8_t *pBuf, std::size_t bufSize, signed long long int *pVal=0)
{
    std::size_t numBytes = 0;
    unsigned long long int val = 0;
    auto res = impl::decode(pBuf, pBuf+bufSize, &numBytes, &val);

    if (!res)
        throw std::invalid_argument("too long serialized varint");

    if (numBytes>sizeof(*pVal))
        throw std::out_of_range("varint value is out of range");

    if (pVal)
        *pVal = zigzagDecode(val);

    return res;
}

//----------------------------------------------------------------------------
template<typename IteratorType>
IteratorType decode( IteratorType b, IteratorType e, signed long long int *pVal=0)
{
    std::size_t numBytes = 0;
    unsigned long long int val = 0;
    auto res = impl::decode(b, e, &numBytes, &val);

    if (!res)
        throw std::invalid_argument("too long serialized varint");

    if (numBytes>sizeof(*pVal))
        throw std::out_of_range("varint value is out of range");

    if (pVal)
        *pVal = zigzagDecode(val);

    return std::next(b, std::ptrdiff_t(res));
}

//----------------------------------------------------------------------------



//----------------------------------------------------------------------------
#define MARTY_VARINT_IMPL_DECODE(TYPENAME)                                   \
                                                                             \
            inline                                                           \
            std::size_t decode(const std::uint8_t *pBuf, std::size_t bufSize, unsigned TYPENAME *pVal=0)\
            {                                                                \
                std::size_t numBytes = 0;                                    \
                unsigned long long int val = 0;                              \
                auto res = impl::decode(pBuf, pBuf+bufSize, &numBytes, &val);\
                                                                             \
                if (!res)                                                    \
                    throw std::invalid_argument("too long serialized varint");\
                                                                             \
                if (numBytes>sizeof(*pVal))                                  \
                    throw std::out_of_range("varint value is out of range"); \
                                                                             \
                if (pVal)                                                    \
                    *pVal = (unsigned TYPENAME)(val);                        \
                                                                             \
                return res;                                                  \
            }                                                                \
                                                                             \
            template<typename IteratorType>                                  \
            IteratorType decode( IteratorType b, IteratorType e, unsigned TYPENAME *pVal=0)\
            {                                                                \
                std::size_t numBytes = 0;                                    \
                unsigned long long int val = 0;                              \
                auto res = impl::decode(b, e, &numBytes, &val);              \
                                                                             \
                if (!res)                                                    \
                    throw std::invalid_argument("too long serialized varint");\
                                                                             \
                if (numBytes>sizeof(*pVal))                                  \
                    throw std::out_of_range("varint value is out of range"); \
                                                                             \
                if (pVal)                                                    \
                    *pVal = (unsigned TYPENAME)(val);                        \
                                                                             \
                return std::next(b, std::ptrdiff_t(res));                    \
            }                                                                \
                                                                             \
            inline                                                           \
            std::size_t decode(const std::uint8_t *pBuf, std::size_t bufSize, signed TYPENAME *pVal=0)\
            {                                                                \
                std::size_t numBytes = 0;                                    \
                unsigned long long int val = 0;                              \
                auto res = impl::decode(pBuf, pBuf+bufSize, &numBytes, &val);\
                                                                             \
                if (!res)                                                    \
                    throw std::invalid_argument("too long serialized varint");\
                                                                             \
                if (numBytes>sizeof(*pVal))                                  \
                    throw std::out_of_range("varint value is out of range"); \
                                                                             \
                if (pVal)                                                    \
                    *pVal = (signed TYPENAME)(zigzagDecode(val));            \
                                                                             \
                return res;                                                  \
            }                                                                \
                                                                             \
            template<typename IteratorType>                                  \
            IteratorType decode( IteratorType b, IteratorType e, signed TYPENAME *pVal=0)\
            {                                                                \
                std::size_t numBytes = 0;                                    \
                unsigned long long int val = 0;                              \
                auto res = impl::decode(b, e, &numBytes, &val);              \
                                                                             \
                if (!res)                                                    \
                    throw std::invalid_argument("too long serialized varint");\
                                                                             \
                if (numBytes>sizeof(*pVal))                                  \
                    throw std::out_of_range("varint value is out of range"); \
                                                                             \
                if (pVal)                                                    \
                    *pVal = (signed TYPENAME)(zigzagDecode(val));            \
                                                                             \
                return std::next(b, std::ptrdiff_t(res));                                    \
            }

MARTY_VARINT_IMPL_DECODE(long int)
MARTY_VARINT_IMPL_DECODE(int)
MARTY_VARINT_IMPL_DECODE(short int)
MARTY_VARINT_IMPL_DECODE(char)

inline
std::size_t decode(const std::uint8_t *pBuf, std::size_t bufSize, char *pVal=0)
{
    return decode(pBuf, bufSize, (signed char*)pVal);
}

template<typename IteratorType>
IteratorType decode( IteratorType b, IteratorType e, char *pVal=0)
{
    return decode(b, e, (signed char*)pVal);
}

//----------------------------------------------------------------------------



//----------------------------------------------------------------------------
template < typename T, std::enable_if_t< std::is_integral_v<T>  /* && ! std::is_signed_v<T> */ , int> = 0 >
std::size_t decode(const std::uint8_t *pBuf, std::size_t bufSize, T &t)
{
    return decode (pBuf, bufSize, &t);
}

//----------------------------------------------------------------------------
template < typename IteratorType, typename T, std::enable_if_t< std::is_integral_v<T>  /* && ! std::is_signed_v<T> */ , int> = 0 >
IteratorType decode(IteratorType b, IteratorType e, T &t)
{
    return decode<IteratorType>(b, e, &t);
}

//----------------------------------------------------------------------------



//----------------------------------------------------------------------------
} // namespace varint
} // namespace marty

// marty::varint::
