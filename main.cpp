#define OFF -1
#define T   0  // Top - Górny czujnik (Pin 2)
#define B   1  // Bottom - Dolny czujnik (Pin 3)

// Nowe piny zgodne z Twoim opisem i grafiką
const int PIR_PIN[]  = {2, 3}; // Tylko te piny obsługują attachInterrupt na Uno
const int RED_PIN[]  = {7, 5}; 
const int GRN_PIN[]  = {6, 4}; // Zielone (lub niebieskie) ledy

unsigned long czasOstatniejZmiany = 0;
const unsigned long czasZielonego = 5000;
const unsigned long czasCzerwonego = 2000;

int stan = T;
int ostatniStan = T;

// Zmienne modyfikowane w przerwaniu MUSZĄ mieć 'volatile'
volatile int kolejka[2] = {-1, -1};
volatile int rozmiarKolejki = 0;

// Funkcja dodawania do kolejki - wywoływana wewnątrz przerwań
void dodajDoKolejki(int kierunek) {
  if (kierunek == stan || (stan == OFF && kierunek == ostatniStan)) return;
  for (int i = 0; i < rozmiarKolejki; i++) {
    if (kolejka[i] == kierunek) return;
  }
  if (rozmiarKolejki < 2) {
    kolejka[rozmiarKolejki] = kierunek;
    rozmiarKolejki++;
  }
}

// FUNKCJE OBSŁUGI PRZERWAŃ (ISR): Muszą być bardzo szybkie i proste
void przerwanieGora() {
  dodajDoKolejki(T);
}

void przerwanieDol() {
  dodajDoKolejki(B);
}

int pobierzZKolejki() {
  if (rozmiarKolejki == 0) return -1;
  int nastepny = kolejka[0];
  for (int i = 0; i < rozmiarKolejki - 1; i++) {
    kolejka[i] = kolejka[i + 1];
  }
  kolejka[rozmiarKolejki - 1] = -1;
  rozmiarKolejki--;
  return nastepny;
}

void aktualizujSwiatla() {
  if (stan == OFF) {
    // Jeśli stan to OFF - włącz czerwone, wyłącz zielone dla OBU kierunków
    digitalWrite(RED_PIN[0], HIGH);
    digitalWrite(GRN_PIN[0], LOW);
    
    digitalWrite(RED_PIN[1], HIGH);
    digitalWrite(GRN_PIN[1], LOW);
  } 
  else if (stan == T) {
    // Jeśli stan to T (Góra) - zielone dla góry, czerwone dla dołu
    digitalWrite(RED_PIN[0], LOW);
    digitalWrite(GRN_PIN[0], HIGH);
    
    digitalWrite(RED_PIN[1], HIGH);
    digitalWrite(GRN_PIN[1], LOW);
  } 
  else if (stan == B) {
    // Jeśli stan to B (Dół) - czerwone dla góry, zielone dla dołu
    digitalWrite(RED_PIN[0], HIGH);
    digitalWrite(GRN_PIN[0], LOW);
    
    digitalWrite(RED_PIN[1], LOW);
    digitalWrite(GRN_PIN[1], HIGH);
  }
}


void setup() {
  for (int i = 0; i < 2; i++) {
    pinMode(PIR_PIN[i], INPUT);
    pinMode(RED_PIN[i], OUTPUT);
    pinMode(GRN_PIN[i], OUTPUT);
  }

  // POPRAWIONE: Teraz każdy pin (2 oraz 3) ma przypisane swoje własne przerwanie
  attachInterrupt(digitalPinToInterrupt(PIR_PIN[0]), przerwanieGora, RISING);
  attachInterrupt(digitalPinToInterrupt(PIR_PIN[1]), przerwanieDol, RISING);

  aktualizujSwiatla();
  czasOstatniejZmiany = millis();
}
void loop() {
  unsigned long t = millis();

  // Zauważ, że w pętli loop() nie ma już ŻADNEGO digitalRead ani tablicy poprPIR!
  // Czujniki działają w tle całkowicie automatycznie.

  if (stan != OFF) {
    if (rozmiarKolejki > 0 && (t - czasOstatniejZmiany >= czasZielonego)) {
      ostatniStan = stan;
      stan = OFF;
      aktualizujSwiatla();
      czasOstatniejZmiany = t;
    }
  } else {
    if (t - czasOstatniejZmiany >= czasCzerwonego) {
      int nastepny = pobierzZKolejki();
      if (nastepny != -1) {
        stan = nastepny;
        aktualizujSwiatla();
        czasOstatniejZmiany = t;
      }
    }
  }
}
