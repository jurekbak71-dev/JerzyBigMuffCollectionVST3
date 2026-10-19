# Testy odsłuchowe 0.2.0

Wykonano 20 renderów efektu na rzeczywistym, czystym nagraniu gitary oraz plik dry. [Odsłuchy i komplet ustawień](../audio-demos/0.2.0/README.md).

- 14 trybów układu: Sustain/Tone/Volume 50%.
- Supa: dodatkowo Sustain 15/90% i Tone 10/90%.
- Ram’s Head 73#18: dodatkowo Sustain 15/90%.
- Wszystkie 20 renderów: skończone próbki, zero zgłoszonych błędów solvera, zgodne kanały dual mono.
- 21 odsłuchowych WAV: 573300 próbek, 44,1 kHz mono, PCM 24-bit, RMS −24 dBFS ±0,001 dB na 12 s frazy, bez clippingu. MP3 również zdekodowano i sprawdzono: prawidłowa długość, skończone próbki, bez przekroczenia pełnej skali.
- Wszystkie 21 WAV mają różne SHA-256. Zweryfikowano również 21 WAV float32 z zachowanym gainem; ich szczyty odpowiadają pomiarom renderera.
- 3/3 istniejące testy CTest przeszły po dodaniu trybu renderowania. Kod DSP wtyczki nie został zmieniony.

Poziom wejściowy modelu: 0,25 V peak, 1 kΩ źródła / 1 MΩ obciążenia. Odsłuchy wyrównano RMS dopiero po przetworzeniu, dlatego nie służą do oceny oryginalnego wzmocnienia modeli. W ZIP `raw-gain/*.wav` zachowują ten poziom (float32; Boost osiąga około 1,476 FS). Źródło i ustawienia są w manifeście, a pomiary gotowych plików w `verification.json`.

Porównanie A/B ma 41 s: dry 0–13 s, Supa 14–27 s, Ram’s Head 28–41 s. W każdym odcinku to samo wykonanie, nie kolejna gra gitarzysty. Efekt jest bez wzmacniacza/kolumny, aby odsłuch ujawniał jego własne brzmienie.

Źródło: GuitarML/GuitarLSTM, commit ee0983bb02af1e2db476466614d6421473beb01d, `data/ts9_test1_in_FP32.wav` (Fender Telecaster bridge, gitara Tone/Volume max), GPL-3.0 w repozytorium źródłowym. W paczce zachowano fragment źródła i informacje licencyjne. Kalibracja sprzętowego nagrania nie jest znana. Render offline pełnego procesora nie zastępuje testu hosta VST3 ani pomiaru fizycznego pedału; odsłuchy nie dowodzą zgodności z analogiem.
