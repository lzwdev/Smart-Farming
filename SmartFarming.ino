#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <RTClib.h>
#include <Adafruit_NeoPixel.h>

#define PIN 13
#define NUMPIXELS 24

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

// ===== Sensoren =====
DHT dht(4, DHT11);
int Trigger = 3;
int ECHO = 2;
int Bodenfeuchtigkeitsensor = A0;
int LEDRot = 5;
int LEDGruen = 6;
int Fotowiderstand = A1;
int PumpenPinIN1 = 12;
int PumpenPinIN2 = 11;
int PumpeSpeed = 50;

// ===== RTC =====
RTC_DS3231 rtc;

// ===== Variablen =====
long Abstand = 0;
long NeoR;
long NeoG;
long NeoB;
float Temperatur = 0.0;
int Bodenfeuchtigkeit = 0;
float PflanzentemperaturLimit = 30.0;
int Trocken = 25;
int Helligkeit = 0;
int Tagbeginn = 13;
int Tagende = 14;

// ===== RGB LED =====
int redPin = 7;
int greenPin = 8;
int bluePin = 9;

// ===== LCD =====
LiquidCrystal_I2C lcd(0x27, 16, 4);

// ===== SETUP =====
void setup() {
  pinMode(Trigger, OUTPUT);
  pinMode(ECHO, INPUT);

  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);

  pinMode(LEDGruen, OUTPUT);
  pinMode(LEDRot, OUTPUT);

  pinMode(Fotowiderstand, INPUT);
  pinMode(PumpenPinIN2, OUTPUT);
  pinMode(PumpenPinIN1, OUTPUT);

  Serial.begin(9600);

  dht.begin();
  pixels.begin();
  pixels.show(); // Alle LEDs aus

  lcd.init();
  lcd.backlight();

  // ===== RTC START =====
  if (!rtc.begin()) {
    lcd.setCursor(0, 0);
    lcd.print("RTC ERROR");
    while (1);
  }

  // 👉 EINMAL aktivieren, dann wieder auskommentieren!
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}

// ===== LOOP =====
void loop() {
  AbstandMessung();
  TemperaturMessung();
  BodenfeuchtigkeitMessung();
  AnzeigeLCD();
  RGB_Steuerung();
  Lueftung();
  Giessen();
  Licht();

  delay(1000);
}

// ===== ABSTAND =====
void AbstandMessung() {
  digitalWrite(Trigger, LOW);
  delay(5);
  digitalWrite(Trigger, HIGH);
  delayMicroseconds(10);
  digitalWrite(Trigger, LOW);

  long Zeit = pulseIn(ECHO, HIGH);
  Abstand = (Zeit / 2) * 0.03432;
}

// ===== TEMPERATUR =====
void TemperaturMessung() {
  Temperatur = dht.readTemperature();
}

// ===== FEUCHTE =====
void BodenfeuchtigkeitMessung() {
  Bodenfeuchtigkeit = map(analogRead(Bodenfeuchtigkeitsensor), 0, 650, 0, 100);
}


// ===== RGB =====
void RGB_Steuerung() {
  if (Bodenfeuchtigkeit < 30) {
    setColor(5, 0, 0);
  } else if (Bodenfeuchtigkeit < 75) {
    setColor(5, 4, 0);
  } else {
    setColor(0, 5, 0);
  }
}

// ===== COLOR =====
void setColor(int r, int g, int b) {
  analogWrite(redPin, r);
  analogWrite(greenPin, g);
  analogWrite(bluePin, b);
}

// ===== LÜFTUNG =====
void Lueftung() {
  if (Temperatur > PflanzentemperaturLimit) {
    analogWrite(LEDGruen, Temperatur * 3);
  } else {
    analogWrite(LEDGruen, 0);
  }
}

// ===== GIESSEN =====
void Giessen() {
  if (Bodenfeuchtigkeit < Trocken) {
    analogWrite(PumpenPinIN2, 255 - PumpeSpeed);
    digitalWrite(PumpenPinIN1, HIGH);
  } else {
    digitalWrite(PumpenPinIN1, LOW);
    digitalWrite(PumpenPinIN2, LOW);
  }
}

// ===== LICHT (NeoPixel) =====
void Licht() {
  DateTime now = rtc.now();  // aktuelle Zeit abrufen

  // NeoPixel zwischen 13 Uhr und 5 Uhr morgens (über Mitternacht)
  if (now.hour() >= Tagbeginn || now.hour() < Tagende) {
    NeoPixelAnim();
  } else {
    pixels.clear();
    pixels.show();
  }
}


void NeoPixelAnim() {
  // Zufällige Farbe für vorwärtslauf
  NeoR = random(0, 255);
  NeoG = random(0, 255);
  NeoB = random(0, 255);

  // Alle LEDs vorwärts setzen
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(NeoR, NeoG, NeoB));
    pixels.show();
    delay(100); // kurze Pausew
  }


  // Neue zufällige Farbe für rückwärtslauf
  NeoR = random(0, 255);
  NeoG = random(0, 255);
  NeoB = random(0, 255);

  // Alle LEDs rückwärts setzen
  for (int i = NUMPIXELS - 1; i >= 0; i--) {
    pixels.setPixelColor(i, pixels.Color(NeoR, NeoG, NeoB));
    pixels.show();
    delay(100); // kurze Pause
  }

}

// ===== LCD ANZEIGE =====
void AnzeigeLCD() {
  DateTime now = rtc.now();

  // Zeile 1: Temperatur
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(Temperatur, 1);
  lcd.print(" ");
  lcd.write(byte(223)); // Gradzeichen
  lcd.print("C   ");    // Leerzeichen zum Überschreiben alter Zeichen

  // Zeile 2: Abstand
  lcd.setCursor(0, 1);
  lcd.print("Abstand: ");
  lcd.print(Abstand);
  lcd.print("cm   ");

  // Zeile 3: Bodenfeuchtigkeit
  lcd.setCursor(0, 2);
  lcd.print("Feuchte: ");
  lcd.print(Bodenfeuchtigkeit);
  lcd.print("%   ");

  // Zeile 4: Uhrzeit und Datum
  lcd.setCursor(0, 3);
  if (now.hour() < 10) lcd.print("0");
  lcd.print(now.hour());
  lcd.print(":");
  if (now.minute() < 10) lcd.print("0");
  lcd.print(now.minute());
  lcd.print(":");
  if (now.second() < 10) lcd.print("0");
  lcd.print(now.second());
  lcd.print(" ");

  if (now.day() < 10) lcd.print("0");
  lcd.print(now.day());
  lcd.print(".");
  if (now.month() < 10) lcd.print("0");
  lcd.print(now.month());
  lcd.print(".");
  lcd.print(now.year());
}
