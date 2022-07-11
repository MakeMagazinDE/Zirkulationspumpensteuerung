/*
 * Softwarestand 12.12.2021
 * Steuerung Umwaelzpumpe der Warmwasserheizung, 
 * wenn ca. 50-100ml heißes Wasser laeuft, ergibt es eine kleine Temperaturaenderung direkt am Anschlussrohr des
 * Heisswasserkessels. Die Umwaelzpumpe wird darauf hin eingeschaltet. Zwischen den Einschaltvorgängen 
 * der Pumpe muss eine Pause von mindestens 36 Minuten liegen. Wird über 24 h keine Anforderung registriert, 
 * wird wegen der thermischen Desinfektion ein Pumpenlauf gestartet.
*/
#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 5 //D3: Dallas Tempsensor 18B20
#define pumpenpin 16 // SSR Anschluss um Pumpe zu steuern
#define TEMPERATURE_PRECISION 12   
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress Thermometer;
//************************************************************************************************************
void setup() {
  Serial.begin(115200);
  pinMode(pumpenpin, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  sensors.begin();
  Serial.print("\nVersion vom 12.12.2021, \n\nLocating devices..."); Serial.print("Found "); 
  Serial.print(sensors.getDeviceCount(), DEC); Serial.println(" devices.");
  if (!sensors.getAddress(Thermometer, 0)) Serial.println("Unable to find address for Device 0"); 
  Serial.print("Device 0 Address: "); 
  for (uint8_t i = 0; i < 8; i++) {
    if (Thermometer[i] < 16) Serial.print("0"); Serial.print(Thermometer[i], HEX);//zero pad the address if necessary
  }
  Serial.println("");
  sensors.setResolution(Thermometer, TEMPERATURE_PRECISION); //hohe Genauigkeit
}
//************************************************************************************************************
void loop() {
  static float t[] = {0.0,  0.0,  0.0,  0.0,  0.0}; //letzten 5 Temepraturwerte speichern
  float temperatur_delta = 0.0;
  static unsigned long timer_24h = 0,timer_1Sekunde = 0, ausgabecnt = 0; //ausgabecount muss bis 86400 gehen, soviel Sekunden hat ein Tag
  static int cnt = 0;
  sensors.requestTemperatures(); // Send the command to get temperatures
  float temperatur = sensors.getTempCByIndex(0);
  Serial.print(++ausgabecnt); 
  Serial.print(" Counter, Software vom 09.11.2021, Temperatur betraegt: "); Serial.println(temperatur); 
  if(++cnt >= 5) cnt = 0;
  t[cnt] = temperatur;
  int cnt_alt = (cnt + 6) % 5;            // Es wird der Wert von vor 5 Messungen aus dem Ringpuffer ermittelt
  temperatur_delta = t[cnt]-t[cnt_alt]; // Temperaturdifferenz der letzten 5 Messzyklen bzw. Sekunden
  if(temperatur_delta >= 0.12) {        // kleinste Temeraturaenderung ist 0,06°C, 
    if(pumpe(true) == true) {           // wenn binnen 5 Sekunden > = 0.12°C gestiegen ist, Pumpe an! 
      ausgabecnt = 0;
      timer_24h = millis();             // 24h-Timer neu starten
    } 
  }
  else pumpe(false);
  if (millis() - timer_24h > 24*60*60*1000) { //1x am Tag soll Pumpe laufen wegen thermische Desinfektion (also vor allem im Urlaub)
    timer_24h = millis();
    if(pumpe(true) == true) ausgabecnt = 0;
  }     
  while((millis()) - timer_1Sekunde < 1000) yield();
  timer_1Sekunde = millis();
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN)); //eingebaute blaue LED blinkt im Sekundenrythmus, d.h. Programm laeuft
}
//*********************************************************************
boolean pumpe(boolean pumpe_soll_laufen) {
  static unsigned long timer_pumpenlaufzeit = 0, timer_pause = 0; //timer_pumpenlaufzeit: steuert Laufzeit der Pumpe, timer_pause steuert die Größe der Pause zwischen den Laufzeiten
  static boolean pumpe_laeuft = false;
  if(pumpe_soll_laufen == true && pumpe_laeuft == false && (millis() - timer_pause) > 36*60*1000) {        //Pumpe soll nur alle 30 Minuten 
    pumpe_laeuft = true;                                                                                   //laufen (36Min-6 Min Laufzeit = 30Min)
    digitalWrite(pumpenpin, HIGH);
    Serial.println("Pumpe ein");
    timer_pumpenlaufzeit = millis();
    timer_pause = millis();
    return(true);
  }
  if(pumpe_soll_laufen == false && pumpe_laeuft == true && (millis() - timer_pumpenlaufzeit) > 6*60*1000) { //Pumpe laeuft für 6 Minuten
    pumpe_laeuft = false;
    digitalWrite(pumpenpin, LOW);
    Serial.println("Pumpe aus");
  }
  return(false);
}
//*****************************************************************************************************

