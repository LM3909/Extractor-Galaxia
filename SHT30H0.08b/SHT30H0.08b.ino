/*
Arduino Uno con shield lcd de DF.Robot y SHT30 en bus I2C
*/
#include <LiquidCrystal.h>
#include <PString.h>
#include <Wire.h>
#include "Adafruit_SHT31.h"
#include <Adafruit_BMP085.h>

Adafruit_BMP085 bmp;

const char version[16] = " v0.02 05/10/22 ";
 
// Loop lento y loop rápido
int deltaLoop = 1000;
int deltaLoop_millis;
int millis_Ant;
int millis_Now;

bool alive = false; //Indicador en lcd de loop corriendo

//SHT30//
bool heaterStatus = false;
/*
Según el estado de heaterstatus: true habilita y false deshabilita
sht31.heater(heaterStatus);
Par saber el estado del heater utilizar esta función:
sht31.isHeaterEnabled();
*/ 

Adafruit_SHT31 sht31 = Adafruit_SHT31(); //Sensor
float t = 0; //Temperatura
float h = 0; //Humedad
float p = 0; //Presion

//LCD DF.Robot//
const int rs = 8, en = 9, d4 = 4, d5 = 5, d6 = 6, d7 = 7;
LiquidCrystal lcd(rs,en,d4,d5,d6,d7);
byte inc[8] = {4,14,31,4,4,4,4,4}; //ram char flecha arriba
byte dec[8] = {4,4,4,4,4,31,14,4}; //ram char flecha abajo
byte con[8] = {0,4,6,31,6,4,0,0}; //ram char flecha derecha
byte corazon[8] = {0,10,31,31,14,4,0,0}; //ran char corazón
byte grado[8] = {24,24,3,4,4,4,3,0}; //ram char grado cent
byte gota[8] = {4,4,14,14,31,29,25,14}; //ram char gota
byte temp[8] = {12, 12, 12, 12, 12, 18, 18, 12}; // ram char temp
byte press[8] = {4, 4, 4, 21, 31, 17, 17, 31};// ram char press

/*********************************************************************
Setup
*********************************************************************/
void setup() {
 lcd.begin(16,2);
 lcd.createChar(1,press);
 lcd.createChar(2,temp);
 lcd.createChar(4,corazon);
 lcd.createChar(5,grado);
 lcd.createChar(6,gota);
//Pantalla de booteo
 lcd.setCursor(0,0);
 lcd.write(2);
 lcd.setCursor(1,0);
 lcd.write(6);
 lcd.setCursor(2,0);
 lcd.write(1);
 lcd.setCursor(5,0);
 lcd.print(" T-H-P");
 lcd.setCursor(13,0);
 lcd.write(1);
 lcd.setCursor(14,0);
 lcd.write(6);
 lcd.setCursor(15,0);
 lcd.write(2);
 lcd.setCursor(0,1);
 lcd.print(version);
 delay(4000);
 lcd.clear();
// Mensaje de booteo por consola
 Serial.begin(9600);
 Serial.println();
 Serial.print("- Boot! ");
 Serial.println(version);
 if (!bmp.begin()) {
  Serial.println("- no existe sensor de presion");
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Error sensor P");
  while (1) delay(1);
 }
 if(! sht31.begin(0x44)) {
  Serial.println("- no existe sensor de humedad");
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Error sensor H");
  while (1) delay(1);
 }
 Serial.print("- calentador: ");
 if(sht31.isHeaterEnabled())
  Serial.println("ON");
 else
  Serial.println("OFF");
}

/*********************************************************************
armarLcd
*********************************************************************/
void armarLcd(void) {
 lcd.clear();
 if (!isnan(t)) {

  lcd.setCursor(0,0);
  if(alive == true)//Indicador de loop corriendo
    lcd.write(4);
  else
    lcd.print(" ");
  alive = !alive;

  lcd.setCursor(2,0); //Arma primera línea lcd
  lcd.print("T:");
  lcd.setCursor(4,0);

  if(t > 0) {
   lcd.print("+");
   lcd.setCursor(5,0);
  }
  lcd.print(t,0);
  lcd.setCursor(7,0);
  lcd.write(5); //Indicador grados centigrados
  lcd.setCursor(9,0);
  lcd.print("H:");
  if(h >= 99.99)
   lcd.print(h,0);
  else
   lcd.print(h,1);
  lcd.setCursor(15,0);
  lcd.print("%");
 }
 else {
  lcd.clear();
  lcd.setCursor(1,0);
  lcd.print("Error en sensor");
 }
 if(!isnan(p)) {
  lcd.setCursor(2,1);
  lcd.print("P:");
  lcd.print(p,0);
  lcd.setCursor(11,1);
  lcd.print("Pa");  
  }
  else {
   lcd.clear();
   lcd.setCursor(1,0);
   lcd.print("Error en sensor");
  }
}

/*********************************************************************
consola
*********************************************************************/
void consola(void) {
  Serial.print("- Presion = ");
  Serial.println(p);
  Serial.print("- Temperatura = ");
  Serial.println(t);
  Serial.print("- Humedad = ");
  Serial.println(h);
}

/*********************************************************************
Loop
*********************************************************************/
void loop() {
 millis_Now = millis();
 deltaLoop_millis = millis_Now - millis_Ant;
 if(deltaLoop_millis >= deltaLoop) {// Loop lento (1seg)
  millis_Ant = millis_Now;
  t = sht31.readTemperature(); //Leer temperatura
  h = sht31.readHumidity(); //Leer humedad
//  p = bmp.readPressure();
  armarLcd(); //Atender armado de lcd
  consola(); //Atender transmisión por consola
 }//Loop lento End
}//Loop End
