#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <modbus.h>
# include <sys/socket.h>

enum {
	TCP,
	TCP_PI,
	RTU
};

int main(int argc, char*argv[])
{
	modbus_t *ctx;
	modbus_mapping_t *mb_mapping;
	int rc;
	int i;
	uint8_t *query;
	const uint16_t UT_BITS_ADDRESS = 0x0;
	const uint16_t UT_BITS_NB = 0x8;
	const uint8_t UT_BITS_TAB[] = { 0x19 };

	if (argc > 1) {
		mb_mapping = modbus_mapping_new_start_address(
		UT_BITS_ADDRESS, UT_BITS_NB,
		0x0, 0x0,
		0x0, 0x0,
		0x0, 0x0);
		if (mb_mapping == NULL) {
			return -1;
		}
		modbus_set_bits_from_bytes(mb_mapping->tab_input_bits, 0, UT_BITS_NB,UT_BITS_TAB);
		ctx = modbus_file_open_request(argv[0]);
		if (ctx != NULL){
			query = malloc(MODBUS_TCP_MAX_ADU_LENGTH);
			rc = modbus_receive(ctx, query);
			if (rc == -1){
				modbus_close(ctx);
				modbus_free(ctx);
				return -1;
			}
			rc = modbus_file_open_response(ctx);
			if (rc == -1 ){
				modbus_close(ctx);
				modbus_free(ctx);
				return -1;
			}
			rc = modbus_reply(ctx, query, rc, mb_mapping);
			if (rc == -1) {
				modbus_close(ctx);
				modbus_free(ctx);
				return -1;
			}
			modbus_close(ctx);
			modbus_free(ctx);
			modbus_mapping_free(mb_mapping);
			free(query);
			return 0;
		}
	}
	else{
		printf("Please a provide an argument\n");
		return 0;
	}
}
