#include "monitor_ui.h"

#include <math.h>

namespace {
const int kPad = 3;
const int kHeaderH = 14;
const int kStatusY = 15;
const int kStatusH = 11;
const int kContentTop = 28;
const int kFooterY = 122;

const uint16_t kPanelBg = 0x0841;
const uint16_t kDimLine = 0x3186;
}  // namespace

void MonitorUi::begin() {
  M5Cardputer.Display.setBrightness(170);
  M5Cardputer.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5Cardputer.Display.setTextSize(1);
}

void MonitorUi::begin(StatsClient& statsClient, WifiManager& wifiManager, ConfigStore& configStore) {
  statsClient_ = &statsClient;
  wifiManager_ = &wifiManager;
  configStore_ = &configStore;
  begin();
}

void MonitorUi::showBootScreen() {
  auto& d = M5Cardputer.Display;
  d.fillScreen(TFT_BLACK);
  drawCornerBrackets(8, 36, 224, 52, TFT_CYAN);
  d.setTextSize(2);
  d.setTextColor(TFT_CYAN, TFT_BLACK);
  d.drawString("SYS-LINK", 52, 48);
  d.setTextSize(1);
  d.setTextColor(TFT_MAGENTA, TFT_BLACK);
  d.drawString("PC MONITOR v2", 72, 72);
  d.setTextColor(TFT_DARKGREY, TFT_BLACK);
  d.drawString("initializing...", 78, 88);
  d.drawFastHLine(0, 16, d.width(), kDimLine);
}

void MonitorUi::update() {
  handleKeyboard();

  PcStats barStats;
  const PcStats* statsPtr = nullptr;
  if (statsClient_ != nullptr) {
    barStats = statsClient_->currentStats();
    statsPtr = &barStats;
  }

  uint32_t now = millis();
  if (now - lastStatusDrawMs_ >= 1000) {
    drawStatusBar(statsPtr);
    lastStatusDrawMs_ = now;
  }

  bool statsUpdated = statsClient_ != nullptr && statsClient_->consumeUpdated();
  if (statsUpdated && statsClient_ != nullptr) {
    const PcStats& s = statsClient_->currentStats();
    float histVal = s.cpuUsage > 0 ? s.cpuUsage : s.cpuCoreMax;
    pushCpuHistory(histVal);
  }

  if (page_ == MONITOR_PAGE_SETTINGS) {
    if (dirty_ || !chromeDrawn_) {
      renderPageFull();
    }
    return;
  }

  if (dirty_ || !chromeDrawn_ || page_ != chromePage_) {
    renderPageFull();
    return;
  }

  if (statsUpdated && statsClient_ != nullptr) {
    renderPagePartial(statsClient_->currentStats());
  }
}

void MonitorUi::handleKeyboard() {
  if (!M5Cardputer.Keyboard.isChange() || !M5Cardputer.Keyboard.isPressed()) {
    return;
  }

  Keyboard_Class::KeysState keys = M5Cardputer.Keyboard.keysState();
  if (keyWordContains(keys, '1')) {
    switchPage(MONITOR_PAGE_CPU_RAM);
  } else if (keyWordContains(keys, '2')) {
    switchPage(MONITOR_PAGE_DISK_NET);
  } else if (keyWordContains(keys, '3')) {
    switchPage(MONITOR_PAGE_GPU_SYS);
  } else if (keyWordContains(keys, '4')) {
    switchPage(MONITOR_PAGE_SETTINGS);
  } else if (keyWordContains(keys, 'r') || keyWordContains(keys, 'R')) {
    if (page_ == MONITOR_PAGE_SETTINGS) {
      dirty_ = true;
    }
  } else if (keyWordContains(keys, 27) || keys.del) {
    switchPage(MONITOR_PAGE_CPU_RAM);
  }
}

void MonitorUi::switchPage(MonitorPage page) {
  if (page_ == page && chromeDrawn_) {
    return;
  }
  page_ = page;
  dirty_ = true;
  chromeDrawn_ = false;
}

void MonitorUi::renderPageFull() {
  auto& d = M5Cardputer.Display;
  d.fillScreen(TFT_BLACK);
  drawPageChrome(page_);
  chromeDrawn_ = true;
  chromePage_ = page_;
  dirty_ = false;

  if (page_ == MONITOR_PAGE_SETTINGS) {
    drawSettingsData();
    return;
  }

  PcStats stats;
  if (statsClient_ != nullptr) {
    stats = statsClient_->currentStats();
  }
  renderPagePartial(stats);
}

