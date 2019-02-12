# CPBR12_MSP430
Código apresentado na CPBR12 para comunicação do MSP430 via Bluetooth

Esquema elétrico:

           MSP430G2xx3                   SERIAL
             master                    Bluetooth
   --------------------------          ---------
  |              P1.1/UCA0TXD|------->|RX       |
  |                          |        |         |
  |                          |        |         |
  |              P1.2/UCA0RXD|<-------|TX       |
  |                          |        |         |
