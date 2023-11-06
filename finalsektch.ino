
#include <LiquidCrystal.h>
#include <WiFi.h>
const int rs=12, en=11, d4=5, d5=3, d6=4, d7=2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

float pulse = 0;
float temp = 0;

// Variables
int pulsePin = A0; // Pulse Sensor purple wire connected to analog pin 0
int blinkPin = 7; // pin to blink LED at each beat
int fadePin = 13; // pin that will fade to your heartbeat
int fadeRate = 0; // used to fade LED on with PWM on fadePin

// Volatile Variables, used in the interrupt service routine
volatile int BPM; // int that holds raw Analog in 0, updated every 2ms
volatile int Signal; // holds the incoming raw data
volatile int IBI = 600; // int that holds the time interval between beats; must be seeded
volatile boolean Pulse = false; // "True" when a heartbeat is detected, "False" when not a "live beat"
volatile boolean QS = false; // becomes true when Arduino finds a beat

// Serial Visual Output flag
static boolean serialVisual = true; // Set to 'false' by default. Set to 'true' to see Arduino Serial Monitor ASCII Visual Pulse
volatile int rate[10]; // array to hold the last ten IBI values
volatile unsigned long sampleCounter = 0; // used to determine pulse timing
volatile unsigned long lastBeatTime = 0; // used to find IBI
volatile int P = 512; // used to find the peak in the pulse wave, seeded
volatile int T = 512; // used to find the trough in the pulse wave, seeded
volatile int thresh = 525; // used to find the instant moment of the heartbeat, seeded
volatile int amp = 100; // used to hold the amplitude of the pulse waveform, seeded
volatile boolean firstBeat = true; // used to seed the rate array so we start up with a reasonable BPM
volatile boolean secondBeat = false; // used to seed the rate array so we start up with a reasonable BPM

char ssid[] = "Shubham";  // Enter your WiFi Network's SSID
char pass[] = "1234567890";  // Enter your WiFi Network's Password
String apiKey = "YourThingSpeakAPIKey";
const char* server = "api.thingspeak.com";
//const int channelID = 2294509; // Enter your ThingSpeak channel ID

