#include <iostream>
#include <map>
#include <climits>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        return 1;
    }

    uint32_t start = { 0u };
    uint32_t finish = std::stoul(argv[1]);
    uint32_t draws_count = { 0u };

    std::map<uint32_t, uint32_t> ranges;
    uint32_t interval_count = finish > 1000 ? finish / 10 : finish;
    uint32_t interval_size = finish / interval_count;
    uint32_t max_freq = 0u;
    uint32_t min_freq = UINT_MAX;

    uint64_t num;
    while (std::cin >> num) {
        ranges[num / interval_size]++;
        draws_count++;
    }

    std::cout << "Freqs:\n";
    for (auto&& item: ranges) {
        std::cout << item.second << " ";
        if (item.second > max_freq) {
            max_freq = item.second;
        }

        if (item.second < min_freq) {
            min_freq = item.second;
        }
    }
    std::cout << "\n\n";

    auto target_freq = (double)draws_count / finish;

    std::cout << "min max freq diff(abs): " << max_freq - min_freq << "\n";
    std::cout << "min max freq diff(rel): " << double(max_freq - min_freq) / target_freq << "\n";
    std::cout << "max freq deviation(abs): " << std::max(max_freq - target_freq, target_freq - min_freq) << "\n";
    std::cout << "max freq deviation(rel): " << std::max((max_freq - target_freq) / target_freq, (target_freq - min_freq) / target_freq) << "\n";
}
