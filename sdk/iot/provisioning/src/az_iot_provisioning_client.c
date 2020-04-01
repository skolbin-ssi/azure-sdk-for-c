// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <stdint.h>

#include "az_iot_provisioning_client.h"
#include <az_precondition_internal.h>
#include <az_result.h>
#include <az_span.h>

#include <_az_cfg.h>

static const az_span provisioning_service_api_version
    = AZ_SPAN_LITERAL_FROM_STR("/api-version=" AZ_IOT_PROVISIONING_SERVICE_VERSION);

AZ_NODISCARD az_iot_provisioning_client_options az_iot_provisioning_client_options_default()
{
  return (az_iot_provisioning_client_options){ .user_agent = AZ_SPAN_NULL };
}

AZ_NODISCARD az_result az_iot_provisioning_client_init(
    az_iot_provisioning_client* client,
    az_span global_device_endpoint,
    az_span id_scope,
    az_span registration_id,
    az_iot_provisioning_client_options const* options)
{
  AZ_PRECONDITION_NOT_NULL(client);
  AZ_PRECONDITION_VALID_SPAN(global_device_endpoint, 1, false);
  AZ_PRECONDITION_VALID_SPAN(id_scope, 1, false);
  AZ_PRECONDITION_VALID_SPAN(registration_id, 1, false);

  client->_internal.global_device_endpoint = global_device_endpoint;
  client->_internal.id_scope = id_scope;
  client->_internal.registration_id = registration_id;

  if (options != NULL)
  {
    client->_internal.options.user_agent = options->user_agent;
  }
  else
  {
    client->_internal.options = az_iot_provisioning_client_options_default();
  }

  return AZ_OK;
}

// <id_scope>/registrations/<registration_id>/api-version=<service_version>
AZ_NODISCARD az_result az_iot_provisioning_client_user_name_get(
    az_iot_provisioning_client const* client,
    az_span mqtt_user_name,
    az_span* out_mqtt_user_name)
{
  AZ_PRECONDITION_NOT_NULL(client);
  AZ_PRECONDITION_VALID_SPAN(mqtt_user_name, 0, false);
  AZ_PRECONDITION_NOT_NULL(out_mqtt_user_name);

  *out_mqtt_user_name
      = az_span_init(az_span_ptr(mqtt_user_name), 0, az_span_capacity(mqtt_user_name));

  const az_span* const user_agent = &(client->_internal.options.user_agent);

  AZ_RETURN_IF_FAILED(
      az_span_append(*out_mqtt_user_name, client->_internal.id_scope, out_mqtt_user_name));
  AZ_RETURN_IF_FAILED(
      az_span_append(*out_mqtt_user_name, AZ_SPAN_FROM_STR("/registrations/"), out_mqtt_user_name));
  AZ_RETURN_IF_FAILED(
      az_span_append(*out_mqtt_user_name, client->_internal.registration_id, out_mqtt_user_name));
  AZ_RETURN_IF_FAILED(
      az_span_append(*out_mqtt_user_name, provisioning_service_api_version, out_mqtt_user_name));

  if (az_span_length(*user_agent) > 0)
  {
    AZ_RETURN_IF_FAILED(az_span_append_uint8(*out_mqtt_user_name, '&', out_mqtt_user_name));
    AZ_RETURN_IF_FAILED(az_span_append(*out_mqtt_user_name, *user_agent, out_mqtt_user_name));
  }

  return AZ_OK;
}

// <registration_id>
AZ_NODISCARD az_result az_iot_provisioning_client_id_get(
    az_iot_provisioning_client const* client,
    az_span mqtt_client_id,
    az_span* out_mqtt_client_id)
{
  AZ_PRECONDITION_NOT_NULL(client);
  AZ_PRECONDITION_VALID_SPAN(mqtt_client_id, 0, false);
  AZ_PRECONDITION_NOT_NULL(out_mqtt_client_id);

  *out_mqtt_client_id
      = az_span_init(az_span_ptr(mqtt_client_id), 0, az_span_capacity(mqtt_client_id));

  AZ_RETURN_IF_FAILED(
      az_span_append(*out_mqtt_client_id, client->_internal.registration_id, out_mqtt_client_id));

  return AZ_OK;
}

