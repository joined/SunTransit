#include <esp_log.h>
#include <esp_sntp.h>
#include <sys/time.h>
#include <time.h>

#include "time.hpp"

static const char *TAG = "Time";
static auto synced = false;

namespace Time {
void callbackOnNtpUpdate(timeval *tv) {
    ESP_LOGI(TAG, "NTP updated, current time is: %s", timeNowAscii().c_str());
    synced = true;
};

esp_err_t initSNTP() {
    if (!synced) {
        setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
        tzset();

        esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
        esp_sntp_setservername(0, "time.google.com");
        esp_sntp_setservername(1, "pool.ntp.com");

        sntp_set_time_sync_notification_cb(&callbackOnNtpUpdate);
        sntp_set_sync_interval(60 * 60 * 1000);

        esp_sntp_init();

        ESP_LOGD(TAG, "SNTP Initialized");
    }

    if (synced) {
        return ESP_OK;
    }
    return ESP_FAIL;
}

const std::chrono::system_clock::time_point timePointNow() { return std::chrono::system_clock::now(); }

int64_t epochMillis() {
    auto result = std::chrono::duration_cast<std::chrono::milliseconds>(timePointNow().time_since_epoch());
    auto count = result.count();
    return count;
}

std::string timeNowAscii() {
    const std::time_t timeNow{std::chrono::system_clock::to_time_t(timePointNow())};

    return std::asctime(std::localtime(&timeNow));
}

const std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>
iSO8601StringToTimePoint(const std::string &iso8601) {
    std::tm t = {};
    // F: Equivalent to %Y-%m-%d, the ISO 8601 date format.
    // T: ISO 8601 time format (HH:MM:SS), equivalent to %H:%M:%S
    auto result = strptime(iso8601.c_str(), "%FT%T", &t);
    if (result == nullptr) {
        ESP_LOGE(TAG, "Failed to parse ISO8601 string: %s", iso8601.c_str());
        return std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>();
    }

    // The BVG API returns times in Berlin local time (CET/CEST).
    // We need to interpret the parsed time as already being in the correct local timezone,
    // without applying any additional timezone conversion.
    // Since mktime() applies local timezone conversion, we need to compensate for it.

    // Get the current timezone offset to compensate for mktime's conversion
    auto now = time(nullptr);
    auto local_tm = *localtime(&now);
    auto utc_tm = *gmtime(&now);

    // Calculate timezone offset in seconds (local - UTC)
    auto local_time = mktime(&local_tm);
    auto utc_time = mktime(&utc_tm);
    auto tz_offset = local_time - utc_time;

    // Use mktime to get the timestamp, then subtract the timezone offset
    // to get the actual time as intended by the BVG API
    auto time = mktime(&t);
    if (time == -1) {
        ESP_LOGE(TAG, "Failed to convert std::tm to std::time_t");
        return std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>();
    }

    // Subtract the timezone offset to get the correct time
    time -= tz_offset;

    return std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>(std::chrono::seconds(time));
}
} // namespace Time
