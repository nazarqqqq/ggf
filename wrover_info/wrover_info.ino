// ESP32-WROVER: полный отчёт о модуле в Serial Monitor
// Скорость порта: 115200

#include <Arduino.h>
#include <WiFi.h>
#include <esp_system.h>

const char* resetReasonText(esp_reset_reason_t r) {
  switch (r) {
    case ESP_RST_POWERON:   return "POWERON (включение питания)";
    case ESP_RST_EXT:       return "EXT (внешний сброс EN)";
    case ESP_RST_SW:        return "SW (программный сброс)";
    case ESP_RST_PANIC:     return "PANIC (краш/исключение)";
    case ESP_RST_INT_WDT:   return "INT_WDT (watchdog)";
    case ESP_RST_TASK_WDT:  return "TASK_WDT (watchdog задачи)";
    case ESP_RST_WDT:       return "WDT (другой watchdog)";
    case ESP_RST_DEEPSLEEP: return "DEEPSLEEP (выход из сна)";
    case ESP_RST_BROWNOUT:  return "BROWNOUT (просадка питания!)";
    case ESP_RST_SDIO:      return "SDIO";
    default:                return "UNKNOWN";
  }
}

const char* flashModeText(FlashMode_t m) {
  switch (m) {
    case FM_QIO:  return "QIO";
    case FM_QOUT: return "QOUT";
    case FM_DIO:  return "DIO";
    case FM_DOUT: return "DOUT";
    default:      return "UNKNOWN";
  }
}

bool testPsram(size_t bytes) {
  uint8_t* buf = (uint8_t*)ps_malloc(bytes);
  if (!buf) return false;
  for (size_t i = 0; i < bytes; i++) buf[i] = (uint8_t)(i * 31 + 7);
  bool ok = true;
  for (size_t i = 0; i < bytes; i++) {
    if (buf[i] != (uint8_t)(i * 31 + 7)) { ok = false; break; }
  }
  free(buf);
  return ok;
}

void printReport() {
  Serial.println();
  Serial.println("==================== ESP32 REPORT ====================");

  Serial.println("--- Чип ---");
  Serial.printf("Модель:            %s\n", ESP.getChipModel());
  Serial.printf("Ревизия:           %d\n", ESP.getChipRevision());
  Serial.printf("Ядер:              %d\n", ESP.getChipCores());
  Serial.printf("Частота CPU:       %u МГц\n", ESP.getCpuFreqMHz());
  uint64_t mac = ESP.getEfuseMac();
  Serial.printf("eFuse MAC / ID:    %04X%08X\n", (uint16_t)(mac >> 32), (uint32_t)mac);
  Serial.printf("WiFi MAC:          %s\n", WiFi.macAddress().c_str());
  Serial.printf("Темп. ядра (≈):    %.1f C (грубая оценка)\n", temperatureRead());
  Serial.printf("Причина сброса:    %s\n", resetReasonText(esp_reset_reason()));

  Serial.println("--- Flash ---");
  Serial.printf("Размер flash:      %u МБ (%u байт)\n", ESP.getFlashChipSize() / (1024 * 1024), ESP.getFlashChipSize());
  Serial.printf("Частота flash:     %u МГц\n", ESP.getFlashChipSpeed() / 1000000);
  Serial.printf("Режим flash:       %s\n", flashModeText(ESP.getFlashChipMode()));
  Serial.printf("Размер скетча:     %u байт\n", ESP.getSketchSize());
  Serial.printf("Свободно под OTA:  %u байт\n", ESP.getFreeSketchSpace());

  Serial.println("--- Память ---");
  Serial.printf("Heap всего:        %u байт\n", ESP.getHeapSize());
  Serial.printf("Heap свободно:     %u байт\n", ESP.getFreeHeap());
  Serial.printf("Heap мин. было:    %u байт\n", ESP.getMinFreeHeap());
  if (psramFound()) {
    Serial.printf("PSRAM всего:       %u байт (%.1f МБ)\n", ESP.getPsramSize(), ESP.getPsramSize() / 1048576.0);
    Serial.printf("PSRAM свободно:    %u байт\n", ESP.getFreePsram());
    size_t testSize = 1024 * 1024;
    Serial.printf("Тест PSRAM 1 МБ:   %s\n", testPsram(testSize) ? "OK" : "ОШИБКА");
  } else {
    Serial.println("PSRAM:             НЕ НАЙДЕНА (или отключена в настройках платы)");
  }

  Serial.println("--- ПО ---");
  Serial.printf("ESP-IDF SDK:       %s\n", ESP.getSdkVersion());
  Serial.printf("Arduino core:      %d.%d.%d\n", ESP_ARDUINO_VERSION_MAJOR, ESP_ARDUINO_VERSION_MINOR, ESP_ARDUINO_VERSION_PATCH);

  Serial.println("--- Тест WiFi (скан сетей) ---");
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  int n = WiFi.scanNetworks();
  if (n < 0) {
    Serial.println("Скан: ОШИБКА");
  } else if (n == 0) {
    Serial.println("Сетей не найдено (антенна/радио? проверь)");
  } else {
    Serial.printf("Найдено сетей: %d\n", n);
    for (int i = 0; i < n && i < 8; i++) {
      Serial.printf("  %d) %-24s RSSI %d дБм, канал %d\n", i + 1, WiFi.SSID(i).c_str(), WiFi.RSSI(i), WiFi.channel(i));
    }
  }
  WiFi.scanDelete();

  Serial.println("======================================================");
  Serial.println("Отправь любой символ в монитор — отчёт повторится.");
}

void setup() {
  Serial.begin(115200);
  delay(2000);  // время подключить монитор порта
  printReport();
}

unsigned long lastReport = 0;

void loop() {
  if (Serial.available()) {
    while (Serial.available()) Serial.read();
    printReport();
    lastReport = millis();
  }
  if (millis() - lastReport > 15000) {
    printReport();
    lastReport = millis();
  }
  delay(20);
}
