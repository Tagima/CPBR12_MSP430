// *********************************************************************************
// LIGA/DESLIGA LED VIA BLUETOOTH
//
// Este código seta a comunicação via UART.
// Recebe um valor:
//   - Se a: Liga o LED P1.0
//           Envia "Ligado"
//   - Se b: Desliga o LED P1.0
//           Envia "Desligado"
//
// Tagima, 02/2019
// Baseado nos exemplos de D. Dang (TI)
// *********************************************************************************

#include <msp430.h>

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                       VARIÁVEIS, CONSTANTES, ETC.
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// Atribuição de pinos
#define TXD BIT1    // P1.1 UCA0TXD
#define RXD BIT2    // P1.2 UCA0RXD
#define LED BIT0    // P1.0

// Variáveis
unsigned char rx_uart_byte = 0;     // Caracter recebido
unsigned char rx_uart_flag = 0;     // Indica se tem caracter novo


// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                           PROTÓTIPOS DE FUNÇÃO
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void Clock1MHz_init(void);                      // Inicia o uC em 1MHz
void Pins_init(void);                           // Inicializa todos os pinos

// UART
void Uart_9600_init_1MHz(void);                 // Configura os pinos para RX e TX
void Uart_9600_tx(unsigned char byte);          // Envia 1 byte
void Uart_9600_tx_string(char *string);         // Envia uma string


// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                             FUNÇÃO PRINCIPAL
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void main(void)
{
    WDTCTL = WDTPW | WDTHOLD;		     	// Pára o Watchdog Timer

    Clock1MHz_init();
    Pins_init();
    Uart_9600_init_1MHz();

    while(1)                                    // Loop infinito
    {
        __bis_SR_register(CPUOFF + GIE);        // Entra em LPM0, Interrupções habilitadas

        if (rx_uart_flag)                       // Se rx_uart_flag for verdadeiro, então temos 1 byte a ser lido
        {
            if(rx_uart_byte == 'a')
            {
                P1OUT |= LED;
                Uart_9600_tx_string("Ligado.\n");

            }

            if(rx_uart_byte == 'b')
            {
                P1OUT &= ~LED;
                Uart_9600_tx_string("Desligado.\n");
            }

            rx_uart_flag = 0;                   // Terminamos de processar a mensagem recebida.
        }
    }
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                               FUNCTIONS
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void Clock1MHz_init(void)
{
    if (CALBC1_1MHZ==0xFF)      // Se as constantes de calibração foram apagadas
    {
        while(1);               // TRAVE O PROGRAMA!
    }

    DCOCTL = 0;                 // Seleciona as opções de DCOx e MODx mais baixas
    BCSCTL1 = CALBC1_1MHZ;
    DCOCTL = CALDCO_1MHZ;
}

void Pins_init(void)
{
    // Atribui o pino do LED
    P1DIR |= LED;

    // Atribui os pinos da UART ao USCI_A0
    P1SEL = TXD + RXD;
    P1SEL2 = TXD + RXD;
}


// =============================================================================
// FUNÇÕES DA UART
// =============================================================================

//------------------------------------------------------------------------------
// Configura os pinos para RX e TX
//------------------------------------------------------------------------------
void Uart_9600_init_1MHz(void)
{
    UCA0CTL1 |= UCSSEL_2;           // SMCLK
    UCA0BR0 = 104;                  // 1MHz 9600
    UCA0BR1 = 0;                    // 1MHz 9600
    UCA0MCTL = UCBRS0;              // Modulation UCBRSx = 1
    UCA0CTL1 &= ~UCSWRST;           // Inicia a máquina de estados da USCIA0
    IE2 |= UCA0RXIE;                // Habilita interrupção USCI_A0 RX

    // OBS: Não precisa habilitar UCA0TXIE pois vamos usar pooling nele.
}

//------------------------------------------------------------------------------
// Envia 1 byte
//------------------------------------------------------------------------------
void Uart_9600_tx(unsigned char byte)
{
    while (!(IFG2&UCA0TXIFG));      // USCI_A0 TX buffer está pronto?
                                    // Pooling da flag de interrupção do TX da Uart
                                    // Mas espere... Por que não está no vetor de interrupção?
    UCA0TXBUF = byte;
}

//------------------------------------------------------------------------------
// Envia uma string
//------------------------------------------------------------------------------
void Uart_9600_tx_string(char *string)
{
    while (*string)
    {
        Uart_9600_tx(*string++);
    }
}


// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                               INTERRUPTIONS
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//------------------------------------------------------------------------------
// USCIA0 UART RX
// Interrompe quando recebe um dado
//------------------------------------------------------------------------------
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector = USCIAB0RX_VECTOR
__interrupt void USCIAB0RX_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(USCIAB0RX_VECTOR))) USCIAB0RX_ISR (void)
#else
#error Compiler not supported!
#endif
{
    if (IFG2 & UCA0RXIFG)                   // Checamos IFG2 para ver se UCA0RXIFG é setado.
    {
        rx_uart_byte = UCA0RXBUF;           // Atribui o buffer UCA0RXBUF à rx_uart_byte
        rx_uart_flag = 1;                   // Como tinha algo no buffer, indicamos que tem um caracter novo

        IFG2 &= ~UCA0RXIFG;                 // Agora precisamos zerar a flag de interrupção, para que ela
                                            // possa ser setada de novo quando tiver uma nova interrupção.
        __bic_SR_register_on_exit(CPUOFF);  // Sai do estado de baixa energia
    }
}

//------------------------------------------------------------------------------
// USCI Bug Fix for USCI29 Interrupt
//------------------------------------------------------------------------------
#pragma vector= TRAPINT_VECTOR
__interrupt void TRAPINT_ISR(void)
{
    __no_operation();
}
