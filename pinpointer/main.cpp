#define F_CPU     1000000
#define BUZZER    PB0
#define LED       PB3
#define BUTTON    PB2
#define SIGNAL_IN PB4

#include <avr/io.h>
#include <avr/eeprom.h>
#include <util/delay.h>

int  base_threshold = 0;
const int  sens_threshold = 2;     // ��� ������ �������, ��� ����������, �� ������ ����������������
bool flag_button = false;
int  adc_f;

void Led(bool x);
bool Button();
void adc_setup();
int  adc_read();
void pwm(const int& x);
void setup_port();
void signal_out(int i);
void threshold(int x, const int& y);
void action();


int main(void)
{
	setup_port();
	adc_setup();

    while (1) 
    {
		base_threshold = eeprom_read_word(0);
		adc_f = adc_read();
		action();
		signal_out(adc_f);
    }
}

// ���������� �����������
void Led(bool x)
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

// ������� ������
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
	ADMUX  |= (1 << ADLAR) | (1 << MUX1); // input PB4 / ADC2
	ADCSRB |= (1 << ACME);
	ADCSRA |= (1 << ADPS1); // �������� 4
	ADCSRA |= (1 << ADEN);  // enable ADC
}

int adc_read ()
{
	ADCSRA |= (1 << ADSC);
	while (ADCSRA & (1 << ADSC));
	return ADCH;
}

void pwm(const int& x)
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
	TCCR0B |= (1 << CS00);
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

void signal_out(int i)
{
	// 10 ��� ~125mV     100 ��� ~1150 mV  
	if ((i > 0) && (i < base_threshold))
	{
		Led(true);
		pwm(i + 256 - base_threshold -2);
		_delay_ms(15);
		pwm(i + 256 - base_threshold -200);
		_delay_ms(5);
		pwm(i + 256 - base_threshold -2);
		_delay_ms(10);
		pwm(i + 256 - base_threshold -200);
	}
	else 
	{
		// ��������� led � buzzer
		Led(false);
		pwm(256);
	}
}

void threshold(int x /* input val base*/, const int& y /* -threshold */) // ����������������
{
	x = x - y;
	eeprom_write_word(0 , x);
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
