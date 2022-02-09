/*
 * mikro_projektv2.c
 *
 * Created: 18.05.2020 20:33:16
 * Author : Paweł  & Mateusz
 */ 
#include "termometr.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#define F_CPU 16000000
#include <util/delay.h>
#include <math.h>
#include <string.h>
#include <stdio.h>


//tablice z wartościami wyświetlanymi na wyświetlaczu 7-seg 
uint8_t seg7[]={0xc0,0xf9,0xa4,0xb0,0x99,0x92,0x82,0xf8,0x80,0x90};
//tablica zawierająca kolejne cyfry z kropką wyświetlane na 3 wyświetlaczu segmentowym
uint8_t seg7_kropka[]={0x40,0x79,0x24,0x30,0x19,0x12,0x2,0x78,0x0,0x10};
//tablica przechowująca wartości wysyłane na wyświetlacz
volatile int dane[3];
//zmienna do wyboru skali
volatile int skala=0;
//zmienna pomocnicza do drgań styków
volatile uint8_t klaw_zwieksz = 0;
volatile int timer=1;

//funkcje obsługujące czujnik DS18B20
uint8_t reset_ds(uint8_t jaki) 
{
  uint8_t stan; 
  DS18B20_DDR |= (1<<(jaki)); //ustawienie pinu 1-wire jako wyjście 
  DS18B20 &= ~(1<<(jaki)); //podanie stanu 0 na 1-wire 
  _delay_us(500); 
  DS18B20_DDR &= ~(1<<(jaki)); //ustawienie pinu 1-wire jako wejście 
  DS18B20 |= (1<<(jaki)); //zwolnienie magistarli 1-wire 
  _delay_us(60); 
  if (!(DS18B20_PIN & (1<<(jaki)))) //sprawdzenie stanu linii 1-wire 
  { 
	  stan = 0;
 }
  else stan = 1;
  _delay_us(100); 
  return stan; 
 }
 
 void write_bit_ds(uint8_t bit, uint8_t jaki) 
 { 
	 DS18B20_DDR |= (1<<jaki); //ustawienie pinu 1-wire jako wyjście 
	 DS18B20 &= ~(1<<jaki); //podanie stanu 0 na 1-wire 
	 if (bit == 0) { 
		 _delay_us(60); 
		 DS18B20 |= (1<<jaki); //zwolnienie magistrali 1-wire 
		 _delay_us(10); 
		 }
 else 
 { 
	 _delay_us(15);
	  DS18B20 |= (1<<jaki); //zwolnienie magistrali 1-wire 
	  _delay_us(20); 
 } 
}

uint8_t read_bit_ds(uint8_t jaki) 
{
	uint8_t stan; 
	DS18B20_DDR |= (1<<jaki); //ustawienie pinu 1-wire jako wyjście 
	DS18B20 &= ~(1<<jaki); //podanie stanu 0 na 1-wire 
	_delay_us(7); 
	DS18B20_DDR &= ~(1<<jaki); //ustawienie pinu 1-wire jako wejście 
	_delay_us(12);
	 if (!(DS18B20_PIN & (1<<jaki))) //sprawdzenie stanu linii 1-wire 
	 { 
		 stan = 0;
	 }
	else stan = 1; 
	_delay_us(10);
	return stan;
  }
  
  void write_byte_ds(uint8_t bajt,uint8_t jaki)
   { 
	   uint8_t i, kod; 
	   for (i=0;i<8;i++) { 
		   kod = bajt >> i;
		    kod &= 0x01; 
			write_bit_ds(kod,jaki); } 
	}
	
  uint8_t read_byte_ds(uint8_t jaki) { 
	  uint8_t i, kod = 0; 
	  for (i=0;i<8;i++) { 
		  if (read_bit_ds(jaki)) { 
			  kod |= 0x01 << i; 
			  }
			}
  return kod;
   }
   
   uint16_t read_temp(uint8_t jaki) {
	    uint16_t wynik=0;
		uint8_t tab[2]; 
		DS18B20_DDR |= (1<<jaki);
		 if (!reset_ds(jaki)) //reset 
		 {
			  write_byte_ds(0xcc,jaki); //komenda SKIP ROM 
			  write_byte_ds(0x44,jaki); //komenda CONVERT T 
			  if (!reset_ds(jaki)) //reset 
			  { write_byte_ds(0xcc,jaki); //komenda SKIP ROM 
				  write_byte_ds(0xbe,jaki); //komenda READ SCRATCHPAD 
				  uint8_t k=0; while(k<2) { 
					  tab[k] = read_byte_ds(jaki); //odczyt temperatury 
					  k++; }
					} 
					 
					  wynik = tab[0] | ((tab[1]<<8) & 0xff00); 
					 if(wynik & 0x8000) wynik = 1-(wynik^0xFFFF);
					  }
   return wynik;
    }
	


//Przerwanie timera T0
ISR(TIMER0_OVF_vect)
{
	
	timer=5000;
	//Wyświetlanie danych na 4 wyświetlaczach 7-seg
	static uint8_t i=0;
	seg_PORT = 0xff;
	wybor |= (1<<(7-i));
	if (i++==3)i=0;	
	if (i==2) seg_PORT = seg7_kropka[dane[i]];
	else
	seg_PORT = seg7[dane[i]];
	wybor &= ~(1<<(7-i));
	
	
	
}