AZ_NODISCARD az_result az_iot_provisioning_client_register_subscribe_topic_filter_get(
    az_iot_provisioning_client const* client,
    az_span mqtt_topic_filter,
    az_span* out_mqtt_topic_filter)
{
  UNUSED(client);

  AZ_PRECONDITION_NOT_NULL(client);
  AZ_PRECONDITION_VALID_SPAN(mqtt_topic_filter, 0, false);
  AZ_PRECONDITION_NOT_NULL(out_mqtt_topic_filter);

  *out_mqtt_topic_filter
      = az_span_init(az_span_ptr(mqtt_topic_filter), 0, az_span_capacity(mqtt_topic_filter));

  az_span filter = AZ_SPAN_LITERAL_FROM_STR("$dps/registrations/res/#");
  AZ_RETURN_IF_FAILED(az_span_append(*out_mqtt_topic_filter, filter, out_mqtt_topic_filter));

  return AZ_OK;
}

AZ_NODISCARD az_result az_iot_provisioning_client_received_topic_payload_parse(
    az_iot_provisioning_client const* client,
    az_span received_topic,
    az_span received_payload,
    az_iot_provisioning_client_register_response* out_response)
{
  UNUSED(client);
  UNUSED(received_topic);
  UNUSED(received_payload);
  UNUSED(out_response);

  return AZ_ERROR_NOT_IMPLEMENTED;
}

// $dps/registrations/PUT/iotdps-register/?$rid={%s}
// Payload: {'registrationId' : '%s'}
AZ_NODISCARD az_result az_iot_provisioning_client_register_publish_topic_get(
    az_iot_provisioning_client const* client,
    az_span mqtt_topic,
    az_span* out_mqtt_topic)
{
  UNUSED(client);

  AZ_PRECONDITION_NOT_NULL(client);
  AZ_PRECONDITION_VALID_SPAN(mqtt_topic, 0, false);
  AZ_PRECONDITION_NOT_NULL(out_mqtt_topic);

  *out_mqtt_topic = az_span_init(az_span_ptr(mqtt_topic), 0, az_span_capacity(mqtt_topic));
  az_span topic = AZ_SPAN_LITERAL_FROM_STR("$dps/registrations/PUT/iotdps-register/?$rid={1}");
  AZ_RETURN_IF_FAILED(az_span_append(*out_mqtt_topic, topic, out_mqtt_topic));

  return AZ_OK;
}

// Topic: $dps/registrations/GET/iotdps-get-operationstatus/?$rid={%s}&operationId=%s
AZ_NODISCARD az_result az_iot_provisioning_client_get_operation_status_publish_topic_get(
    az_iot_provisioning_client const* client,
    az_span request_id,
    az_span mqtt_topic,
    az_span* out_mqtt_topic)
{
  UNUSED(client);

  AZ_PRECONDITION_NOT_NULL(client);
  AZ_PRECONDITION_VALID_SPAN(mqtt_topic, 0, false);
  AZ_PRECONDITION_NOT_NULL(out_mqtt_topic);

  *out_mqtt_topic = az_span_init(az_span_ptr(mqtt_topic), 0, az_span_capacity(mqtt_topic));
  az_span topic = AZ_SPAN_LITERAL_FROM_STR(
      "$dps/registrations/GET/iotdps-get-operationstatus/?$rid={1}&operationId=");
  AZ_RETURN_IF_FAILED(az_span_append(*out_mqtt_topic, topic, out_mqtt_topic));
  AZ_RETURN_IF_FAILED(az_span_append(*out_mqtt_topic, request_id, out_mqtt_topic));

  return AZ_OK;
}