void MonitorUi::renderPagePartial(const PcStats& stats) {
  switch (page_) {
    case MONITOR_PAGE_DISK_NET:
      drawDiskNetData(stats);
      break;
    case MONITOR_PAGE_GPU_SYS:
      drawGpuSysData(stats);
      break;
    case MONITOR_PAGE_CPU_RAM:
    default:
      drawCpuRamData(stats);
      break;
  }
}

void MonitorUi::drawPageChrome(MonitorPage page) {
  PcStats stats;
  if (statsClient_ != nullptr) {
    stats = statsClient_->currentStats();
  }
  drawHeader(page);
  drawStatusBar(&stats);
  drawFooter();

  uint16_t accent = accentForPage(page);
  switch (page) {
    case MONITOR_PAGE_DISK_NET:
      drawSciFiPanel(kPad, kContentTop, 234, 42, accent);
      drawSciFiPanel(kPad, 72, 234, 48, TFT_BLUE);
      break;
    case MONITOR_PAGE_GPU_SYS:
      drawSciFiPanel(kPad, kContentTop, 234, 50, accent);
      drawSciFiPanel(kPad, 80, 234, 40, kDimLine);
      break;
    case MONITOR_PAGE_SETTINGS:
      break;
    case MONITOR_PAGE_CPU_RAM:
    default:
      drawSciFiPanel(kPad, kContentTop, 114, 46, accent);
      drawSciFiPanel(121, kContentTop, 116, 46, TFT_YELLOW);
      drawSciFiPanel(kPad, 76, 234, 44, kDimLine);
      break;
  }
}

void MonitorUi::drawCpuRamData(const PcStats& stats) {
  auto& d = M5Cardputer.Display;
  uint16_t accent = accentForPage(MONITOR_PAGE_CPU_RAM);
  const int cpuX = kPad + 6;
  const int ramX = 125;
  const int row1 = kContentTop + 4;
  const int row2 = kContentTop + 16;
  const int barY = kContentTop + 30;

  // --- CPU panel (114 x 46) ---
  clearRegion(kPad + 2, kContentTop + 2, 110, 42);
  d.setTextSize(1);
  d.setTextColor(TFT_DARKGREY, kPanelBg);
  d.drawString("CPU", cpuX, row1);
  d.setTextColor(accent, kPanelBg);
  d.drawString(formatPercent(stats.cpuUsage), cpuX + 30, row1);
  d.setTextColor(TFT_DARKGREY, kPanelBg);
  d.drawString(formatFreqMhz(stats.cpuFreqMhz), cpuX, row2);
  drawSegmentBar(cpuX, barY, 102, 5, stats.cpuUsage, accent);

  // --- RAM panel (116 x 46) ---
  clearRegion(123, kContentTop + 2, 112, 42);
  d.setTextColor(TFT_DARKGREY, kPanelBg);
  d.drawString("RAM", ramX, row1);
  d.setTextColor(TFT_YELLOW, kPanelBg);
  d.drawString(formatPercent(stats.ramUsage), ramX + 30, row1);
  String ramDetail = String(stats.ramUsedGb, 1) + " / " + String(stats.ramTotalGb, 1) + " GB";
  d.setTextColor(TFT_WHITE, kPanelBg);
  d.drawString(ramDetail, ramX, row2);
  drawSegmentBar(ramX, barY, 106, 5, stats.ramUsage, TFT_YELLOW);

  // --- Thermal strip ---
  clearRegion(kPad + 2, 78, 230, 40);
  d.setTextColor(TFT_DARKGREY, kPanelBg);
  d.drawString("TEMP", kPad + 6, 80);

  if (stats.cpuTempC >= 0) {
    if (displayTempC_ < 0) {
      displayTempC_ = stats.cpuTempC;
    } else {
      float diff = stats.cpuTempC - displayTempC_;
      if (fabs(diff) > 3.0f) {
        displayTempC_ = stats.cpuTempC;
      } else {
        displayTempC_ = displayTempC_ * 0.85f + stats.cpuTempC * 0.15f;
      }
    }
  } else {
    displayTempC_ = -1.0f;
  }

  float showTemp = displayTempC_ >= 0 ? displayTempC_ : stats.cpuTempC;
  uint16_t tColor = showTemp >= 0 ? tempColor(showTemp) : TFT_DARKGREY;
  d.setTextColor(tColor, kPanelBg);
  d.drawString(formatTemp(showTemp), kPad + 40, 80);

  d.setTextColor(TFT_DARKGREY, kPanelBg);
  d.drawString("PEAK", kPad + 88, 80);
  d.setTextColor(TFT_WHITE, kPanelBg);
  d.drawString(formatPercent(stats.cpuCoreMax), kPad + 120, 80);

  d.setTextColor(TFT_DARKGREY, kPanelBg);
  d.drawString("HOST", kPad + 6, 92);
  d.setTextColor(TFT_CYAN, kPanelBg);
  d.drawString(truncateText(stats.hostname, 20), kPad + 38, 92);

  if (!stats.online && stats.error.length() > 0) {
    d.setTextColor(TFT_RED, kPanelBg);
    d.drawString(truncateText(stats.error, 18), kPad + 150, 92);
  }

  clearRegion(kPad + 4, 112, 232, 8);
  drawSparkline(kPad + 4, 112, 232, 8);
}

