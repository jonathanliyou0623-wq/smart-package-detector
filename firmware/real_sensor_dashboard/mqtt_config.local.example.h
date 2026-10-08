#pragma once

// Copy to mqtt_config.local.h and replace every placeholder. The local file is
// ignored by Git. Use the root CA published by your MQTT broker; do not disable
// certificate validation for production notifications.
#define MQTT_NOTIFICATIONS_ENABLED 1
#define MQTT_BROKER_HOST "YOUR_MQTT_BROKER"
#define MQTT_BROKER_PORT 8883
#define MQTT_USERNAME "YOUR_MQTT_USERNAME"
#define MQTT_PASSWORD "YOUR_MQTT_PASSWORD"
#define MQTT_TOPIC_PREFIX "smart-package-detector/YOUR_DEVICE_NAME"
#define MQTT_ROOT_CA \
  "-----BEGIN CERTIFICATE-----\n" \
  "PASTE_BROKER_ROOT_CA_HERE\n" \
  "-----END CERTIFICATE-----\n"
