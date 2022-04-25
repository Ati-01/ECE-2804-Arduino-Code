#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 127 // OLED display width, in pixels
#define SCREEN_HEIGHT 31 // OLED display height, in pixels
// (0,0) on the display is top right corner with pins above the display

#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Initialize constants
const int RedACPin = A0; // Pin of AC Red signal
const int RedDCPin = A1; // Pin of DC Red signal
const int IredACPin = A2; // Pin of AC IR signal
const int IredDCPin = A3; // Pin of DC IR signal
const int PWMPin = 5; // Pin for square wave
const int PWMInputPin = 6; // Pin for measuring square wave
const int sizeArray = 128;  // Size of pixelArray
const int height = SCREEN_HEIGHT; // OLED screen height
const float multiplier = 1; // Multipler making the waveform bigger or smaller
const float samplingFrequency = 41; // Sampling frequency for sampling waveform
const float samplingDelay = 22.2222; // Delay between each sample
const float DCoffset = 1.55;

// Initialize variables
int voltage = 0; // Output of analogRead() for displaying waveform
int pixelArray[sizeArray]; // Array of pixels for the length of the OLED
int displayPix = 0; // y-coordinate for pixel
unsigned long timeStart = 0; // Initial time for a timer
int maximum = -1; // Peak of waveform
int minimum = 32; // Trough of waveform
int t = 0; // Time
float RedAC = 0; // Red AC signal value
float RedDC = 0; // Red DC signal value
float IredAC = 0; // Infrared AC signal value
float IredDC = 0; // Infrared DC signal value
float r = 0; // Calcluated R value
float Spo2 = 0; // Calculated SPO2%
float frequency = 0; // Frequency of waveform

int a = 0;

// Initialize states
enum class States {INCREASING, DECREASING};
States state{States::INCREASING};

void setup()
{
  // Initialize pins
  pinMode(RedACPin, INPUT);
  pinMode(RedDCPin, INPUT);
  pinMode(IredACPin, INPUT);
  pinMode(IredDCPin, INPUT);
  pinMode(PWMPin, OUTPUT);
  pinMode(PWMInputPin, INPUT);

  // Initialize display
  Serial.begin(115200);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println("SSD1306 allocation failed");
    for (;;); // Don't proceed, loop forever
  }
  delay(2000);
  display.clearDisplay();
  display.display();

  analogWrite(PWMPin, 127);
}

void loop()
{
  // Calcluate SPO2%
  while (digitalRead(PWMInputPin) == HIGH)
  {
    // Do nothing
  }
  while (digitalRead(PWMInputPin) == LOW)
  {
    // Do nothing
  }
  RedAC = analogRead(RedACPin);
  RedDC = analogRead(RedDCPin);
  RedAC = RedAC + analogRead(RedACPin);
  RedDC = RedDC + analogRead(RedDCPin);
  RedAC = RedAC + analogRead(RedACPin);
  RedDC = RedDC + analogRead(RedDCPin);
  while (digitalRead(PWMInputPin) == LOW)
  {
    // Do nothing
  }
  while (digitalRead(PWMInputPin) == HIGH)
  {
    // Do nothing
  }
  IredAC = analogRead(IredACPin);
  IredDC = analogRead(IredDCPin);
  IredAC = IredAC + analogRead(IredACPin);
  IredDC = IredDC + analogRead(IredDCPin);
  IredAC = IredAC + analogRead(IredACPin);
  IredDC = IredDC + analogRead(IredDCPin);
  
  RedAC = abs((RedAC - DCoffset) / 3);
  RedDC = RedDC / 3;
  IredAC = abs((IredAC - DCoffset) / 3);
  IredDC = IredDC / 3;
  
  r = ((RedAC / RedDC) / (IredAC / IredDC));
  Spo2 = 110 - 25 * r;
  
  // Display waveform
  display.clearDisplay(); // Clears display to show the next part of the waveform
  for (int i = 0; i < sizeArray; i++)
  {
    timeStart = millis(); // Gets a reference time
    voltage = analogRead(IredACPin); // Reads input voltage from infrared signal
    //Serial.println(voltage);
    displayPix = multiplier * map(voltage, 0, 1023, height, 0); // Maps input voltage to a value corresponding to the height of the display
    display.drawPixel(i, displayPix, WHITE); // Draws pixel
    pixelArray[i] = displayPix; // Put pixel in pixel array
    while (millis() < timeStart + samplingDelay) // Do nothing until amount of time equal to samplingDelay has elapsed
    {
      // Do nothing
    }
  }

  // Find frequency
  // This state machine switches between when the waveform at the right side of the display is increasing or decreasing
  frequency = 0;
  a = 0;
  for (int i = 2; i < sizeArray; i++)
  {
    switch (state)
    {
      case States::INCREASING:
        if (maximum <= pixelArray[i - 1])
        {
          maximum = pixelArray[i - 1];
          t++;
        }
        else
        {
          state = States::DECREASING;
          maximum = -1;
        }
        break;
      case States::DECREASING:
        if (minimum >= pixelArray[i - 1])
        {
          minimum = pixelArray[i - 1];
          t++;
        }
        else
        {
          state = States::INCREASING;
          if (t > 20)
          {
            frequency = frequency + samplingFrequency / t;
            a++;
          }
          t = 0;
          minimum = 32;
        }
        break;
    }
  }

  for (int i = 0; i < sizeArray; i++)
  {
    timeStart = millis(); // Gets a reference time
    voltage = analogRead(IredACPin); // Reads input voltage from infrared signal
    pixelArray[i] = multiplier * map(voltage, 0, 1023, height, 0); // Maps input voltage to a value corresponding to the height of the display
    while (millis() < timeStart + samplingDelay) // Do nothing until amount of time equal to samplingDelay has elapsed
    {
      // Do nothing
    }
  }
  for (int i = 2; i < sizeArray; i++)
  {
    switch (state)
    {
      case States::INCREASING:
        if (maximum <= pixelArray[i - 1])
        {
          maximum = pixelArray[i - 1];
          t++;
        }
        else
        {
          state = States::DECREASING;
          maximum = -1;
        }
        break;
      case States::DECREASING:
        if (minimum >= pixelArray[i - 1])
        {
          minimum = pixelArray[i - 1];
          t++;
        }
        else
        {
          state = States::INCREASING;
          if (t > 20)
          {
            frequency = frequency + samplingFrequency / t;
            a++;
          }
          t = 0;
          minimum = 32;
        }
        break;
    }
  }

  frequency = frequency / a;
  

  // Update display
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print(frequency * 60);
  display.print(" BPM, ");
  display.print(Spo2);
  display.println("%");
  display.display();
}
