# LED display

A 4x4 RGB display powered by an ATtiny2313 and controlled over UART.

Wyświetlacz RGB o rozdzielczości 4x4 zbudowany w oparciu o mikrokontroler AVR ATtiny2313, sterowany za pośrednictwem interfejsu UART.

<img src=demo.gif></img>

## Zasada działania

Wyświetlacz to tak naprawdę taki multiplekser na sterydach i odrobina pamięci - stąd też nazwy programów, niezmienne od zarania dziejów.

### mux
W folderze `mux` znajduje się program dla mikrokontrolera. Jego rolą jest oczywiście odbieranie poleceń rysowania wysłanych z komputera sterującego za pośrednictwem interfejsu UART (baudrate konfigurowany w `makefile`) i sterowanie macierzą diod.

Podczas interakcji ze sterownikiem wyświetlacza, należy wiedzieć, że na jego stan składają się **pozycja kursora**, **akutalnie wyświetlany bufor** oraz zawartość buforów obrazu.
Kontroler wspiera podwójne buforowanie danych obrazu by uniknąć nieprzyjemnego mrugania - wyświetlacz wyświetla zawartość bufora **przedniego**, podczas gdy przychodzące dane zapisywane są do bufora **tylnego**. W przypadku gdy podwójne buforowanie jest wyłączone, bufory tylny i przedni są tożsame.

Każdy bajt odebrany przez UART jest interpretowany w przerwaniu odbiornika. Podobnie jak w przypadku interfejsu MIDI, najstarszy bit determinuje typ danych.
Jeśli bajt stanowi fragment obrazu, przyjmuje on wartość 0, a jeśli jest poleceniem sterującym - 1. Bity 0-2 określają typ polecenia sterującego, a bity 3-6 stanowią jego parametr.
Obsługiwane polecenia wraz z krótkimi opisami zostały przedstawione w poniższej tabeli:
|ID polecenia|Nazwa|Parametr|Opis|
|:--:|:---:|---|---|
|0|`CBK`||Powrót kursora do początku bufora tylnego|
|1|`CLS`||Czyszczenie bufora tylnego|
|2|`BCL`|1 lub 0|Włącza (1) lub wyłącza (0) podwójne buforowanie obrazu|
|3|`BSW`||Zamiana bufora przedniego i tylnego, przeniesienie kursora na początek bufora tylnego|

_Wyjaśnienie pochodzenia skrótowych nazw poleceń pozostawiamy jako ćwiczenie dla czytelnika._

Każdy bajt niebędący poleceniem sterującym traktowany jest jako część obrazu. Odebranie takiego bajtu skutkuje przesunięciem kursora o jedną pozycję.
Przez "pozycję" rozumiemy kolejno składowe R, G i B, kolejne diody w wierszu (od lewej do prawej) i w końcu kolejne wiersze. "Wyjechanie" kursora poza ekran skutkuje jego powrotem na początek bufora.
Należy tutaj zwrócić uwagę, że wartości RGB muszą być z zakresu 0-127, bo najstarszy bit jest zarezerwowany.
Nie ma to jednak większego znaczenia dla "jakości obrazu", ponieważ wyświetlacz i tak obsługuje jedynie 32 poziomy jasności dla każdej składowej RGB.

Uwaga: Nasz mikrokontroler był upośledzony i wymagał korekty wewnętrzengo oscylatora, stąd `OSCCAL` w pliku `makefile`.

### mux-driver

W folderze `mux-driver` znajduje się program na Linuxa do sterowania wyświetlaczem. Kod powinien być prosty do zrozumienia (na pewno bardziej niż ten dla AVR), więc chyba nie ma po co się rozpisywać.
Program wysyła kolejne fragmenty 4x4 obrazu o rozmiarze 4x4N (dla naturalnych N) (czytanego za pomocą biblioteki stb) do wyświetlacza jako kolejne klatki w odpowiednich odstępach czasu za pośrednictwem wybranego interfejsu. 

Użycie: `./muxd <interfejs> <plik obrazu> [czas między klatkami w ms]`<br>
Przykład: `./muxd /dev/ttyUSB0 foo.png 200`<br>
