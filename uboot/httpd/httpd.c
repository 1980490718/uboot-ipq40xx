#include "uip.h"
#include "httpd.h"
#include "fs.h"
#include "fsdata.h"
#include <malloc.h>
#include "ipq40xx_api.h"

// HTTP headers
#define SERVER_HEADER			"Server: uIP\r\n"
#define X_CONTENT_TYPE_OPTIONS	"X-Content-Type-Options: nosniff\r\n"
#define CACHE_CONTROL_NO_CACHE	"Cache-Control: no-cache\r\n"
#define CONNECTION_CLOSE		"Connection: close\r\n"
#define CONTENT_TYPE			"Content-Type: text/plain; charset=utf-8\r\n"
#define HTTP_200_OK				"HTTP/1.1 200 OK\r\n"
#define HTTP_404_NOT_FOUND		"HTTP/1.1 404 Not Found\r\n"
#define HTTP_500_INTERNAL_ERROR	"HTTP/1.1 500 Internal Server Error\r\n"
#define COMMON_SECURITY_HEADERS X_CONTENT_TYPE_OPTIONS CACHE_CONTROL_NO_CACHE

#define STATE_NONE				0 // empty state (waiting for request...)
#define STATE_FILE_REQUEST		1 // remote host sent GET request
#define STATE_UPLOAD_REQUEST	2 // remote host sent POST request

// ASCII characters
#define ISO_G		0x47 // GET
#define ISO_E		0x45
#define ISO_P		0x50 // POST
#define ISO_O		0x4f
#define ISO_S		0x53
#define ISO_T		0x54
#define ISO_slash	0x2f // control and other characters
#define ISO_space	0x20
#define ISO_nl		0x0a
#define ISO_cr		0x0d
#define ISO_tab		0x09

// we use this so that we can do without the ctype library
#define is_digit(c) ((c) >= '0' && (c) <= '9')

// html files
extern const struct fsdata_file file_index_html;
extern const struct fsdata_file file_404_html;
extern const struct fsdata_file file_flashing_html;
extern const struct fsdata_file file_fail_html;

extern int web_handle_read(char *response_buffer, size_t buffer_size);
extern int web_handle_download(unsigned char **firmware_data, unsigned int *firmware_size);

extern int webfailsafe_ready_for_upgrade;
extern int webfailsafe_upgrade_type;
extern ulong NetBootFileXferSize;
extern unsigned char *webfailsafe_data_pointer;
extern void gpio_twinkle_value(int gpio_num);
extern void wan_led_toggle(void);

int web_macrw_handle(int argc, char **argv, char *resp_buf, int bufsize);
int web_setenv_handle(int argc, char **argv, char *resp_buf, int bufsize);
// http app state
struct httpd_state *hs;

static int webfailsafe_post_done = 0;
static int webfailsafe_upload_failed = 0;
static int data_start_found = 0;
static unsigned char post_packet_counter = 0;
static unsigned char post_line_counter = 0;

// 0x0D -> CR 0x0A -> LF
static const char eol[] = "\r\n";
static const char eol2[] = "\r\n\r\n";
static char *boundary_value;

// str to int
static int atoi(const char *s) {
	int i = 0;
	while (is_digit(*s)) {
		i = i * 10 + *(s++) - '0';
	}
	return (i);
}

// url decode
static int parse_url_args(char *s, int *argc, char **argv, int max_args) {
	int n = 0;
	char *tok = strtok(s, "&");
	while (tok && n < max_args) {
		argv[n++] = tok;
		tok = strtok(NULL, "&");
	}
	*argc = n;
	return n;
}

// print downloading progress
extern void NetSendHttpd(void);
static void httpd_download_progress(void) {
	if (post_packet_counter == 39) {
		puts("\n         ");
		post_packet_counter = 0;
		post_line_counter++;
	}
	if (post_line_counter == 10) {
		wan_led_toggle();
		post_line_counter = 0;
		gpio_twinkle_value(led_tftp_transfer_flashing);
	}
	puts("#");
	post_packet_counter++;
}

