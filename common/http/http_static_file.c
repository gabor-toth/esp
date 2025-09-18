#include "ctype.h"
#include <errno.h>
#include <esp_log.h>
#include <esp_spiffs.h>
#include <esp_vfs.h>
#include "http_server.h"
#include "http_static_file.h"
#include <fcntl.h>
#include "rest_util.h"

static const char *TAG = "http-static";

#if defined(CONFIG_EXAMPLE_CONNECT_WIFI) && defined(CONFIG_EXAMPLE_WEB_MOUNT_POINT)
#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + 128)

#define CHECK_FILE_EXTENSION( filename, ext ) (strcasecmp(&(filename)[strlen(filename) - strlen(ext)], ext) == 0)

/* Set HTTP response content type according to file extension */
esp_err_t set_content_type_from_file( httpd_req_t *req, const char *filepath ) {
    const char *type = "text/plain";
    if ( CHECK_FILE_EXTENSION( filepath, ".html" ) ) {
        type = HTTPD_TYPE_TEXT;
    } else if ( CHECK_FILE_EXTENSION( filepath, ".js" ) ) {
        type = "application/javascript";
    } else if ( CHECK_FILE_EXTENSION( filepath, ".css" ) ) {
        type = "text/css";
    } else if ( CHECK_FILE_EXTENSION( filepath, ".png" ) ) {
        type = "image/png";
    } else if ( CHECK_FILE_EXTENSION( filepath, ".ico" ) ) {
        type = "image/x-icon";
    } else if ( CHECK_FILE_EXTENSION( filepath, ".svg" ) ) {
        type = "text/xml";
    }
    return httpd_resp_set_type( req, type );
}

static bool has_2_dots_in_file_name( char *filepath ) {
    char *p;

    return ( ( p = strchr( filepath, '.' ) ) != NULL ) && strchr( p + 1, '.' ) != NULL;
}

static bool is_angular_18_file( char *filepath ) {
    char *p = strchr( filepath, '-' );
    if ( p == NULL ) {
        return false;
    }
    int chars = 0;
    for ( p++; *p != 0 && *p != '.'; p++, chars++ ) {
        int c = (int) *p;
        if ( !isdigit( c ) && !isupper( c ) ) {
            return false;
        }
    }
    return chars == 8;
}

static bool is_file_cacheable( char *filepath ) {
    return has_2_dots_in_file_name( filepath )
           || is_angular_18_file( filepath );
}

