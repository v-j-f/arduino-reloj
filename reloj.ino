#include <DHT.h>                      // Control del sensor DHT22
#include <RTClib.h>                   // Control del módulo DS3231 (RTC)
#include <LowPower.h>                 // Poner Arduino en modo standby
#include <MD_Parola.h>                // Control de la matriz de LEDs (mostrar texto con efectos)
#include <MD_MAX72xx.h>               // Control de la matriz de LEDs (dependencia de Parola)
#include <SPI.h>                      // Comunicación SPI con la matriz de LEDs

#define NOP __asm__ __volatile__ ("nop\n\t") // Comando nop

/*
 * Configuración de Parola
 */
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW 
#define MAX_DEVICES 4
#define SPEED_TIME  250
#define PAUSE_TIME  2500
#define CLK_PIN     13
#define DATA_PIN    11
#define CS_PIN      10

uint8_t degC[] = { 7, 3, 3, 60, 66, 66, 66, 66 }; // Deg C = ºC

/*
Serial Monitor usa UTF-8.
º character = Hex UTF-8 bytes C2 B0
Serial.print("\xC2\xB0");
Serial.print("C ");
*/

#define DHTTYPE   DHT22            // Tipo de sensor DHT
#define DHTPin    2                // Pin del sensor DHT22
#define PIRPin    3                // Pin del sensor PIR
volatile int pirState = HIGH;      // Estado inicial de PIR
int val = LOW;                     // Lectura inicial del PIRPin

#define MINUTOS       5            // Nº de minutos mostrando la hora 
#define FIRST_SETTING false        // true para conf. la fecha y hora en la primera ejecución.
                                   // fasle después de conf. la fecha y hora tras la primera ejecución.
volatile bool sleeping = false;    // Estado inicial de Arduino 

String daysOfTheWeek[7] = { "Dom", "Lun", "Mar", "Mie", "Jue", "Vie", "Sab" };

RTC_DS3231 rtc;                                               // Declaramos un RTC DS3231
DHT dht(DHTPin, DHTTYPE);                                     // Declaramos un DHT22
MD_Parola lc = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES); // Declaramos una matrix de 4 dispositivos

void setup() {
  //Serial.begin(9600);
  myDelay(1000);
 
  if (! rtc.begin()) {
    //Serial.println(F("RTC no disponible..."));
    while (1);
  }
  //Serial.println(F("RTC disponible..."));
  myDelay(1000);
  
  /*
   * Ponemos en hora, solo la primera vez, luego comentar y volver a cargar.
   */
  if ( FIRST_SETTING == true or rtc.lostPower() ) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    myDelay(1000);
  }

  dht.begin();
  myDelay(1000);

  pinMode(PIRPin, INPUT);
  myDelay(1000);

  lc.begin();
  lc.setIntensity(0);
  lc.addChar('*', degC);
  myDelay(1000);
  
  //Serial.println(F("Ready!"));
  myDelay(1000);
  attachInterrupt(digitalPinToInterrupt(PIRPin), PIRmotionDetected, RISING); 
}

void loop() {
  val = digitalRead(PIRPin);
  if (val == HIGH)
  { 
    if (pirState == LOW)
    {
      display(getTemp(),0);
      display(getHumidity(),1);
      display(getDate(),2);
      unsigned long t_actual = millis(); 
      while(millis() < t_actual + MINUTOS * 60000)
        display(getTime(),3);
    }
  } 
  else
  {
    if (pirState == HIGH or sleeping == false)
    {
      sleeping = true;
      pirState = LOW;
      //lc.displayClear();
      lc.displayShutdown(true);
      LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
    }
  }  
}

void PIRmotionDetected() {
  lc.displayShutdown(false);
  val = HIGH;
  pirState = LOW;
  sleeping = false;
}

void myDelay(unsigned long period) {
  unsigned long t_actual = millis(); 
  while(millis() < t_actual + period)
    NOP;
}

String get2Digits(int number) {
  if (number >= 0 && number < 10)
    return "0" + String(number);
  else
    return String(number);
}

String getHumidity() {
   char H[5];
   float h = dht.readHumidity();
   dtostrf(h, 3, 1, H);
   if (isnan(h))
      return "H: Err";
   else
      return String(H) + " %";
}

String getTemp() {
   char T[5];
   float t = dht.readTemperature();
   dtostrf(t, 3, 1, T);
   if (isnan(t)) 
      return "T: Err";
   else 
      return String(T) + "*";
}

String getDate()
{
   DateTime date = rtc.now();
   return get2Digits(date.day()) + "/" + get2Digits(date.month()) + "/" + date.year() + " (" +  daysOfTheWeek[date.dayOfTheWeek()] + ")";
}

String getTime()
{
   DateTime date = rtc.now();
   return get2Digits(date.hour()) + ":" + get2Digits(date.minute());
}

void display(String text, int mode) {
  unsigned long t_total = 0;
  int str_len = text.length() + 1; // Length (with one extra character for the null terminator)
  char msg[str_len];
  text.toCharArray(msg, str_len);

  switch (mode) {
    case 0:
      lc.displayText(msg, PA_CENTER, SPEED_TIME, PAUSE_TIME, PA_SCROLL_DOWN, PA_SCROLL_UP);
      t_total = PAUSE_TIME + 15 * SPEED_TIME;
      break;
    case 1:
      lc.displayText(msg, PA_CENTER, SPEED_TIME, PAUSE_TIME, PA_SCROLL_UP, PA_SCROLL_DOWN);
      t_total = PAUSE_TIME + 15 * SPEED_TIME;
      break;
    case 2:
      lc.displayScroll(msg, PA_CENTER, PA_SCROLL_LEFT, SPEED_TIME / 2);
      t_total = PAUSE_TIME + 48 * SPEED_TIME;
      break;
    case 3:
      lc.displayText(msg, PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
      t_total = 1000; // 1 segundo
      break;
  }

  unsigned long t_transcurrido=0;
  unsigned long t_inicio=millis();
  while (t_transcurrido < t_total) {
    if (lc.displayAnimate())
      lc.displayReset();
    t_transcurrido = millis() - t_inicio;
  }
}
