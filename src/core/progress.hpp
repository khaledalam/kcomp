#pragma once

#include <cstdio>
#include <cstdint>
#include <string>

class ProgressBar {
public:
  ProgressBar(size_t total, const std::string& label, bool enabled = true)
    : total_(total), current_(0), label_(label), enabled_(enabled), last_percent_(-1) {
    if (enabled_ && total_ > 0) {
      render();
    }
  }

  void update(size_t current) {
    if (!enabled_ || total_ == 0) return;
    current_ = current;
    int percent = static_cast<int>((current_ * 100) / total_);
    if (percent != last_percent_) {
      last_percent_ = percent;
      render();
    }
  }

  void increment(size_t delta = 1) {
    update(current_ + delta);
  }

  void finish() {
    if (!enabled_) return;
    current_ = total_;
    last_percent_ = 100;
    render();
    std::fprintf(stderr, "\n");
  }

  void set_label(const std::string& label) {
    label_ = label;
    if (enabled_) render();
  }

private:
  void render() {
    if (!enabled_) return;

    int percent = total_ > 0 ? static_cast<int>((current_ * 100) / total_) : 0;
    int bar_width = 30;
    int filled = (percent * bar_width) / 100;

    std::fprintf(stderr, "\r%-20s [", label_.c_str());
    for (int i = 0; i < bar_width; i++) {
      if (i < filled) std::fprintf(stderr, "=");
      else if (i == filled) std::fprintf(stderr, ">");
      else std::fprintf(stderr, " ");
    }
    std::fprintf(stderr, "] %3d%%", percent);
    std::fflush(stderr);
  }

  size_t total_;
  size_t current_;
  std::string label_;
  bool enabled_;
  int last_percent_;
};

// Simple spinner for indeterminate progress
class Spinner {
public:
  Spinner(const std::string& label, bool enabled = true)
    : label_(label), enabled_(enabled), frame_(0) {
    if (enabled_) render();
  }

  void tick() {
    if (!enabled_) return;
    frame_ = (frame_ + 1) % 4;
    render();
  }

  void finish(const std::string& status = "done") {
    if (!enabled_) return;
    std::fprintf(stderr, "\r%-20s [%s]\n", label_.c_str(), status.c_str());
    std::fflush(stderr);
  }

  void set_label(const std::string& label) {
    label_ = label;
    if (enabled_) render();
  }

private:
  void render() {
    static const char* frames[] = {"|", "/", "-", "\\"};
    std::fprintf(stderr, "\r%-20s [%s]", label_.c_str(), frames[frame_]);
    std::fflush(stderr);
  }

  std::string label_;
  bool enabled_;
  int frame_;
};

// Format bytes to human-readable string
inline std::string format_size(size_t bytes) {
  char buf[64];
  if (bytes >= 1024 * 1024 * 1024) {
    std::snprintf(buf, sizeof(buf), "%.2f GB", bytes / (1024.0 * 1024.0 * 1024.0));
  } else if (bytes >= 1024 * 1024) {
    std::snprintf(buf, sizeof(buf), "%.2f MB", bytes / (1024.0 * 1024.0));
  } else if (bytes >= 1024) {
    std::snprintf(buf, sizeof(buf), "%.2f KB", bytes / 1024.0);
  } else {
    std::snprintf(buf, sizeof(buf), "%zu B", bytes);
  }
  return buf;
}