esp_err_t init_fs( void ) {
    esp_vfs_spiffs_conf_t conf = {
            .base_path = CONFIG_EXAMPLE_WEB_MOUNT_POINT,
            .partition_label = NULL,
            .max_files = 5,
            .format_if_mount_failed = false
    };
    esp_err_t ret = esp_vfs_spiffs_register( &conf );

    if ( ret != ESP_OK ) {
        if ( ret == ESP_FAIL ) {
            ESP_LOGE( TAG, "Failed to mount or format filesystem" );
        } else if ( ret == ESP_ERR_NOT_FOUND ) {
            ESP_LOGE( TAG, "Failed to find SPIFFS partition" );
        } else {
            ESP_LOGE( TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name( ret ) );
        }
        return ESP_FAIL;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info( NULL, &total, &used );
    if ( ret != ESP_OK ) {
        ESP_LOGE( TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name( ret ) );
    } else {
        ESP_LOGI( TAG, "Partition size: total: %d, used: %d", total, used );
    }
    return ESP_OK;
}

static void set_cache_forever( httpd_req_t *req, char *filepath ) {
    ESP_LOGI( TAG, "Set cache forever for %s", filepath );
    /*
    Last-Modified: Mon, 08 Dec 2014 19:23:51 GMT
    ETag: "5485fac7-ae74"
    Cache-Control: max-age=533280
    Expires: Sun, 03 May 2015 23:02:37 GMT
     */
//    static char last_modified_header_value[32];
//    static char max_age_header_value[32];

    httpd_resp_set_hdr( req, "Cache-Control", "private, max-age=31536000, immutable" ); // 1 year in seconds

//    struct stat file_state;
//    stat( filepath, &file_state );
//    struct tm timeinfo = { 0 };
//    localtime_r( &file_state.st_mtim.tv_sec, &timeinfo );
//    strftime( last_modified_header_value, sizeof last_modified_header_value, "%r", &timeinfo );
//    httpd_resp_set_hdr( req, "Last-Modified", last_modified_header_value );
}

/* Send HTTP response with the contents of the requested file */
static esp_err_t http_file_get_handler( httpd_req_t *req ) {
    char filepath[FILE_PATH_MAX];
    char error_message[255];

    http_server_context_t *http_context = (http_server_context_t *) req->user_ctx;
    strlcpy( filepath, http_context->fs_base_path, sizeof( filepath ) );
    if ( req->uri[ strlen( req->uri ) - 1 ] == '/' || !strchr( req->uri, '.' ) ) {
        // serve index.html for Angular routes
        ESP_LOGW( TAG, "index.html for %s", req->uri );
        strlcat( filepath, "/index.html", sizeof( filepath ) );
    } else {
        strlcat( filepath, req->uri, sizeof( filepath ) );
    }
    int fd = open( filepath, O_RDONLY, 0 );
    if ( fd == -1 ) {
        ESP_LOGW( TAG, "Failed to open file : %s", filepath );
        snprintf( error_message, sizeof( error_message ), "Failed to read file: %d", errno );
        httpd_resp_set_hdr( req, "Connection", "close" );
        httpd_resp_send_err( req, HTTPD_404_NOT_FOUND, error_message );
        return ESP_FAIL;
    }

    ESP_LOGI( TAG, "Sending file %s", filepath );
    if ( is_file_cacheable( filepath ) ) {
        set_cache_forever( req, filepath );
    }
    set_content_type_from_file( req, filepath );

    char *chunk = http_context->scratch;
    ssize_t read_bytes;
    do {
        /* Read file in chunks into the scratch buffer */
        read_bytes = read( fd, chunk, HTTP_SCRATCH_BUFFER_SIZE );
        if ( read_bytes == -1 ) {
            ESP_LOGE( TAG, "Failed to read file : %s", filepath );
        } else if ( read_bytes > 0 ) {
            /* Send the buffer contents as HTTP response chunk */
            if ( httpd_resp_send_chunk( req, chunk, read_bytes ) != ESP_OK ) {
                close( fd );
                ESP_LOGE( TAG, "File sending failed: %d %s", errno, strerror( errno ) );
                /* Abort sending file */
                httpd_resp_set_hdr( req, "Connection", "close" );
                httpd_resp_sendstr_chunk( req, NULL );
                /* Respond with 500 Internal Server Error */
                httpd_resp_send_err( req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file" );
                return ESP_FAIL;
            }
        }
    } while ( read_bytes > 0 );
    /* Close file after sending complete */
    close( fd );
    ESP_LOGI( TAG, "File sending complete" );
    /* Respond with an empty chunk to signal HTTP response completion */
    httpd_resp_set_hdr( req, "Connection", "close" );
    httpd_resp_send_chunk( req, NULL, 0 );
    return ESP_OK;
}

esp_err_t http_static_files_register_handler( httpd_handle_t server, http_server_context_t *http_context,
                                              const char *static_files_base_path ) {
    if ( static_files_base_path == NULL ) {
        return ESP_ERR_INVALID_ARG;
    }
    strncpy( http_context->fs_base_path, CONFIG_EXAMPLE_WEB_MOUNT_POINT, sizeof( http_context->fs_base_path ) );
    if ( *static_files_base_path != 0 ) {
        strncat( http_context->fs_base_path, static_files_base_path, ESP_VFS_PATH_MAX );
    }
    /* URI handler for getting web server files */
    httpd_uri_t common_get_uri = {
            .uri = "/*",
            .method = HTTP_GET,
            .handler = http_file_get_handler,
            .user_ctx = http_context
    };
    return http_register_uri_handler( server, TAG, &common_get_uri );
}

void http_static_files_register( void ) {
    ESP_ERROR_CHECK( init_fs() );
    // call http_static_files_register_handler as last to register it as a fallback handler
}

#endif