void MonitorUi::drawDiskNetData(const PcStats& stats) {
  uint16_t accent = accentForPage(MONITOR_PAGE_DISK_NET);

  clearRegion(kPad + 2, kContentTop + 2, 230, 38);
  M5Cardputer.Display.setTextColor(TFT_DARKGREY, kPanelBg);
  M5Cardputer.Display.drawString("STORAGE C:", kPad + 6, kContentTop + 4);
  drawValueAt(kPad + 6, kContentTop + 16, 80, String(stats.diskUsage, 0) + "%", accent, 2);
  drawValueAt(kPad + 70, kContentTop + 18, 160,
              String(stats.diskUsedGb, 0) + "/" + String(stats.diskTotalGb, 0) + " GB", TFT_WHITE, 1);
  drawSegmentBar(kPad + 6, kContentTop + 32, 222, 6, stats.diskUsage, accent);

  clearRegion(kPad + 2, 74, 230, 44);
  drawValueAt(kPad + 6, 76, 220, "TX " + String(stats.uploadKbps, 0) + " kb/s", TFT_GREEN, 1);
  drawValueAt(kPad + 6, 88, 220, "RX " + String(stats.downloadKbps, 0) + " kb/s", TFT_BLUE, 1);
  drawValueAt(kPad + 6, 100, 220,
              "IO " + String(stats.diskReadMbps, 1) + "/" + String(stats.diskWriteMbps, 1) + " MB/s", TFT_CYAN, 1);

  if (stats.batteryPercent >= 0) {
    String bat = "PWR " + String(stats.batteryPercent) + "%";
    bat += stats.batteryPlugged ? " AC" : " DC";
    drawValueAt(kPad + 6, 112, 100, bat, TFT_YELLOW, 1);
  }
}

void MonitorUi::drawGpuSysData(const PcStats& stats) {
  uint16_t accent = accentForPage(MONITOR_PAGE_GPU_SYS);

  clearRegion(kPad + 2, kContentTop + 2, 230, 46);
  if (stats.gpuAvailable) {
    M5Cardputer.Display.setTextColor(TFT_DARKGREY, kPanelBg);
    M5Cardputer.Display.drawString("GPU", kPad + 6, kContentTop + 4);
    drawValueAt(kPad + 6, kContentTop + 16, 60, String(stats.gpuUsage, 0) + "%", accent, 2);
    String gpuLine = truncateText(stats.gpuName, 16);
    if (stats.gpuTempC >= 0) {
      gpuLine += " " + formatTemp(stats.gpuTempC);
    }
    drawValueAt(kPad + 60, kContentTop + 18, 170, gpuLine, TFT_WHITE, 1);
    if (stats.gpuMemTotalMb > 0) {
      drawValueAt(kPad + 6, kContentTop + 32, 220,
                  "VRAM " + String(stats.gpuMemUsedMb) + "/" + String(stats.gpuMemTotalMb) + "M", TFT_DARKGREY, 1);
    }
    drawSegmentBar(kPad + 6, kContentTop + 42, 222, 4, stats.gpuUsage, accent);
  } else {
    drawValueAt(kPad + 6, kContentTop + 16, 220, "GPU N/A", TFT_DARKGREY, 1);
  }

  clearRegion(kPad + 2, 82, 230, 36);
  drawValueAt(kPad + 6, 84, 220, "UPTIME  " + String(stats.uptimeH, 1) + " h", TFT_WHITE, 1);
  drawValueAt(kPad + 6, 96, 220, "PROC    " + String(stats.processCount), TFT_WHITE, 1);
  drawValueAt(kPad + 6, 108, 220, "WIFI    " + wifiStatusText(), TFT_CYAN, 1);
}