// http server init
void httpd_init(void) {
	fs_init();
	uip_listen(HTONS(80));
}

// reset app state
static void httpd_state_reset(void) {
	hs->state = STATE_NONE;
	hs->count = 0;
	hs->dataptr = 0;
	hs->upload = 0;
	hs->upload_total = 0;
	hs->firmware_data = NULL;
	hs->firmware_size = 0;
	hs->firmware_sent = 0;
	hs->is_firmware_download = 0;
	data_start_found = 0;
	post_packet_counter = 0;
	post_line_counter = 0;
	if (boundary_value) {
		free(boundary_value);
		boundary_value = NULL;
	}
	hs->is_backupfw_resp = 0;
	hs->is_macrw_resp = 0;
	hs->backupfw_len = 0;
	hs->macrw_len = 0;
}

// find and get first chunk of data
static int httpd_findandstore_firstchunk(void) {
	char *start = NULL;
	char *end = NULL;
	if (!boundary_value) {
		return (0);
	}
	start = (char *)strstr((char *)uip_appdata, (char *)boundary_value);
	if (start) {
		end = (char *)strstr((char *)start, "name=\"firmware\"");
		if (end) {
			printf("Upgrade type: firmware\n");
			webfailsafe_upgrade_type = WEBFAILSAFE_UPGRADE_TYPE_FIRMWARE;
		}
		else {
			end = (char *)strstr((char *)start, "name=\"uboot\"");
			if (end) {
				webfailsafe_upgrade_type = WEBFAILSAFE_UPGRADE_TYPE_UBOOT;
				printf("Upgrade type: U-Boot\n");
			}
			else {
				end = (char *)strstr((char *)start, "name=\"art\"");
				if (end) {
					printf("Upgrade type: ART\n");
					webfailsafe_upgrade_type = WEBFAILSAFE_UPGRADE_TYPE_ART;
				}
				else {
					end = (char *)strstr((char *)start, "name=\"mibib\"");
					if (end) {
						printf("Upgrade type: MIBIB\n");
						webfailsafe_upgrade_type = WEBFAILSAFE_UPGRADE_TYPE_MIBIB;
					}
					else {
						printf("## Error: input name not found!\n");
						return (0);
					}
				}
			}
		}
		end = NULL;
		end = (char *)strstr((char *)start, eol2);
		if (end) {
			if ((end - (char *)uip_appdata) < uip_len) {
				end += 4;
				hs->upload_total = hs->upload_total - (int)(end - start) - strlen(boundary_value) - 6;
				printf("Upload file size: %d bytes\n", hs->upload_total);
				if ((webfailsafe_upgrade_type == WEBFAILSAFE_UPGRADE_TYPE_UBOOT) && (hs->upload_total > WEBFAILSAFE_UPLOAD_UBOOT_SIZE_IN_BYTES)) {
					printf("## Error: wrong file size, should be less than: %d bytes!\n", WEBFAILSAFE_UPLOAD_UBOOT_SIZE_IN_BYTES);
					webfailsafe_upload_failed = 1;
				}
				else if ((webfailsafe_upgrade_type == WEBFAILSAFE_UPGRADE_TYPE_ART) && (hs->upload_total != WEBFAILSAFE_UPLOAD_ART_SIZE_IN_BYTES)) {
					printf("## Error: wrong file size, should be: %d bytes!\n", WEBFAILSAFE_UPLOAD_ART_SIZE_IN_BYTES);
					webfailsafe_upload_failed = 1;
				}
				else if ((webfailsafe_upgrade_type == WEBFAILSAFE_UPGRADE_TYPE_MIBIB) && (hs->upload_total != WEBFAILSAFE_UPLOAD_MIBIB_SIZE_IN_BYTES)) {
					printf("## Error: wrong file size, should be: %d bytes!\n", WEBFAILSAFE_UPLOAD_MIBIB_SIZE_IN_BYTES);
					webfailsafe_upload_failed = 1;
				}
				printf("Loading: ");
				hs->upload = (unsigned int)(uip_len - (end - (char *)uip_appdata));
				memcpy((void *)webfailsafe_data_pointer, (void *)end, hs->upload);
				webfailsafe_data_pointer += hs->upload;
				httpd_download_progress();
				return (1);
			}
		}
		else {
			printf("## Error: couldn't find start of data!\n");
		}
	}
	return (0);
}

