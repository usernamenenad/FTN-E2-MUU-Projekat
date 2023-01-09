#include <display.h>

#define GR_PREKIDAC_1 P0_0
#define GR_PREKIDAC_2 P0_1
#define POTPATOSNI_PREKIDAC_1 P0_2
#define POTPATOSNI_PREKIDAC_2 P0_3
#define RESTART P0_4

#define MOTOR_UP P2_0
#define MOTOR_DOWN P2_1
#define DIODE P2_7

// -- Debouncing promjenljive -- //
unsigned char PHS_GR_1    = 1;
unsigned char PHS_GR_2    = 0;
unsigned char PHS_POT_1   = 0;
unsigned char PHS_POT_2   = 0;
unsigned char PHS_RESTART = 0;

unsigned char CHS_GR_1    = 1;
unsigned char CHS_GR_2    = 0;
unsigned char CHS_POT_1   = 0;
unsigned char CHS_POT_2   = 0;
unsigned char CHS_RESTART = 0;

unsigned char CSS_GR_1    = 1;
unsigned char CSS_GR_2    = 0;
unsigned char CSS_POT_1   = 0;
unsigned char CSS_POT_2   = 0;
unsigned char CSS_RESTART = 0;
// ----------------------------- //


// -- Counteri -- //
unsigned char counter_GR_1    = 0x00;
unsigned char counter_GR_2    = 0x00;
unsigned char counter_POT_1   = 0x00;
unsigned char counter_POT_2   = 0x00;
unsigned char counter_RESTART = 0x00;
unsigned int counter_3s       = 0x00;
unsigned int counter_5s       = 0x00;
unsigned char buffer_counter  = 0x00;
// ------------- //

// -- Flagovi -- //
bit emergency_stop = 0;
bit diode_flag     = 0;
bit move_ramp      = 0;
bit wait_ramp      = 0;
bit all_filled     = 0;
bit request_in     = 0;
bit request_out    = 0;
bit in_out_flag    = 0;
// ------------- //

unsigned char taken_spots =    0x00;
unsigned char buffer[70];

void checkCurrentState(unsigned char* CHS, unsigned char* PHS, unsigned char* CSS, unsigned char* counter) {
	
	if(*CHS == *PHS) {
		if(++(*counter) == 0x05) {
			*counter = 0x00;
			*CSS = *CHS;
			return;
		}
	}
	else {
		*counter = 0x00;	
	}				
}

void timer_1_interrupt(void) interrupt 3{

	TH1 = 0xEE;
	TL1 = 0x00;
	
	if(move_ramp) {
		
		switch(++counter_3s) {
			case 150: case 300: case 450:
				DIODE = ~DIODE;
				break;
			case 600:
				move_ramp = 0;
				counter_3s = 0x00;
				DIODE = 1;
				break;
		}		
	}
	
	if(wait_ramp) {
		if(++counter_5s == 1000) {
			wait_ramp = 0; counter_5s = 0x00;
		}	
	}	
	
	CHS_GR_1    = GR_PREKIDAC_1;
	CHS_GR_2    = GR_PREKIDAC_2;
	CHS_POT_1   = POTPATOSNI_PREKIDAC_1;
	CHS_POT_2   = POTPATOSNI_PREKIDAC_2;
	CHS_RESTART = RESTART;

	checkCurrentState(&CHS_GR_1, &PHS_GR_1, &CSS_GR_1, &counter_GR_1);
	checkCurrentState(&CHS_GR_2, &PHS_GR_2, &CSS_GR_2, &counter_GR_2);
	checkCurrentState(&CHS_POT_1, &PHS_POT_1, &CSS_POT_1, &counter_POT_1);
	checkCurrentState(&CHS_POT_2, &PHS_POT_2, &CSS_POT_2, &counter_POT_2);
	checkCurrentState(&CHS_RESTART, &PHS_RESTART, &CSS_RESTART, &counter_RESTART);

	PHS_GR_1    = CHS_GR_1;
	PHS_GR_2    = CHS_GR_2;
	PHS_POT_1   = CHS_POT_1;
	PHS_POT_2   = CHS_POT_2;
	PHS_RESTART = CHS_RESTART;			
}

void uart_interrupt(void) interrupt 4{

	if(TI) {
		TI = 0;
		if(*(buffer + buffer_counter) != '\0') {
			SBUF = *(buffer + buffer_counter++);
		}
		else {
			buffer_counter = 0x00;
			*buffer = '\0';
		}
	}
} 

