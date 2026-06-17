#include "stats_client.h"

#include "wifi_manager.h"

namespace {
const uint32_t kStatsIntervalMs = 2500;
const uint16_t kHttpTimeoutMs = 3000;

String formatHttpError(int code) {
  if (code == -1) {
    return "Agent offline";
  }
  if (code == -11) {
    return "Agent timeout";
  }
  if (code < 0) {
    return "Network error";
  }
  return "HTTP " + String(code);
}
}  // namespace

void StatsClient::begin() {
  stats_ = PcStats{};
  lastRequestMs_ = 0;
  statsUpdated_ = false;
}

void StatsClient::begin(ConfigStore& configStore, WifiManager& wifiManager) {
  configStore_ = &configStore;
  wifiManager_ = &wifiManager;
  begin();
}

void StatsClient::loop() {
  uint32_t now = millis();
  if (now - lastRequestMs_ >= kStatsIntervalMs) {
    lastRequestMs_ = now;
    fetchNow();
  }
}

PcStats StatsClient::fetchNow() {
  if (wifiManager_ == nullptr || !wifiManager_->isConnected()) {
    setOffline("Wi-Fi not connected");
    statsUpdated_ = true;
    return stats_;
  }
  if (!hasAgentConfig()) {
    setOffline("Agent IP not set");
    statsUpdated_ = true;
    return stats_;
  }

  PcStats fetched;
  if (requestStats(fetched)) {
    stats_ = fetched;
    failCount_ = 0;
    lastSuccessMs_ = millis();
    statsUpdated_ = true;
    return stats_;
  }

  failCount_++;
  if (failCount_ < kMaxFailBeforeOffline && lastSuccessMs_ > 0 &&
      millis() - lastSuccessMs_ < kStaleGraceMs) {
    statsUpdated_ = false;
    return stats_;
  }

  setOffline(stats_.error.length() > 0 ? stats_.error : "Agent request failed");
  statsUpdated_ = true;
  return stats_;
}

PcStats StatsClient::currentStats() const {
  return stats_;
}

bool StatsClient::consumeUpdated() {
  if (!statsUpdated_) {
    return false;
  }
  statsUpdated_ = false;
  return true;
}

String StatsClient::buildBaseUrl() const {
  if (configStore_ == nullptr) {
    return "";
  }
  String url = "http://";
  url += configStore_->getAgentIp();
  url += ":";
  url += String(configStore_->getAgentPort());
  return url;
}

bool StatsClient::hasAgentConfig() const {
  return configStore_ != nullptr && configStore_->getAgentIp().length() > 0 &&
         configStore_->getAgentPort() > 0;
}

bool StatsClient::requestStats(PcStats& outStats) {
  HTTPClient http;
  String url = buildBaseUrl() + "/stats";
  http.setTimeout(kHttpTimeoutMs);
  http.setReuse(false);

  bool ok = false;
  if (http.begin(url)) {
    http.addHeader("Connection", "close");
    int code = http.GET();
    if (code >= 200 && code < 300) {
      String payload = http.getString();
      parseStatsJson(payload, outStats);
      ok = outStats.error.length() == 0;
    } else {
      stats_.error = formatHttpError(code);
    }
  } else {
    stats_.error = "Agent offline";
  }

  http.end();
  return ok;
}

void StatsClient::setOffline(const String& error) {
  stats_.online = false;
  stats_.status = "offline";
  stats_.error = error;
}

void StatsClient::parseStatsJson(const String& payload, PcStats& outStats) {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    outStats = PcStats{};
    outStats.error = "JSON parse failed";
    stats_.error = outStats.error;
    return;
  }

  outStats.status = doc["status"] | "offline";
  outStats.hostname = doc["hostname"] | "";
  outStats.time = doc["time"] | "";
  outStats.cpuUsage = doc["cpu"]["usage"].isNull() ? -1.0f : doc["cpu"]["usage"].as<float>();
  outStats.cpuCoreMax = doc["cpu"]["core_max"].isNull() ? -1.0f : doc["cpu"]["core_max"].as<float>();
  outStats.cpuFreqMhz = doc["cpu"]["freq_mhz"].isNull() ? 0.0f : doc["cpu"]["freq_mhz"].as<float>();
  if (!doc["cpu"]["temp_c"].isNull()) {
    outStats.cpuTempC = doc["cpu"]["temp_c"] | -1.0f;
  } else {
    outStats.cpuTempC = -1.0f;
  }
  outStats.ramUsage = doc["ram"]["usage"] | 0.0f;
  outStats.ramUsedGb = doc["ram"]["used_gb"] | 0.0f;
  outStats.ramTotalGb = doc["ram"]["total_gb"] | 0.0f;
  outStats.diskUsage = doc["disk"]["usage"] | 0.0f;
  outStats.diskUsedGb = doc["disk"]["used_gb"] | 0.0f;
  outStats.diskTotalGb = doc["disk"]["total_gb"] | 0.0f;
  outStats.diskReadMbps = doc["disk_io"]["read_mbps"] | 0.0f;
  outStats.diskWriteMbps = doc["disk_io"]["write_mbps"] | 0.0f;
  outStats.uploadKbps = doc["network"]["upload_kbps"] | 0.0f;
  outStats.downloadKbps = doc["network"]["download_kbps"] | 0.0f;
  outStats.gpuAvailable = doc["gpu"]["available"] | false;
  outStats.gpuName = doc["gpu"]["name"] | "";
  outStats.gpuUsage = doc["gpu"]["usage"] | 0.0f;
  if (!doc["gpu"]["temp_c"].isNull()) {
    outStats.gpuTempC = doc["gpu"]["temp_c"] | -1.0f;
  } else {
    outStats.gpuTempC = -1.0f;
  }
  outStats.gpuMemUsedMb = doc["gpu"]["mem_used_mb"] | 0;
  outStats.gpuMemTotalMb = doc["gpu"]["mem_total_mb"] | 0;
  if (!doc["battery"]["percent"].isNull()) {
    outStats.batteryPercent = doc["battery"]["percent"] | -1;
  } else {
    outStats.batteryPercent = -1;
  }
  outStats.batteryPlugged = doc["battery"]["plugged"] | false;
  outStats.uptimeH = doc["system"]["uptime_h"] | 0.0f;
  outStats.processCount = doc["system"]["process_count"] | 0;
  outStats.online = outStats.status == "online";
  outStats.error = "";
}
