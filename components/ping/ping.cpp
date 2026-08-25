#include "ping.h"

#ifdef USE_ESP32

#include <algorithm>
#include <cinttypes>

#include "esphome/core/log.h"

#include "esp_err.h"
#include "lwip/ip_addr.h"

namespace esphome::ping {

static const char *const TAG = "ping";

// ESP-IDF defaults the ping task to 2048 bytes, which is not enough once lwIP's
// socket path and the logging call are on the stack - a batch overruns it and
// corrupts the return address.
static constexpr uint32_t PING_TASK_STACK_SIZE = 4096;

void Pinger::setup() { this->disable_loop(); }

void Pinger::dump_config() { ESP_LOGCONFIG(TAG, "Ping"); }

bool Pinger::start(const std::string &address, uint32_t count, PingListener *listener) {
  if (this->handle_ != nullptr) {
    ESP_LOGW(TAG, "Batch already running, ignoring request for '%s'", address.c_str());
    return false;
  }

  ip_addr_t target{};
  if (ipaddr_aton(address.c_str(), &target) != 1) {
    ESP_LOGW(TAG, "'%s' is not a valid IP address", address.c_str());
    return false;
  }

  esp_ping_config_t config = ESP_PING_DEFAULT_CONFIG();
  config.target_addr = target;
  config.count = count;
  config.task_stack_size = PING_TASK_STACK_SIZE;

  esp_ping_callbacks_t callbacks{};
  callbacks.cb_args = this;
  callbacks.on_ping_success = Pinger::on_success_;
  callbacks.on_ping_end = Pinger::on_end_;

  this->reply_time_total_ = 0;
  this->requested_count_ = count;
  this->finished_.store(false, std::memory_order_relaxed);
  this->listener_ = listener;

  esp_err_t err = esp_ping_new_session(&config, &callbacks, &this->handle_);
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "esp_ping_new_session failed: %s", esp_err_to_name(err));
    this->handle_ = nullptr;
    this->listener_ = nullptr;
    return false;
  }

  err = esp_ping_start(this->handle_);
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "esp_ping_start failed: %s", esp_err_to_name(err));
    esp_ping_delete_session(this->handle_);
    this->handle_ = nullptr;
    this->listener_ = nullptr;
    return false;
  }

  ESP_LOGD(TAG, "Pinging %s, %" PRIu32 " packets", address.c_str(), count);
  this->enable_loop();
  return true;
}

void Pinger::on_success_(esp_ping_handle_t handle, void *args) {
  auto *pinger = static_cast<Pinger *>(args);
  uint32_t elapsed = 0;
  esp_ping_get_profile(handle, ESP_PING_PROF_TIMEGAP, &elapsed, sizeof(elapsed));
  pinger->reply_time_total_ += elapsed;
}

void Pinger::on_end_(esp_ping_handle_t handle, void *args) {
  auto *pinger = static_cast<Pinger *>(args);
  uint32_t transmitted = 0;
  uint32_t received = 0;
  esp_ping_get_profile(handle, ESP_PING_PROF_REQUEST, &transmitted, sizeof(transmitted));
  esp_ping_get_profile(handle, ESP_PING_PROF_REPLY, &received, sizeof(received));

  const uint32_t requested = pinger->requested_count_;
  pinger->last_result_.requested = requested;
  pinger->last_result_.transmitted = transmitted;
  pinger->last_result_.received = received;
  pinger->last_result_.latency_ms = received == 0 ? 0 : pinger->reply_time_total_ / received;
  // Measure loss against the packets asked for, not the ones that made it onto
  // the wire. ESP-IDF only counts a packet as transmitted once sendto() has
  // succeeded, so a link that goes down mid-batch would otherwise report a
  // short batch as 0% loss.
  pinger->last_result_.loss =
      requested == 0 ? 100.0f
                     : 100.0f * static_cast<float>(requested - std::min(received, requested)) /
                           static_cast<float>(requested);

  // Hand off to loop(); the session is torn down and the callbacks run on the
  // main task, not on the ping task.
  pinger->finished_.store(true, std::memory_order_release);
}

void Pinger::loop() {
  if (!this->finished_.load(std::memory_order_acquire)) {
    return;
  }
  this->finished_.store(false, std::memory_order_relaxed);

  if (this->handle_ != nullptr) {
    esp_ping_delete_session(this->handle_);
    this->handle_ = nullptr;
  }
  this->disable_loop();

  const PingResult result = this->last_result_;
  ESP_LOGD(TAG, "%" PRIu32 "/%" PRIu32 " received (%" PRIu32 " sent), %.0f%% loss, %" PRIu32 " ms", result.received,
           result.requested, result.transmitted, result.loss, result.latency_ms);

  this->result_callback_.call(result.loss, result.latency_ms);

  PingListener *listener = this->listener_;
  this->listener_ = nullptr;
  if (listener != nullptr) {
    listener->on_ping_finished(result);
  }
}

}  // namespace esphome::ping

#endif  // USE_ESP32
