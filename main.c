/*
 * Projekt z przedmiotu Zastosowania Procesorów Sygna³owych
 * Grupa T1 1 Stencel Kaczorek
 */


// Do³¹czenie wszelkich potrzebnych plików nag³ówkowych
#include "usbstk5515.h"
#include "usbstk5515_led.h"
#include "aic3204.h"
#include "PLL.h"
#include "bargraph.h"
#include "oled.h"
#include "pushbuttons.h"
#include "dsplib.h"


// Czêstotliwoœæ próbkowania (48 kHz)
#define CZESTOTL_PROBKOWANIA 48000L
// Wzmocnienie wejœcia w dB: 0 dla wejœcia liniowego, 30 dla mikrofonowego
#define WZMOCNIENIE_WEJSCIA_dB 30
//Krok fazy dla sygna³u pi³okszta³tnego (100Hz)
#define KROK_FAZY_PILO 136

// wartosci min i max do zad 2
#define MAX  32767
#define MIN -32768
#define PROG 0
#define PROG_JEDNA_TRZECIA -16384


const short WSP_IIR[] = {62,124,62,-28801,12665};

DATA buf_iir[5];




#define NBUF 5000
short bufor[NBUF];
int idx = 0;


// G³ówna procedura programu

void main(void) {


    //mo¿e siê przyda wyzerowanie bufora

    // for (idx = 0; i < NBUF; i++)
    // {
            // bufor[i] = 0;
    // }



    // Inicjalizacja uk³adu DSP
    USBSTK5515_init();          // BSL
    pll_frequency_setup(100);   // PLL 100 MHz
    aic3204_hardware_init();    // I2C
    aic3204_init();             // kodek dŸwiêku AIC3204
    USBSTK5515_ULED_init();     // diody LED
    SAR_init_pushbuttons();     // przyciski
    oled_init();                // wyœwielacz LED 2x19 znaków

    // ustawienie czêstotliwoœci próbkowania i wzmocnienia wejœcia
    set_sampling_frequency_and_gain(CZESTOTL_PROBKOWANIA, WZMOCNIENIE_WEJSCIA_dB);

    // wypisanie komunikatu na wyœwietlaczu
    // 2 linijki po 19 znaków, tylko wielkie angielskie litery
    oled_display_message("PROJEKT ZPS          ", "                   ");

    // 'tryb_pracy' oznacza tryb pracy wybrany przyciskami
    unsigned int tryb_pracy = 0;
    unsigned int poprzedni_tryb_pracy = 99;
    int i;
    int akumulator_fazy;
    // zmienne do przechowywania wartoœci próbek
    short lewy_wejscie, prawy_wejscie, lewy_wyjscie, prawy_wyjscie;

    // Przetwarzanie próbek sygna³u w pêtli
    while (1) {

        // odczyt próbek audio, zamiana na mono
        aic3204_codec_read(&lewy_wejscie, &prawy_wejscie);
        short mono_wejscie = (lewy_wejscie >> 1) + (prawy_wejscie >> 1);

        // sprawdzamy czy wciœniêto przycisk
        // argument: maksymalna liczba obs³ugiwanych trybów
        tryb_pracy = pushbuttons_read(4);
        if (tryb_pracy == 0) // oba wciœniête - wyjœcie
            break;
        else if (tryb_pracy != poprzedni_tryb_pracy) {
            // nast¹pi³a zmiana trybu - wciœniêto przycisk
            USBSTK5515_ULED_setall(0x0F); // zgaszenie wszystkich diód
            if (tryb_pracy == 1) {
                // wypisanie informacji na wyœwietlaczu
                oled_display_message("PROJEKT ZPS      ", "TRYB 1 - PILO");
                // zapalenie diody nr 1
                USBSTK5515_ULED_on(0);
            } else if (tryb_pracy == 2) {
                oled_display_message("PROJEKT ZPS      ", "TRYB 2 - PROSTO");
                USBSTK5515_ULED_on(1);

                for (i=0;i<5;i++)
                {
                buf_iir[i] = 0;
                }

            } else if (tryb_pracy == 3) {
                oled_display_message("PROJEKT ZPS      ", "TRYB 3 - SINUS");
                USBSTK5515_ULED_on(2);
            } else if (tryb_pracy == 4) {
                oled_display_message("PROJEKT ZPS      ", "TRYB 4 - SZUM");
                USBSTK5515_ULED_on(3);
            }
            // zapisujemy nowo ustawiony tryb
            poprzedni_tryb_pracy = tryb_pracy;
        }


        // zadadnicze przetwarzanie w zale¿noœci od wybranego trybu pracy
        tryb_pracy = 4;
        if (tryb_pracy == 1) //pi³a
        {
            lewy_wyjscie = (akumulator_fazy);
            prawy_wyjscie = (akumulator_fazy);

        }
        else if (tryb_pracy == 2) //prostok¹t
        {
            if(akumulator_fazy < PROG) // tutaj jest podpunkt, ¿e trzeba u¿yæ PROG_JEDNA_TRZECIA
            {
                lewy_wyjscie = MAX;
                prawy_wyjscie = lewy_wyjscie;
            }
            else
            {
                lewy_wyjscie = MIN;
                prawy_wyjscie = lewy_wyjscie;
            }
        }
        else if (tryb_pracy == 3) //sinusoida
        {
            sine((DATA*)&akumulator_fazy, (DATA*)&lewy_wyjscie,1);
            prawy_wyjscie = lewy_wyjscie;
        }
        else if (tryb_pracy == 4) //white noise
        {
            rand16(&lewy_wyjscie, 1);
            prawy_wyjscie = lewy_wyjscie;
        }

        // zapisanie wartoœci na wyjœcie audio
        aic3204_codec_write(lewy_wyjscie, prawy_wyjscie);


        akumulator_fazy += KROK_FAZY_PILO;


        bufor[i] = lewy_wyjscie;


        i = i+1; //i++


        if(i == NBUF)
        {
            i = 0; // jak chcemy wykres to tutaj stawiamy BREAKPOINT
        }


    }


    // zakoñczenie pracy - zresetowanie kodeka
    aic3204_disable();
    oled_display_message("KONIEC PRACY       ", "                   ");
    while (1);
}
