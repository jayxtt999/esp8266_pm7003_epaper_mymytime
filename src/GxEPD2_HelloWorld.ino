#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <TimeLib.h>
#include <NTPClient.h>
#include <DNSServer.h> //密码直连将其三个库注释
#include <ESP8266WebServer.h>
#include <CustomWiFiManager.h>
#include <coredecls.h>
#include <ESP8266_Seniverse.h>

#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <Fonts/FreeMonoBold9pt7b.h>

#include <U8g2_for_Adafruit_GFX.h>
#include <WiFiUdp.h>
#include "ArduinoOTA.h"
#include <Wire.h>
// select the display class and display driver class in the following file (new style):
//BUSY -> GPIO4, RST -> GPIO2, DC -> GPIO0, CS -> GPIO15, CLK -> GPIO14, DIN -> GPIO13, GND -> GND, 3.3V -> 3.3V
#define ENABLE_GxEPD2_GFX 0

#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <GxEPD2_7C.h>
#include <SimpleDHT.h>
#include <PMserial.h> // Arduino library for PM sensors with serial interface
#include<string.h>
//esp8266mcu
//CS    D8
//DC    D3
//RST   D4
//BUSY  D2
//SCK   D5
//SD1   D7

//d1mini
// BUSY -> D2, RST -> D4, DC -> D3, CS -> D8, CLK -> D5, DIN -> D7, GND -> GND, 3.3V -> 3.3V

GxEPD2_BW<GxEPD2_290_T5D, GxEPD2_290_T5D::HEIGHT> display(GxEPD2_290_T5D(/*CS=15*/ SS, /*DC=0*/ 0, /*RST=2*/ 2, /*BUSY=4*/ 4));
U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "ntp1.aliyun.com", 60 * 60 * 8, 30 * 60 * 1000);

SerialPM pms(PMSx003, Serial); // PMSx003, UART

// for DHT22,
//      VCC: 5V or 3V
//      GND: GND
//      DATA: 2
int pinDHT22 = D6;
SimpleDHT22 dht22(pinDHT22);

const char *wifihostname = "ESP_Epaper";
const char *jobname = "epaper";

char * HelloWorld[19] ={
"My! My! Time flies! One step and we're on the moon",
"next step into the stars",
"My! My! Time flies! Maybe we could be there soon",
"a one way ticket to mars",
"My! My! Time flies! A man underneath a tree",
"an apple falls on his head",
"my my time flies a man wrote a symphony, it's 1812",
"My! My! Time flies! Four guys across abbey road",
"one forgot to wear shoes",
"My! My! Time flies! A rap on a rhapsody",
"a king who's still in the news",
"a king to sing you the blues",
"My! My! Time flies! A man in a winter sleigh",
"white white white as the snow",
"My! My! Time flies! A new day is on its way",
"so let's let yesterday go",
"Could be we step out again",
"Could be tomorrow but then",
"could be 2012"
};


int maxPos = 18;
int currentPos = 0;

uint32_t targetTime = 0;
bool first = true; //首次更新标志

int16_t tw = 0;
int16_t ta = 0;
int16_t td = 0;
int16_t th = 0;
int16_t x = 0;
int16_t y = 0;
uint16_t bg = GxEPD_WHITE;
uint16_t fg = GxEPD_BLACK;
unsigned long update = 0;
int pm10 = 0;
int pm100 = 0;
int pm25 = 0;
float temperature = 0;
float humidity = 0;

const char *WIFI_SSID = "TP-LINK_2E4B50"; //填写你的WIFI名称及密码
const char *WIFI_PWD = "130413041304";
#define SHOW_TIME_PERIOD 60000
#define SHOW_ENV_PERIOD 10000
#define SHOW_ONE_SAY_PERIOD 20000

void setup()
{

    Serial.begin(9600);
    Serial.println();
    Serial.println("setup");
    display.init();
    pms.init();
    u8g2Fonts.begin(display); // connect u8g2 procedures to Adafruit GFX
    display.hibernate();
    display.setRotation(1);
    display.setPartialWindow(0, 0, display.width(), display.height());
    drawString("Starte Network");
    //Web配网，密码直连请注释
    //webconnect();
    wificonnect();
    drawString("WLAN connected");
    Serial.println("connected...yeey :)");
    ArduinoOTA.setHostname(wifihostname);

    ArduinoOTA.onStart([]()
                       {
                           if (ArduinoOTA.getCommand() == U_FLASH)
                           {
                               drawString("Uploading Firmware");
                               Serial.println("Start updating sketch");
                           }
                           else
                           { // U_SPIFFS
                               Serial.println("Start updating filesystem - unsupported?");
                           }
                       });
    ArduinoOTA.onEnd([]()
                     { Serial.println("End"); });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                          {
                              Serial.print("Progress:");
                              Serial.println(progress / (total / 100));
                          });
    ArduinoOTA.onError([](ota_error_t error)
                       {
                           if (error == OTA_AUTH_ERROR)
                           {
                               Serial.println("Auth Failed");
                           }
                           else if (error == OTA_BEGIN_ERROR)
                           {
                               Serial.println("Begin Failed");
                           }
                           else if (error == OTA_CONNECT_ERROR)
                           {
                               Serial.println("Connect Failed");
                           }
                           else if (error == OTA_RECEIVE_ERROR)
                           {
                               Serial.println("Receive Failed");
                           }
                           else if (error == OTA_END_ERROR)
                           {
                               Serial.println("End Failed");
                           }
                       });

    timeClient.begin();
    delay(2000);
}

