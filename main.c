/*
 * Projekt z przedmiotu Zastosowania Procesor闚 Sygna這wych
 */


// Do章czenie wszelkich potrzebnych plik闚 nag堯wkowych
#include "usbstk5515.h"
#include "usbstk5515_led.h"
#include "aic3204.h"
#include "PLL.h"
#include "bargraph.h"
#include "oled.h"
#include "pushbuttons.h"
#include "dsplib.h"


// Cz瘰totliwo pr鏏kowania (48 kHz)
#define CZESTOTL_PROBKOWANIA 48000L
// Wzmocnienie wejia w dB: 0 dla wejia liniowego, 30 dla mikrofonowego
#define WZMOCNIENIE_WEJSCIA_dB 30
//Krok fazy dla sygna逝 pi這kszta速nego (100Hz)
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


// G堯wna procedura programu

void main(void) {


    //mo瞠 si� przyda wyzerowanie bufora

    // for (idx = 0; i < NBUF; i++)
    // {
            // bufor[i] = 0;
    // }



    // Inicjalizacja uk豉du DSP
    USBSTK5515_init();          // BSL
    pll_frequency_setup(100);   // PLL 100 MHz
    aic3204_hardware_init();    // I2C
    aic3204_init();             // kodek di瘯u AIC3204
    USBSTK5515_ULED_init();     // diody LED
    SAR_init_pushbuttons();     // przyciski
    oled_init();                // wyielacz LED 2x19 znak闚

    // ustawienie cz瘰totliwoi pr鏏kowania i wzmocnienia wejia
    set_sampling_frequency_and_gain(CZESTOTL_PROBKOWANIA, WZMOCNIENIE_WEJSCIA_dB);

    // wypisanie komunikatu na wyietlaczu
    // 2 linijki po 19 znak闚, tylko wielkie angielskie litery
    oled_display_message("PROJEKT ZPS          ", "                   ");

    // 'tryb_pracy' oznacza tryb pracy wybrany przyciskami
    unsigned int tryb_pracy = 0;
    unsigned int poprzedni_tryb_pracy = 99;
    int i;
    int akumulator_fazy;
    // zmienne do przechowywania wartoi pr鏏ek
    short lewy_wejscie, prawy_wejscie, lewy_wyjscie, prawy_wyjscie;

    // Przetwarzanie pr鏏ek sygna逝 w p皻li
    while (1) {

        // odczyt pr鏏ek audio, zamiana na mono
        aic3204_codec_read(&lewy_wejscie, &prawy_wejscie);
        short mono_wejscie = (lewy_wejscie >> 1) + (prawy_wejscie >> 1);

        // sprawdzamy czy wcii皻o przycisk
        // argument: maksymalna liczba obs逝giwanych tryb闚
        tryb_pracy = pushbuttons_read(4);
        if (tryb_pracy == 0) // oba wcii皻e - wyjie
            break;
        else if (tryb_pracy != poprzedni_tryb_pracy) {
            // nast雷i豉 zmiana trybu - wcii皻o przycisk
            USBSTK5515_ULED_setall(0x0F); // zgaszenie wszystkich di鏚
            if (tryb_pracy == 1) {
                // wypisanie informacji na wyietlaczu
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


        // zadadnicze przetwarzanie w zale積oi od wybranego trybu pracy
        tryb_pracy = 4;
        if (tryb_pracy == 1) //pi豉
        {
            lewy_wyjscie = (akumulator_fazy);
            prawy_wyjscie = (akumulator_fazy);

        }
        else if (tryb_pracy == 2) //prostok靖
        {
            if(akumulator_fazy < PROG) // tutaj jest podpunkt, 瞠 trzeba u篡� PROG_JEDNA_TRZECIA
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

        // zapisanie wartoi na wyjie audio
        aic3204_codec_write(lewy_wyjscie, prawy_wyjscie);


        akumulator_fazy += KROK_FAZY_PILO;


        bufor[i] = lewy_wyjscie;


        i = i+1; //i++


        if(i == NBUF)
        {
            i = 0; // jak chcemy wykres to tutaj stawiamy BREAKPOINT
        }


    }


    // zako鎍zenie pracy - zresetowanie kodeka
    aic3204_disable();
    oled_display_message("KONIEC PRACY       ", "                   ");
    while (1);
}
