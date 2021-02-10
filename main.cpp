/*
 * mikro_projektv2.c
 *
 * Created: 18.05.2020 20:33:16
 * Author : Pawe�
 */ 
#include "termometr.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#define F_CPU 16000000
#include <util/delay.h>
#include <math.h>
#include <string.h>
#include <stdio.h>


//tablice z warto�ciami wy�wietlanymi na wy�wietlaczu 7-seg 
uint8_t seg7[]={0xc0,0xf9,0xa4,0xb0,0x99,0x92,0x82,0xf8,0x80,0x90};
//tablica zawieraj�ca kolejne cyfry z kropk� wy�wietlane na 3 wy�wietlaczu segmentowym
uint8_t seg7_kropka[]={0x40,0x79,0x24,0x30,0x19,0x12,0x2,0x78,0x0,0x10};
//tablica przechowuj�ca warto�ci wysy�ane na wy�wietlacz
volatile int dane[3];
//zmienna do wyboru skali
volatile int skala=0;
//zmienna pomocnicza do drga� styk�w
volatile uint8_t klaw_zwieksz = 0;
volatile int timer=1;

//funkcje obs�uguj�ce czujnik DS18B20
uint8_t reset_ds(uint8_t jaki) 
{
  uint8_t stan; 
  DS18B20_DDR |= (1<<(jaki)); //ustawienie pinu 1-wire jako wyj�cie 
  DS18B20 &= ~(1<<(jaki)); //podanie stanu 0 na 1-wire 
  _delay_us(500); 
  DS18B20_DDR &= ~(1<<(jaki)); //ustawienie pinu 1-wire jako wej�cie 
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
	 DS18B20_DDR |= (1<<jaki); //ustawienie pinu 1-wire jako wyj�cie 
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
	DS18B20_DDR |= (1<<jaki); //ustawienie pinu 1-wire jako wyj�cie 
	DS18B20 &= ~(1<<jaki); //podanie stanu 0 na 1-wire 
	_delay_us(7); 
	DS18B20_DDR &= ~(1<<jaki); //ustawienie pinu 1-wire jako wej�cie 
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
	//Wy�wietlanie danych na 4 wy�wietlaczach 7-seg
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
	//aktywacja port�w porzebnych do obs�ugi wy�wietlacza
	seg_DDR = 0xff; 
	wybor_DDR |= 0xf0;
		
	przycisk_DDR &= ~(1<<zwieksz); //ustawianie przycisku jako urz�dzenie wej�ciowe
	przycisk_port |= (1<<zwieksz); //ustawienie pull up dla tego przycisku 
	
	
	TCCR0 = (1<<CS01) | (1<<CS00); //konfiguracja licznika T0
	TCCR1B = (1<<WGM12) | (1<<CS12);//tryb CTC i preskaler 256
	OCR1A = 976; //zapisanie do zawarto�ci rejestru
	TIMSK = (1<<TOIE0) | (1<<OCIE1A); //konfiguracja timer interrupt mask

	//Ustawienie pinu 0 jako wej�cie danych z 1-wire
	DDRB |=0b00000001;
	PORTB =0b00000001;
	
	sei(); //zezwolenie na przerwania
	
	while (1)
	{
		
		//Obs�uga drgania styk�w
	if (klaw_zwieksz==2)
	{
		if (skala++==2) skala=0;
		klaw_zwieksz = 3;
	}
	if(timer==0){	
		uint16_t temp=0;
		
		uint16_t temp_p=read_temp(0); //Pobranie temperatury z czujnika DS18B20 (Warto�� domy�lnie w stopniach Celciusza)
		uint16_t K = 4368; //Sta�a do przeliczenia C -> K
		uint16_t F = 512; //Sta�a do przeliczenia C -> F
		
		//Sprawdzanie przycisku odpowiedzialnego za wyb�r skali
		if (skala==0){
			
			temp = temp_p; //konwersja
			DDRD &= ~(1<<5); //gaszenie diody oznaczaj�c� skal� F
			DDRD |= (1<<7); //zapalanie diody oznaczaj�c� skal� C
		}
		if(skala==1){
			temp = temp_p+K; //konwersja
			DDRD &= ~(1<<7); //gaszenie diody oznaczaj�c� skal� C
			DDRD |= (1<<6); //zapalenie diody oznaczaj�c� skal� K
		}
		if(skala==2){
			temp=temp_p*18+F*10;
			temp=temp/10;	//konwersja
			DDRD &= ~(1<<6); //gaszenie diody oznaczaj�c� skal� K
			DDRD |= (1<<5); //zapalanie diody oznaczaj�c� skal� F
		}
		
		
		uint16_t dec = temp; //Zmienna pomocnicza do obliczenia warto�ci po przecinku
		temp &= 0xfff0; //Maska wydzielaj�ca warto�ci ca�kowite z 16-bit�w informacji z czujnika
		
		uint16_t temp_pom = temp >> 4; //przesuni�cie bitowe ucinaj�ce 4 ostatnie bity odpowiedzialne za cz�� po przecinku
		
		
		
		
		//Wyznaczenie kolejno cz�sci setnych, dziesi�tek i jedno�ci oraz zapisanie ich do poszczeg�lnych zmiennych
		uint16_t temp1=temp_pom/100; //setki
		uint16_t temp2=temp_pom%100;
		uint16_t temp3=temp2/10; //dziesi�tki
		uint16_t temp4=temp2%10; //jedno�ci
		
		//Obliczenie warto�ci po przecinku o maksymalnej dok�adno�ci dla zadanej rozdzielczo�ci
		
		//zmienne pomocnicze
		int pom=0;
		uint16_t temp_przec_1=dec;
		uint16_t temp_przec_2=dec;
		uint16_t temp_przec_3=dec;
		uint16_t temp_przec_4=dec;
		//Maskowanie i badanie ka�dego kolejnego bitu z czw�rki odpowiedzialnej za warto�ci po przecinku
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
		int pom1=pom/1000; //wyci�gniecie liczby ca�kowitej bo nie mo�emy wysy�a� double na licznik 7-seg
		
		dane[0]=temp1; //Przes�anie setek na wy�wietlacz
		dane[1]=temp3; //Przes�anie dziesi�tek na wy�wietlacz
		dane[2]=temp4; //Przes�anie jedno�ci na wy�wietlacz
		dane[3]=pom1; //Przes�anie cz�ci dziesi�tnych na wy�wietlacz
		
	}
	timer = timer-1;
	}
}

ISR(TIMER1_COMPA_vect){
	//Instrukcje obs�uguj�ce drgania styk�w przycisku

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



