#include "esp_http_server.h"
#include "esp_timer.h"
#include "esp_camera.h"
#include "img_converters.h"

typedef struct {
  httpd_req_t *req;
  size_t len;
} jpg_chunking_t;

static size_t jpg_encode_stream(void *arg, size_t index, const void *data, size_t len)
{
  jpg_chunking_t *j = (jpg_chunking_t *)arg;
  if (!index) {
    j->len = 0;
  }

  if (httpd_resp_send_chunk(j->req, (const char *)data, len) != ESP_OK) {
    return 0;
  }
  j->len += len;
  return len;
}

static esp_err_t stream_handler(httpd_req_t *req)
{
  camera_fb_t *fb = NULL;
  esp_err_t res = ESP_OK;

  fb = esp_camera_fb_get();
  if (!fb) {
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }

  httpd_resp_set_type(req, "image/jpeg");

  jpg_chunking_t jchunk = {req, 0};

  if (fb->format == PIXFORMAT_JPEG) {
    res = httpd_resp_send_chunk(req, (const char *)fb->buf, fb->len);
  } else {
    res = frame2jpg_cb(fb, 80, jpg_encode_stream, &jchunk) ? ESP_OK : ESP_FAIL;
  }

  esp_camera_fb_return(fb);

  httpd_resp_send_chunk(req, NULL, 0);
  return res;
}

void startCameraServer()
{
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();

  httpd_uri_t stream_uri = {
      .uri = "/",
      .method = HTTP_GET,
      .handler = stream_handler,
      .user_ctx = NULL};

  httpd_handle_t stream_httpd = NULL;

  if (httpd_start(&stream_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(stream_httpd, &stream_uri);
  }
}
