#ifndef MODBUS_FILE_H
#define MODBUS_FILE_H

#include "modbus.h"

#define _MODBUS_RESPONSE_FILENAME	"response.bin"

#define MODBUS_FILE_SLAVE	0xFF

/* Modbus_Application_Protocol_V1_1b.pdf Chapter 4 Section 1 Page 5
 * TCP MODBUS ADU = 253 bytes + MBAP (7 bytes) = 260 bytes
 */
#define MODBUS_FILE_MAX_ADU_LENGTH	260

MODBUS_API modbus_t* modbus_file_open_request(const char *input_file);
MODBUS_API static int modbus_file_open_response(modbus_t *ctx);

#endif /* MODBUS_FILE_H */