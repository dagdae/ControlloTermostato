# Stub per la compilazione di controllo

Intestazioni *finte* che imitano le API di Arduino-ESP32 3.x usate dal firmware.
Servono solo a `make -C firmware/test compile-check`, che compila il firmware sul
PC con `-fsyntax-only` per trovare errori di sintassi e di tipo senza installare
la toolchain ESP32. Non producono un programma eseguibile e non sostituiscono la
compilazione vera con PlatformIO o Arduino IDE (fatta dalla CI di GitHub).
