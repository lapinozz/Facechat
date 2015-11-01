#include "Utility.hpp"

std::string replaceAll(std::string str, const std::string& from, const std::string& to)
{
    size_t pos = 0;
    while((pos = str.find(from, pos)) != std::string::npos)
    {
        str.replace(pos, from.length(), to);
        pos += to.length();
    }

    return str;
}

std::string toBase(unsigned long long int value, int base)
{
    if (value == 0) return "0";
    std::string result;
    while (value != 0)
    {
        int digit = value % base;
        result += (digit > 9 ? 'A' + digit - 10 : digit + '0');
        value /= base;
    }
    std::reverse(result.begin(), result.end());
    return result;
}

std::string binaryStringToDecimalString(std::string binary)
{
    std::string decimal;

    for(char& binChar : binary)
    {
        int carry = 0;

        for(char& decChar : decimal)
        {
            int d = decChar *2 + carry;
            carry = d > 9;
            decChar = d % 10;
        }

        if (binChar == '1')
            decimal[0] |= 1;
    }

    return decimal;
}

bool isBase64(const unsigned char& toTest)
{
    return std::isalnum(toTest) || (toTest == '+') || (toTest == '/');
}

std::string encodeBase64(const std::string& toEncode)
{
    int length = toEncode.length();
    unsigned char const* currentByte = reinterpret_cast<const unsigned char*>(toEncode.c_str());
    unsigned char array_3[3];
    int i = 0;
    int j = 0;
    std::string returnString;

    while(length--)
    {
        array_3[i++] = *(currentByte++);
        if (i == 3)
        {
            returnString += base64Chars[(array_3[0] & 0xfc) >> 2];
            returnString += base64Chars[((array_3[0] & 0x03) << 4) + ((array_3[1] & 0xf0) >> 4)];
            returnString += base64Chars[((array_3[1] & 0x0f) << 2) + ((array_3[2] & 0xc0) >> 6)];
            returnString += base64Chars[array_3[2] & 0x3f];

            i = 0;
        }
    }

    if (i)
    {
        for(j = i; j < 3; j++)
            array_3[j] = '\0';

        returnString += base64Chars[(array_3[0] & 0xfc) >> 2];
        returnString += base64Chars[((array_3[0] & 0x03) << 4) + ((array_3[1] & 0xf0) >> 4)];
        if(i > 1)
            returnString += base64Chars[((array_3[1] & 0x0f) << 2) + ((array_3[2] & 0xc0) >> 6)];

        while(i++ < 3)
            returnString += '=';
    }

    return returnString;
}

std::string decodeBase64(const std::string& toDecode)
{
    int lenght = toDecode.length();
    int i = 0;
    int j = 0;
    int index = 0;
    unsigned char char_array_4[4];
    std::string returnString;

    while(true)
    {
        while((i < 4) && (index < lenght) && (toDecode[index] != '=') && isBase64(toDecode[index]))
            char_array_4[i++] = base64Chars.find(toDecode[index++]);

        if((index < lenght) && (i == 4))
        {
            returnString += (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            returnString += ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            returnString += ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            i = 0;
        }
        else
            break;
    }

    if (i)
    {
        for (j = i; j <4; j++)
            char_array_4[j] = 0;

        if(i > 1)
            returnString += (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        if(i > 2)
            returnString += ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        if(i > 3)
            returnString += ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
    }

    return returnString;
}

std::string timestampToString(time_t timestamp, bool elapsed)
{
    std::string res = "";

    struct tm * timeinfo;

    if(elapsed)
    {
        time_t tmpTime = difftime(time (NULL), timestamp);
        timeinfo = gmtime(&tmpTime);
        timeinfo->tm_year -= 70;

        if(!timeinfo->tm_year && (timeinfo->tm_mday > 3))
            timeinfo = localtime(&timestamp);
    }
    else
        timeinfo = localtime(&timestamp);

    if(!timeinfo->tm_year)
    {
        if(!timeinfo->tm_yday)
        {
            if(!timeinfo->tm_hour)
            {
                if(!timeinfo->tm_min)
                {
                    if(!timeinfo->tm_sec)
                        res = "now";
                    else
                        res = std::to_string(timeinfo->tm_sec) + " seconds ago";
                }
                else
                    res = std::to_string(timeinfo->tm_min) +  " minutes ago";
            }
            else
                res = std::to_string(timeinfo->tm_hour) +  " hours ago";
        }
        else if(timeinfo->tm_mday < 4)
            res = std::to_string(timeinfo->tm_mday) +  " days ago";
    }

    if(!res.length())
    {
        char buffer[80];
        strftime (buffer,80,"%A, %B %d, %Y %I:%M:%S %p",timeinfo);
        res = buffer;
    }

    return res;
}
