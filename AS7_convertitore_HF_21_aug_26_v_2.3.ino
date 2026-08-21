//=====================================================
// AS7 CONVERTITORE HF by A. Santucci I5SKK
// V 2.3  Aug. 2026 20th  Creative Common Licence
// 
// Arduino UNO + SI5351
// LCD 16x2 I2C
//
// Selettore bande 1P12T
// Shift cumulativo: +/- 500 kHz
//=====================================================

#include <Wire.h>
#include <si5351.h>
#include <LiquidCrystal_I2C.h>

Si5351 si5351;

LiquidCrystal_I2C lcd(0x27, 16, 2);

//=====================================================
// PIN BANDE
// Comune commutatore a GND
// Ingressi con INPUT_PULLUP
//=====================================================

#define LW_PIN     2
#define M160_PIN   3
#define M80_PIN    4
#define M60_PIN    5
#define M40_PIN    6
#define M30_PIN    7
#define M20_PIN    8
#define M17_PIN    9
#define M15_PIN    A0
#define M10_PIN    A1

//=====================================================
// SHIFT
//=====================================================

#define SHIFT_UP   10
#define SHIFT_DOWN 11

//=====================================================
// TABELLA FREQUENZE SI5351
//=====================================================

struct Banda
{
  const char *nome;
  unsigned long freqSI;
};

Banda bande[] =
{
  {"LW",  7510000},
  {"160", 9000000},
  {"80",  11000000},
  {"60",  12600000},
  {"40",  0},
  {"30",  17500000},
  {"20",  21500000},
  {"17",  25500000},
  {"15",  28500000},
  {"10",  35500000}
};

//=====================================================
// VARIABILI
//=====================================================

byte bandaAttuale = 0;

// Shift cumulativo:
//  0  = normale
// +1  = +500 kHz
// +2  = +1000 kHz
// +3  = +1500 kHz
//
// -1  = -500 kHz
// -2  = -1000 kHz
// -3  = -1500 kHz
// ecc.

long shift = 0;

unsigned long frequenzaSI = 0;

//=====================================================
// SETUP
//=====================================================

void setup()
{

  Wire.begin();

  pinMode(LW_PIN, INPUT_PULLUP);
  pinMode(M160_PIN, INPUT_PULLUP);
  pinMode(M80_PIN, INPUT_PULLUP);
  pinMode(M60_PIN, INPUT_PULLUP);
  pinMode(M40_PIN, INPUT_PULLUP);
  pinMode(M30_PIN, INPUT_PULLUP);
  pinMode(M20_PIN, INPUT_PULLUP);
  pinMode(M17_PIN, INPUT_PULLUP);
  pinMode(M15_PIN, INPUT_PULLUP);
  pinMode(M10_PIN, INPUT_PULLUP);

  pinMode(SHIFT_UP, INPUT_PULLUP);
  pinMode(SHIFT_DOWN, INPUT_PULLUP);

  lcd.init();
  lcd.backlight();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("AS7 CONVERTER");

  lcd.setCursor(0, 1);
  lcd.print("AVVIO");

  delay(1000);

  //===================================================
  // INIZIALIZZAZIONE SI5351
  //===================================================

  si5351.init(
    SI5351_CRYSTAL_LOAD_8PF,
    0,
    0
  );

  //===================================================
  // LETTURA BANDA
  //===================================================

  leggiBanda();

  //===================================================
  // IMPOSTAZIONE FREQUENZA
  //===================================================

  aggiornaSI5351();

  //===================================================
  // AGGIORNAMENTO DISPLAY
  //===================================================

  aggiornaDisplay();

}

//=====================================================
// LETTURA COMMUTATORE BANDE
//=====================================================

void leggiBanda()
{

  byte nuovaBanda = bandaAttuale;

  if(digitalRead(LW_PIN) == LOW)
    nuovaBanda = 0;

  else if(digitalRead(M160_PIN) == LOW)
    nuovaBanda = 1;

  else if(digitalRead(M80_PIN) == LOW)
    nuovaBanda = 2;

  else if(digitalRead(M60_PIN) == LOW)
    nuovaBanda = 3;

  else if(digitalRead(M40_PIN) == LOW)
    nuovaBanda = 4;

  else if(digitalRead(M30_PIN) == LOW)
    nuovaBanda = 5;

  else if(digitalRead(M20_PIN) == LOW)
    nuovaBanda = 6;

  else if(digitalRead(M17_PIN) == LOW)
    nuovaBanda = 7;

  else if(digitalRead(M15_PIN) == LOW)
    nuovaBanda = 8;

  else if(digitalRead(M10_PIN) == LOW)
    nuovaBanda = 9;

  //===================================================
  // CONTROLLO CAMBIO BANDA
  //===================================================

  if(nuovaBanda != bandaAttuale)
  {

    bandaAttuale = nuovaBanda;

    // Ad ogni cambio banda lo shift torna a zero
    shift = 0;

    aggiornaSI5351();
    aggiornaDisplay();

  }

}

