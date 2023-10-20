#define F_CPU     9600000
#define BUZZER    PB0
#define LED       PB3
#define BUTTON    PB2
#define SIGNAL_IN PB4

#include <avr/io.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <avr/interrupt.h>

uint8_t base_threshold = 0;
uint8_t  sens_threshold = 1;     // ��� ������ �������, ��� ����������, �� ������ ����������������
bool flag_eeprom = false;
uint8_t  adc_f;
uint8_t  tone = 25;

void Led(const bool& x);
void adc_setup();
uint8_t  adc_read();
void pwm(const uint8_t& x);
void setup_port();
void signal_out(const uint8_t& i);
void threshold(uint8_t& x, const uint8_t& y);
void action();
void indication(const uint8_t& tone, const int& times);
void my_delay_ms(int ms);
void interrupt_setup();
void read_eeprom(bool flag_eeprom);


int main(void)
{
	setup_port();
	adc_setup();
	interrupt_setup();
	indication(tone, 50);

    while (1) 
    {
		read_eeprom(flag_eeprom);
		adc_f = adc_read();
		signal_out(adc_f);
    }
}

// management led
void Led(const bool& x)
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

void adc_setup ()
{
	ADMUX  |= (1 << ADLAR) | (1 << MUX1);                 // input PB4 / ADC2
	ADCSRB |= (1 << ACME);
	ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // div 128
	ADCSRA |= (1 << ADEN);                                // enable ADC
}

uint8_t adc_read ()
{
	ADCSRA |= (1 << ADSC);
	while (ADCSRA & (1 << ADSC));
	return ADCH;
}

void pwm(const uint8_t& x)
{
	// ��������� ������������
	// TCCR0B |= (1 << CS00);               // ��� ��������
	// TCCR0B |= (1 << CS01);               // �������� 8
	// TCCR0B |= (1 << CS01) | (1 << CS00); // �������� 64
	// TCCR0B |= (1 << CS02);               // �������� 256
	// TCCR0B |= (1 << CS00) | (1 << CS02); // �������� 1024
	
	// ��������� �������
	// TCCR0A |= (1 << WGM01);                // ����� CTC, ����� ��� ����������
	// TCCR0A |= (1 << WGM00);                // ����� Pashe correct pwm, ���� ���� �� 255, � ����� � �������� ����������� �� 0
	// TCCR0A |= (1 << WGM00) | (1 << WGM01); //����� fast pwm, ���� �� 255 � ����� ����� � 0
	
	//����� CTC
	// ������� ������� = F_CPU / 2 * �������� * (OCR0A + 1)
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

void signal_out(const uint8_t& i)
{
	// 10 this ~125mV     100 this ~1150 mV  
	if (/*(i >= 0) &&*/ (i <= base_threshold))
	{		
		if (/*(i >= 0) &&*/ (i < 30))
		{
			indication(tone, 1);
		}
		else if ((i >= 30) && (i < 50))
		{
			indication(tone, 10);
		}
		else if ((i >= 50) && (i < 70))
		{
			indication(tone, 20);
		}
		else if (i >= 70)
		{
			indication(tone, 50);
		}
		
	}
	else 
	{
		// off led and buzzer
		Led(false);
		pwm(0);
	}
}

void threshold(uint8_t& x /* input val base*/, const uint8_t& y /* -threshold */) // sensitivity
{
	if (x >= y)
	{
		x -= y;
		eeprom_write_byte(0 , x);
		flag_eeprom = true;
	}
}

void read_eeprom(bool flag_eeprom)
{	
	if (flag_eeprom)
	{
		base_threshold = eeprom_read_byte(0);
		flag_eeprom = !flag_eeprom;
	}
}

void indication(const uint8_t& tone, const int& times)
{
	Led(true);
	pwm(tone);
	my_delay_ms(times);
	Led(false);
	pwm(0);
	my_delay_ms(times);
}

void my_delay_ms(int ms)
{
	while (0 < ms)
	{
		_delay_ms(1);
		--ms;
	}
}

void interrupt_setup()
{
	GIMSK |= (1 << PCIE);
	PCMSK |= (1 << PCINT2);
	sei();
}

ISR(PCINT0_vect) // event for press button
{
	if (!((PINB >> BUTTON) & 1))
	{
		threshold(adc_f, sens_threshold);
	}
}