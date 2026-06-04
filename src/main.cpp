/*
  WISADEV — Claude Code Monitor v3.0
  =====================================
  Hardware: ESP32 DevKit + OLED SSD1306 0.91" (128x32)
  Communication: BLE UART (Nordic UART Service)
  Mac ส่งข้อมูลผ่าน Bluetooth → แสดงบน OLED

  Wiring:
  OLED VCC → 3.3V  |  OLED GND → GND
  OLED SCL → GPIO 22  |  OLED SDA → GPIO 21
*/

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Fonts/TomThumb.h>
#include <Adafruit_SSD1306.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ArduinoJson.h>

// ─── Display ───
#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT   32
#define OLED_RESET      -1
#define SCREEN_ADDRESS  0x3C
#define SDA_PIN         21
#define SCL_PIN         22

// ─── BLE Nordic UART Service ───
#define NUS_SERVICE_UUID  "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define NUS_RX_UUID       "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"  // Mac → ESP32
#define NUS_TX_UUID       "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"  // ESP32 → Mac

// ─── Objects ───
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
BLEServer*         pServer          = nullptr;
BLECharacteristic* pTxCharacteristic = nullptr;

// ─── State ───
bool          bleConnected     = false;
bool          dataLoaded       = false;
int           sesPct           = 0;
String        sesReset         = "--";
int           allPct           = 0;
String        allReset         = "--";
String        lastUpdate       = "--:--";

// Software clock
unsigned long clockSyncMillis  = 0;
int           clockHour        = 0;
int           clockMin         = 0;

unsigned long lastBlink = 0;
bool          blinkState = false;

// ─── BLE Callbacks ───
class ServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* s) override {
        bleConnected = true;
        Serial.println("BLE Connected");
    }
    void onDisconnect(BLEServer* s) override {
        bleConnected = false;
        Serial.println("BLE Disconnected — restarting advertising");
        s->startAdvertising();
    }
};

class RxCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* c) override {
        String value = c->getValue().c_str();
        Serial.println("Received: " + value);

        JsonDocument doc;
        if (!deserializeJson(doc, value)) {
            sesPct     = doc["ses"]  | 0;
            sesReset   = doc["sr"].as<String>();
            allPct     = doc["all"]  | 0;
            allReset   = doc["ar"].as<String>();
            lastUpdate = doc["time"].as<String>();

            // sync software clock
            String t   = lastUpdate;
            clockHour  = t.substring(0, 2).toInt();
            clockMin   = t.substring(3, 5).toInt();
            clockSyncMillis = millis();
            dataLoaded = true;
        }
    }
};

// ─── Setup ───
void setup() {
    Serial.begin(115200);
    Serial.println("\n=== WISADEV Claude Monitor v3 ===");

    Wire.begin(SDA_PIN, SCL_PIN);
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println("SSD1306 failed!");
        for (;;);
    }
    display.setRotation(2);  // หมุนจอ 180 องศา (แก้จอกลับหัว)

    // Splash screen
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(2);
    display.setCursor(22, 0);
    display.print("WISADEV");
    display.setTextSize(1);
    display.setCursor(8, 22);
    display.print("Claude Monitor v3.0");
    display.display();
    delay(2000);

    // BLE init
    BLEDevice::init("WISADEV");
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    BLEService* pService = pServer->createService(NUS_SERVICE_UUID);

    pTxCharacteristic = pService->createCharacteristic(
        NUS_TX_UUID,
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pTxCharacteristic->addDescriptor(new BLE2902());

    BLECharacteristic* pRxChar = pService->createCharacteristic(
        NUS_RX_UUID,
        BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
    );
    pRxChar->setCallbacks(new RxCallbacks());

    pService->start();

    BLEAdvertising* pAdv = BLEDevice::getAdvertising();
    pAdv->addServiceUUID(NUS_SERVICE_UUID);
    pAdv->setScanResponse(true);
    BLEDevice::startAdvertising();

    Serial.println("BLE advertising as 'WISADEV'");
}

// ─── Loop ───
void loop() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    if (dataLoaded) {
        unsigned long elapsed = (millis() - clockSyncMillis) / 1000;
        int totalMins = clockHour * 60 + clockMin + (int)(elapsed / 60);
        int curH = (totalMins / 60) % 24;
        int curM = totalMins % 60;
        char nowBuf[6];
        snprintf(nowBuf, sizeof(nowBuf), "%02d:%02d", curH, curM);

        const int COLS = SCREEN_WIDTH / 6;  // 21 chars per row

        // ── Row 0: 19:23 (left)   U19:20 (right) ──
        char udStr[10];
        snprintf(udStr, sizeof(udStr), "U%s", lastUpdate.c_str());
        display.setCursor(0, 0);
        display.print(nowBuf);
        display.setCursor(SCREEN_WIDTH - (int)strlen(udStr) * 6, 0);
        display.print(udStr);

        // ── Row 1: S:28% ||        | 1h34m  (each | = 10%) ──
        char sesLabel[8];
        snprintf(sesLabel, sizeof(sesLabel), "S:%d%% ", sesPct);
        int sesFill = sesPct / 10;

        display.setTextWrap(false);
        display.setCursor(0, 12);
        display.print(sesLabel);
        for (int i = 0; i < 10; i++)
            display.print(i < sesFill ? '|' : '.');
        display.print(sesReset);

        // ── Row 2: A:64% ||||||....Wed8AM  (each | = 10%) ──
        char allLabel[8];
        snprintf(allLabel, sizeof(allLabel), "A:%d%% ", allPct);
        int allFill = allPct / 10;

        display.setCursor(0, 23);
        display.print(allLabel);
        for (int i = 0; i < 10; i++)
            display.print(i < allFill ? '|' : '.');
        display.print(allReset);

        // BLE dot (bottom-right)
        if (bleConnected) display.fillCircle(126, 30, 1, SSD1306_WHITE);
        else              display.drawCircle(126, 30, 1, SSD1306_WHITE);

    } else {
        if (millis() - lastBlink > 600) {
            blinkState = !blinkState;
            lastBlink  = millis();
        }
        display.setCursor(0, 0);
        display.print("WISADEV");
        display.setCursor(86, 0);
        display.print(blinkState ? "* BLE" : "  BLE");
        display.drawLine(0, 9, SCREEN_WIDTH, 9, SSD1306_WHITE);
        display.setCursor(10, 18);
        display.print(bleConnected ? "Waiting data..." : "Waiting for Mac...");
    }

    display.display();
    delay(100);
}