void MonitorUi::drawSettingsData() {
  String agentIp = "";
  uint16_t agentPort = DEFAULT_AGENT_PORT;
  size_t wifiCount = 0;
  if (configStore_ != nullptr) {
    agentIp = configStore_->getAgentIp();
    agentPort = configStore_->getAgentPort();
    wifiCount = configStore_->getWifiCount();
  }

  clearRegion(0, kContentTop, 240, kFooterY - kContentTop);
  M5Cardputer.Display.fillRect(0, kContentTop, 240, kFooterY - kContentTop, TFT_BLACK);
  int y = kContentTop + 4;
  drawTextLine(y, "WIFI", wifiStatusText());
  y += 12;
  drawTextLine(y, "SSID", wifiManager_ != nullptr ? truncateText(wifiManager_->getSSID(), 20) : "");
  y += 12;
  drawTextLine(y, "LAN", wifiManager_ != nullptr ? truncateText(wifiManager_->localIp(), 20) : "");
  y += 12;
  drawTextLine(y, "SAVE", String(wifiCount));
  y += 12;
  drawTextLine(y, "AGNT", truncateText(agentIp, 20));
  y += 12;
  drawTextLine(y, "PORT", String(agentPort));
  y += 12;
  drawTextLine(y, "AP", "PCMonitor-Setup");
  y += 12;
  drawTextLine(y, "WEB", "192.168.4.1");
  y += 14;
  M5Cardputer.Display.setTextColor(TFT_DARKGREY, TFT_BLACK);
  M5Cardputer.Display.drawString("[R] refresh", kPad + 2, y);
}

void MonitorUi::drawHeader(MonitorPage page) {
  auto& d = M5Cardputer.Display;
  d.setTextSize(1);
  d.setTextColor(TFT_CYAN, TFT_BLACK);
  d.drawString("SYS", kPad, 3);
  d.setTextColor(TFT_MAGENTA, TFT_BLACK);
  d.drawString("-LINK", 22, 3);

  const char* tabs[] = {"CPU", "IO", "GPU", "CFG"};
  MonitorPage pages[] = {MONITOR_PAGE_CPU_RAM, MONITOR_PAGE_DISK_NET, MONITOR_PAGE_GPU_SYS, MONITOR_PAGE_SETTINGS};
  int tx = 130;
  for (int i = 0; i < 4; ++i) {
    bool active = pages[i] == page;
    d.setTextColor(active ? accentForPage(pages[i]) : TFT_DARKGREY, TFT_BLACK);
    d.drawString(active ? String("[") + tabs[i] + "]" : String(" ") + tabs[i] + " ", tx, 3);
    tx += 26;
  }
  d.drawFastHLine(0, kHeaderH, d.width(), kDimLine);
}

void MonitorUi::drawStatusBar(const PcStats* stats) {
  auto& d = M5Cardputer.Display;

  String datetime = formatDateTime();
  int batt = M5Cardputer.Power.getBatteryLevel();

  String link = "----";
  uint16_t linkColor = TFT_DARKGREY;
  if (stats != nullptr) {
    link = linkStatusText(*stats);
    linkColor = linkStatusColor(*stats);
  }

  String wifiTag = "--";
  if (wifiManager_ != nullptr) {
    if (wifiManager_->isConnected()) {
      wifiTag = String(wifiManager_->getRSSI());
    } else if (wifiManager_->getStatus() == WIFI_AP_MODE) {
      wifiTag = "AP";
    }
  }

  String status = datetime + "  " + link + "  " + String(batt) + "%  " + wifiTag;
  if (status == lastStatusText_) {
    return;
  }
  lastStatusText_ = status;

  clearRegion(0, kStatusY, d.width(), kStatusH);
  d.fillRect(0, kStatusY, d.width(), kStatusH, kPanelBg);
  d.drawFastHLine(0, kStatusY + kStatusH - 1, d.width(), kDimLine);

  d.setTextSize(1);
  d.setTextColor(TFT_CYAN, kPanelBg);
  d.drawString(datetime, kPad, kStatusY + 2);

  int linkX = 128;
  d.setTextColor(linkColor, kPanelBg);
  d.drawString(link, linkX, kStatusY + 2);

  d.setTextColor(TFT_DARKGREY, kPanelBg);
  d.drawString(String(batt) + "%", 188, kStatusY + 2);
  d.setTextColor(TFT_MAGENTA, kPanelBg);
  d.drawString(wifiTag, 214, kStatusY + 2);
}

