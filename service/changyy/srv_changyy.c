/*
 *  Copyright (C) 2004-2008 Christos Tsantilas
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA  02110-1301  USA.
 */

#include "common.h"
#include "c-icap.h"
#include "service.h"
#include "header.h"
#include "body.h"
#include "simple_api.h"
#include "debug.h"

#define DEBUG_TAG " [changyy] "

int changyy_init_service(ci_service_xdata_t * srv_xdata, struct ci_server_conf *server_conf);
int changyy_check_preview_handler(char *preview_data, int preview_data_len, ci_request_t *);
int changyy_end_of_data_handler(ci_request_t * req);
void *changyy_init_request_data(ci_request_t * req);
void changyy_close_service();
void changyy_release_request_data(void *data);
int changyy_io(char *wbuf, int *wlen, char *rbuf, int *rlen, int iseof, ci_request_t * req);

CI_DECLARE_MOD_DATA ci_service_module_t service = {
	"changyy",			// mod_name, The module name
	"changyy service",		// mod_short_descr,  Module short description
	ICAP_RESPMOD /*| ICAP_REQMOD*/,	// mod_type, The service type is responce or request modification
	changyy_init_service,		// mod_init_service. Service initialization
	NULL,				// post_init_service. Service initialization after c-icap configured. Not used here
	changyy_close_service,		// mod_close_service. Called when service shutdowns.
	changyy_init_request_data,	// mod_init_request_data
	changyy_release_request_data,	// mod_release_request_data
	changyy_check_preview_handler,	// mod_check_preview_handler
	changyy_end_of_data_handler,	// mod_end_of_data_handler
	changyy_io,			// mod_service_io
	NULL,
	NULL
};

// The changyy_req_data structure will store the data required to serve an ICAP request.
struct changyy_req_data {
	// the body data
	//ci_ring_buf_t *body;
	//
	ci_simple_file_t *body_file;
	// flag for marking the eof
	int eof;
};

// This function will be called when the service loaded 
// http://tools.ietf.org/html/rfc3507
int changyy_init_service(ci_service_xdata_t * srv_xdata, struct ci_server_conf *server_conf)
{
	ci_debug_printf(5, DEBUG_TAG "Initialization of changyy module......\n");
     
	// Tell to the icap clients that we can support up to 1024 size of preview data
	ci_service_set_preview(srv_xdata, 1024);

	// Tell to the icap clients that we support 204 responses
	ci_service_enable_204(srv_xdata);

	// Tell to the icap clients to send preview data for all files
	ci_service_set_transfer_preview(srv_xdata, "*");

	//ci_service_set_transfer_ignore(srv_xdata, "js,css,gif,jpg,jpeg,png,pdf,doc,docx,ppt,pptx,xls,xlsx");
	//ci_service_set_transfer_complete(srv_xdata, "html,htm");

	// Tell to the icap clients that we want the X-Authenticated-User and X-Authenticated-Groups headers which contains the username and the groups in which belongs.
	ci_service_set_xopts(srv_xdata,  CI_XAUTHENTICATEDUSER|CI_XAUTHENTICATEDGROUPS);

	return CI_OK;
}

// This function will be called when the service shutdown
void changyy_close_service() 
{
	ci_debug_printf(5, DEBUG_TAG "Service shutdown!\n");
	// Nothing to do
}

// This function will be executed when a new request for changyy service arrives. This function will initialize the required structures and data to serve the request.
void *changyy_init_request_data(ci_request_t * req)
{
	struct changyy_req_data *changyy_data;

	ci_debug_printf(5, DEBUG_TAG "changyy_init_request_data\n");
	// Allocate memory fot the changyy_data
	changyy_data = malloc(sizeof(struct changyy_req_data));

	// If the ICAP request encuspulates a HTTP objects which contains body data and not only headers allocate a ci_cached_file_t object to store the body data.
	//if (ci_req_hasbody(req))
	//	changyy_data->body = ci_ring_buf_new(4096);
	//else
	//	changyy_data->body = NULL;
	changyy_data->body_file = NULL;

	changyy_data->eof = 0;
	// Return to the c-icap server the allocated data
	return changyy_data;
}

//This function will be executed after the request served to release allocated data
void changyy_release_request_data(void *data)
{
	// The data points to the changyy_req_data struct we allocated in function changyy_init_service
	struct changyy_req_data *changyy_data = (struct changyy_req_data *)data;
	ci_debug_printf(5, DEBUG_TAG "changyy_release_request_data\n");
    
	// if we had body data, release the related allocated data
	//if(changyy_data->body)
	//	ci_ring_buf_destroy(changyy_data->body);
	if(changyy_data->body_file)
		ci_simple_file_destroy(changyy_data->body_file);

	free(changyy_data);
}

