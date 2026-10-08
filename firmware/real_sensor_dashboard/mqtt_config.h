#pragma once

#if __has_include("mqtt_config.local.h")
#include "mqtt_config.local.h"
#endif

#ifndef MQTT_NOTIFICATIONS_ENABLED
#define MQTT_NOTIFICATIONS_ENABLED 0
#endif
#ifndef MQTT_BROKER_HOST
#define MQTT_BROKER_HOST ""
#endif
#ifndef MQTT_BROKER_PORT
#define MQTT_BROKER_PORT 8883
#endif
#ifndef MQTT_USERNAME
#define MQTT_USERNAME ""
#endif
#ifndef MQTT_PASSWORD
#define MQTT_PASSWORD ""
#endif
#ifndef MQTT_TOPIC_PREFIX
#define MQTT_TOPIC_PREFIX "smart-package-detector"
#endif
#ifndef MQTT_ROOT_CA
#define MQTT_ROOT_CA ""
#endif