void MonitorUi::drawFooter() {
  auto& d = M5Cardputer.Display;
  d.drawFastHLine(0, kFooterY, d.width(), kDimLine);
  d.setTextColor(TFT_DARKGREY, TFT_BLACK);
  d.drawString("1 CPU  2 IO  3 GPU  4 CFG", kPad, kFooterY + 3);
}

void MonitorUi::drawSciFiPanel(int x, int y, int w, int h, uint16_t accent) {
  auto& d = M5Cardputer.Display;
  d.fillRect(x, y, w, h, kPanelBg);
  d.drawRect(x, y, w, h, accent);
  drawCornerBrackets(x, y, w, h, accent);
}

void MonitorUi::drawCornerBrackets(int x, int y, int w, int h, uint16_t color) {
  auto& d = M5Cardputer.Display;
  const int len = 5;
  d.drawFastHLine(x, y, len, color);
  d.drawFastVLine(x, y, len, color);
  d.drawFastHLine(x + w - len, y, len, color);
  d.drawFastVLine(x + w - 1, y, len, color);
  d.drawFastHLine(x, y + h - 1, len, color);
  d.drawFastVLine(x, y + h - len, len, color);
  d.drawFastHLine(x + w - len, y + h - 1, len, color);
  d.drawFastVLine(x + w - 1, y + h - len, len, color);
}

void MonitorUi::drawSegmentBar(int x, int y, int w, int h, float percent, uint16_t color) {
  auto& d = M5Cardputer.Display;
  if (percent < 0.0f) {
    percent = 0.0f;
  }
  if (percent > 100.0f) {
    percent = 100.0f;
  }
  clearRegion(x, y, w, h);
  const int segs = 16;
  int segW = (w - segs + 1) / segs;
  int filled = static_cast<int>((percent / 100.0f) * segs + 0.5f);
  for (int i = 0; i < segs; ++i) {
    int sx = x + i * (segW + 1);
    uint16_t c = i < filled ? color : TFT_DARKGREY;
    d.fillRect(sx, y, segW, h, c);
  }
}

void MonitorUi::drawValueAt(int x, int y, int w, const String& text, uint16_t color, uint8_t textSize) {
  auto& d = M5Cardputer.Display;
  clearRegion(x, y, w, textSize == 1 ? 10 : 16);
  d.setTextSize(textSize);
  d.setTextColor(color, kPanelBg);
  d.drawString(text, x, y);
  d.setTextSize(1);
}

void MonitorUi::drawSparkline(int x, int y, int w, int h) {
  auto& d = M5Cardputer.Display;
  clearRegion(x, y, w, h);
  d.drawRect(x, y, w, h, kDimLine);

  int count = cpuHistoryFilled_ ? kHistoryLen : cpuHistoryIdx_;
  if (count < 2) {
    return;
  }

  int step = max(1, w / kHistoryLen);
  int prevX = x;
  int prevY = y + h - 1;

  for (int i = 0; i < count; ++i) {
    int idx = cpuHistoryFilled_ ? (cpuHistoryIdx_ + i) % kHistoryLen : i;
    int px = x + i * step;
    if (px >= x + w - 1) {
      break;
    }
    int val = cpuHistory_[idx];
    if (val > 100) {
      val = 100;
    }
    int py = y + h - 1 - (val * (h - 2) / 100);
    if (i > 0) {
      d.drawLine(prevX, prevY, px, py, TFT_CYAN);
    }
    prevX = px;
    prevY = py;
  }
}

