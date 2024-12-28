#define BLYNK_TEMPLATE_ID "TMPL6h6-kvOyp"
#define BLYNK_TEMPLATE_NAME "WateringPlant"
#define BLYNK_AUTH_TOKEN "OZQpk48k1Jgu5YCqF-iLWLR8Xg5JzIW7"

/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial

#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <DHT.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ThingSpeak.h>

// Pin Definitions
#define DHTPIN 4
#define DHTTYPE DHT22
#define SOIL_MOISTURE_PIN 34
#define WATER_LEVEL_PIN 35
#define RELAY_PIN 5
#define DHT_LED_PIN 18
#define PUMP_LED_PIN 19
#define pumpPin 5 // Chân thực tế điều khiển máy bơm, thay đổi theo kết nối 

// OLED display width and height
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Define the display address for I2C
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// WiFi Credentials
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// ThingSpeak Settings
const int myChannelNumber = 2561337;
const char* apiKey = "OPC27012AG41Z88J";
const char* server = "api.thingspeak.com";

bool pumpControl = false; // Trạng thái máy bơm
bool manualMode = false;  // Trạng thái chế độ (false: Tự động, true: Thủ công)

// // Nhận giá trị từ Blynk qua Virtual Pin V8 (bật/tắt máy bơm ở chế độ thủ công)
// BLYNK_WRITE(V8) {
//   manualMode = param.asInt(); // 0: Tự động, 1: Thủ công
//   if (manualMode) {
//     Serial.println("Chế độ: Thủ công");
//   } else {
//     Serial.println("Chế độ: Tự động");
//   }
// }

DHT dht(DHTPIN, DHTTYPE);
WiFiClient client;

void setup() {
  Serial.begin(115200);
  dht.begin();

  // Initialize OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.display();
  delay(2000);
  display.clearDisplay();

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW); // Initialize relay to off state

  pinMode(DHT_LED_PIN, OUTPUT);
  digitalWrite(DHT_LED_PIN, LOW); // Turn off DHT LED initially

  pinMode(PUMP_LED_PIN, OUTPUT);
  digitalWrite(PUMP_LED_PIN, LOW); // Initialize the LED to off state

  pinMode(pumpPin, OUTPUT);
  digitalWrite(pumpPin, LOW); // Mặc định tắt máy bơm

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.println("Local IP: " + String(WiFi.localIP()));
  WiFi.mode(WIFI_STA);
  ThingSpeak.begin(client);

  // Blynk setup
  Blynk.config(BLYNK_AUTH_TOKEN);
  Blynk.connect();

}


