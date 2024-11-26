#include "src/httpd_basic_auth.h"
#include "mbedtls/base64.h"
 
#include <esp_log.h>

static const char *TAG = "httpd_basic_auth";

esp_err_t httpd_basic_auth_resp_send_401(httpd_req_t* req) {
	esp_err_t ret = httpd_resp_set_status(req, HTTPD_401);
	
    ESP_LOGD(TAG, "httpd_basic_auth_resp_send_401");
	if(ret == ESP_OK) {
		ret = httpd_resp_set_hdr(req, "WWW-Authenticate", "Basic realm=\"User Visible Realm\"");
	}

	return ret;
}

esp_err_t httpd_basic_auth(httpd_req_t* req, const char* username, const char* password) {
	size_t auth_head_len = 1 + httpd_req_get_hdr_value_len(req, "Authorization");
	size_t n = 0;
	size_t out;
	
	ESP_LOGD(TAG, "httpd_basic_auth");

	if(auth_head_len <= 1 + 7) {

		ESP_LOGE(TAG, "ESP_ERR_HTTPD_BASIC_AUTH_HEADER_NOT_FOUND");
		return ESP_ERR_HTTPD_BASIC_AUTH_HEADER_NOT_FOUND;
	}

	char* auth_head = (char*)malloc(auth_head_len);

	if(auth_head == NULL) {
		return ESP_ERR_NO_MEM;
	}

	if(httpd_req_get_hdr_value_str(req, "Authorization", auth_head, auth_head_len) != ESP_OK) {
		free(auth_head);
		return ESP_ERR_HTTPD_BASIC_AUTH_FAIL_TO_GET_HEADER;
	}

	ESP_LOGD(TAG, "Header: '%s'", auth_head);

	if(strncmp("Basic", auth_head, 5) != 0) {
		free(auth_head);
		return ESP_ERR_HTTPD_BASIC_AUTH_HEADER_INVALID;
	}
	
	mbedtls_base64_decode(NULL, 0, &n, (const unsigned char *)(auth_head + 6), auth_head_len - 6 - 1);

	unsigned char* decoded = (unsigned char*) calloc(1, 6 + n + 1);

    if (decoded) {
        strcpy((char*)decoded, "Basic ");
		int err = mbedtls_base64_decode(decoded, n, &out, (const unsigned char *)(auth_head + 6), auth_head_len - 6 - 1);
        if(err != 0) {
           free(auth_head);
		   free(decoded);
		   ESP_LOGE(TAG, "ESP_ERR_HTTPD_BASIC_AUTH_HEADER_INVALID");
		   return ESP_ERR_HTTPD_BASIC_AUTH_HEADER_INVALID;
	    }
   }

	free(auth_head);
	
	char* colonDelimiter = strchr((const char*) decoded, ':');

	if(colonDelimiter == NULL) {
		free(decoded);
		ESP_LOGE(TAG, "ESP_ERR_HTTPD_BASIC_AUTH_HEADER_INVALID2");

		return ESP_ERR_HTTPD_BASIC_AUTH_HEADER_INVALID;
	}

	size_t head_username_len = colonDelimiter - (const char*) decoded;
	size_t head_password_len = strlen(colonDelimiter + 1);

	if(strlen(username) != head_username_len
		|| strlen(password) != head_password_len
		|| strncmp(username, (const char*) decoded, head_username_len) != 0 
		|| strncmp(password, colonDelimiter + 1, head_password_len) != 0) {
		free(decoded);
		ESP_LOGD(TAG, "ESP_ERR_HTTPD_BASIC_AUTH_NOT_AUTHORIZED");
		return ESP_ERR_HTTPD_BASIC_AUTH_NOT_AUTHORIZED;
	}

	free(decoded);
	
	return ESP_OK;
}