void webconnect()
{ ////Web配网，密码直连将其注释
    // showWifiTips();
    WiFiManager wifiManager;                                //实例化WiFiManager
    wifiManager.setDebugOutput(false);                      //关闭Debug
    wifiManager.setPageTitle("请选择你的WIFI并且配置密码"); //设置页标题
    if (!wifiManager.autoConnect("baidu2"))
    { //AP模式
        Serial.println("连接失败并超时");
        //重新设置并再试一次，或者让它进入深度睡眠状态
        ESP.restart();
        delay(1000);
    }
    Serial.println("connected...^_^");
    yield();
}

void wificonnect()
{ //WIFI密码连接，Web配网请注释
    WiFi.begin(WIFI_SSID, WIFI_PWD);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.println("wificonnect!!!");
    delay(500);
}

void loop()
{
    static int nextShowTime = millis() + SHOW_TIME_PERIOD;
    static int nextshowEnv = millis() + SHOW_ENV_PERIOD;
    static int nextshowOneSay = millis() + SHOW_ONE_SAY_PERIOD;

    if (first)
    {
        //首次加载
        showTime(false);
        delay(200);
        showOneSay();
        delay(200);
        getEnvData();
        delay(200);
        showEnv();
        delay(200);
        first = false;
    }

    if (millis() > nextShowTime)
    {
        showTime(true);
        nextShowTime = millis() + SHOW_TIME_PERIOD;
    }

    if (millis() > nextshowOneSay)
    {
        showOneSay();
        nextshowOneSay = millis() + SHOW_ONE_SAY_PERIOD;
    }
    if (millis() > nextshowEnv)
    {
        getEnvData();
        showEnv();
        nextshowEnv = millis() + SHOW_ENV_PERIOD;
    }
};

void drawString(const char *str)
{
    display.setRotation(1);
    display.setFont(&FreeMonoBold9pt7b);
    display.setTextColor(GxEPD_BLACK);
    int16_t tbx, tby;
    uint16_t tbw, tbh;
    display.getTextBounds(str, 0, 0, &tbx, &tby, &tbw, &tbh);
    uint16_t x = (display.width() - tbw) / 2;
    uint16_t y = (display.height() + tbh) / 2; // y is base line!
    display.firstPage();
    display.setPartialWindow(0, y - 14, display.width(), 20);

    display.fillScreen(bg);
    display.setCursor(x, y);
    display.print(str);
    while (display.nextPage())
        ;
}

void showTime(bool partial)
{
    Serial.println("showTime");
    char buff[16];
    tmElements_t time;
    timeClient.update();
    unsigned long unix_epoch = timeClient.getEpochTime();
    time.Second = second(unix_epoch);
    time.Minute = minute(unix_epoch);
    time.Hour = hour(unix_epoch);
    time.Wday = weekday(unix_epoch);
    time.Day = day(unix_epoch);
    time.Month = month(unix_epoch);
    time.Year = year(unix_epoch);
    sprintf_P(buff, PSTR("%02d%s%02d"), time.Hour, ":", time.Minute);

    u8g2Fonts.setFontMode(1);
    u8g2Fonts.setFontDirection(0);
    u8g2Fonts.setForegroundColor(fg);
    u8g2Fonts.setBackgroundColor(bg);
    u8g2Fonts.setFont(u8g2_font_7Segments_26x42_mn);
    int16_t tw = u8g2Fonts.getUTF8Width(buff);
    int16_t ta = u8g2Fonts.getFontAscent();
    int16_t td = u8g2Fonts.getFontDescent();
    int16_t th = ta - td;
    uint16_t x = (display.width() - tw) / 2;
    uint16_t y = (display.height() - th) / 2 + ta;
    display.firstPage();

    // Serial.print("x:");
    // Serial.println(x);
    // Serial.print("y:");
    // Serial.println(y);
    // Serial.print("tw:");
    // Serial.println(tw);
    // Serial.print("th:");
    // Serial.println(th);

    do
    {
        if (partial)
        {

            display.setPartialWindow(x, display.height() - y, tw, th);
        }
        else
        {
            display.setFullWindow();
        }

        display.fillScreen(bg);
        u8g2Fonts.setCursor(x, y); // start writing at this position
        u8g2Fonts.print(buff);
    } while (display.nextPage());
}