void MonitorUi::pushCpuHistory(float cpuUsage) {
  int val = static_cast<int>(cpuUsage);
  if (val < 0) {
    val = 0;
  }
  if (val > 100) {
    val = 100;
  }
  cpuHistory_[cpuHistoryIdx_] = static_cast<uint8_t>(val);
  cpuHistoryIdx_ = (cpuHistoryIdx_ + 1) % kHistoryLen;
  if (cpuHistoryIdx_ == 0) {
    cpuHistoryFilled_ = true;
  }
}

void MonitorUi::clearRegion(int x, int y, int w, int h) {
  M5Cardputer.Display.fillRect(x, y, w, h, kPanelBg);
}

uint16_t MonitorUi::tempColor(float tempC) const {
  if (tempC < 0) {
    return TFT_DARKGREY;
  }
  if (tempC < 60.0f) {
    return TFT_GREEN;
  }
  if (tempC < 80.0f) {
    return TFT_YELLOW;
  }
  return TFT_RED;
}

uint16_t MonitorUi::accentForPage(MonitorPage page) const {
  switch (page) {
    case MONITOR_PAGE_DISK_NET:
      return TFT_MAGENTA;
    case MONITOR_PAGE_GPU_SYS:
      return TFT_ORANGE;
    case MONITOR_PAGE_SETTINGS:
      return TFT_WHITE;
    case MONITOR_PAGE_CPU_RAM:
    default:
      return TFT_CYAN;
  }
}

String MonitorUi::formatTemp(float tempC) const {
  if (tempC < 0) {
    return "-- C";
  }
  return String(tempC, 0) + " C";
}

String MonitorUi::formatFreqMhz(float mhz) const {
  if (mhz <= 0) {
    return "-- GHz";
  }
  if (mhz >= 1000.0f) {
    return String(mhz / 1000.0f, 1) + " GHz";
  }
  return String(static_cast<int>(mhz)) + " MHz";
}

String MonitorUi::formatPercent(float value) const {
  if (value < 0.0f) {
    return "--%";
  }
  if (value < 10.0f) {
    return String(value, 1) + "%";
  }
  return String(value, 0) + "%";
}

String MonitorUi::formatDateTime() const {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 0)) {
    return "--/--/-- --:--:--";
  }
  char buf[20];
  strftime(buf, sizeof(buf), "%Y/%m/%d %H:%M:%S", &timeinfo);
  return String(buf);
}

void MonitorUi::drawTextLine(int y, const String& label, const String& value, uint16_t color) {
  auto& d = M5Cardputer.Display;
  d.setTextColor(TFT_DARKGREY, TFT_BLACK);
  d.drawString(label, kPad + 2, y);
  d.setTextColor(color, TFT_BLACK);
  d.drawString(value, kPad + 36, y);
}

String MonitorUi::truncateText(const String& value, size_t maxLength) const {
  if (value.length() <= maxLength) {
    return value;
  }
  if (maxLength <= 3) {
    return value.substring(0, maxLength);
  }
  return value.substring(0, maxLength - 3) + "...";
}

String MonitorUi::wifiStatusText() const {
  if (wifiManager_ == nullptr) {
    return "?";
  }
  if (wifiManager_->isConnected()) {
    return String(wifiManager_->getRSSI()) + "dBm";
  }
  if (wifiManager_->getStatus() == WIFI_AP_MODE) {
    return "AP";
  }
  if (wifiManager_->getStatus() == WIFI_CONNECTING) {
    return "...";
  }
  return "X";
}

String MonitorUi::linkStatusText(const PcStats& stats) const {
  if (stats.online) {
    return "LINK";
  }
  if (stats.error.indexOf("timeout") >= 0 || stats.error.indexOf("Timeout") >= 0) {
    return "SLOW";
  }
  if (stats.error.indexOf("offline") >= 0 || stats.error.indexOf("Offline") >= 0) {
    return "DOWN";
  }
  return "DOWN";
}

uint16_t MonitorUi::linkStatusColor(const PcStats& stats) const {
  if (stats.online) {
    return TFT_GREEN;
  }
  if (stats.error.indexOf("timeout") >= 0 || stats.error.indexOf("Timeout") >= 0) {
    return TFT_YELLOW;
  }
  return TFT_RED;
}

bool MonitorUi::keyWordContains(const Keyboard_Class::KeysState& keys, char expected) const {
  for (auto key : keys.word) {
    String keyText = String(key);
    if (keyText.length() > 0 && keyText.charAt(0) == expected) {
      return true;
    }
  }
  return false;
}