void displayState(void) {
	
	clearDisplay();
	if(!emergency_stop) {
		writeLine("PR");
	}
	else {
		writeLine("PNR");
	}
	newLine();
	writeLine("BZM: ");
	if(taken_spots < 0x09) {writeChar('0');}
	else                   {writeChar('1');}
	
	writeChar(taken_spots%10 + 0x30);

	{
		unsigned char temp[] = "Stanje: \0";
		unsigned char temp_counter = 0x00;
		while(*(temp + temp_counter) != '\0') {
			*(buffer + buffer_counter++) = *(temp + temp_counter++);
		}
	}

	{
		if(!emergency_stop) {
			unsigned char temp[] = "Radi.\n\0";
			unsigned char temp_counter = 0x00;
			while (*(temp + temp_counter) != '\0') {
				*(buffer + buffer_counter++) = *(temp + temp_counter++);
			}		
		}
		else {
		  	unsigned char temp[] = "Ne radi.\n\0";
			unsigned char temp_counter = 0x00;
			while (*(temp + temp_counter) != '\0') {
				*(buffer + buffer_counter++) = *(temp + temp_counter++);
			}	
		}
	}

	{
		unsigned char temp[] = "Trenutno na parkingu: \0";
		unsigned char temp_counter = 0x00;
		while (*(temp + temp_counter) != '\0') {
				*(buffer + buffer_counter++) = *(temp + temp_counter++);
		}
	}
	
	{
		if(taken_spots < 0x09) {
			*(buffer + buffer_counter++) = '0';
		}
		else {
			*(buffer + buffer_counter++) = '1';
		}
		*(buffer + buffer_counter++) = taken_spots%10 + 0x30;
		*(buffer + buffer_counter++) = '\n';
	}
	
	{
		unsigned char temp[] = "---------------------\0";
		unsigned char temp_counter = 0x00;
		while (*(temp + temp_counter) != '\0') {
				*(buffer + buffer_counter++) = *(temp + temp_counter++);
		}
	}

	*(buffer + buffer_counter++) = '\n';
	*(buffer + buffer_counter) = '\0';
	buffer_counter = 0x00;
	SBUF = *(buffer + buffer_counter++);
}

void initController(void) {

	EA = 0;

	initDisplay();

	// -- Potrebe simulacije, podrazumijevane vrijednosti prekidaca na pocetku -- //

	GR_PREKIDAC_1 = 0x01;
	GR_PREKIDAC_2 = 0x00;
	POTPATOSNI_PREKIDAC_1 = 0x00;
	POTPATOSNI_PREKIDAC_2 = 0x00;
	RESTART = 0x00;

	// -- Inicijalizacija 8-bit UART komunikacije sa promjenljivim baud-rate-om -- //

	SCON = 0x50;
	PCON = (PCON & 0x3F) | 0x80;
   	BDRCON = (BDRCON & 0xE0) | 0x1C;
	BRL = 0xFD;
	ES = 1;

	// -- Inicijalizacija tajmera 1 sa 16-bit modom i prekidom na svakih 5ms -- //

	TH1 = 0xEE;
	TL1 = 0x00;
	TMOD = (TMOD & 0x0F) | 0x10;
	ET1 = 1;
	TR1 = 1;
	ET1 = 1;

	displayState();
	
	P2 = 0x00;	
	DIODE = 1;

	EA = 1;
} 