int changyy_check_preview_handler(char *preview_data, int preview_data_len, ci_request_t * req)
{
	ci_off_t content_len;
     
	// Get the changyy_req_data we allocated using the  changyy_init_service  function
	struct changyy_req_data *changyy_data = ci_service_data(req);
	ci_debug_printf(5, DEBUG_TAG "changyy_check_preview_handler\n");

	// If there are is a Content-Length header in encupsulated Http object read it and display a debug message (used here only for debuging purposes)
	content_len = ci_http_content_length(req);
	//ci_debug_printf(9, "We expect to read :%" PRINTF_OFF_T " body data\n", (CAST_OFF_T) content_len);
	ci_debug_printf(5, DEBUG_TAG "We expect to read :%" PRINTF_OFF_T " body data\n", (CAST_OFF_T) content_len);

	// If there are not body data in HTTP encapsulated object but only headers respond with Allow204 (no modification required) and terminate here the ICAP transaction
	if(!ci_req_hasbody(req))
		return CI_MOD_ALLOW204;

	int iscompressed, filetype;
	filetype = ci_magic_req_data_type(req, &iscompressed);
	ci_debug_printf(5, DEBUG_TAG " filetype:%d, iscompressed:%d\n", filetype, iscompressed);

	changyy_data->body_file = ci_simple_file_new(0);
        ci_simple_file_lock_all(changyy_data->body_file);
	if( !changyy_data->body_file )
		return CI_ERROR;

	// Unlock the request body data so the c-icap server can send data before all body data has received
	//ci_req_unlock_data(req);

	// If there are not preview data tell to the client to continue sending data (http object modification required).
	if (!preview_data_len)
		return CI_MOD_CONTINUE;

	// In most real world services we should decide here if we must modify/process or not the encupsulated HTTP object and return CI_MOD_CONTINUE or CI_MOD_ALLOW204 respectively. The decision can be taken examining the http object headers or/and the preview_data buffer.

	// In this example service we just use the whattodo variable to decide if we want to process or not the HTTP object.

	char *value = NULL;
	if( (value = ci_http_request(req) ) )
		ci_debug_printf(5, DEBUG_TAG "URL: %s\n", value);
	if( (value = ci_http_request_get_header(req,"Cookie") ) )
		ci_debug_printf(5, DEBUG_TAG "Cookie: %s\n", value);
	if( (value = ci_http_request_get_header(req,"Content-Type") ) )
		ci_debug_printf(5, DEBUG_TAG "Content-Type: %s\n", value);
	//if( (value = ci_http_request_get_header(req,"Content-Encoding") ) )
	//	ci_debug_printf(5, DEBUG_TAG "Content-Encoding: %s\n", value);
	if( req->request_header && (value = ci_headers_value(req->response_header, "Content-Type")) )
		ci_debug_printf(5, DEBUG_TAG "Content-Type: %s\n", value);

	int whattodo = 0;
	if (whattodo == 0) {
		ci_debug_printf(5, DEBUG_TAG "changyy service will process the request\n");

		// if we have preview data and we want to proceed with the request processing we should store the preview data. There are cases where all the body data of the encapsulated HTTP object included in preview data. Someone can use the ci_req_hasalldata macro to  identify these cases
		if (preview_data_len) {
			//ci_ring_buf_write(changyy_data->body, preview_data, preview_data_len);
			changyy_data->eof = ci_req_hasalldata(req);

			if (ci_simple_file_write(changyy_data->body_file, preview_data, preview_data_len, ci_req_hasalldata(req)) == CI_ERROR)
				return CI_ERROR;
		}
		return CI_MOD_CONTINUE;
	} else {

		// Nothing to do just return an allow204 (No modification) to terminate here the ICAP transaction
		ci_debug_printf(5, DEBUG_TAG "Allow 204...\n");
		return CI_MOD_ALLOW204;
	}
}

/* This function will called if we returned CI_MOD_CONTINUE in  changyy_check_preview_handler
 function, after we read all the data from the ICAP client*/
int changyy_end_of_data_handler(ci_request_t * req)
{
	struct changyy_req_data *changyy_data = ci_service_data(req);
	ci_debug_printf(5, DEBUG_TAG "changyy_end_of_data_handler\n");
	//mark the eof
	changyy_data->eof = 1;
	//and return CI_MOD_DONE

	ci_req_unlock_data(req);

	ci_simple_file_unlock_all(changyy_data->body_file);
	ci_debug_printf(5, DEBUG_TAG "ci_simple_file_size(changyy_data->body_file): %ld\n", ci_simple_file_size(changyy_data->body_file) );
	
	return CI_MOD_DONE;
}

// This function will called if we returned CI_MOD_CONTINUE in  changyy_check_preview_handler function, when new data arrived from the ICAP client and when the ICAP client is ready to get data.
int changyy_io(char *wbuf, int *wlen, char *rbuf, int *rlen, int iseof, ci_request_t * req)
{
	int ret;
	struct changyy_req_data *changyy_data = ci_service_data(req);
	ci_debug_printf(5, DEBUG_TAG "changyy_io, finish:%d, rbuf length:%d, wbuf length:%d\n", iseof, *rlen, *wlen);
	ret = CI_OK;

	// write the data read from icap_client to the changyy_data->body
	if(rlen && rbuf) {
		//*rlen = ci_ring_buf_write(changyy_data->body, rbuf, *rlen);
		*rlen = ci_simple_file_write(changyy_data->body_file, rbuf, *rlen, iseof);
		if (*rlen < 0)
			//ret = CI_ERROR;
			ret = CI_OK;
	} else if(iseof) {
		if(ci_simple_file_write(changyy_data->body_file, NULL, 0, iseof)==CI_ERROR)
			return CI_ERROR;
	}

	// read some data from the changyy_data->body and put them to the write buffer to be send to the ICAP client
	if (wbuf && wlen) {
		//*wlen = ci_ring_buf_read(changyy_data->body, wbuf, *wlen);
		*wlen = ci_simple_file_read(changyy_data->body_file, wbuf, *wlen);
	}
	if(*wlen==0 && changyy_data->eof==1)
		*wlen = CI_EOF;

	return ret;
}
