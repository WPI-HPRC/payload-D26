#include <Wire.h>

#define SLAVE_ADDR 0x08 // Define the slave address
#define MAX_IMAGE_SIZE 1024 // Adjust based on memory

byte imageBuffer[MAX_IMAGE_SIZE];
int bufferIndex = 0;

void setup() {
  Serial.begin(115200);
  Wire.begin(SLAVE_ADDR); // Initialize I2C as slave
  Wire.onReceive(receiveEvent); // Register receive callback
  Serial.println("I2C Receiver Initialized");
}

void loop() {
  // Main loop can process data when buffer is full
  if (bufferIndex >= MAX_IMAGE_SIZE) {
    // Process imageBuffer here
    Serial.println("Image Received");
    bufferIndex = 0; // Reset for next image
  }
}

// Callback function triggered when data is received
void receiveEvent(int howMany) {
  while (Wire.available()) {
    byte c = Wire.read();
    if (bufferIndex < MAX_IMAGE_SIZE) {
      imageBuffer[bufferIndex++] = c;
    }
  }
}