void main(void) {
	
	unsigned char CS_GR_1;
	unsigned char CS_GR_2;
	unsigned char CS_POT_1;
	unsigned char CS_POT_2;
	unsigned char CS_RESTART;

	unsigned char PS_GR_1;
	unsigned char PS_GR_2;
	unsigned char PS_POT_1;
	unsigned char PS_POT_2;
	unsigned char PS_RESTART;

	initController();
	
	while(1) {
		
		CS_GR_1    = CSS_GR_1;
		CS_GR_2    = CSS_GR_2;
		CS_POT_1   = CSS_POT_1;
		CS_POT_2   = CSS_POT_2;
		CS_RESTART = CSS_RESTART;

		if(!emergency_stop) {
			// Ako je ukljucen jedan od potpatosnih prekidaca
			if((CS_POT_1 > PS_POT_1 || CS_POT_2 > PS_POT_2) || (request_in || request_out)) {
				// Ako je zahtjev za ulazom, ali parking je pun
				if((CS_POT_1 > PS_POT_1 || request_in) && taken_spots == 0x0F) {
					
					// Ispis da ne postoji vise slobodnih mjesta
					{
						unsigned char temp[] = "Parking zauzet.\n\0";
						unsigned char temp_counter = 0x00;
						while(*(temp + temp_counter) != '\0') {
							*(buffer + buffer_counter++) = *(temp + temp_counter++); 
						}
					}

					{
					 	unsigned char temp[] = "---------------------\n\0";
						unsigned char temp_counter = 0x00;
						while (*(temp + temp_counter) != '\0') {
							*(buffer + buffer_counter++) = *(temp + temp_counter++);
						}
					}

					*(buffer + buffer_counter) = '\0';
					buffer_counter = 0x00;
					SBUF = *(buffer + buffer_counter++);	
					clearDisplay();
					writeLine("ZAUZ");
					wait1s();
					displayState();					
				}
				else {
					// Iniciranje podizanja rampe
					move_ramp = 1;
					while(move_ramp) {MOTOR_UP = 1;}
					MOTOR_UP = 0;
					wait1s();
	
					// Ako je rampa uspjesno podignuta, aktivira se granicni prekidac 2
					if(CSS_GR_2) {
						
						wait_ramp = 1;
						// Obezbjedjen je slucaj kada se, tokom dizanja rampe,
						// pojavi auto sa druge strane. Tada ce rampa cekati 
						// da oba auta sigurno prodju
						while(wait_ramp || CSS_POT_1 || CSS_POT_2) {
							if(CS_POT_1 > PS_POT_1 && CSS_POT_2) {in_out_flag = 1;}
							if(CS_POT_2 > PS_POT_2 && CSS_POT_1) {in_out_flag = 1;}
						}
						wait1s();
					}
					// Ako rampa nije uspjesno podignuta, daje se izvjestaj o gresci
					else {emergency_stop = 1; displayState(); continue;}
					if(!in_out_flag) {
						// Ako je bio zahjev za ulaz, inkrementira se broj zauzetih mjesta
						if(CS_POT_1 > PS_POT_1 || request_in) {
							if(++taken_spots == 0x0F) {all_filled = 1;}
						}
						// Ako je bio zahtjev za izlaz, dekrementira se broj zauzetih mjesta
						if(CS_POT_2 > PS_POT_2 || request_out) {
							if(--taken_spots == 0x0E) {all_filled = 0;}
						}
					}
					displayState();
	
					// Iniciranje spustanja rampe
					move_ramp = 1;
					while(move_ramp) {MOTOR_DOWN = 1;}
					MOTOR_DOWN = 0;
					wait1s();
	
					// Ako rampa nije uspjesno spustena, daje se izvjestaj o gresci 
					if(!CSS_GR_1) {emergency_stop = 1; displayState(); continue;}
	
					request_in = 0;
					request_out = 0;
					in_out_flag = 0;
				}
			}
		}
		else {
			// Ako je prijavljena greska, ceka se pritisak na taster RESTART
			// kako bi se rampa ponovo aktivirala
			if(CS_RESTART > PS_RESTART) {
				{
					unsigned char temp[] = "Rampa se restartuje...\n\0";
					unsigned char temp_counter = 0x00;
					while(*(temp + temp_counter) != '\0') {
						*(buffer + buffer_counter++) = *(temp + temp_counter++); 
					}
				}

				{
				 	unsigned char temp[] = "---------------------\0";
					unsigned char temp_counter = 0x00;
					while (*(temp + temp_counter) != '\0') {
						*(buffer + buffer_counter++) = *(temp + temp_counter++);
					}
				}
				*(buffer + buffer_counter++) = '\n';
				*(buffer + buffer_counter) = '\0';
				buffer_counter = 0x00;
				SBUF = *(buffer + buffer_counter++);

				clearDisplay();
				writeLine("RST...");
				wait1s();

				if(CSS_POT_1) {request_in = 1;}
				if(CSS_POT_2) {request_out = 1;} 
				emergency_stop = 0;
				displayState();
			}
		}
		
		PS_GR_1    = CS_GR_1;		
		PS_GR_2    = CS_GR_2;
		PS_POT_1   = CS_POT_1;
		PS_POT_2   = CS_POT_2;
		PS_RESTART = CS_RESTART;
	}	
}