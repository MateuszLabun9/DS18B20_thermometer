/*IncFile1.h

Created 02.06.2020 115721
Author Pawel

*/

#ifndef INCFILE1_H_
#define INCFILE1_H_
#include <inttypes.h>
//definicje wejśćwyjść

//porty obsługujące czujnik DS18B20
#define DS18B20 PORTB
#define DS18B20_DDR DDRB
#define DS18B20_PIN PINB
//Porty obsługujące przycisk wyboru skali oraz diody informujące
#define przycisk_DDR DDRD
#define przycisk_port PORTD
#define przycisk_pin PIND
#define zwieksz 0// pin wejściowy z przycisku


//Porty obsługujące wyświetlacz siedmio segmentowy
#define seg_DDR DDRA
#define seg_PORT PORTA
#define wybor PORTC //piny 7-4 jako wybór cyfry
#define wybor_DDR DDRC

extern uint8_t reset_ds(uint8_t jaki);
extern void write_bit_ds(uint8_t bit, uint8_t jaki);
extern uint8_t read_bit_ds(uint8_t jaki);
extern void write_byte_ds(uint8_t bajt,uint8_t jaki);
extern uint8_t read_byte_ds(uint8_t jaki);
extern uint16_t read_temp(uint8_t jaki);



#endif /* INCFILE1_H_ */