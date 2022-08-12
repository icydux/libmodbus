#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
# include <sys/socket.h>
# include <sys/ioctl.h>
# include <netinet/in.h>
# include <netinet/ip.h>
# include <netinet/tcp.h>
# include <arpa/inet.h>
# include <netdb.h>

/*#if !defined(MSG_NOSIGNAL)
#define MSG_NOSIGNAL 0
#endif

#if defined(_AIX) && !defined(MSG_DONTWAIT)
#define MSG_DONTWAIT MSG_NONBLOCK
#endif*/

#include "modbus-private.h"
#include "modbus-tcp.h"
#include "modbus-tcp-private.h"


const modbus_backend_t _modbus_file_backend = {
	_MODBUS_BACKEND_TYPE_TCP,
	_MODBUS_TCP_HEADER_LENGTH,
	_MODBUS_TCP_CHECKSUM_LENGTH,
	MODBUS_TCP_MAX_ADU_LENGTH,
	_modbus_file_set_slave,
	_modbus_file_build_request_basis,
	_modbus_file_build_response_basis,
	_modbus_file_prepare_response_tid,
	_modbus_file_send_msg_pre,
	_modbus_file_send,
	_modbus_file_receive,
	_modbus_file_recv,
	_modbus_file_check_integrity,
	_modbus_file_pre_check_confirmation,
	_modbus_file_connect,
	_modbus_file_close,
	_modbus_file_flush,
	_modbus_file_select,
	_modbus_file_free
};

modbus_t* modbus_file_open_request(const char *input_file)
{
	modbus_t *ctx;
	modbus_file_t *ctx_file;
	int fd_request;

	ctx = (modbus_t *)malloc(sizeof(modbus_t));
	if (ctx == NULL) {
		return NULL;
	}
	//_modbus_init_common(ctx);

	/* Could be changed after to reach a remote serial Modbus device */
	ctx->slave = MODBUS_TCP_SLAVE;

	/*Have to construct the _modbus_file_backend array*/	
	ctx->backend = &_modbus_file_backend;

	ctx->backend_data = (modbus_file_t *)malloc(sizeof(modbus_file_t));
	if (ctx->backend_data == NULL) {
		modbus_free(ctx);
		return NULL;
	}
	ctx_file = (modbus_file_t *)ctx->backend_data;
	fd_request = fopen(input_file,"r");

	if (fd_request != -1) {
		ctx_file->fd_request = fd_request;
		return ctx;
	}
	else{
		modbus_free(ctx);
		return NULL;
	}
}
static int modbus_file_open_response(modbus_t *ctx){
	int fd_response;

	fd_response = fopen(_MODBUS_RESPONSE_FILENAME,"w");
	if (fd_response == -1){
		return -1;
	}
	else{
		ctx->backend_data->fd_reponse = fd_response;
		return 0;
	}
}
static int _modbus_file_receive(modbus_t *ctx, uint8_t *req) {
	return _modbus_receive_msg(ctx, req, MSG_INDICATION);
}
static ssize_t _modbus_file_recv(modbus_t *ctx, uint8_t *rsp, int rsp_length) {
	return recv(ctx->backend_data->fd_request, (char *)rsp, rsp_length, 0);
}
static void _modbus_file_free(modbus_t *ctx) {
	free(ctx->backend_data);
	free(ctx);
}
static void _modbus_file_close(modbus_t *ctx){
	int fd_request;
	int fd_response;
	fd_request = ctx->backend_data->fd_request;
	fd_response = ctx->backend_data->fd_response;
	if (fd_request != -1) {
		shutdown(fd_request, SHUT_RDWR);
		close(fd_request);
	}
	if (fd_response != -1) {
		shutdown(fd_response, SHUT_RDWR);
		close(fd_response);
	}
}
static int _modbus_file_check_integrity(modbus_t *ctx, uint8_t *msg, const int msg_length)
{
	return msg_length;
}
static int _modbus_file_prepare_response_tid(const uint8_t *req, int *req_length)
{
	return (req[0] << 8) + req[1];
}
/* Builds a TCP response header */
static int _modbus_file_build_response_basis(sft_t *sft, uint8_t *rsp)
{
	/* Extract from MODBUS Messaging on TCP/IP Implementation
	   Guide V1.0b (page 23/46):
	   The transaction identifier is used to associate the future
	   response with the request. */
	rsp[0] = sft->t_id >> 8;
	rsp[1] = sft->t_id & 0x00ff;

	/* Protocol Modbus */
	rsp[2] = 0;
	rsp[3] = 0;

	/* Length will be set later by send_msg (4 and 5) */

	/* The slave ID is copied from the indication */
	rsp[6] = sft->slave;
	rsp[7] = sft->function;

	return _MODBUS_TCP_PRESET_RSP_LENGTH;
}
static int _modbus_file_send_msg_pre(uint8_t *req, int req_length)
{
	/* Subtract the header length to the message length */
	int mbap_length = req_length - 6;

	req[4] = mbap_length >> 8;
	req[5] = mbap_length & 0x00FF;

	return req_length;
}
static ssize_t _modbus_file_send(modbus_t *ctx, const uint8_t *req, int req_length)
{
	return send(ctx->backend_data->fd_response, (const char *)req, req_length, MSG_NOSIGNAL);
}
static int _modbus_file_build_request_basis(modbus_t *ctx, int function,
										   int addr, int nb,
										   uint8_t *req)
{
	modbus_file_t *ctx_file = ctx->backend_data;

	/* Increase transaction ID */
	if (ctx_file->t_id < UINT16_MAX)
		ctx_file->t_id++;
	else
		ctx_file->t_id = 0;
	req[0] = ctx_file->t_id >> 8;
	req[1] = ctx_file->t_id & 0x00ff;

	/* Protocol Modbus */
	req[2] = 0;
	req[3] = 0;

	/* Length will be defined later by set_req_length_tcp at offsets 4
	   and 5 */

	req[6] = ctx->slave;
	req[7] = function;
	req[8] = addr >> 8;
	req[9] = addr & 0x00ff;
	req[10] = nb >> 8;
	req[11] = nb & 0x00ff;

	return _MODBUS_TCP_PRESET_REQ_LENGTH;
}
static int _modbus_file_pre_check_confirmation(modbus_t *ctx, const uint8_t *req,
											  const uint8_t *rsp, int rsp_length)
{
	/* Check transaction ID */
	if (req[0] != rsp[0] || req[1] != rsp[1]) {
		return -1;
	}

	/* Check protocol ID */
	if (rsp[2] != 0x0 && rsp[3] != 0x0) {
		return -1;
	}

	return 0;
}