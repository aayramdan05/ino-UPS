#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>

// Fungsi untuk menghubungkan ke MQTT
void connectToMQTT();
void mqttCallback(char* topic, byte* message, unsigned int length); // Deklarasi fungsi


const int batteryModePin = 8; 
const int lowbatteryModePin = 7;
const int relayPin = 2;

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; // Ganti dengan MAC address yang sesuai
// Tentukan alamat IP, subnet mask, dan gateway
IPAddress ip(0,0,0,0);        // Ganti dengan alamat IP yang diinginkan
// IPAddress myDns(192, 168, 56, 14);      // Ganti dengan DNS server jika perlu
// IPAddress gateway(10, 92, 160, 1);    // Ganti dengan gateway router
// IPAddress subnet(255, 255, 255, 0);   // Ganti dengan subnet mask

const char* mqttServer = "10.92.49.15"; // Alamat IP broker MQTT
const int mqttPort = 1883; // Port broker MQTT

String ipSwitch = "10.66.90.30";

EthernetClient ethClient; // Objek untuk Ethernet
PubSubClient mqttClient(ethClient); // Objek untuk MQTT

void setup() {
  Serial.begin(9600);
  Serial.println("Arduino running"); // Mulai komunikasi serial
  pinMode(batteryModePin, INPUT_PULLUP); // Set pin sebagai input dengan pull-up
  pinMode(lowbatteryModePin, INPUT_PULLUP);
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);
  // Mulai Ethernet
  delay(2000);
  Ethernet.begin(mac);
  
  // Cek hardware Ethernet
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield was not found. Sorry, can't run without hardware. :(");
    while (true) {
      delay(1); // Jika tidak ada hardware, hentikan program
    }
  }
  
  // Cek status link Ethernet
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable is not connected.");
  }

  // // Tunggu hingga IP address valid didapat
  // while (Ethernet.localIP() == IPAddress(0, 0, 0, 0)) {
  //   Serial.println("Waiting for DHCP...");
  //   delay(1000);
  // }

  // // Tampilkan IP address yang didapat
  // Serial.print("Assigned IP address: ");
  // Serial.println(Ethernet.localIP());
  
  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(mqttCallback);
  // connectToMQTT();
}

void loop() {
  if (Ethernet.localIP() == IPAddress(0, 0, 0, 0)) {
        // Baca status pin
    int batteryModeState = digitalRead(batteryModePin);
    int lowbatteryModeState = digitalRead(lowbatteryModePin);

    String batteryModeStatus = (batteryModeState == LOW) ? "ON" : "OFF";
    String lowBatteryModeStatus = (lowbatteryModeState == LOW) ? "ON" : "OFF";

    Serial.print("Battery Mode : ");
    Serial.println(batteryModeStatus);
    Serial.print("Low Battery Mode : ");
    Serial.println(lowBatteryModeStatus);

    Serial.println("Waiting for DHCP...");
    delay(1000); // Tunggu 1 detik sebelum coba lagi
        // Restart Ethernet
    Ethernet.begin(mac);
  } else {
    Serial.print("Assigned IP address: ");
    Serial.println(Ethernet.localIP());
    // Kode utama Anda di sini
    if (!mqttClient.connected()) {
      connectToMQTT(); // Hubungkan kembali jika tidak terhubung
    }
    
    mqttClient.loop(); // Perbarui status MQTT
    
    // Baca status pin
    int batteryModeState = digitalRead(batteryModePin);
    int lowbatteryModeState = digitalRead(lowbatteryModePin);

    // if (lowbatteryModeState == HIGH && batteryModeState == HIGH ) { // Jika low battery mode aktif
    //       digitalWrite(relayPin, LOW); // Nyalakan relay (aktifkan relay jika menggunakan relay aktif low)
    //       Serial.println("Relay ON (Low Battery Mode ON)");
    // } else {
    //       digitalWrite(relayPin, HIGH); // Matikan relay
    //       Serial.println("Relay OFF (Low Battery Mode OFF)");
    // }
    
    // Construct IP address string inline
    String ipIOT = String(Ethernet.localIP()[0]) + "." +
                   String(Ethernet.localIP()[1]) + "." +
                   String(Ethernet.localIP()[2]) + "." +
                   String(Ethernet.localIP()[3]);

    //MAC ADDRESS
    String macString = "";

    for (int i = 0; i < 6; i++) {
        if (i > 0) {
            macString += ":";  // Add colon separator
        }
        macString += String(mac[i], HEX);  // Convert byte to hex string
    }

    // Ensure the string is in uppercase
    macString.toUpperCase();

    String batteryModeStatus = (batteryModeState == LOW) ? "ON" : "OFF";
    String lowBatteryModeStatus = (lowbatteryModeState == LOW) ? "ON" : "OFF";
    String message = "On Battery Mode: " + batteryModeStatus +
    ", Low Battery Mode: " + lowBatteryModeStatus + 
    ", IP IOT: " + ipIOT +
    ", MAC IOT: " + String(macString) + 
    ", IP Switch: " + String(ipSwitch);
    
    // Publish status ke topik MQTT
    if (!mqttClient.publish("ups/status", message.c_str())) {
      Serial.println("Failed to publish ups/status");
    }

    Serial.print("Battery Mode : ");
    Serial.println(batteryModeStatus);
    Serial.print("Low Battery Mode : ");
    Serial.println(lowBatteryModeStatus);

    if (lowBatteryModeStatus == "ON") {
      Serial.println("Proses reset switch mulai berjalan ...");
       // Countdown 5 menit
      for (int i = 3; i > 0; i--) {
        Serial.print("Waktu tersisa: ");
        Serial.print(i);
        Serial.println(" menit");
        delay(60000); // Delay 1 menit (60000 ms)
      }
      // Perbarui status setelah setiap iterasi
      batteryModeState = digitalRead(batteryModePin);
      batteryModeStatus = (batteryModeState == LOW) ? "ON" : "OFF";

      // Jika Anda juga ingin memeriksa lowBatteryMode, Anda bisa menambahkannya di sini
      lowbatteryModeState = digitalRead(lowbatteryModePin);
      lowBatteryModeStatus = (lowbatteryModeState == LOW) ? "ON" : "OFF";

      Serial.print("Mode Battery Mode Sekarang : ");
      Serial.println(batteryModeStatus);
      Serial.print("Mode Low Battery Mode Sekarang : ");
      Serial.println(lowBatteryModeStatus);

      if (batteryModeStatus == "OFF" && lowBatteryModeStatus == "OFF")
      {
        Serial.println("Switch reset power");
        digitalWrite(relayPin, HIGH);
        delay(200);
        digitalWrite(relayPin, LOW);
        delay(200);
        Serial.print("Kondisi relay");
        Serial.println(relayPin);
      }
        
    }

    delay(3000); // Delay 3 detik sebelum publish lagi
  }
}

void connectToMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Connecting to MQTT...");
    if (mqttClient.connect("ArduinoClient")) {
      Serial.println("connected");
      mqttClient.subscribe("ups/status"); // Subscribe ke topik jika perlu
    } else {
      Serial.print("failed, rc=");
      Serial.println(mqttClient.state());
      delay(2000);
    }
  }
}

void mqttCallback(char* topic, byte* message, unsigned int length) {
  // Convert the message to a string
  String msg;
  for (int i = 0; i < length; i++) {
    msg += (char)message[i];
  }

  // Print topic and message to the serial monitor
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(msg);

  // Additional handling for specific topics
  if (String(topic) == "ups/status") {
    Serial.print("Received UPS status: ");
    Serial.println(msg);
  }
}


