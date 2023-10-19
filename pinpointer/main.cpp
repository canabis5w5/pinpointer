#define F_CPU     9600000
#define BUZZER    PB0
#define LED       PB3
#define BUTTON    PB2
#define SIGNAL_IN PB4

#include <avr/io.h>
#include <avr/eeprom.h>
#include <util/delay.h>

int  base_threshold = 0;
int  sens_threshold = 1;     // чем больше зачение, тем стабильнее, но меньше чувствительность
bool flag_button = false;
bool flag_eeprom = false;
int  adc_f;

void Led(const bool x);
bool Button();
void adc_setup();
int  adc_read();
void pwm(const int& x);
void setup_port();
void signal_out(const int& i);
void threshold(int& x, const int& y);
void action();


int main(void)
{
	setup_port();
	adc_setup();

    while (1) 
    {
		if (flag_eeprom)
		{
			base_threshold = eeprom_read_word(0);
			_delay_ms(10);
			flag_eeprom = false;
		}
		adc_f = adc_read();
		action();
		signal_out(adc_f);
    }
}

// management led
void Led(const bool x)
{	
	if (x)
	{
		PORTB |= (1 << LED);
	}
	else
	{
		PORTB &= ~(1 << LED);
	}
}

// push button
bool Button()
{
	bool result = false;
	
	if (PINB == 2)
	{
		result = true;
	}
	
	return result;
}

void adc_setup ()
{
	ADMUX  |= (1 << ADLAR) | (1 << MUX1);                 // input PB4 / ADC2
	ADCSRB |= (1 << ACME);
	ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // div 128
	ADCSRA |= (1 << ADEN);                                // enable ADC
}

int adc_read ()
{
	ADCSRA |= (1 << ADSC);
	while (ADCSRA & (1 << ADSC));
	return ADCH;
}

void pwm(const int& x)
{
	// настройка тактирования
	// TCCR0B |= (1 << CS00);               // без делителя
	// TCCR0B |= (1 << CS01);               // делитель 8
	// TCCR0B |= (1 << CS01) | (1 << CS00); // делитель 64
	// TCCR0B |= (1 << CS02);               // делитель 256
	// TCCR0B |= (1 << CS00) | (1 << CS02); // делитель 1024
	
	// настройка режимов
	// TCCR0A |= (1 << WGM01);                // Ружим CTC, сброс при совпадении
	// TCCR0A |= (1 << WGM00);                // Режим Pashe correct pwm, счет идет до 255, а затем в обратном направлении до 0
	// TCCR0A |= (1 << WGM00) | (1 << WGM01); //Режим fast pwm, счет до 255 и затем сброс в 0
	
	//режим CTC
	// частота сигнала = F_CPU / 2 * делитель * (OCR0A + 1)
	TCCR0A |= (1 << WGM01);
	TCCR0A |= (1 << COM0A0);
	TCCR0B |= (1 << CS01) | (1 << CS00);
	OCR0A = x;
}

void setup_port()
{
	DDRB  &= ~(1 << SIGNAL_IN);
	PORTB &= ~(1 << SIGNAL_IN);
	DDRB  |=  (1 << LED);
	DDRB  |=  (1 << BUZZER);
	PORTB |=  (1 << BUZZER);
	DDRB  &= ~(1 << BUTTON);
	PORTB |=  (1 << BUTTON);
}

void signal_out(const int& i)
{
	// 10 this ~125mV     100 this ~1150 mV  
	if ((i >= 0) && (i < base_threshold))
	{		
		if ((i >= 0) && (i < 30))
		{
			Led(true);
			pwm(10);
		}
		if ((i >= 30) && (i < 60))
		{
			Led(true);
			pwm(15);
		}
		if ((i >= 60) && (i < 90))
		{
			pwm(20);
		}
		if (i >= 90)
		{
			pwm(25);
		}
		
	}
	else 
	{
		// off led and buzzer
		Led(false);
		pwm(0);
	}
}

void threshold(int& x /* input val base*/, const int& y /* -threshold */) // sensitivity
{
	x = x - y;
	eeprom_write_word(0 , x);
	_delay_ms(10);
	flag_eeprom = true;
}

void action()
{
	if (Button() && !flag_button)
	{
		flag_button = true;
		threshold(adc_f, sens_threshold);
	}
	if (!Button() && flag_button)
	{
		flag_button = false;
	}
}