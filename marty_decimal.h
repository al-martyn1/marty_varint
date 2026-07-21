/* \file
   \brief Protobuf varint encoding an decoding for marty::Decimal

 */

#pragma once

#include "integral.h"
//
#include "marty_decimal/marty_decimal.h"

//----------------------------------------------------------------------------



//----------------------------------------------------------------------------
namespace marty{

    // IEData exportData() const
    // void importData(IEData ieData)
    // struct IEData // Import/Export Data
    // {
    //     int     totalLen  = 0; // +sign
    //     int     precision = 0; // number of digits after decimal dot
    //  
    // #ifndef MARTY_BCD_USE_VECTOR
    //     std::basic_string<bcd::decimal_udigit_t>   packedBcd;
    // #else
    //     std::vector<bcd::decimal_udigit_t>         packedBcd;
    // #endif
    //  
    // }; // struct IEData

//----------------------------------------------------------------------------
inline
size_t encode(std::uint8_t *pBuf, std::size_t bufSize, marty::Decimal d)
{
    auto ieData = d.exportData();

    const std::size_t digitsNumber = std::size_t(ieData.totalLen<0 ? -ieData.totalLen : ieData.totalLen);
    const std::size_t packedDigitsNumber = digitsNumber&1 ? digitsNumber/2+1 : digitsNumber/2;
    if (packedDigitsNumber!=ieData.packedBcd.size())
        throw std::invalid_argument("inconsistent marty::Decimal::IEData");

    std::size_t encSize = marty::varint::encode(pBuf, bufSize, ieData.totalLen);
    if (encSize==0)
        return encSize; // что-то пошло не так

    // Ноль сериализуется в одно нулевое поле
    if (ieData.totalLen==0)
         return encSize;

    std::size_t encSizeTotal = encSize;
    bufSize -= encSize;
    if (pBuf)
        pBuf += encSize;

    encSize = marty::varint::encode(pBuf, bufSize, ieData.precision);
    if (encSize==0)
        return encSize; // что-то пошло не так

    encSizeTotal += encSize;
    bufSize -= encSize;
    if (pBuf)
        pBuf += encSize;

    if (bufSize<ieData.packedBcd.size())
        return 0;

    if (pBuf)
    {
        for(std::size_t i=0; i!=ieData.packedBcd.size(); ++i)
        {
            *pBuf++ = ieData.packedBcd[i];
        }
    }

    return encSizeTotal + ieData.packedBcd.size();
}

//----------------------------------------------------------------------------
template<typename BackInsertIteratorType>
BackInsertIteratorType encode(BackInsertIteratorType iter, marty::Decimal d)
{
    auto ieData = d.exportData();

    const std::size_t digitsNumber = std::size_t(ieData.totalLen<0 ? -ieData.totalLen : ieData.totalLen);
    const std::size_t packedDigitsNumber = digitsNumber&1 ? digitsNumber/2+1 : digitsNumber/2;
    if (packedDigitsNumber!=ieData.packedBcd.size())
        throw std::invalid_argument("inconsistent marty::Decimal::IEData");

    iter = marty::varint::encode(iter, ieData.totalLen);
    iter = marty::varint::encode(iter, ieData.precision);

    for(std::size_t i=0; i!=ieData.packedBcd.size(); ++i)
    {
        *iter++ = ieData.packedBcd[i];
    }

    return iter;
}

//----------------------------------------------------------------------------
inline
std::size_t decode(const std::uint8_t *pBuf, std::size_t bufSize, marty::Decimal *pVal=0)
{
    marty::Decimal::IEData ieData;

    std::size_t readed = marty::varint::decode(pBuf, bufSize, ieData.totalLen);
    if (!readed)
         return readed;

    if (ieData.totalLen==0)
    {
        if (pVal)
            pVal->importData(ieData);
        return readed;
    }

    std::size_t readedTotal = readed;
    bufSize -= readed;
    pBuf += readed;

    readed = marty::varint::decode(pBuf, bufSize, ieData.precision);
    if (!readed)
        return readed;

    readedTotal += readed;
    bufSize -= readed;
    pBuf += readed;

    const std::size_t digitsNumber = std::size_t(ieData.totalLen<0 ? -ieData.totalLen : ieData.totalLen);
    const std::size_t packedDigitsNumber = digitsNumber&1 ? digitsNumber/2+1 : digitsNumber/2;

    if (bufSize<packedDigitsNumber)
        return 0;

    for(std::size_t i=0; i!=packedDigitsNumber; ++i)
    {
        ieData.packedBcd.push_back(*pBuf++);
    }

    if (pVal)
        pVal->importData(ieData);

    return readedTotal+packedDigitsNumber;
}

//----------------------------------------------------------------------------
template<typename IteratorType>
IteratorType decode( IteratorType b, IteratorType e, marty::Decimal *pVal=0)
{
    marty::Decimal::IEData ieData;

    b = marty::varint::decode(b, e, ieData.totalLen);

    if (ieData.totalLen==0)
    {
        if (pVal)
            pVal->importData(ieData);
        return b;
    }

    b = marty::varint::decode(b, e, ieData.precision);

    const std::size_t digitsNumber = std::size_t(ieData.totalLen<0 ? -ieData.totalLen : ieData.totalLen);
    const std::size_t packedDigitsNumber = digitsNumber&1 ? digitsNumber/2+1 : digitsNumber/2;

    std::size_t i = 0;

    for(; i!=packedDigitsNumber && b!=e; ++i, ++b)
    {
        ieData.packedBcd.push_back(*b);
    }

    if (i!=packedDigitsNumber)
    {
        // b!=e сработало раньше
        throw std::invalid_argument("unexpected end of data while parsing varint marty::Decimal");
    }

    if (pVal)
        pVal->importData(ieData);

    return b;
}

//----------------------------------------------------------------------------
inline
std::size_t decode(const std::uint8_t *pBuf, std::size_t bufSize, marty::Decimal &d)
{
    return decode(pBuf, bufSize, &d);
}

//----------------------------------------------------------------------------
template < typename IteratorType >
IteratorType decode(IteratorType b, IteratorType e, marty::Decimal &d)
{
    return decode(b, e, &d);
}




} // namespace marty
