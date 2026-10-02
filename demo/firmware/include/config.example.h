#ifndef CONFIG_H
#define CONFIG_H

// Copy to config.h and fill in.
// config.h holds Wi-Fi credentials and endpoint secrets — gitignored; never commit it.
// After demo/aws/provision-thing.sh, set AWS_IOT_ENDPOINT from out/<thing>/iot-endpoint.txt.

#ifndef WIFI_SSID
#define WIFI_SSID "your-ssid"
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "your-password"
#endif

#ifndef AWS_IOT_ENDPOINT
#define AWS_IOT_ENDPOINT "xxxxxxxxxxxx-ats.iot.ap-southeast-2.amazonaws.com"
#endif

#ifndef THING_NAME
#define THING_NAME "esp32-c61-01"
#endif

#ifndef PUBLISH_INTERVAL_SEC
#define PUBLISH_INTERVAL_SEC 15
#endif

#ifndef NTP_SERVER
#define NTP_SERVER "pool.ntp.org"
#endif

#if PUBLISH_INTERVAL_SEC < 10 || PUBLISH_INTERVAL_SEC > 3600
#error "PUBLISH_INTERVAL_SEC must be between 10 and 3600"
#endif

#endif  // CONFIG_H