//=====================================================
// AGGIORNA SI5351
//=====================================================

void aggiornaSI5351()
{

  //===================================================
  // 40 METRI DIRETTA
  //===================================================

  if(bande[bandaAttuale].freqSI == 0)
  {

    si5351.output_enable(SI5351_CLK0, 0);

    frequenzaSI = 0;

    return;

  }

  //===================================================
  // CALCOLO FREQUENZA SI5351 CON SHIFT CUMULATIVO
  //===================================================

  frequenzaSI =
    bande[bandaAttuale].freqSI +
    ((long)shift * 500000L);

  //===================================================
  // ATTIVA USCITA SI5351
  //===================================================

  si5351.output_enable(SI5351_CLK0, 1);

  //===================================================
  // IMPOSTA FREQUENZA
  //===================================================

  si5351.set_freq(
    (uint64_t)frequenzaSI * 100ULL,
    SI5351_CLK0
  );

}

//=====================================================
// STAMPA FREQUENZA
//=====================================================

void stampaFrequenza(long f)
{

  if(f < 0)
  {
    lcd.print("-");
    f = -f;
  }

  lcd.print(f / 1000000);
  lcd.print(".");

  unsigned int khz =
    (f % 1000000) / 1000;

  if(khz < 100)
    lcd.print("0");

  if(khz < 10)
    lcd.print("0");

  lcd.print(khz);

}

//=====================================================
// STAMPA SHIFT
// Mostra il valore corrente dello shift in alto a
// destra del display (riga 0), es: "0", "+500k",
// "-1.0M"
//=====================================================

void stampaShift()
{

  long shiftHz = shift * 500000L;

  lcd.setCursor(8, 0);

  lcd.print("sh=");

  //===================================================
  // SHIFT A ZERO
  //===================================================

  if(shiftHz == 0)
  {
    lcd.print("0");
    return;
  }

  //===================================================
  // SEGNO
  //===================================================

  if(shiftHz > 0)
    lcd.print("+");
  else
  {
    lcd.print("-");
    shiftHz = -shiftHz;
  }

  //===================================================
  // FORMATO: sotto 1 MHz in kHz, altrimenti in MHz
  //===================================================

  if(shiftHz >= 1000000L)
  {
    lcd.print(shiftHz / 1000000L);
    lcd.print(".");
    lcd.print((shiftHz % 1000000L) / 100000L);
    lcd.print("M");
  }
  else
  {
    lcd.print(shiftHz / 1000L);
    lcd.print("k");
  }

}

//=====================================================
// DISPLAY
//=====================================================

void aggiornaDisplay()
{

  lcd.clear();

  lcd.setCursor(0,0);

  //===================================================
  // VISUALIZZA BANDA
  //===================================================

  lcd.print(bande[bandaAttuale].nome);
  lcd.print(" m");

  //===================================================
  // VISUALIZZA SHIFT
  //===================================================

  stampaShift();

  lcd.setCursor(0,1);

  //===================================================
  // 40 METRI DIRETTA
  //===================================================

  if(bandaAttuale == 4)
  {
    lcd.print("40m DIRETTA");
    return;
  }

  //===================================================
  // FREQUENZA REALE DI ASCOLTO
  //===================================================

  long fmin =
    (long)frequenzaSI - 7500000L;

  long fmax =
    (long)frequenzaSI - 7000000L;

  stampaFrequenza(fmin);

  lcd.print("-");

  stampaFrequenza(fmax);

}

//=====================================================
// PULSANTI SHIFT
//=====================================================

void leggiShift()
{

  //===================================================
  // SHIFT +500 kHz
  //===================================================

  if(digitalRead(SHIFT_UP) == LOW)
  {

    delay(30);   // antirimbalzo

    if(digitalRead(SHIFT_UP) == LOW)
    {

      shift++;

      aggiornaSI5351();
      aggiornaDisplay();

      // Attende il rilascio del pulsante
      while(digitalRead(SHIFT_UP) == LOW)
      {
        delay(5);
      }

    }

  }

  //===================================================
  // SHIFT -500 kHz
  //===================================================

  if(digitalRead(SHIFT_DOWN) == LOW)
  {

    delay(30);   // antirimbalzo

    if(digitalRead(SHIFT_DOWN) == LOW)
    {

      shift--;

      aggiornaSI5351();
      aggiornaDisplay();

      // Attende il rilascio del pulsante
      while(digitalRead(SHIFT_DOWN) == LOW)
      {
        delay(5);
      }

    }

  }

}

//=====================================================
// LOOP
//=====================================================

void loop()
{

  leggiBanda();

  leggiShift();

  delay(20);

}
