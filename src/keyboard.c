#include "keyboard.h"
#include "midi.h"
#include "../printf/printf.h"

#include <ti/devices/msp432p4xx/driverlib/driverlib.h>


static uint8_t keys[KEYBOARD_NB_KEYS/8+1];
static uint8_t old_keys[KEYBOARD_NB_KEYS/8+1];
static uint8_t velocity = MIDI_STANDARD_VELOCITY;
static uint8_t channel = 1;


/* SPI Master Configuration Parameter */
const eUSCI_SPI_MasterConfig spiMasterConfig =
{
        EUSCI_B_SPI_CLOCKSOURCE_ACLK,              // ACLK Clock Source
        32768,                                     // ACLK = LFXT = 32.768khz
        500000,                                    // SPICLK = 500khz
        EUSCI_B_SPI_MSB_FIRST,                     // MSB First
        EUSCI_B_SPI_PHASE_DATA_CAPTURED_ONFIRST_CHANGED_ON_NEXT,    // Phase
        EUSCI_B_SPI_CLOCKPOLARITY_INACTIVITY_LOW, // low polarity
        EUSCI_B_SPI_3PIN                           // 3Wire SPI Mode
};


void keyboard_init(void)
{
	midi_init();

	/* Starting and enabling LFXT (32kHz) */
	GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_PJ, GPIO_PIN0 | GPIO_PIN1, GPIO_PRIMARY_MODULE_FUNCTION);
	CS_setExternalClockSourceFrequency(32768, 0);
	CS_initClockSignal(CS_ACLK, CS_LFXTCLK_SELECT, CS_CLOCK_DIVIDER_1);
	CS_startLFXT(CS_LFXT_DRIVE0);

	/* Selecting P5.0 as PL */
	GPIO_setAsOutputPin(GPIO_PORT_P5, GPIO_PIN0);
	GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN0);

	/* SPI --> P5.2 = CE, P1.5 = CLK, P1.6 = MOSI & P1.7 = MISO */
	GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P1, GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7, GPIO_PRIMARY_MODULE_FUNCTION);
	GPIO_setAsOutputPin(GPIO_PORT_P5, GPIO_PIN2);
	GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN2);

	/* Configuring SPI in 3-wire master mode & enabling it & interrupts */
    SPI_initMaster(EUSCI_B0_BASE, &spiMasterConfig);
    SPI_enableModule(EUSCI_B0_BASE);

    /* Delaying waiting for the module to initialize */
    for(int i=0;i<100;i++);

    /* Reset old keys */
    for (int i=0; i<(KEYBOARD_NB_KEYS/8+1); i++)
    	old_keys[i] = 0x00;

}


void keyboard_scan(uint8_t *keys, int n)
{
    /* Load keys by toggling PL */
    GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN0);
    for(int i=0;i<10;i++);
    GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN0);

    /* CE = 0 */
    GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN2);

    while(n--)
    {
    	SPI_transmitData(EUSCI_B0_BASE, 0);
    	// ...wait...
    	while(UCB0STATW & BIT0);
    	// -->receive
    	keys[n] = ~SPI_receiveData(EUSCI_B0_BASE);
    }

    /* CE = 1 */
    GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN2);
}


void do_keyboard(void)
{
	uint8_t mask;
	uint8_t idx;
	uint8_t bit;
	uint8_t change;
	uint8_t note;

	while (1) {

//	    if (!queue_empty(&rx_queue)) {
//	        uint8_t c = queue_pop_front(&rx_queue);
//
//	        if (c & 0x80) {
//
//	        }
//	    }

	    keyboard_scan(keys, KEYBOARD_NB_KEYS/8+1);

	    idx = 0;

	    for (int i=0; i<(KEYBOARD_NB_KEYS/8+1); i++) {

	        change = old_keys[idx] ^ keys[idx];

	        if (change) {
	            mask = 0x01;

	            for (bit=0; bit<8; bit++) {
	                if (change & mask) {
	                    note = i*8+bit+KEYBOARD_NOTE_OFFSET;

	                    if (keys[idx] & mask) {
	                        // Note on
	                        midi_noteon(channel, note, velocity);
//                              uart_printf(EUSCI_A0_BASE, "Note on %u %u %u\r\n", channel, note, velocity);
	                    } else {
	                        // Note off
	                        midi_noteoff(channel, note, velocity);
//                              uart_printf(EUSCI_A0_BASE, "Note off %u %u %u\r\n", channel, note, velocity);
	                    }
	                }

	                mask = mask << 1;
	            }
			}

	        old_keys[idx] = keys[idx];
	        idx++;
	    }
	}
}