void loop() {
  Blynk.run();

  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  int soilMoistureValue = analogRead(SOIL_MOISTURE_PIN);
  int waterLevelValue = analogRead(WATER_LEVEL_PIN);

  // Nếu ở chế độ thủ công
  if (manualMode) {
    if (pumpControl) {
      digitalWrite(RELAY_PIN, HIGH); // Bật máy bơm
      digitalWrite(PUMP_LED_PIN, HIGH);
    } else {
      digitalWrite(pumpPin, LOW); // Tắt máy bơm
      digitalWrite(PUMP_LED_PIN, LOW);
    }
  } else { // Chế độ tự động
    const int SOIL_MOISTURE_THRESHOLD = 40;
    int soilMoisturePercentage = map(soilMoistureValue, 0, 4095, 0, 100);

    if (soilMoisturePercentage < SOIL_MOISTURE_THRESHOLD) {
      digitalWrite(pumpPin, HIGH); // Bật máy bơm
      digitalWrite(PUMP_LED_PIN, HIGH);
      Blynk.virtualWrite(V5, 1); // Update trạng thái Blynk
      Serial.println("Soil moisture is low. Pump is turned on.");
    } else {
      digitalWrite(pumpPin, LOW); // Tắt máy bơm
      digitalWrite(PUMP_LED_PIN, LOW);
      Blynk.virtualWrite(V5, 0); // Update trạng thái Blynk
    }

    // Debug trạng thái để kiểm tra
    Serial.print("Manual Mode: ");
    Serial.println(manualMode ? "ON" : "OFF");
    Serial.print("Pump Control: ");
    Serial.println(pumpControl ? "ON" : "OFF");

    delay(1000); // Đợi 1 giây để giảm tần suất log
  }

  // Cập nhật thông tin lên OLED và Blynk
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("Mode: ");
  display.println(manualMode ? "Manual" : "Auto");
  display.print("Soil Moisture: ");
  display.print(map(soilMoistureValue, 0, 4095, 0, 100));
  display.println("%");
  display.print("Water Level: ");
  display.print(map(waterLevelValue, 0, 4095, 0, 100));
  display.println("%");
  display.print("Temp: ");
  display.print(temperature);
  display.println(" C");
  display.print("Humidity: ");
  display.print(humidity);
  display.println(" %");
  display.display();

  Serial.println("Temp: " + String(temperature, 2) + "°C");
  Serial.println("Humidity: " + String(humidity, 1) + "%");
  Serial.println("Soil Moisture: " + String(map(soilMoistureValue, 0, 4095, 0, 100)) + "%");

// Gửi dữ liệu lên Blynk
  Blynk.virtualWrite(V1, temperature);             // Gửi nhiệt độ lên Virtual Pin V1
  Blynk.virtualWrite(V2, humidity);                // Gửi độ ẩm không khí lên Virtual Pin V2
  Blynk.virtualWrite(V3, map(soilMoistureValue, 0, 4095, 0, 100)); // Gửi độ ẩm đất lên Virtual Pin V3
  Blynk.virtualWrite(V4, map(waterLevelValue, 0, 4095, 0, 100));   // Gửi mực nước lên Virtual Pin V4

  // Gửi dữ liệu lên ThingSpeak
  ThingSpeak.setField(1, temperature);
  ThingSpeak.setField(2, humidity);
  ThingSpeak.setField(3, map(soilMoistureValue, 0, 4095, 0, 100));
  ThingSpeak.setField(4, map(waterLevelValue, 0, 4095, 0, 100));
  
  // Viết dữ liệu lên ThingSpeak
  int response = ThingSpeak.writeFields(myChannelNumber, apiKey);

  if (response == 200) {
    Serial.println("Data pushed successfully");
  } else {
    Serial.println("Push error: " + String(response));
  }
  Serial.println("---");

  delay(15000); // Đợi trước khi lặp lại
}

// Blynk: Chọn chế độ hoạt động (V8)
BLYNK_WRITE(V8) {
  manualMode = param.asInt(); // 1: Thủ công, 0: Tự động
  if (manualMode) {
    Serial.println("Chế độ: Thủ công");
  } else {
    Serial.println("Chế độ: Tự động");
    digitalWrite(pumpPin, LOW); // Tắt máy bơm khi trở về chế độ tự động
    Blynk.virtualWrite(V7, 0); // Reset trạng thái nút bơm
  }
}

// Blynk: Điều khiển máy bơm trong chế độ Thủ công (V7)
BLYNK_WRITE(V7) {
  if (manualMode) { // Chỉ cho phép bật/tắt máy bơm nếu ở chế độ Thủ công
    pumpControl = param.asInt(); // Lấy trạng thái nút (1: Bật, 0: Tắt)
    digitalWrite(pumpPin, pumpControl); // Điều khiển chân máy bơm
    digitalWrite(PUMP_LED_PIN, pumpControl); // Cập nhật LED máy bơm
    Serial.print("Máy bơm: ");
    Serial.println(pumpControl ? "Bật" : "Tắt");
  } else {
    Serial.println("Không thể điều khiển máy bơm khi ở chế độ Tự động");
    Blynk.virtualWrite(V7, 0); // Reset nút bơm nếu không ở chế độ Thủ công
  }
}

// Blynk: Điều khiển máy bơm trong chế độ tự động (V5)
BLYNK_WRITE(V5) {
  if (!manualMode) { // Chỉ hoạt động trong chế độ tự động
    int autoPumpState = param.asInt();
    digitalWrite(pumpPin, autoPumpState);
    Serial.print("Máy bơm (Tự động): ");
    Serial.println(autoPumpState ? "Bật" : "Tắt");
  }
}