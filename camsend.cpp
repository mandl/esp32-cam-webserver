#include <HTTPClient.h>
#include "cert.h"
#include "myconfig2.h"
#include <esp_camera.h>


// Functions from the main .ino
extern void flashLED(int flashtime);
extern void setLamp(int newVal);

extern int lampVal;
extern bool autoLamp;
extern bool debugData;

/*

    send weather data to villa using https

*/
esp_err_t SendPictureHttp()
{

    camera_fb_t *fb = NULL;
    esp_err_t res = ESP_OK;

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
        
        http.begin(CAMSERVER, root_ca); // Specify the URL and certificate
        http.addHeader("Content-Type", "image/jpeg");
        http.addHeader("Content-Length", String(fb_len));
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

    return res;
}