int main(void)
{
	//aktywacja portów porzebnych do obsługi wyświetlacza
	seg_DDR = 0xff; 
	wybor_DDR |= 0xf0;
		
	przycisk_DDR &= ~(1<<zwieksz); //ustawianie przycisku jako urządzenie wejściowe
	przycisk_port |= (1<<zwieksz); //ustawienie pull up dla tego przycisku 
	
	
	TCCR0 = (1<<CS01) | (1<<CS00); //konfiguracja licznika T0
	TCCR1B = (1<<WGM12) | (1<<CS12);//tryb CTC i preskaler 256
	OCR1A = 976; //zapisanie do zawartości rejestru
	TIMSK = (1<<TOIE0) | (1<<OCIE1A); //konfiguracja timer interrupt mask

	//Ustawienie pinu 0 jako wejście danych z 1-wire
	DDRB |=0b00000001;
	PORTB =0b00000001;
	
	sei(); //zezwolenie na przerwania
	
	while (1)
	{
		
		//Obsługa drgania styków
	if (klaw_zwieksz==2)
	{
		if (skala++==2) skala=0;
		klaw_zwieksz = 3;
	}
	if(timer==0){	
		uint16_t temp=0;
		
		uint16_t temp_p=read_temp(0); //Pobranie temperatury z czujnika DS18B20 (Wartość domyślnie w stopniach Celciusza)
		uint16_t K = 4368; //Stała do przeliczenia C -> K
		uint16_t F = 512; //Stała do przeliczenia C -> F
		
		//Sprawdzanie przycisku odpowiedzialnego za wybór skali
		if (skala==0){
			
			temp = temp_p; //konwersja
			DDRD &= ~(1<<5); //gaszenie diody oznaczającą skalę F
			DDRD |= (1<<7); //zapalanie diody oznaczającą skalę C
		}
		if(skala==1){
			temp = temp_p+K; //konwersja
			DDRD &= ~(1<<7); //gaszenie diody oznaczającą skalę C
			DDRD |= (1<<6); //zapalenie diody oznaczającą skalę K
		}
		if(skala==2){
			temp=temp_p*18+F*10;
			temp=temp/10;	//konwersja
			DDRD &= ~(1<<6); //gaszenie diody oznaczającą skalę K
			DDRD |= (1<<5); //zapalanie diody oznaczającą skalę F
		}
		
		
		uint16_t dec = temp; //Zmienna pomocnicza do obliczenia wartości po przecinku
		temp &= 0xfff0; //Maska wydzielająca wartości całkowite z 16-bitów informacji z czujnika
		
		uint16_t temp_pom = temp >> 4; //przesunięcie bitowe ucinające 4 ostatnie bity odpowiedzialne za część po przecinku
		
		
		
		
		//Wyznaczenie kolejno częsci setnych, dziesiątek i jedności oraz zapisanie ich do poszczególnych zmiennych
		uint16_t temp1=temp_pom/100; //setki
		uint16_t temp2=temp_pom%100;
		uint16_t temp3=temp2/10; //dziesiątki
		uint16_t temp4=temp2%10; //jedności
		
		//Obliczenie wartości po przecinku o maksymalnej dokładności dla zadanej rozdzielczości
		
		//zmienne pomocnicze
		int pom=0;
		uint16_t temp_przec_1=dec;
		uint16_t temp_przec_2=dec;
		uint16_t temp_przec_3=dec;
		uint16_t temp_przec_4=dec;
		//Maskowanie i badanie każdego kolejnego bitu z czwórki odpowiedzialnej za wartości po przecinku
		temp_przec_1 &=0x1;
		if(temp_przec_1== 1) pom+=625;
		temp_przec_2 &= 0x2;
		uint16_t temp22=temp_przec_2 >> 1;
		if(temp22 == 1) pom+=1250;
		temp_przec_3 &= 0x4;
		uint16_t temp33 = temp_przec_3 >> 2;
		if(temp33 == 1) pom+=2500;
		temp_przec_4 &= 0x8;
		uint16_t temp44 = temp_przec_4 >> 3;
		if(temp44 == 1) pom+=5000;
		int pom1=pom/1000; //wyciągniecie liczby całkowitej bo nie możemy wysyłać double na licznik 7-seg
		
		dane[0]=temp1; //Przesłanie setek na wyświetlacz
		dane[1]=temp3; //Przesłanie dziesiątek na wyświetlacz
		dane[2]=temp4; //Przesłanie jedności na wyświetlacz
		dane[3]=pom1; //Przesłanie części dziesiętnych na wyświetlacz
		
	}
	timer = timer-1;
	}
}

ISR(TIMER1_COMPA_vect){
	//Instrukcje obsługujące drgania styków przycisku

	switch (klaw_zwieksz){
		case 0: if (!(przycisk_pin & (1<<zwieksz))) klaw_zwieksz=1;
		break;
		case 1: if (!(przycisk_pin & (1<<zwieksz))) klaw_zwieksz=2;
		else klaw_zwieksz=0;
		break;
		
		case 3: if (przycisk_pin & (1<<zwieksz)) klaw_zwieksz=4;
		break;
		case 4: if (przycisk_pin & (1<<zwieksz)) klaw_zwieksz=0;
		break;
		default:
		klaw_zwieksz = 0;
		break;
	}
}



