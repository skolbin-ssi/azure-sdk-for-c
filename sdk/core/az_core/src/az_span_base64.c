// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_hex_private.h"
#include "az_span_private.h"
#include <az_platform_internal.h>
#include <az_precondition.h>
#include <az_precondition_internal.h>
#include <az_span.h>
#include <az_span_internal.h>

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>

#include <_az_cfg.h>


#define splitInt(intVal, bytePos)   (uint8_t)((intVal >> (bytePos << 3)) & 0xFF)
#define joinChars(a, b, c, d) (uint32_t)((uint32_t)a + ((uint32_t)b << 8) + ((uint32_t)c << 16) + ((uint32_t)d << 24))

static uint8_t base64char(int32_t val)
{
    uint8_t result;

    if (val < 26)
    {
        result = (uint8_t)('A' + val);
    }
    else if (val < 52)
    {
        result = (uint8_t)('a' + (val - 26));
    }
    else if (val < 62)
    {
        result = (uint8_t)('0' + (val - 52));
    }
    else if (val == 62)
    {
        result = '+';
    }
    else
    {
        result = '/';
    }

    return result;
}

static uint8_t base64b16(int32_t val)
{
    const uint32_t base64b16values[4] = {
        joinChars('A', 'E', 'I', 'M'),
        joinChars('Q', 'U', 'Y', 'c'),
        joinChars('g', 'k', 'o', 's'),
        joinChars('w', '0', '4', '8')
    };
    return splitInt(base64b16values[val >> 2], (val & 0x03));
}

static uint8_t base64b8(unsigned char val)
{
    const uint32_t base64b8values = joinChars('A', 'Q', 'g', 'w');
    return splitInt(base64b8values, val);
}

static int base64toValue(uint8_t base64character, uint8_t* value)
{
    int result = 0;
    if (('A' <= base64character) && (base64character <= 'Z'))
    {
        *value = (uint8_t)(base64character - 'A');
    }
    else if (('a' <= base64character) && (base64character <= 'z'))
    {
        *value = (uint8_t)(('Z' - 'A') + 1 + (base64character - 'a'));
    }
    else if (('0' <= base64character) && (base64character <= '9'))
    {
        *value = (uint8_t)(('Z' - 'A') + 1 + ('z' - 'a') + 1 + (base64character - '0'));
    }
    else if ('+' == base64character)
    {
        *value = 62;
    }
    else if ('/' == base64character)
    {
        *value = 63;
    }
    else
    {
        *value = 0;
        result = -1;
    }
    return result;
}

static size_t numberOfBase64Characters(const uint8_t* encodedString)
{
    size_t length = 0;
    uint8_t junkChar;
    while (base64toValue(encodedString[length],&junkChar) != -1)
    {
        length++;
    }
    return length;
}

/*returns the count of original bytes before being base64 encoded*/
/*notice NO validation of the content of encodedString. Its length is validated to be a multiple of 4.*/
static int32_t Base64decode_len(uint8_t* encodedString, int32_t sourceLength)
{
    int32_t result;

    if (sourceLength == 0)
    {
        result = 0;
    }
    else
    {
        result = sourceLength / 4 * 3;
        if (encodedString[sourceLength - 1] == '=')
        {
            if (encodedString[sourceLength - 2] == '=')
            {
                result --;
            }
            result--;
        }
    }
    return result;
}

