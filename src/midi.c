#include "midi.h"
#include "queue.h"
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>


/* Transmit queue */
#define TX_QUEUE_SIZE 32
struct queue_t tx_queue;
uint8_t tx_buffer[TX_QUEUE_SIZE];

/* Transmit queue */
#define RX_QUEUE_SIZE 32
struct queue_t rx_queue;
uint8_t rx_buffer[RX_QUEUE_SIZE];


const eUSCI_UART_ConfigV1 midi_uartConfig =
{
        EUSCI_A_UART_CLOCKSOURCE_SMCLK,          // SMCLK Clock Source
        24,                                      // BRDIV = 78
        2,                                       // UCxBRF = 2
        0,                                       // UCxBRS = 0
        EUSCI_A_UART_NO_PARITY,                  // No Parity
        EUSCI_A_UART_LSB_FIRST,                  // LSB First
        EUSCI_A_UART_ONE_STOP_BIT,               // One stop bit
        EUSCI_A_UART_MODE,                       // UART mode
        EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION,  // Oversampling
        EUSCI_A_UART_8_BIT_LEN                   // 8 bit data length
};


/* EUSCI_A2_UART ISR */
void EUSCIA2_IRQHandler(void)
{
    uint32_t status = UART_getEnabledInterruptStatus(EUSCI_A2_BASE);

    if (status & EUSCI_A_UART_RECEIVE_INTERRUPT) {
        queue_push_back(&rx_queue, UART_receiveData(EUSCI_A2_BASE));
    }

    if (status & EUSCI_A_UART_TRANSMIT_INTERRUPT) {
        if (!queue_empty(&tx_queue)) {
            UART_transmitData(EUSCI_A2_BASE, queue_pop_front(&tx_queue));
        } else {
            // Nothing else to send
            UART_disableInterrupt(EUSCI_A2_BASE, EUSCI_A_UART_TRANSMIT_INTERRUPT);
        }
    }
}


void midi_init(void)
{
    /* Initialise the UART RX and TX queues */
    queue_init(&tx_queue);
    queue_init(&rx_queue);
    tx_queue.buffer = tx_buffer;
    tx_queue.size = TX_QUEUE_SIZE;
    rx_queue.buffer = rx_buffer;
    rx_queue.size = RX_QUEUE_SIZE;

    /* Selecting P3.2 and P3.3 in UART mode */
    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P3,
            GPIO_PIN1 | GPIO_PIN2 | GPIO_PIN3, GPIO_PRIMARY_MODULE_FUNCTION);

    /* Configuring UART Module */
    UART_initModule(EUSCI_A2_BASE, &midi_uartConfig);

    /* Enable UART module */
    UART_enableModule(EUSCI_A2_BASE);

    /* Enabling interrupts */
    UART_enableInterrupt(EUSCI_A2_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT);
    Interrupt_enableInterrupt(INT_EUSCIA2);
    Interrupt_enableMaster();
}


void midi_send(uint8_t c)
{
    queue_push_back(&tx_queue, c);

    // Trigger UART TX interrupt
    UART_enableInterrupt(EUSCI_A2_BASE, EUSCI_A_UART_TRANSMIT_INTERRUPT);
}


void midi_noteon(uint8_t ch, uint8_t note, uint8_t vel)
{
    midi_send(MIDI_COMMAND_NOTE_ON | ((ch-1) & 0x0F));
    midi_send(note);
    midi_send(vel);
}


void midi_noteoff(uint8_t ch, uint8_t note, uint8_t vel)
{
    midi_send(MIDI_COMMAND_NOTE_OFF | ((ch-1) & 0x0F));
    midi_send(note);
    midi_send(vel);
}


void midi_all_sound_off()
{
    midi_send(MIDI_COMMAND_CHANNEL_MODE);
    midi_send(120);
    midi_send(0);
}


void midi_reset_all_controllers()
{
//    midi_send(MIDI_COMMAND_CHANNEL_MODE);
//    midi_send(121);
//    midi_send(x);
}


void midi_local_control_on()
{
    midi_send(MIDI_COMMAND_CHANNEL_MODE);
    midi_send(122);
    midi_send(127);
}


void midi_local_control_off()
{
    midi_send(MIDI_COMMAND_CHANNEL_MODE);
    midi_send(122);
    midi_send(0);
}


void midi_all_notes_off()
{
    midi_send(MIDI_COMMAND_CHANNEL_MODE);
    midi_send(MIDI_CHANNEL_MODE_ALL_NOTES_OFF);
    midi_send(0);
}


void midi_omni_mode_off()
{
    midi_send(MIDI_COMMAND_CHANNEL_MODE);
    midi_send(MIDI_CHANNEL_MODE_OMNI_MODE_OFF);
    midi_send(0);
}


void midi_omni_mode_on()
{
    midi_send(MIDI_COMMAND_CHANNEL_MODE);
    midi_send(MIDI_CHANNEL_MODE_OMNI_MODE_ON);
    midi_send(0);
}


void midi_mono_mode_on(uint8_t nchannels)
{
    midi_send(MIDI_COMMAND_CHANNEL_MODE);
    midi_send(MIDI_CHANNEL_MODE_MONO_MODE_ON);
    midi_send(nchannels);
}


void midi_poly_mode_on()
{
    midi_send(MIDI_COMMAND_CHANNEL_MODE);
    midi_send(MIDI_CHANNEL_MODE_POLY_MODE_ON);
    midi_send(0);
}
