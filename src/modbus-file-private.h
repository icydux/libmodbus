#ifndef MODBUS_FILE_PRIVATE_H
#define MODBUS_FILE_PRIVATE_H

typedef struct _modbus_file {
	char *request_filename;
	uint16_t t_id;
	int fd_request;
	int fd_response;
} modbus_file_t;

#endif /* MODBUS_FILE_PRIVATE_H */