#include <HTTPClient.h>
#include "cert.h"
#include "myconfig2.h"
#include <esp_camera.h>
#include <ArduinoJson.h>
#include "myconfig2.h"

// Functions from the main .ino
extern void flashLED(int flashtime);
extern void setLamp(int newVal);

extern int lampVal;
extern bool autoLamp;
extern bool debugData;
extern char mdnsName;


String serverurl = CAMSERVER;
String bearertokenserver = BEARERTOKEN;

/*
   send status

*/
esp_err_t SendStatusHttp()
{
    HTTPClient http;

    JsonDocument doc;

    doc["esphostname"] = String(mdnsName);
    doc["rssi"] = String(WiFi.RSSI());

    // Serialize JSON document
    String json;
    serializeJson(doc, json);

    http.begin(serverurl + "/espstatus/", root_ca); // Specify the URL and certificate
    http.addHeader("Content-Type", "application/json");

    int httpCode = http.POST(json);

    http.end();

    if (httpCode > 0)
    {
        Serial.println(httpCode);
        return ESP_OK;
    }
    return ESP_FAIL;
}

/*

    send weather data to villa using https

*/

esp_err_t SendPictureHttp()
{

    esp_err_t res = ESP_OK;
    if (PUSH_PICTURE == true)
    {

        camera_fb_t *fb = NULL;

        Serial.print("SendPictureHttp ");
        Serial.println(PICTURE);
        if (autoLamp && (lampVal != -1))
        {
            setLamp(lampVal);
            delay(75); // coupled with the status led flash this gives ~150ms for lamp to settle.
        }
        flashLED(75); // little flash of status LED

        int64_t fr_start = esp_timer_get_time();

        fb = esp_camera_fb_get();
        if (!fb)
        {
            Serial.println("CAPTURE: failed to acquire frame");
            if (autoLamp && (lampVal != -1))
                setLamp(0);
            esp_camera_return_all();
            return ESP_FAIL;
        }

        size_t fb_len = 0;
        if (fb->format == PIXFORMAT_JPEG)
        {
            fb_len = fb->len;
            HTTPClient http;

            http.begin(serverurl + "/muccam/", root_ca); // Specify the URL and certificate
            http.addHeader("Content-Type", "image/jpeg");
            http.addHeader("Content-Length", String(fb_len));
            http.addHeader("authorization", String(bearertokenserver));
            http.addHeader("Filename", PICTURE);

            int httpCode = http.POST(fb->buf, fb->len);

            if (httpCode > 0)
            { // Check for the returning code

                String payload = http.getString();
                Serial.println(httpCode);
            }
            else
            {
                Serial.println("Error on HTTP request " + String(httpCode));
                res = ESP_FAIL;
            }
            http.end(); // Free the resources
        }
        else
        {
            res = ESP_FAIL;
            Serial.println("Capture Error: Non-JPEG image returned by camera module");
        }
        esp_camera_fb_return(fb);
        fb = NULL;

        int64_t fr_end = esp_timer_get_time();
        if (debugData)
        {
            Serial.printf("JPG: %uB %ums\r\n", (uint32_t)(fb_len), (uint32_t)((fr_end - fr_start) / 1000));
        }

        if (autoLamp && (lampVal != -1))
        {
            setLamp(0);
        }
    }

    return res;
}