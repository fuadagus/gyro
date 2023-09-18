#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Adafruit_NeoPixel.h>
#include <LiquidCrystal.h>

LiquidCrystal lcd(12, 11, 10, 9, 8, 7);
Adafruit_SSD1306 display(128, 64);

#define LED_PIN   6
#define LED_COUNT 1
#define SWITCHER 2
Adafruit_NeoPixel neoPixel(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

const int mpuAddress = 0x68;

float xByGyro, yByGyro, zByGyro;

const int xOrigin = 64;
const int yOrigin = 32;

const float viewDistance = 150.0;

#define NUM_VERTICES 8
const int cube_vertex[NUM_VERTICES][3] =
{
  { -20, -20,  20 },     // x, y, z
  {  20, -20,  20 },
  {  20,  20,  20 },
  { -20,  20,  20 },
  { -20, -20, -20 },
  {  20, -20, -20 },
  {  20,  20, -20 },
  { -20,  20, -20 }
};

int wireframe[NUM_VERTICES][2];


String formatAngle(float angle)
{
  // Normalize angle to the range [0, 360)
  while (angle >= 360.0*PI/180)
    angle -= 3360.0*PI/180;
  while (angle < 0.0)
    angle += 360.0*PI/180;

  int degrees = int(angle*180.0/PI);
  int minutes = int((angle*180.0/PI - degrees) * 60);
  int seconds = int(((angle*180.0/PI - degrees) * 60 - minutes) * 60);
  return String(degrees) + "° " + minutes + "' " + seconds + "\"";
}



void drawWireframe(void)
{
  display.clearDisplay();

  display.drawLine(wireframe[0][0], wireframe[0][1], wireframe[1][0], wireframe[1][1], SSD1306_WHITE);
  display.drawLine(wireframe[1][0], wireframe[1][1], wireframe[2][0], wireframe[2][1], SSD1306_WHITE);
  display.drawLine(wireframe[2][0], wireframe[2][1], wireframe[3][0], wireframe[3][1], SSD1306_WHITE);
  display.drawLine(wireframe[3][0], wireframe[3][1], wireframe[0][0], wireframe[0][1], SSD1306_WHITE);

  display.drawLine(wireframe[4][0], wireframe[4][1], wireframe[5][0], wireframe[5][1], SSD1306_WHITE);
  display.drawLine(wireframe[5][0], wireframe[5][1], wireframe[6][0], wireframe[6][1], SSD1306_WHITE);
  display.drawLine(wireframe[6][0], wireframe[6][1], wireframe[7][0], wireframe[7][1], SSD1306_WHITE);
  display.drawLine(wireframe[7][0], wireframe[7][1], wireframe[4][0], wireframe[4][1], SSD1306_WHITE);

  display.drawLine(wireframe[0][0], wireframe[0][1], wireframe[4][0], wireframe[4][1], SSD1306_WHITE);
  display.drawLine(wireframe[1][0], wireframe[1][1], wireframe[5][0], wireframe[5][1], SSD1306_WHITE);
  display.drawLine(wireframe[2][0], wireframe[2][1], wireframe[6][0], wireframe[6][1], SSD1306_WHITE);
  display.drawLine(wireframe[3][0], wireframe[3][1], wireframe[7][0], wireframe[7][1], SSD1306_WHITE);

  display.drawLine(wireframe[1][0], wireframe[1][1], wireframe[3][0], wireframe[3][1], SSD1306_WHITE);
  display.drawLine(wireframe[0][0], wireframe[0][1], wireframe[2][0], wireframe[2][1], SSD1306_WHITE);

  display.display();
  
  lcd.setCursor(3, 0);
  lcd.print(formatAngle(xByGyro));
  lcd.setCursor(3, 1);
  lcd.print(formatAngle(yByGyro));
  lcd.setCursor(3, 2);
  lcd.print(formatAngle(zByGyro));
}

void setup()
{
  Serial.begin(115200);

  Wire.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }

  Wire.beginTransmission(mpuAddress);
  Wire.write(0x6B);
  Wire.write(0);
  auto error = Wire.endTransmission();
  if (error != 0)
  {
    Serial.println(F("Error, MPU-6050 not found"));
    for (;;);
  }

  neoPixel.begin();

  lcd.begin(20, 4);
  lcd.print("X: ");
  lcd.setCursor(0, 1);
  lcd.print("Y: ");
  lcd.setCursor(0, 2);
  lcd.print("Z: ");
  
}

void loop()
{
  
  Wire.beginTransmission(mpuAddress);
  Wire.write(0x3B);
  Wire.endTransmission(false);

  int16_t AcX, AcY, AcZ, Tmp, GyX, GyY, GyZ;

  Wire.requestFrom(mpuAddress, 14);
  AcX = Wire.read() << 8 | Wire.read();
  AcY = Wire.read() << 8 | Wire.read();
  AcZ = Wire.read() << 8 | Wire.read();
  Tmp = Wire.read() << 8 | Wire.read();
  GyX = Wire.read() << 8 | Wire.read();
  GyY = Wire.read() << 8 | Wire.read();
  GyZ = Wire.read() << 8 | Wire.read();

  float xByAccel = (float)AcX * 0.0001;
  float yByAccel = (float)AcY * 0.0001;
  float zByAccel = (float)AcZ * 0.0001;

  xByGyro += (float)GyX * 0.00001;
  yByGyro += (float)GyY * 0.00001;
  zByGyro += (float)GyZ * 0.00001;

  float x = xByAccel + xByGyro;
  float y = yByAccel + yByGyro;
  float z = zByAccel + zByGyro;

  if (x < 0.0)
    x += 2.0 * M_PI;
  else if (x > 2.0 * M_PI)
    x -= 2.0 * M_PI;
  if (y < 0.0)
    y += 2.0 * M_PI;
  else if (y > 2.0 * M_PI)
    y -= 2.0 * M_PI;
  if (z < 0.0)
    z += 2.0 * M_PI;
  else if (z > 2.0 * M_PI)
    z -= 2.0 * M_PI;

  for (int i = 0; i < NUM_VERTICES; i++)
  {
    // Rotate Y
    float rotx = cube_vertex[i][2] * sin(y) + cube_vertex[i][0] * cos(y);
    float roty = cube_vertex[i][1];
    float rotz = cube_vertex[i][2] * cos(y) - cube_vertex[i][0] * sin(y);

    // Rotate X
    float rotxx = rotx;
    float rotyy = roty * cos(x) - rotz * sin(x);
    float rotzz = roty * sin(x) + rotz * cos(x);

    // Rotate Z
    float rotxxx = rotxx * cos(z) - rotyy * sin(z);
    float rotyyy = rotxx * sin(z) + rotyy * cos(z);
    float rotzzz = rotzz;

    // Add depth perspective
    rotxxx *= viewDistance / (viewDistance + rotzzz);
    rotyyy *= viewDistance / (viewDistance + rotzzz);

    // Bring to middle of screen
    rotxxx += (float)xOrigin;
    rotyyy += (float)yOrigin;

    // Store new vertices values for wireframe drawing
    wireframe[i][0] = (int)rotxxx;
    wireframe[i][1] = (int)rotyyy;
    wireframe[i][2] = (int)rotzzz;
  }
  
  drawWireframe();
  float Celsius = ((float)Tmp / 340.00) + 36.53;
  float hue = (Celsius + 40.0) / 125.0 * 65535.0;
  uint32_t rgbcolor = neoPixel.ColorHSV((uint16_t)hue);
  neoPixel.setPixelColor(0, rgbcolor);
  neoPixel.show();

  
}

