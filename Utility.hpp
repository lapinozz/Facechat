#ifndef UTILITY_H
#define UTILITY_H

#include <stdinclude.hpp>

std::string replaceAll(std::string str, const std::string& from, const std::string& to);

bool isBase64(const unsigned char& toTest);
std::string encodeBase64(const std::string& toEncode);
std::string decodeBase64(const std::string& toDecode);
static const std::string base64Chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

std::string timestampToString(time_t timestamp, bool elapsed = false);

std::string toBase(unsigned long long int value, int base);
std::string binaryStringToDecimalString(std::string binary);

namespace __range_to_initializer_list
{

constexpr size_t DEFAULT_MAX_LENGTH = 256;

template <typename V> struct backingValue
{
    static V value;
};
template <typename V> V backingValue<V>::value(V("",""));

template <typename V, typename... Vcount> struct backingList
{
    static std::initializer_list<V> list;
};
template <typename V, typename... Vcount>
std::initializer_list<V> backingList<V, Vcount...>::list = {(Vcount)backingValue<V>::value...};

template <size_t maxLength, typename It, typename V = typename It::value_type, typename... Vcount>
static typename std::enable_if< sizeof...(Vcount) >= maxLength,
       std::initializer_list<V> >::type generate_n(It begin, It end, It current)
{
    throw std::length_error("More than maxLength elements in range.");
}

template <size_t maxLength = DEFAULT_MAX_LENGTH, typename It, typename V = typename It::value_type, typename... Vcount>
static typename std::enable_if< sizeof...(Vcount) < maxLength,
       std::initializer_list<V> >::type generate_n(It begin, It end, It current)
{
    if (current != end)
        return generate_n<maxLength, It, V, V, Vcount...>(begin, end, ++current);

    current = begin;
    for (auto it = backingList<V,Vcount...>::list.begin();
            it != backingList<V,Vcount...>::list.end();
            ++current, ++it)
        *const_cast<V*>(&*it) = *current;

    return backingList<V,Vcount...>::list;
}

}

template <typename It>
std::initializer_list<typename It::value_type> range_to_initializer_list(It begin, It end)
{
    return __range_to_initializer_list::generate_n(begin, end, begin);
}

#endif // UTILITY_H
