/*
Arduino Uno con shield lcd de DF.Robot y SHT30 en bus I2C
*/
#include <LiquidCrystal.h>
#include <PString.h>
#include <Wire.h>
#include "Adafruit_SHT31.h"
#include <Adafruit_BMP085.h>

Adafruit_BMP085 bmp;

const char version[16] = " v0.01 28/09/22 ";
// Relay
const int relay = A1;
 
// Loop lento y loop rápido
int deltaLoop = 1000;
int deltaLoop_millis;
int millis_Ant;
int millis_Now;

//Pstring
//char AuxStr[4]; // Array auxiliar para PString
//int  AuxStrPtr; //Puntero auxiliar para PString


bool alive = false; //Indicador en lcd de loop corriendo
bool booteando = true; //Flag para marcar que recién se bootea

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
float promedio = 0; //Promedio de deltaProm lecturas sucesivas de h
float promedioTemp = 0; //Variable guardado temporal de promedio
float promedioAnt = 0; //Promedio anterior
float lineaBase = 0; //Promedio al pasar el threshold, definitivo
float promedioBase1 = 0; //Promedio al pasar el threshold, copia 1
float promedioBase2 = 0; //Promedio al pasar el threshold, copia 2
float promedioBase3 = 0; //Promedio al pasar el threshold, copia 3
float promedioBase4 = 0; //Promedio al pasar el threshold, copia 4
float promedioBase5 = 0; //Promedio al pasar el threshold, copia 5
float tolerancia = 10; //Tolerancia para el apagado en porcentaje de lineaBase
float variacion = 1.0; //Variación mínima entre promedio y promedioAnt necesaria para disparar encendido de motor
int deltaProm = 10; //Cantidad de muestras a promediar (en segundos)
int contProm = 0; //Contador de vueltas para calcular el promedio
int contVarPos = 0;//Contador de variaciones positivas
int threshold = 2;//Cantidad de minutos con variación positiva, máximo 5 minutos
bool humedadVarOut = false; //Resultado de variación de humedad, false = motor off, true = motor on

//LCD DF.Robot//
const int rs = 8, en = 9, d4 = 4, d5 = 5, d6 = 6, d7 = 7;
LiquidCrystal lcd(rs,en,d4,d5,d6,d7);
byte inc[8] = {4,14,31,4,4,4,4,4}; //ram char flecha arriba
byte dec[8] = {4,4,4,4,4,31,14,4}; //ram char flecha abajo
byte con[8] = {0,4,6,31,6,4,0,0}; //ram char flecha derecha
byte corazon[8] = {0,10,31,31,14,4,0,0}; //ran char corazón
byte grado[8] = {24,24,3,4,4,4,3,0}; //ram char grado cent
byte gota[8] = {4,4,14,14,31,29,25,14}; //ram char gota
              // 0123456789012345
char lcdStr[32]; // string para el lcd
int  lcdPtr = 0; //Puntero de escritura del LCD. Va de 0 a 31


/*********************************************************************
Setup
*********************************************************************/
void setup() {

 pinMode(relay, OUTPUT);
 digitalWrite(relay,HIGH); //Relay active low
 
 lcd.begin(16,2);
 lcd.createChar(1,inc);
 lcd.createChar(2,dec);
 lcd.createChar(3,con);
 lcd.createChar(4,corazon);
 lcd.createChar(5,grado);
 lcd.createChar(6,gota);
//Pantalla de booteo
 lcd.setCursor(0,0);
 lcd.write(6);
 lcd.setCursor(1,0);
 lcd.write(6);
 lcd.setCursor(5,0);
 lcd.print(" T-H-P");
 lcd.setCursor(14,0);
 lcd.write(6);
 lcd.setCursor(15,0);
 lcd.write(6);
 lcd.setCursor(0,1);
 lcd.print(version);
// Mensaje de booteo por consola
 Serial.begin(9600);
 if (!bmp.begin()) {
 Serial.println("Could not find a valid BMP085 sensor, check wiring!");
 while (1) {}
 }
 
 Serial.println();
 Serial.print("- Boot! ");
 Serial.println(version);



 delay(4000);
 lcd.clear();
//Arranque del sensor
 if(! sht31.begin(0x44)) {
  Serial.println("- no existe sensor");
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Error en sensor");
  while (1) delay(1);
 }
 Serial.print("- calentador: ");
 if(sht31.isHeaterEnabled())
  Serial.println("ON");
 else
  Serial.println("OFF");
}

/*********************************************************************
LCDRefresh
*********************************************************************/

void lcdRefresh()// Actualiza toda la pantalla
{
//Escribo la fila 0
if(lcdPtr < 16)
	{
	if(lcdPtr == 0)
    lcd.setCursor(lcdPtr,0);
	lcd.print(lcdStr[lcdPtr]);
	}
//Escribo la fila 1
if((lcdPtr >= 16) && (lcdPtr < 31))
	{
	if(lcdPtr == 16)
    lcd.setCursor(lcdPtr - 16,1);
	lcd.print(lcdStr[lcdPtr]);
	}
//Incremento el puntero y si llegó a 32 lo reseteo
lcdPtr = lcdPtr + 1;
if(lcdPtr == 32)
  lcdPtr = 0;
}


