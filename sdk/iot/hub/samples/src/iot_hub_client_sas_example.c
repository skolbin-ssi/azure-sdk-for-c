// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifdef _MSC_VER
// warning C4201: nonstandard extension used: nameless struct/union
#pragma warning(disable : 4201)
#endif
#include <paho-mqtt/MQTTClient.h>
#ifdef _MSC_VER
#pragma warning(default : 4201)
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <az_iot_hub_client.h>
#include <az_result.h>
#include <az_span.h>
#include <az_span_internal.h>

#include <openssl/x509.h>
#include <openssl/hmac.h>

// TODO: #564 - Remove the use of the _az_cfh.h header in samples.
//              Note: this is required to work-around MQTTClient.h as well as az_span init issues.
#include <_az_cfg.h>

#define iothub_fqdn "iot-sdks-tcpstreaming.azure-devices.net"
#define device_id "ewertons-device2"
static char* my_device_key = "cwYKVJvNmyWlrAxOc44loraO3RLJT6epMnwHZvLya/I=";
#define sas_token_expiration_time  1589506975

#define SAS_TOKEN_BUFFER_SIZE 512

static int generate_sas_token(az_iot_hub_client* client, char* device_key, size_t device_key_length, uint32_t expiration, char* sas_token, uint32_t sas_token_size)
{
    uint8_t signature[512];

    az_span signature_span = AZ_SPAN_FROM_BUFFER(signature);
    az_span out_signature_span;

    az_span device_key_span = az_span_init((uint8_t*)device_key, (int32_t)device_key_length);

    char encryption_key[512];
    size_t encryption_key_length;

    uint8_t encrypted_signature[512];
    uint32_t encrypted_signature_length;

    if (az_failed(az_iot_hub_client_sas_get_signature(client, expiration, signature_span, &out_signature_span)))
    {
        printf("Failed getting SAS signature\r\n");
        return 1;
    }

    if (az_failed(az_iot_hub_client_sas_get_encryption_key(device_key_span, encryption_key, _az_COUNTOF(encryption_key), &encryption_key_length)))
    {
        printf("Failed getting encryption key\r\n");
        return 1;
    }

    HMAC(EVP_sha256(),
        encryption_key, (int)encryption_key_length,
        az_span_ptr(out_signature_span), (size_t)az_span_size(out_signature_span),
        encrypted_signature, &encrypted_signature_length);

    uint8_t base64_hmac_sha256_signature[512];
    az_span base64_hmac_sha256_signature_span = az_span_init(base64_hmac_sha256_signature, _az_COUNTOF(base64_hmac_sha256_signature));
    int32_t base64_hmac_sha256_signature_length;

    if (az_failed(_az_span_base64_encode(base64_hmac_sha256_signature_span, az_span_init(encrypted_signature, (int32_t)encrypted_signature_length), &base64_hmac_sha256_signature_length)))
    {
        printf("Failed base64 encoding encrypted signature\r\n");
        return 1;        
    }

    size_t sas_token_length = 0;
    if (az_failed(az_iot_hub_client_sas_get_password(
            client, 
            az_span_slice(base64_hmac_sha256_signature_span, 0, base64_hmac_sha256_signature_length), 
            expiration, 
            AZ_SPAN_NULL, 
            sas_token, 
            sas_token_size, 
            &sas_token_length)))
    {
        printf("Failed getting SAS token\r\n");
        return 1;
    }
    else if (sas_token_length == sas_token_size)
    {
        printf("Failed setting SAS token null terminator, not enough space\r\n");
        return 1;
    }

    sas_token[sas_token_length] = '\0';

    return 0;
}

int main()
{
    az_iot_hub_client client;
    char sas_token[SAS_TOKEN_BUFFER_SIZE];

    const char* expected_token = "SharedAccessSignature sr=iot-sdks-tcpstreaming.azure-devices.net%2Fdevices%2Fewertons-device2&sig=0tHJQbYeDyHuTpL9nPL2YBh1Gn%2BsXYPndvvgeLU2Arg%3D&se=1589506975";

    if (az_failed(az_iot_hub_client_init(
        &client,
        AZ_SPAN_FROM_STR(iothub_fqdn),
        AZ_SPAN_FROM_STR(device_id),
        NULL)))
    {
        printf("Failed initializing iot hub client\r\n");
        return 1;
    }

    if (generate_sas_token(&client, my_device_key, strlen(my_device_key), sas_token_expiration_time, sas_token, _az_COUNTOF(sas_token)) != 0)
    {
        printf("Failed generating sas token\r\n");
    }

    printf("generated: %s\r\n", sas_token);
    printf("expected:  %s\r\n", expected_token);

    return 0;
}
