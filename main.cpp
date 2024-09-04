#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <cmath>
#include <algorithm>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Callback function for cURL (unchanged)
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t total_size = size * nmemb;
    output->append((char*)contents, total_size);
    return total_size;
}

class TimeSeriesAnalyzer {
private:
    std::string api_key;
    std::string symbol;
    int update_interval;
    std::vector<double> prices;
    std::vector<long long> timestamps;

    // Fetch and parse data methods (unchanged)
    std::string fetchData() {
        CURL* curl;
        CURLcode res;
        std::string readBuffer;

        curl = curl_easy_init();
        if (curl) {
            std::string url = "https://www.alphavantage.co/query?function=TIME_SERIES_INTRADAY&symbol=" +
                              symbol + "&interval=1min&apikey=" + api_key;

            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
            res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);
        }
        return readBuffer;
    }

    void parseData(const std::string& data) {
        json j = json::parse(data);
        auto time_series = j["Time Series (1min)"];

        prices.clear();
        timestamps.clear();

        for (auto it = time_series.begin(); it != time_series.end(); ++it) {
            double price = std::stod(it.value()["4. close"].get<std::string>());
            prices.push_back(price);

            std::string timestamp = it.key();
            std::tm tm = {};
            std::istringstream ss(timestamp);
            ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
            auto tp = std::chrono::system_clock::from_time_t(std::mktime(&tm));
            auto duration = tp.time_since_epoch();
            auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
            timestamps.push_back(millis);
        }
    }

    // Moving Averages
    double calculateSMA(int period) {
        if (prices.size() < period) return 0;
        return std::accumulate(prices.begin(), prices.begin() + period, 0.0) / period;
    }

    double calculateEMA(int period) {
        if (prices.size() < period) return 0;
        double alpha = 2.0 / (period + 1);
        double ema = prices[0];
        for (int i = 1; i < period; i++) {
            ema = alpha * prices[i] + (1 - alpha) * ema;
        }
        return ema;
    }

    double calculateWMA(int period) {
        if (prices.size() < period) return 0;
        double sum = 0;
        double weightSum = 0;
        for (int i = 0; i < period; i++) {
            double weight = period - i;
            sum += prices[i] * weight;
            weightSum += weight;
        }
        return sum / weightSum;
    }

    // Volatility Calculations
    double calculateStandardDeviation(int period) {
        if (prices.size() < period) return 0;
        double mean = calculateSMA(period);
        double sum_sq_diff = 0;
        for (int i = 0; i < period; i++) {
            sum_sq_diff += std::pow(prices[i] - mean, 2);
        }
        return std::sqrt(sum_sq_diff / period);
    }

    double calculateBollingerBands(int period, double multiplier, bool upper) {
        double sma = calculateSMA(period);
        double std_dev = calculateStandardDeviation(period);
        return upper ? sma + multiplier * std_dev : sma - multiplier * std_dev;
    }

    // Trend Detection
    std::string detectTrend(int shortPeriod, int longPeriod) {
        if (prices.size() < longPeriod) return "Insufficient data";

        double shortMA = calculateSMA(shortPeriod);
        double longMA = calculateSMA(longPeriod);

        if (shortMA > longMA) {
            return "Uptrend";
        } else if (shortMA < longMA) {
            return "Downtrend";
        } else {
            return "Sideways";
        }
    }

    double calculateRSI(int period) {
        if (prices.size() < period + 1) return 0;

        double avg_gain = 0, avg_loss = 0;
        for (int i = 1; i <= period; i++) {
            double change = prices[i-1] - prices[i];
            if (change > 0) {
                avg_gain += change;
            } else {
                avg_loss -= change;
            }
        }
        avg_gain /= period;
        avg_loss /= period;

        double rs = avg_gain / avg_loss;
        return 100 - (100 / (1 + rs));
    }

public:
    TimeSeriesAnalyzer(const std::string& api_key, const std::string& symbol, int update_interval)
        : api_key(api_key), symbol(symbol), update_interval(update_interval) {}

    void run() {
        while (true) {
            std::string data = fetchData();
            parseData(data);

            std::cout << "Analysis for " << symbol << ":\n";
            std::cout << "Latest price: $" << prices[0] << std::endl;
            std::cout << "Moving Averages:\n";
            std::cout << "  5-period SMA: $" << calculateSMA(5) << std::endl;
            std::cout << "  10-period EMA: $" << calculateEMA(10) << std::endl;
            std::cout << "  20-period WMA: $" << calculateWMA(20) << std::endl;

            std::cout << "Volatility:\n";
            std::cout << "  20-period Standard Deviation: $" << calculateStandardDeviation(20) << std::endl;
            std::cout << "  20-period Bollinger Bands: $" << calculateBollingerBands(20, 2, true) << " (upper), $"
                      << calculateBollingerBands(20, 2, false) << " (lower)" << std::endl;

            std::cout << "Trend Detection:\n";
            std::cout << "  Short-term trend (10 vs 30 periods): " << detectTrend(10, 30) << std::endl;
            std::cout << "  14-period RSI: " << calculateRSI(14) << std::endl;

            std::cout << "\nNext update in " << update_interval << " seconds...\n\n";
            std::this_thread::sleep_for(std::chrono::seconds(update_interval));
        }
    }
};

int main() {
    std::string api_key = "your_api_key";
    std::string symbol;
    int update_interval;

    std::cout << "Enter stock symbol: ";
    std::cin >> symbol;
    std::cout << "Enter update interval (in seconds): ";
    std::cin >> update_interval;

    TimeSeriesAnalyzer analyzer(api_key, symbol, update_interval);
    analyzer.run();

    return 0;
}