// handle firmware read request
static void handle_read_firmware_request(void) {
	char resp_buf[1024];
	(void)web_handle_read(resp_buf, sizeof(resp_buf));
	static char http_header[] = HTTP_200_OK CONTENT_TYPE SERVER_HEADER COMMON_SECURITY_HEADERS CONNECTION_CLOSE "\r\n";
	hs->is_backupfw_resp = 1;
	hs->backupfw_buf = (u8_t *)malloc(strlen(http_header) + strlen(resp_buf) + 1);
	if (hs->backupfw_buf) {
		strcpy((char *)hs->backupfw_buf, http_header);
		strcat((char *)hs->backupfw_buf, resp_buf);
		hs->backupfw_len = strlen((char *)hs->backupfw_buf);
	}
	else {
		hs->backupfw_buf = (u8_t *)http_header;
		hs->backupfw_len = strlen(http_header);
	}
	hs->dataptr = hs->backupfw_buf;
	hs->upload = hs->backupfw_len;
	uip_send(hs->dataptr, (hs->upload > uip_mss() ? uip_mss() : hs->upload));
}

// handle firmware download request
static void handle_download_firmware_request(void) {
	unsigned char *firmware_data;
	unsigned int firmware_size;
	int result = web_handle_download(&firmware_data, &firmware_size);
	if (result != 0) {
		static char error_resp[] = HTTP_404_NOT_FOUND CONTENT_TYPE SERVER_HEADER COMMON_SECURITY_HEADERS CONNECTION_CLOSE "\r\n" "No firmware loaded";
		hs->is_backupfw_resp = 1;
		hs->backupfw_buf = (u8_t *)error_resp;
		hs->backupfw_len = strlen(error_resp);
		hs->dataptr = hs->backupfw_buf;
		hs->upload = hs->backupfw_len;
		uip_send(hs->dataptr, (hs->upload > uip_mss() ? uip_mss() : hs->upload));
		return;
	}
	char *header_buf = malloc(256);
	if (header_buf) {
		snprintf(header_buf, 256, HTTP_200_OK "Content-Type: application/octet-stream\r\n"
			"Content-Disposition: attachment; filename=\"firmware_backup.bin\"\r\n"
			"Content-Length: %u\r\n" SERVER_HEADER COMMON_SECURITY_HEADERS CONNECTION_CLOSE "\r\n", firmware_size);
		hs->is_backupfw_resp = 1;
		hs->backupfw_buf = (u8_t *)header_buf;
		hs->backupfw_len = strlen(header_buf);
		hs->dataptr = hs->backupfw_buf;
		hs->upload = hs->backupfw_len;
		hs->firmware_data = firmware_data;
		hs->firmware_size = firmware_size;
		hs->firmware_sent = 0;
		hs->is_firmware_download = 1;
		uip_send(hs->dataptr, (hs->upload > uip_mss() ? uip_mss() : hs->upload));
	}
	else {
		static char error_resp[] = HTTP_500_INTERNAL_ERROR CONTENT_TYPE SERVER_HEADER COMMON_SECURITY_HEADERS CONNECTION_CLOSE "\r\n";
		hs->is_backupfw_resp = 1;
		hs->backupfw_buf = (u8_t *)error_resp;
		hs->backupfw_len = strlen(error_resp);
		hs->dataptr = hs->backupfw_buf;
		hs->upload = hs->backupfw_len;
		uip_send(hs->dataptr, (hs->upload > uip_mss() ? uip_mss() : hs->upload));
	}
}

// send http response for macrw request
extern void gpio_set_value(int gpio_num, int value);
static void send_http_response(u8_t *buf, int len) {
	hs->is_macrw_resp = 1;
	hs->macrw_buf = buf;
	hs->macrw_len = len;
	hs->dataptr = hs->macrw_buf;
	hs->upload = hs->macrw_len;
	uip_send(hs->dataptr, (hs->upload > uip_mss() ? uip_mss() : hs->upload));
}

