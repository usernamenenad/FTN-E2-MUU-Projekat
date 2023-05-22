#include <display.h>

#define GR_PREKIDAC_1 P0_0
#define GR_PREKIDAC_2 P0_1
#define POTPATOSNI_PREKIDAC_1 P0_2
#define POTPATOSNI_PREKIDAC_2 P0_3
#define RESTART P0_4

#define MOTOR_UP P2_0
#define MOTOR_DOWN P2_1
#define DIODE P2_7

// -- Struktura za prekidace -- // 
typedef struct switch_st {

	unsigned char PHS;
	unsigned char CHS;
	unsigned char CSS;
	unsigned char CS;
	unsigned char PS;

}SWITCH;

SWITCH GR_1;
SWITCH GR_2;
SWITCH POT_1;
SWITCH POT_2;
SWITCH RST;
// -------------------------- //

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

void checkCurrentState(SWITCH* sw, unsigned char* counter) {
	
	if(sw->CHS == sw->PHS) {
		if(++(*counter) == 0x05) {
			*counter = 0x00;
			sw->CSS = sw->CHS;
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
	
	GR_1.CHS    = GR_PREKIDAC_1;
	GR_2.CHS    = GR_PREKIDAC_2;
	POT_1.CHS   = POTPATOSNI_PREKIDAC_1;
	POT_2.CHS   = POTPATOSNI_PREKIDAC_2;
	RST.CHS     = RESTART;

	checkCurrentState(&GR_1, &counter_GR_1);
	checkCurrentState(&GR_2, &counter_GR_2);
	checkCurrentState(&POT_1, &counter_POT_1);
	checkCurrentState(&POT_2, &counter_POT_2);
	checkCurrentState(&RST, &counter_RESTART);

	GR_1.PHS    = GR_1.CHS;
	GR_2.PHS    = GR_2.CHS;
	POT_1.PHS   = POT_1.CHS;
	POT_2.PHS   = POT_2.CHS;
	RST.PHS     = RST.CHS;			
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

void initSwitch(SWITCH* sw) {
	
	if(sw == &GR_1) {
		sw->PHS = 1;
		sw->PS  = 1;
		sw->CHS = 1;
		sw->CSS = 1;
		sw->CS  = 1;
		return;
	}
	
	sw->PHS = 0;
	sw->PS  = 0;
	sw->CHS = 0;
	sw->CSS = 0;
	sw->CS  = 0; 
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

	initSwitch(&GR_1);
	initSwitch(&GR_2);
	initSwitch(&POT_1);
	initSwitch(&POT_2);
	initSwitch(&RST);

	EA = 1;
} 

void main(void) {
	
	initController();
	
	while(1) {
		
		GR_1.CS  = GR_1.CSS;
		GR_2.CS  = GR_2.CSS;
		POT_1.CS = POT_1.CSS;
		POT_2.CS = POT_2.CSS;
		RST.CS   = RST.CSS; 

		if(!emergency_stop) {
			// Ako je ukljucen jedan od potpatosnih prekidaca
			if((POT_1.CS > POT_1.PS || POT_2.CS > POT_2.CSS) || (request_in || request_out)) {
				// Ako je zahtjev za ulazom, ali parking je pun
				if((POT_1.CS > POT_1.PS || request_in) && taken_spots == 0x0F) {
					
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
					if(GR_2.CSS) {
						
						wait_ramp = 1;
						// Obezbjedjen je slucaj kada se, tokom dizanja rampe,
						// pojavi auto sa druge strane. Tada ce rampa cekati 
						// da oba auta sigurno prodju
						while(wait_ramp || POT_1.CSS || POT_2.CSS) {
							if(POT_1.CS > POT_1.PS && POT_2.CSS) {in_out_flag = 1;}
							if(POT_2.CS > POT_2.PS && POT_1.CSS) {in_out_flag = 1;}
						}
						wait1s();
					}
					// Ako rampa nije uspjesno podignuta, daje se izvjestaj o gresci
					else {emergency_stop = 1; displayState(); continue;}
					if(!in_out_flag) {
						// Ako je bio zahjev za ulaz, inkrementira se broj zauzetih mjesta
						if(POT_1.CS > POT_1.PS || request_in) {
							if(++taken_spots == 0x0F) {all_filled = 1;}
						}
						// Ako je bio zahtjev za izlaz, dekrementira se broj zauzetih mjesta
						if(POT_2.CS > POT_2.PS || request_out) {
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
					if(!GR_1.CSS) {emergency_stop = 1; displayState(); continue;}
	
					request_in = 0;
					request_out = 0;
					in_out_flag = 0;
				}
			}
		}
		else {
			// Ako je prijavljena greska, ceka se pritisak na taster RESTART
			// kako bi se rampa ponovo aktivirala
			if(RST.CS > RST.PS) {
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

				if(POT_1.CSS && !request_in)  {request_in = 1;}
				if(POT_2.CSS && !request_out) {request_out = 1;} 

				emergency_stop = 0;
				displayState();
			}
		}
		
		GR_1.PS  = GR_1.CS;
		GR_2.PS  = GR_2.CS;
		POT_1.PS = POT_1.CS;
		POT_2.PS = POT_2.CS;
		RST.PS   = RST.CS;
	}	
}