void setup() {
  lcd.begin(16, 2);
  pinMode(blinkPin, OUTPUT); // pin that will blink to your heartbeat
  pinMode(fadePin, OUTPUT); // pin that will fade to your heartbeat
  Serial.begin(9600); // we agree to talk fast!
  interruptSetup(); // sets up to read Pulse Sensor signal every 2ms

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(" Patient Health");
  lcd.setCursor(0, 1);
  lcd.print(" Monitoring ");
  delay(4000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Initializing....");
  delay(5000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Getting Data....");

  // Connect to Wi-Fi
  connectToWiFi();
}

void loop() {
  serialOutput();
  if (QS == true) // A Heartbeat Was Found
  {
    // BPM and IBI have been Determined
    // Quantified Self "QS" true when Arduino finds a heartbeat
    fadeRate = 255; // Makes the LED Fade Effect Happen, set 'fadeRate' Variable to 255 to fade LED with pulse
    serialOutputWhenBeatHappens(); // A Beat Happened, Output that to the serial.
    sendDataToThingSpeak(); // Send data to ThingSpeak
    QS = false; // reset the Quantified Self flag for the next time
  }
  ledFadeToBeat(); // Makes the LED Fade Effect Happen
  delay(20); // take a break
  read_temp();
}

void connectToWiFi() {
  // Connect to Wi-Fi
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}

void sendDataToThingSpeak() {
  WiFiClient client;
  if (client.connect(server, 80)) {
    String postStr = apiKey;
    postStr += "&field1=" + String(BPM);
    postStr += "&field2=" + String(temp);
    postStr += "\r\n\r\n";

    client.print("POST /update HTTP/1.1\n");
    client.print("Host: " + String(server) + "\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + apiKey + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: " + String(postStr.length()) + "\n\n");
    client.print(postStr);

    Serial.println("Data sent to ThingSpeak");

    client.stop();
  } else {
    Serial.println("Connection to ThingSpeak failed");
  }
}

void ledFadeToBeat() {
  fadeRate -= 15; // set LED fade value
  fadeRate = constrain(fadeRate, 0, 255); // keep LED fade value from going into negative numbers
  analogWrite(fadePin, fadeRate); // fade LED
}

void interruptSetup() {
  // Initializes Timer2 to throw an interrupt every 2ms.
  TCCR2A = 0x02; // DISABLE PWM ON DIGITAL PINS 3 AND 11, AND GO INTO CTC MODE
  TCCR2B = 0x06; // DON'T FORCE COMPARE, 256 PRESCALER
  OCR2A = 0x7C; // SET THE TOP OF THE COUNT TO 124 FOR 500Hz SAMPLE RATE
  TIMSK2 = 0x02; // ENABLE INTERRUPT ON MATCH BETWEEN TIMER2 AND OCR2A
  sei(); // MAKE SURE GLOBAL INTERRUPTS ARE ENABLED
}

void serialOutput() {
  // Decide how to output serial.
  if (serialVisual == true) {
    arduinoSerialMonitorVisual('-', Signal); // goes to function that makes Serial Monitor Visualizer
  }
}

void serialOutputWhenBeatHappens() {
  if (serialVisual == true) // Code to make the Serial Monitor Visualizer work
  {
    Serial.print("*** Heart-Beat Happened *** "); // ASCII Art Madness
    Serial.print("BPM: ");
    Serial.println(BPM);
  }
}

void arduinoSerialMonitorVisual(char symbol, int data) {
  const int sensorMin = 0; // Sensor minimum, discovered through experiment
  const int sensorMax = 1024; // Sensor maximum, discovered through experiment
  int sensorReading = data; // Map the sensor range to a range of 12 options:
  int range = map(sensorReading, sensorMin, sensorMax, 0, 11);
  // Do something different depending on the range value:
  switch (range) {
    case 0:
      Serial.println(""); /////ASCII Art Madness
      break;
    case 1:
      Serial.println("---");
      break;
    case 2:
      Serial.println("------");
      break;
    case 3:
      Serial.println("---------");
      break;
    case 4:
      Serial.println("------------");
      break;
    case 5:
      Serial.println("--------------|-");
      break;
    case 6:
      Serial.println("--------------|---");
      break;
    case 7:
      Serial.println("--------------|-------");
      break;
    case 8:
      Serial.println("--------------|----------");
      break;
    case 9:
      Serial.println("--------------|----------------");
      break;
    case 10:
      Serial.println("--------------|-------------------");
      break;
    case 11:
      Serial.println("--------------|-----------------------");
      break;
  }
}

void sendDataToSerial(char symbol, int data) {
  Serial.print(symbol);
  Serial.println(data);
}

ISR(TIMER2_COMPA_vect) // Triggered when Timer2 counts to 124
{
  cli(); // Disable interrupts while we do this
  Signal = analogRead(pulsePin); // Read the Pulse Sensor
  sampleCounter += 2; // Keep track of the time in ms with this variable
  int N = sampleCounter - lastBeatTime; // Monitor the time since the last beat to avoid noise
  // Find the peak and trough of the pulse wave

  if (Signal < thresh && N > (IBI / 5) * 3) // Avoid dichrotic noise by waiting 3/5 of the last IBI
  {
    if (Signal < T) // T is the trough
    {
      T = Signal; // Keep track of the lowest point in the pulse wave
    }
  }
  if (Signal > thresh && Signal > P) { // Thresh condition helps avoid noise
    P = Signal; // P is the peak
  } // Keep track of the highest point in the pulse wave
  // NOW IT'S TIME TO LOOK FOR THE HEART BEAT
  // Signal surges up in value every time there is a pulse
  if (N > 250) { // Avoid high-frequency noise
    if ((Signal > thresh) && (Pulse == false) && (N > (IBI / 5) * 3)) {
      Pulse = true; // Set the Pulse flag when we think there is a pulse
      digitalWrite(blinkPin, HIGH); // Turn on pin 13 LED
      IBI = sampleCounter - lastBeatTime; // Measure time between beats in ms
      lastBeatTime = sampleCounter; // Keep track of time for the next pulse

      if (secondBeat) { // If this is the second beat, if secondBeat == TRUE
        secondBeat = false; // Clear secondBeat flag
        for (int i = 0; i <= 9; i++) { // Seed the running total to get a reasonable BPM at startup
          rate[i] = IBI;
        }
      }
      if (firstBeat) // If it's the first time we found a beat, if firstBeat == TRUE
      {
        firstBeat = false; // Clear the firstBeat flag
        secondBeat = true; // Set the second beat flag
        sei(); // Enable interrupts again
        return; // IBI value is unreliable, so discard it
      }
      // Keep a running total of the last 10 IBI values
      word runningTotal = 0; // Clear the runningTotal variable
      for (int i = 0; i <= 8; i++) { // Shift data in the rate array
        rate[i] = rate[i + 1]; // Drop the oldest IBI value
        runningTotal += rate[i]; // Add up the 9 oldest IBI values
      }
      rate[9] = IBI; // Add the latest IBI to the rate array
      runningTotal += rate[9]; // Add the latest IBI to runningTotal
      runningTotal /= 10; // Average the last 10 IBI values
      BPM = 60000 / runningTotal; // How many beats can fit into a minute? That's BPM!
      QS = true; // Set Quantified Self flag
      pulse = BPM;
    }
  }
  if (Signal < thresh && Pulse == true) { // When the values are going down, the beat is over
    digitalWrite(blinkPin, LOW); // Turn off pin 13 LED
    Pulse = false; // Reset the Pulse flag so we can do it again
    amp = P - T; // Get the amplitude of the pulse wave
    thresh = amp / 2 + T; // Set thresh at 50% of the amplitude
    P = thresh; // Reset these for the next time
    T = thresh;
  }
  if (N > 2500) { // If 2.5 seconds go by without a beat
    thresh = 512; // Set thresh default
    P = 512; // Set P default
    T = 512; // Set T default
    lastBeatTime = sampleCounter; // Bring the lastBeatTime up to date
    firstBeat = true; // Set these to avoid noise
    secondBeat = false; // When we get the heartbeat back
  }
  sei(); // Enable interrupts when you're done!
}

void read_temp()
{
int temp_val = analogRead(A1);
float mv = (temp_val/1024.0)*5000;
float cel = mv/10;
temp = (cel*9)/5 + 32;
Serial.print("Temperature:");
Serial.println(temp);
lcd.clear();
lcd.setCursor(0,0);
lcd.print("BPM :");
lcd.setCursor(7,0);
lcd.print(BPM);
lcd.setCursor(0,1);
lcd.print("Temp.:");
lcd.setCursor(7,1);
lcd.print(temp);
lcd.setCursor(13,1);
lcd.print("F");
delay(1500);
}
