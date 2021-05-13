## Force feedback wheel decoder

'''
This decoder stacks on top of the 'uart' PD and decodes the ForceFeedback protocol.

this is layered on top of the UART (async serial) protocol, with a fixed
baud rate of 31250 baud.
'''

from .pd import Decoder
