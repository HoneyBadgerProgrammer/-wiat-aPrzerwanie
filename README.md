# -wiat-aPrzerwanie


🚦 Inteligentny System Sygnalizacji Świetlnej z Przerwaniami (Arduino Uno)

Repozytorium zawiera projekt inteligentnego sterownika sygnalizacji świetlnej dla skrzyżowania lub zwężenia dwukierunkowego (kierunek Górny T oraz Dolny B). System optymalizuje przepływ ruchu drogowego, wykorzystując czujniki obecności pojazdów (PIR) działające w oparciu o przerwania sprzętowe (Hardware Interrupts) oraz strukturę danych typu kolejka (FIFO).
Projekt został w pełni przygotowany i przetestowany w środowisku Tinkercad.

🛠️ Specyfikacja Sprzętowa (Mapowanie Pinów)

System optymalnie wykorzystuje dedykowane piny przerwań sprzętowych mikrokontrolera ATmega328P (Arduino Uno):

Komponent	Kierunek / Rola	Pin Arduino	Uwagi
PIR (Top)	Górny czujnik ruchu	Pin 2	Przerwanie sprzętowe (INT0)
PIR (Bottom)	Dolny czujnik ruchu	Pin 3	Przerwanie sprzętowe (INT1)
LED Red (Top)	Górne światło czerwone	Pin 7	Wymagany rezystor 220 Ω
LED Green (Top)	Górne światło zielone	Pin 6	Wymagany rezystor 220 Ω
LED Red (Bottom)	Dolne światło czerwone	Pin 5	Wymagany rezystor 220 Ω
LED Green (Bottom)	Dolne światło zielone	Pin 4	Wymagany rezystor 220 Ω


🚀 Jak Działają Przerwania i Słowo volatile?


1. Przerwania Sprzętowe (attachInterrupt)

Tradycyjne programy sprawdzają stan czujników w nieskończonej pętli (polling), co obciąża procesor i może prowadzić do przeoczenia sygnału, gdy mikrokontroler wykonuje inne zadania. W tym projekcie pętla loop() zajmuje się wyłącznie odliczaniem czasu maszynowego.
Gdy czujnik PIR wykryje pojazd, generuje sygnał wysokiego napięcia, który natychmiast przerywa główny kod i wywołuje funkcję dedykowaną (przerwanieGora lub przerwanieDol). Pojazd zostaje natychmiast wpisany do kolejki w ułamku mikrosekundy, a procesor wraca do odmierzania czasu.

2. Znaczenie Modyfikatora volatile

Zmienne kolejka oraz rozmiarKolejki są modyfikowane bezpośrednio wewnątrz funkcji obsługi przerwanej (ISR), a odczytywane w pętli loop(). Użycie słowa kluczowego volatile:
• Informuje kompilator, że wartość zmiennej może zmienić się nagle, poza głównym biegiem programu.
• Wymusza odczyt i zapis zmiennej bezpośrednio w pamięci RAM, wyłączając optymalizację rejestrową procesora. Dzięki temu pętla główna zawsze widzi aktualny stan kolejki pojazdów.

📝 Analiza Struktury Funkcji Programu

• dodajDoKolejki(int kierunek)
Sprawdza warunki bezpieczeństwa (czy kierunek nie ma już zielonego światła oraz czy nie został przed chwilą zamknięty) oraz zapobiega dublowaniu tego samego kierunku w pamięci. Jeśli warunki są spełnione, dodaje kierunek na koniec tablicy.
• pobierzZKolejki()
Zwraca pierwszy oczekujący w kolejce kierunek, po czym przesuwa pozostałe elementy w lewo (klasyczna logika kolejki FIFO) i zmniejsza licznik oczekujących o jeden.
• aktualizujSwiatla()
Steruje stanami logicznymi pinów LED na podstawie aktualnej zmiennej stan. Implementacja w oparciu o strukturę if-else zapewnia czytelne i bezbłędne przypisywanie stanów HIGH/LOW dla każdego z kierunków oddzielnie.
• loop()
Działa jako nieblokująca maszyna stanów (millis()). Odlicza czas trwania zielonego światła (5000 ms) oraz czas bezpiecznego czyszczenia skrzyżowania w stanie OFF (2000 ms), w którym oba kierunki mają bezwzględne czerwone światło.

📋 Kod źródłowy projektu

cpp
#define OFF -1
#define T   0  // Top - Górny czujnik (Pin 2)
#define B   1  // Bottom - Dolny czujnik (Pin 3)

const int PIR_PIN[]  = {2, 3}; 
const int RED_PIN[]  = {7, 5}; 
const int GRN_PIN[]  = {6, 4}; 

unsigned long czasOstatniejZmiany = 0;
const unsigned long czasZielonego = 5000;
const unsigned long czasCzerwonego = 2000;

int stan = T;
int ostatniStan = T;

volatile int kolejka[2] = {-1, -1};
volatile int rozmiarKolejki = 0;

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

void przerwanieGora() { dodajDoKolejki(T); }
void przerwanieDol()  { dodajDoKolejki(B); }

int pobierzZKolejki() {
  if (rozmiarKolejki == 0) return -1;
  int nastepny = kolejka[0];
  for (int i = 0; i < rozmiarKolejki - 1; i++) {
    kolejka[i] = kolejka[i + 1];
  }
  kolejka[1] = -1;
  rozmiarKolejki--;
  return nastepny;
}

void aktualizujSwiatla() {
  if (stan == OFF) {
    digitalWrite(RED_PIN[0], HIGH);
    digitalWrite(GRN_PIN[0], LOW);
    digitalWrite(RED_PIN[1], HIGH);
    digitalWrite(GRN_PIN[1], LOW);
  } 
  else if (stan == T) {
    digitalWrite(RED_PIN[0], LOW);
    digitalWrite(GRN_PIN[0], HIGH);
    digitalWrite(RED_PIN[1], HIGH);
    digitalWrite(GRN_PIN[1], LOW);
  } 
  else if (stan == B) {
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

  attachInterrupt(digitalPinToInterrupt(PIR_PIN[0]), przerwanieGora, RISING);
  attachInterrupt(digitalPinToInterrupt(PIR_PIN[1]), przerwanieDol, RISING);

  aktualizujSwiatla();
  czasOstatniejZmiany = millis();
}

void loop() {
  unsigned long t = millis();

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
Używaj kodu z rozwagą.

⚡ Rozwiązywanie Problemów (Tinkercad FAQ)


Czerwona dioda na pinie 5 nie świeci?

Jeśli logika programu działa dla pozostałych diod, a dolna czerwona dioda nie reaguje, wykonaj następujące kroki w edytorze graficznym Tinkercad:
1. Zweryfikuj parametry rezystora: Kliknij na rezystor podpięty bezpośrednio do pinu 5. Upewnij się, że jego wartość to 220, a jednostka w rozwijanej liście to Ω (omy). Domyślnie Tinkercad przypisuje nowym rezystorom wartość 1 kΩ, co drastycznie ogranicza prąd i sprawia, że dioda wygląda na zgaszoną w symulacji.
2. Sprawdź polaryzację LED: Upewnij się, że zagięta nóżka diody (Anoda, dodatnia) łączy się z rezystorem/pinem 5, a prosta nóżka (Katoda, ujemna) jest prawidłowo połączona przewodami z szyną uziemienia GND.
3. Sprawdź rzędy płytki stykowej: Upewnij się w powiększeniu, czy przewód z pinu 5, nóżka rezystora i nóżka diody znajdują się dokładnie w tej samej pionowej linii otworów.