void showOneSay()
{
    if (currentPos >= maxPos)
    {
        currentPos = 0;
    }
    const char* currentStr = HelloWorld[currentPos];
    currentPos++;
    Serial.println("showOneSay");
    display.setRotation(1);
    u8g2Fonts.setFont(u8g2_font_lubI08_te);
    u8g2Fonts.setForegroundColor(fg);
    u8g2Fonts.setBackgroundColor(bg);
    int16_t tw = u8g2Fonts.getUTF8Width(currentStr); // text box width
    int16_t ta = u8g2Fonts.getFontAscent();
    int16_t td = u8g2Fonts.getFontDescent();
    int16_t th = ta - td;
    uint16_t x = (display.width() - tw) / 2;
    uint16_t y = 110;

    // Serial.print("x:");
    // Serial.println(x);
    // Serial.print("y:");
    // Serial.println(y);
    // Serial.print("tw:");
    // Serial.println(tw);
    // Serial.print("th:");
    // Serial.println(th);
    // //display.setFullWindow();

    display.setPartialWindow(0, 95, display.width(), 30);

    display.firstPage();

    do
    {
        display.fillScreen(bg);
        u8g2Fonts.setCursor(x, y); // start writing at this position
        u8g2Fonts.print(currentStr);

    } while (display.nextPage());
}

void getEnvData()
{

    Serial.println("getEnvData");
    Serial.println("Sample DHT22...");

    // read without samples.
    // @remark We use read2 to get a float data, such as 10.1*C
    //    if user doesn't care about the accurate data, use read to get a byte data, such as 10*C.

    int err = SimpleDHTErrSuccess;
    if ((err = dht22.read2(&temperature, &humidity, NULL)) != SimpleDHTErrSuccess)
    {
        Serial.print("Read DHT22 failed, err=");
        Serial.print(SimpleDHTErrCode(err));
        Serial.print(",");
        Serial.println(SimpleDHTErrDuration(err));
        //delay(2000);
        //return;
    }
    else
    {
        Serial.print("Sample OK: ");
        Serial.print((float)temperature);
        Serial.print(" *C, ");
        Serial.print((float)humidity);
        Serial.println(" RH%");
    }

    pms.read();
    if (pms)
    {

        pm10 = pms.pm01;
        pm100 = pms.pm10;
        pm25 = pms.pm25;

        Serial.printf("PM1.0 %2d, PM2.5 %2d, PM10 %2d [ug/m3]\n",
                      pm10, pm25, pm100);
    }
    else
    {

        switch (pms.status)
        {
        case pms.OK:
            Serial.println("OK");
            break;
        case pms.ERROR_TIMEOUT:
            Serial.println(F(PMS_ERROR_TIMEOUT));
            break;
        case pms.ERROR_MSG_UNKNOWN:
            Serial.println(F(PMS_ERROR_MSG_UNKNOWN));
            break;
        case pms.ERROR_MSG_HEADER:
            Serial.println(F(PMS_ERROR_MSG_HEADER));
            break;
        case pms.ERROR_MSG_BODY:
            Serial.println(F(PMS_ERROR_MSG_BODY));
            break;
        case pms.ERROR_MSG_START:
            Serial.println(F(PMS_ERROR_MSG_START));
            break;
        case pms.ERROR_MSG_LENGTH:
            Serial.println(F(PMS_ERROR_MSG_LENGTH));
            break;
        case pms.ERROR_MSG_CKSUM:
            Serial.println(F(PMS_ERROR_MSG_CKSUM));
            break;
        case pms.ERROR_PMS_TYPE:
            Serial.println(F(PMS_ERROR_PMS_TYPE));
            break;
        default:
            Serial.println(F("Unknown error"));
            break;
        }
    }
}

void showEnv()
{
    Serial.println("showEnv...");

    display.setRotation(1);
    u8g2Fonts.setFont(u8g2_font_8x13_tf);
    u8g2Fonts.setForegroundColor(fg);
    u8g2Fonts.setBackgroundColor(bg);
    display.firstPage();
    display.setPartialWindow(0, 0, display.width(), 20);

    char buff[64];
    sprintf_P(buff, PSTR("Hum:%.2f%s  Temp:%.2f.C  Pm25:%d"), humidity, "%", temperature, pm25);
    do
    {
        display.fillScreen(bg);

        u8g2Fonts.setCursor(10, 15);
        u8g2Fonts.print(buff);

    } while (display.nextPage());
}