// handle post request
static int handle_post_request(int (*handler)(int, char **, char *, int), char *resp_buf, int bufsize) {
	char *body = strstr((char *)uip_appdata, "\r\n\r\n");
	int resp_len = 0;
	if (body) {
		body += 4;
		int header_len = body - (char *)uip_appdata;
		int body_len = uip_len - header_len;
		char *body_copy = malloc(body_len + 1);
		if (body_copy) {
			memcpy(body_copy, body, body_len);
			body_copy[body_len] = '\0';
			int argc = 0;
			char *argv[10];
			parse_url_args(body_copy, &argc, argv, 10);
			resp_len = handler(argc, argv, resp_buf, bufsize);
			send_http_response((u8_t *)resp_buf, resp_len);
			free(body_copy);
		}
	}
	return resp_len;
}
void httpd_appcall(void) {
	struct fs_file fsfile;
	unsigned int i;
	switch (uip_conn->lport) {
	case HTONS(80):
		hs = &(uip_conn->appstate);
		if (uip_closed()) {
			httpd_state_reset();
			uip_close();
			return;
		}
		if (uip_aborted() || uip_timedout()) {
			httpd_state_reset();
			uip_abort();
			return;
		}
		if (uip_poll()) {
			if (hs->count++ >= 3000) {
				httpd_state_reset();
				uip_abort();
			}
			return;
		}
		if (uip_connected()) {
			httpd_state_reset();
			return;
		}
		if (uip_newdata() && hs->state == STATE_NONE) {
			if (uip_appdata[0] == ISO_G && uip_appdata[1] == ISO_E && uip_appdata[2] == ISO_T && (uip_appdata[3] == ISO_space || uip_appdata[3] == ISO_tab)) {
				hs->state = STATE_FILE_REQUEST;
			}
			else if (uip_appdata[0] == ISO_P && uip_appdata[1] == ISO_O && uip_appdata[2] == ISO_S && uip_appdata[3] == ISO_T && (uip_appdata[4] == ISO_space || uip_appdata[4] == ISO_tab)) {
				hs->state = STATE_UPLOAD_REQUEST;
			}
			if (hs->state == STATE_NONE) {
				httpd_state_reset();
				uip_abort();
				return;
			}
			if (hs->state == STATE_FILE_REQUEST) {
				if (strncmp((char *)&uip_appdata[4], "/macrw", 6) == 0) {
					char *query = strchr((char *)&uip_appdata[4], '?');
					int argc = 0;
					char *argv[10];
					if (query) {
						*query = 0;
						query++;
						parse_url_args(query, &argc, argv, 10);
					}
					char resp_buf[512];
					int resp_len = web_macrw_handle(argc, argv, resp_buf, sizeof(resp_buf));
					send_http_response((u8_t *)resp_buf, resp_len);
					return;
				}
				else if (strncmp((char *)&uip_appdata[4], "/download_firmware", 18) == 0) {
					handle_download_firmware_request();
					return;
				}
			}
			if (hs->state == STATE_UPLOAD_REQUEST) {
				if (strncmp((char *)uip_appdata, "POST /macrw", 11) == 0) {
					char resp_buf[512];
					int resp_len = handle_post_request(web_macrw_handle, resp_buf, sizeof(resp_buf));
					// print to console
					printf("[WEB macrw] %.*s\n", resp_len, resp_buf);
					return;
				}
				else if (strncmp((char *)uip_appdata, "POST /setenv", 12) == 0) {
					char resp_buf[8192];
					int resp_len = handle_post_request(web_setenv_handle, resp_buf, sizeof(resp_buf));
					// print to console
					printf("[WEB setenv] %.*s\n", resp_len, resp_buf);
					return;
				}
				else if (strncmp((char *)uip_appdata, "POST /reset", 11) == 0) {
					static char resp_buf[] = HTTP_200_OK CONTENT_TYPE SERVER_HEADER COMMON_SECURITY_HEADERS CONNECTION_CLOSE "\r\n" "Rebooting...\n";
					send_http_response((u8_t *)resp_buf, strlen(resp_buf));
					do_reset(NULL, 0, 0, NULL);
					return;
				}
				else if (strncmp((char *)uip_appdata, "POST /read_firmware", 19) == 0) {
					handle_read_firmware_request();
					return;
				}
			}
			if (hs->state == STATE_FILE_REQUEST) {
				for (i = 4; i < 30; i++) {
					if (uip_appdata[i] == ISO_space || uip_appdata[i] == ISO_cr || uip_appdata[i] == ISO_nl || uip_appdata[i] == ISO_tab) {
						uip_appdata[i] = 0;
						i = 0;
						break;
					}
				}
				if (i != 0) {
					printf("## Error: request file name too long!\n");
					httpd_state_reset();
					uip_abort();
					return;
				}
				printf("Request for: ");
				printf("%s\n", &uip_appdata[4]);
				if (uip_appdata[4] == ISO_slash && uip_appdata[5] == 0) {
					fs_open(file_index_html.name, &fsfile);
				}
				else {
					if (!fs_open((const char *)&uip_appdata[4], &fsfile)) {
						printf("## Error: file not found!\n");
						fs_open(file_404_html.name, &fsfile);
					}
				}
				hs->state = STATE_FILE_REQUEST;
				hs->dataptr = (u8_t *)fsfile.data;
				hs->upload = fsfile.len;
				uip_send(hs->dataptr, (hs->upload > uip_mss() ? uip_mss() : hs->upload));
				return;
			}
			else if (hs->state == STATE_UPLOAD_REQUEST) {
				char *start = NULL;
				char *end = NULL;
				uip_appdata[uip_len] = '\0';
				start = (char *)strstr((char *)uip_appdata, "Content-Length:");
				if (start) {
					start += sizeof("Content-Length:");
					end = (char *)strstr(start, eol);
					if (end) {
						hs->upload_total = atoi(start);
					}
					else {
						printf("## Error: couldn't find \"Content-Length\"!\n");
						httpd_state_reset();
						uip_abort();
						return;
					}
				}
				else {
					printf("## Error: couldn't find \"Content-Length\"!\n");
					httpd_state_reset();
					uip_abort();
					return;
				}
				if (hs->upload_total < 10240) {
					printf("## Error: request for upload < 10 KB data!\n");
					httpd_state_reset();
					uip_abort();
					return;
				}
				start = NULL;
				end = NULL;
				start = (char *)strstr((char *)uip_appdata, "boundary=");
				if (start) {
					start += 9;
					end = (char *)strstr((char *)start, eol);
					if (end) {
						boundary_value = (char *)malloc(end - start + 3);
						if (boundary_value) {
							memcpy(&boundary_value[2], start, end - start);
							boundary_value[0] = '-';
							boundary_value[1] = '-';
							boundary_value[end - start + 2] = 0;
							printf("Found boundary value: \"%s\"\n", boundary_value);
						}
						else {
							printf("## Error: couldn't allocate memory for boundary!\n");
							httpd_state_reset();
							uip_abort();
							return;
						}
					}
					else {
						printf("## Error: couldn't find boundary!\n");
						httpd_state_reset();
						uip_abort();
						return;
					}
				}
				else {
					printf("## Error: couldn't find boundary!\n");
					httpd_state_reset();
					uip_abort();
					return;
				}
				webfailsafe_data_pointer = (u8_t *)WEBFAILSAFE_UPLOAD_RAM_ADDRESS;
				if (!webfailsafe_data_pointer) {
					printf("## Error: couldn't allocate RAM for data!\n");
					httpd_state_reset();
					uip_abort();
					return;
				}
				else {
					gpio_set_value(power_led, !power_led_active_low);
					printf("Data will be downloaded at 0x%X in RAM\n", WEBFAILSAFE_UPLOAD_RAM_ADDRESS);
				}
				if (httpd_findandstore_firstchunk()){
					data_start_found = 1;
				}
				else {
					data_start_found = 0;
				}
				return;
			}
		}
		if (uip_acked()) {
			if (hs->is_macrw_resp) {
				if (hs->upload <= uip_mss()) {
					httpd_state_reset();
					uip_close();
					hs->is_macrw_resp = 0;
					return;
				}
				hs->dataptr += uip_conn->len;
				hs->upload -= uip_conn->len;
				uip_send(hs->dataptr, (hs->upload > uip_mss() ? uip_mss() : hs->upload));
				return;
			}
			if (hs->is_backupfw_resp) {
				if (hs->is_firmware_download && hs->firmware_data && hs->firmware_size > 0) {
					hs->is_backupfw_resp = 0;
					hs->dataptr = (u8_t *)hs->firmware_data;
					hs->upload = hs->firmware_size;
					uip_send(hs->dataptr, (hs->upload > uip_mss() ? uip_mss() : hs->upload));
				}
				else {
					httpd_state_reset();
					uip_close();
					hs->is_backupfw_resp = 0;
				}
				return;
			}
			if (hs->is_firmware_download && hs->firmware_data) {
				if (hs->upload <= uip_mss()) {
					httpd_state_reset();
					uip_close();
					hs->is_firmware_download = 0;
					return;
				}
				hs->dataptr += uip_conn->len;
				hs->upload -= uip_conn->len;
				hs->firmware_sent += uip_conn->len;
				uip_send(hs->dataptr, (hs->upload > uip_mss() ? uip_mss() : hs->upload));
				return;
			}
			if (hs->state == STATE_FILE_REQUEST) {
				if (hs->upload <= uip_mss()) {
					if (webfailsafe_post_done) {
						if (!webfailsafe_upload_failed) {
							webfailsafe_ready_for_upgrade = 1;
						}
						webfailsafe_post_done = 0;
						webfailsafe_upload_failed = 0;
					}
					httpd_state_reset();
					uip_close();
					return;
				}
				hs->dataptr += uip_conn->len;
				hs->upload -= uip_conn->len;
				uip_send(hs->dataptr, (hs->upload > uip_mss() ? uip_mss() : hs->upload));
			}
			return;
		}
		if (uip_rexmit()) {
			if (hs->state == STATE_FILE_REQUEST) {
				uip_send(hs->dataptr, (hs->upload > uip_mss() ? uip_mss() : hs->upload));
			}
			return;
		}
		if (uip_newdata()) {
			if (hs->state == STATE_UPLOAD_REQUEST) {
				uip_appdata[uip_len] = '\0';
				if (!data_start_found) {
					if (!httpd_findandstore_firstchunk()) {
						printf("## Error: couldn't find start of data in next packet!\n");
						httpd_state_reset();
						uip_abort();
						return;
					}
					else {
						data_start_found = 1;
					}
					return;
				}
				hs->upload += (unsigned int)uip_len;
				if (!webfailsafe_upload_failed) {
					memcpy((void *)webfailsafe_data_pointer, (void *)uip_appdata, uip_len);
					webfailsafe_data_pointer += uip_len;
				}
				httpd_download_progress();
				if (hs->upload >= hs->upload_total + strlen(boundary_value) + 6) {
					printf("\n\ndone!\n");
					if (led_tftp_transfer_flashing != power_led)
					gpio_set_value(led_tftp_transfer_flashing, LED_OFF);
					gpio_set_value(power_led, !power_led_active_low);
					webfailsafe_post_done = 1;
					NetBootFileXferSize = (ulong)hs->upload_total;
					printf("NetBootFileXferSize = %lu\n", NetBootFileXferSize);
					if (!webfailsafe_upload_failed) {
						fs_open(file_flashing_html.name, &fsfile);
					}
					else {
						fs_open(file_fail_html.name, &fsfile);
					}
					httpd_state_reset();
					hs->state = STATE_FILE_REQUEST;
					hs->dataptr = (u8_t *)fsfile.data;
					hs->upload = fsfile.len;
					uip_send(hs->dataptr, (hs->upload > uip_mss() ? uip_mss() : hs->upload));
				}
			}
			return;
		}
		break;
	default:
		printf("default..\n");
		uip_abort();
		break;
	}
}