AZ_NODISCARD az_result _az_span_base64_decode(az_span destination, az_span source, int32_t* out_length)
{
    _az_PRECONDITION_VALID_SPAN(source, 1, false);
    _az_PRECONDITION_VALID_SPAN(destination, 1, false);
    _az_PRECONDITION_NOT_NULL(out_length);

    if ((az_span_size(source) % 4) != 0)
    {
        return AZ_ERROR_ARG;
    }
    
    AZ_RETURN_IF_NOT_ENOUGH_SIZE(destination, Base64decode_len(az_span_ptr(source), az_span_size(source)));

    size_t numberOfEncodedChars;
    size_t indexOfFirstEncodedChar;
    size_t decodedIndex;

    uint8_t* base64String = az_span_ptr(source);
    uint8_t* decodedString = az_span_ptr(destination);
    //
    // We can only operate on individual bytes.  If we attempt to work
    // on anything larger we could get an alignment fault on some
    // architectures
    //

    numberOfEncodedChars = numberOfBase64Characters(base64String);
    indexOfFirstEncodedChar = 0;
    decodedIndex = 0;
    while (numberOfEncodedChars >= 4)
    {
        uint8_t c1;
        uint8_t c2;
        uint8_t c3;
        uint8_t c4;
        (void)base64toValue(base64String[indexOfFirstEncodedChar], &c1);
        (void)base64toValue(base64String[indexOfFirstEncodedChar + 1], &c2);
        (void)base64toValue(base64String[indexOfFirstEncodedChar + 2], &c3);
        (void)base64toValue(base64String[indexOfFirstEncodedChar + 3], &c4);
        decodedString[decodedIndex] = (uint8_t)((c1 << 2) | (c2 >> 4));
        decodedIndex++;
        decodedString[decodedIndex] = (uint8_t)(((c2 & 0x0f) << 4) | (c3 >> 2));
        decodedIndex++;
        decodedString[decodedIndex] = (uint8_t)(((c3 & 0x03) << 6) | c4);
        decodedIndex++;
        numberOfEncodedChars -= 4;
        indexOfFirstEncodedChar += 4;

    }

    if (numberOfEncodedChars == 2)
    {
        uint8_t c1;
        uint8_t c2;
        (void)base64toValue(base64String[indexOfFirstEncodedChar], &c1);
        (void)base64toValue(base64String[indexOfFirstEncodedChar + 1], &c2);
        decodedString[decodedIndex] = (uint8_t)((c1 << 2) | (c2 >> 4));
    }
    else if (numberOfEncodedChars == 3)
    {
        uint8_t c1;
        uint8_t c2;
        uint8_t c3;
        (void)base64toValue(base64String[indexOfFirstEncodedChar], &c1);
        (void)base64toValue(base64String[indexOfFirstEncodedChar + 1], &c2);
        (void)base64toValue(base64String[indexOfFirstEncodedChar + 2], &c3);
        decodedString[decodedIndex] = (uint8_t)((c1 << 2) | (c2 >> 4));
        decodedIndex++;
        decodedString[decodedIndex] = (uint8_t)(((c2 & 0x0f) << 4) | (c3 >> 2));
    }

    *out_length = (int32_t)(decodedIndex + 1);

    return AZ_OK;
}

AZ_NODISCARD az_result _az_span_base64_encode(az_span destination, az_span source, int32_t* out_length)
{
    _az_PRECONDITION_VALID_SPAN(destination, 1, false);
    _az_PRECONDITION_VALID_SPAN(source, 0, false);
    _az_PRECONDITION_NOT_NULL(out_length);

    int32_t required_size = 
        (az_span_size(source) == 0) ? (0) : ((((az_span_size(source) - 1) / 3) + 1) * 4) + 
        1; /*+1 because \0 at the end of the string*/

    AZ_RETURN_IF_NOT_ENOUGH_SIZE(destination, required_size);

    int32_t currentPosition = 0;
    uint8_t* source_ptr = az_span_ptr(source);
    int32_t size = az_span_size(source);
    uint8_t* encoded = az_span_ptr(destination);

    {
        /*b0            b1(+1)          b2(+2)
        7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0
        |----c1---| |----c2---| |----c3---| |----c4---|
        */

        int32_t destinationPosition = 0;
        while (size - currentPosition >= 3)
        {
            uint8_t c1 = base64char(source_ptr[currentPosition] >> 2);
            uint8_t c2 = base64char(
                (int32_t)((source_ptr[currentPosition] & 3) << 4) |
                    (source_ptr[currentPosition + 1] >> 4)
            );
            uint8_t c3 = base64char(
                (int32_t)((source_ptr[currentPosition + 1] & 0x0F) << 2) |
                    ((source_ptr[currentPosition + 2] >> 6) & 3)
            );
            uint8_t c4 = base64char(
                (int32_t)(source_ptr[currentPosition + 2] & 0x3F)
            );
            currentPosition += 3;
            encoded[destinationPosition++] = c1;
            encoded[destinationPosition++] = c2;
            encoded[destinationPosition++] = c3;
            encoded[destinationPosition++] = c4;

        }
        if (size - currentPosition == 2)
        {
            uint8_t c1 = base64char((int32_t)(source_ptr[currentPosition] >> 2));
            uint8_t c2 = base64char(
                ((source_ptr[currentPosition] & 0x03) << 4) |
                    (source_ptr[currentPosition + 1] >> 4)
            );
            uint8_t c3 = base64b16(source_ptr[currentPosition + 1] & 0x0F);
            encoded[destinationPosition++] = c1;
            encoded[destinationPosition++] = c2;
            encoded[destinationPosition++] = c3;
            encoded[destinationPosition++] = '=';
        }
        else if (size - currentPosition == 1)
        {
            uint8_t c1 = base64char(source_ptr[currentPosition] >> 2);
            uint8_t c2 = base64b8(source_ptr[currentPosition] & 0x03);
            encoded[destinationPosition++] = c1;
            encoded[destinationPosition++] = c2;
            encoded[destinationPosition++] = '=';
            encoded[destinationPosition++] = '=';
        }

        /*null terminating the string*/
        encoded[destinationPosition] = '\0';

        *out_length = destinationPosition;
    }

    return AZ_OK;
}
