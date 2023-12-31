#pragma once

#include <ArduinoJson.h>
#include <ctime>
#include <esp_http_client.h>
#include <optional>
#include <string>
#include <vector>

struct Trip {
    const std::string tripId;
    const std::optional<std::chrono::system_clock::time_point> departureTime;
    const std::string directionName;
    const std::string lineName;
};

class BvgApiClient {
  public:
    BvgApiClient(const std::string &stationId);
    ~BvgApiClient();
    std::vector<Trip> fetchAndParseTrips(const std::string &stationId, const std::vector<std::string> &enabledProducts);

  private:
    esp_http_client_handle_t client;
    esp_err_t http_event_handler(esp_http_client_event_t *evt);
    void setUrl(const std::string &stationId, const std::vector<std::string> &enabledProducts);
    int buffer_pos = 0;

    static constexpr const int N_RESULTS = 12;
};
