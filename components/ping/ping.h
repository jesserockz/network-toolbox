#pragma once

#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"

#ifdef USE_ESP32

#include <atomic>
#include <cstdint>
#include <string>
#include <tuple>

#include "ping/ping_sock.h"

namespace esphome::ping {

/// Outcome of a single ping batch.
struct PingResult {
  uint32_t requested;   ///< packets asked for, which is what loss is measured against
  uint32_t transmitted;
  uint32_t received;
  uint32_t latency_ms;  ///< mean round trip across the replies that arrived
  float loss;           ///< percent, 0-100
};

/// Implemented by whatever needs to run when a batch finishes. A plain interface
/// rather than a std::function so a pending request costs one pointer.
class PingListener {
 public:
  virtual void on_ping_finished(const PingResult &result) = 0;
};

class Pinger : public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;

  /// Start a batch. Returns false when a batch is already in flight or the
  /// address is not a valid IP, in which case the listener is never called.
  bool start(const std::string &address, uint32_t count, PingListener *listener = nullptr);

  bool is_running() const { return this->handle_ != nullptr; }
  const PingResult &get_last_result() const { return this->last_result_; }

  /// Detach a listener that is going away, so a batch already in flight does not
  /// call back into it.
  void clear_listener(PingListener *listener) {
    if (this->listener_ == listener) {
      this->listener_ = nullptr;
    }
  }

  template<typename F> void add_on_result_callback(F &&callback) {
    this->result_callback_.add(std::forward<F>(callback));
  }

 protected:
  static void on_success_(esp_ping_handle_t handle, void *args);
  static void on_end_(esp_ping_handle_t handle, void *args);

  esp_ping_handle_t handle_{nullptr};
  PingListener *listener_{nullptr};
  uint32_t requested_count_{0};
  PingResult last_result_{};
  uint32_t reply_time_total_{0};
  // Written from the ping task, read from loop().
  std::atomic<bool> finished_{false};
  LazyCallbackManager<void(float, uint32_t)> result_callback_;
};

template<typename... Ts> class PingAction : public Action<Ts...>, public Parented<Pinger>, public PingListener {
 public:
  TEMPLATABLE_VALUE(std::string, ip_address)
  TEMPLATABLE_VALUE(uint32_t, count)

  void play_complex(const Ts &...x) override {
    this->num_running_++;
    this->var_ = std::make_tuple(x...);
    if (!this->parent_->start(this->ip_address_.value(x...), this->count_.value(x...), this)) {
      // Nothing will call back, so do not leave the automation stalled.
      this->play_next_tuple_(this->var_);
    }
  }

  void on_ping_finished(const PingResult & /*result*/) override { this->play_next_tuple_(this->var_); }

 protected:
  void play(const Ts &...x) override {}

  void stop() override { this->parent_->clear_listener(this); }

  std::tuple<Ts...> var_{};
};

}  // namespace esphome::ping

#endif  // USE_ESP32
