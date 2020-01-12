#include <M5StickC.h>
#include "smartmeter.h"
#include <WiFi.h>
#include <WiFiMulti.h>
#include "config.h"
#include "Free_Fonts.h"

void smartmeterCallback(SmartMeter *sm);

int fontWidth;
int fontHeight;
WiFiMulti wm;
void setup()
{
    M5.begin();
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setRotation(1);
    Serial.begin(115200);

    Serial.print("Connecting to WIFI");
    wm.addAP(WIFI_SSID, WIFI_PASS);
    while (wm.run() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(100);
    }
    Serial.println("¥nWiFi connected");
    Serial.println("IP address: ");
    IPAddress addr;
    addr = WiFi.localIP();
    char buf[64];
    snprintf(buf, sizeof(buf), "%s", addr.toString().c_str());
    M5.Lcd.setTextFont(1);
    M5.Lcd.setTextColor(ORANGE);
    M5.Lcd.printf(buf);
    Serial.println(buf);
    M5.Lcd.setFreeFont(FF9); // Select the font
    fontWidth = M5.Lcd.textWidth("P");
    fontHeight = M5.Lcd.fontHeight();
    int x = 0;
    int y = 15;
    M5.Lcd.setTextColor(BLUE);
    M5.Lcd.drawString("Power:     [W]", x, y);
    y += fontHeight;
    M5.Lcd.drawString("Cur.R:     [A]", x, y);
    y += fontHeight;
    M5.Lcd.drawString("Cur.T:     [A]", x, y);

    echonetUdp.init();
    smartmeter.init(&echonetUdp, SMARTMETER_ADDRESS, smartmeterCallback);
    smartmeter.request();

    configTime(9 * 3600, 0, NTP_SERVER);
}

bool bSma = true;
long power = 0;
float curR = 0.0f;
float curT = 0.0f;
void smartmeterCallback(SmartMeter *sm)
{
    power = sm->getPower();
    curR = sm->getCurrentR() / 10.0f;
    curT = sm->getCurrentT() / 10.0f;
    bSma = true;
}

#define VIEW_INTERVAL (1 * 1000)
#define INTERVAL (30 * 1000)
unsigned long preMeas = millis();
unsigned long preView = millis();
uint8_t bright = 0;
void loop()
{
    long cur = millis();
    long d = cur - preMeas;
    if (d < 0)
        d += ULONG_MAX;
    if (d >= INTERVAL) // debug
    {
        preMeas = cur;
        smartmeter.request();
    }
    // echonetUdp.getServer();
    d = cur - preView;
    if (d < 0)
        d += ULONG_MAX;
    if (d >= VIEW_INTERVAL) // debug
    {
        preView = cur;
        char buf[64];
        if (bSma)
        {
            M5.Lcd.setFreeFont(FF9); // Select the font
            M5.Lcd.setTextColor(YELLOW);
            Serial.println("Smartmeter callback");

            int x = 6 * fontWidth;
            int w = 5 * fontWidth;

            int y = 15;
            snprintf(buf, sizeof(buf), "%5ld", power);
            M5.Lcd.fillRect(x, y, w, fontHeight, BLACK);
            M5.Lcd.drawString(buf, x, y);
            Serial.printf("Power:%s[W]\n", buf);

            y += fontHeight;
            snprintf(buf, sizeof(buf), "%5.1f", curR);
            M5.Lcd.fillRect(x, y, w, fontHeight, BLACK);
            M5.Lcd.drawString(buf, x, y);
            Serial.printf("Cur.R:%s[A]\n", buf);

            y += fontHeight;
            snprintf(buf, sizeof(buf), "%5.1f", curT);
            M5.Lcd.fillRect(x, y, w, fontHeight, BLACK);
            M5.Lcd.drawString(buf, x, y);
            Serial.printf("Cur.T:%s[A]\n", buf);

            bSma = false;
        }

        struct tm timeInfo;
        if (getLocalTime(&timeInfo))
        {
            snprintf(buf, sizeof(buf), "%04d/%02d/%02d %02d:%02d:%02d", timeInfo.tm_year + 1900, timeInfo.tm_mon + 1, timeInfo.tm_mday, timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec);
            M5.Lcd.setTextFont(1);
            M5.Lcd.setTextColor(ORANGE);
            int w = M5.Lcd.textWidth(buf);
            int h = M5.Lcd.fontHeight();
            int x = M5.Lcd.width() - w;
            int y = M5.Lcd.height() - h;
            M5.Lcd.fillRect(x, y, w, h, BLACK);
            M5.Lcd.drawString(buf, x, y);
        }
    }
    if (M5.BtnA.wasPressed())
    {
        if (++bright > 5)
            bright = 0;
        M5.Axp.ScreenBreath(bright + 7);
        Serial.printf("bright: %d\n", bright);
    }

    delay(10);
    M5.update();
}