/*********************************************************************
calcVarHum: cálculo de variaciónde humedad, sólo si hay un nuevo
 promedio
*********************************************************************/
void calcVarHum(void) {
 //Cálculo del promedio cada deltaProm mediciones
  if(contProm < deltaProm) {
   promedioTemp = promedioTemp + h;
   contProm++;
  }
  else {
   promedioAnt = promedio;
   promedio = promedioTemp/deltaProm; //Ultimo promedio
   contProm = 0;
   promedioTemp = 0;
  }
 if(contProm == 0) {
  if(booteando == true) {
   booteando = false;
   promedioAnt = promedio;
  }
  if((promedio - promedioAnt) >= variacion) {
   contVarPos = contVarPos + 1;
   if(contVarPos >= threshold)
    contVarPos = threshold;
   switch(contVarPos) {
    case 1: {
     promedioBase1 = promedioAnt;
     break;
    }
    case 2: {
     promedioBase2 = promedioBase1;
     promedioBase1 = promedioAnt;
     break;
    }
    case 3: {
     promedioBase3 = promedioBase2;
     promedioBase2 = promedioBase1;
     promedioBase1 = promedioAnt;
     break;
    }
    case 4: {
     promedioBase4 = promedioBase3;
     promedioBase3 = promedioBase2;
     promedioBase2 = promedioBase1;
     promedioBase1 = promedioAnt;
     break;
    }
    case 5: {
     promedioBase5 = promedioBase4;
     promedioBase4 = promedioBase3;
     promedioBase3 = promedioBase2;
     promedioBase2 = promedioBase1;
     promedioBase1 = promedioAnt;
     break;
    }
    default:
    break;
   }
  }
 else {
  contVarPos = contVarPos - 1;
  if(contVarPos < 0)
   contVarPos = 0;
  }
 if(contVarPos == threshold) {
  if(humedadVarOut == false) {
   switch(threshold) {
    case 1: {
     lineaBase = promedioBase1;
     break;
    }
    case 2: {
     lineaBase = promedioBase2;
     break;
    }
    case 3: {
     lineaBase = promedioBase3;
     break;
    }
    case 4: {
     lineaBase = promedioBase4;
     break;
    }
    case 5: {
     lineaBase = promedioBase5;
     break;
    }
    default:
     break;
   }
  }
  humedadVarOut = true; //Enciende extractor
 }
 else
  if((promedio - lineaBase) <= (tolerancia * lineaBase)/100) {
   humedadVarOut = false; //Apaga extractor
   lineaBase = 0;
   promedioBase1 = 0;
   promedioBase2 = 0;
   promedioBase3 = 0;
   promedioBase4 = 0;
   promedioBase5 = 0;
  }
 }
}

/*********************************************************************
armarLcd

//Pstring
//char AuxStr[4]; // Array auxiliar para PString
//int  AuxStrPtr; //Puntero auxiliar para PString



*********************************************************************/
void armarLcd(void) {
 if (!isnan(t)) {

  lcd.setCursor(0,0);
  if(alive == true)//Indicador de loop corriendo
    lcd.write(4);
  else
    lcd.print(" ");
  alive = !alive;

// PString str(lcdStr,sizeof(lcdStr));
// str.print("G T:+25C i:50.1%b:50 % p: 50% >0");
             //0123456789012345
// lcdStr = str;

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
/*
 if (!isnan(t)) {
  Serial.print("- Temp *C = ");
  Serial.print(t);
  Serial.print(" ");
 }
 else {
  Serial.println("- Error en sensor");
 }
 if(!isnan(h)) {
  Serial.print("H% = ");
  Serial.println(h);
  }
  else {
   Serial.println("- Error en sensor");
  }
*/
 if(contProm == 0) {
  Serial.println();
  Serial.print("- tolerancia (c) = ");
  Serial.println(tolerancia);
  Serial.print("- variacion (c) = ");
  Serial.println(variacion);
  Serial.print("- deltaProm (c) = ");
  Serial.println(deltaProm);
  Serial.print("- threshold (c) = ");
  Serial.println(threshold);
  Serial.print("- contVarPos = ");
  Serial.println(contVarPos);
  Serial.print("- promedioAnt = ");
  Serial.println(promedioAnt);
  Serial.print("- promedio = ");
  Serial.println(promedio);
  Serial.print("- lineaBase = ");
  Serial.println(lineaBase);
  Serial.print("- promedioBase1 = ");
  Serial.println(promedioBase1);
  Serial.print("- promedioBase2 = ");
  Serial.println(promedioBase2);
  Serial.print("- promedioBase3 = ");
  Serial.println(promedioBase3);
  Serial.print("- promedioBase4 = ");
  Serial.println(promedioBase4);
  Serial.print("- promedioBase5 = ");
  Serial.println(promedioBase5);
  Serial.print("- humedadVarOut = ");
  Serial.println(humedadVarOut);
  Serial.print("- Presion = ");
  Serial.println(p);
 }
}

/*********************************************************************
relaycontrol
*********************************************************************/
void relayControl() {
 if(humedadVarOut == true)
  digitalWrite(relay, LOW); //Enciende extractor
 else
  digitalWrite(relay, HIGH); //Apaga extractor
}

/*********************************************************************
Loop
*********************************************************************/
void loop() {
 millis_Now = millis();
 deltaLoop_millis = millis_Now - millis_Ant;
 // lcdRefresh();
 if(deltaLoop_millis >= deltaLoop) {// Loop lento (1seg)
  millis_Ant = millis_Now;
  t = sht31.readTemperature(); //Leer temperatura
  h = sht31.readHumidity(); //Leer humedad
  p = bmp.readPressure();
  armarLcd(); //Atender armado de lcd
  consola(); //Atender transmisión por consola
  calcVarHum(); //Cálcular variación de humedad
  relayControl(); //Actuar sobre el relay
 }//Loop lento End
}//Loop End
