#define NON_VALIDATING_TEST
#include <game_tester/game_tester.hpp>

#include <boost/program_options.hpp>
#include <boost/multiprecision/cpp_int.hpp>

#include <iostream>
#include <optional>
#include <string>
#include <vector>
#include <chrono>

#include "contracts.hpp"


using namespace testing;
using namespace boost::unit_test;
namespace po = boost::program_options;
namespace mp = boost::multiprecision;

struct colors {
    inline static const std::string red   = "\033[1;31m";
    inline static const std::string green = "\033[1;32m";
    inline static const std::string reset = "\033[0m";
};


// tester class
class prng_tester : public game_tester {
  public:
    static const name contract_name;
    static const name player_name;

    static constexpr uint16_t prng_action_type{0u};

  public:
    prng_tester() {
        create_account(contract_name);

        // deploy PRNG contract
        deploy_game<prng_stub>(contract_name, {});

        // create tester account
        create_player(player_name);

        // transfer initial funds
        transfer(N(eosio), player_name, STRSYM("1000.0000"));
        transfer(N(eosio), casino_name, STRSYM("1000.0000"));
    }

    // generate next random numbers line
    std::vector<uint64_t> line(uint64_t range, uint64_t positions) {
        auto player_bet = STRSYM("1.0000"); ///< player's bet amount, that stuff doesn't affect to random result

        // create new game session
        auto ses_id = new_game_session(contract_name, player_name, casino_id, player_bet);

        // request new random
        game_action(contract_name, ses_id, 0, { range, positions });

        // process signidice protocol
        signidice(contract_name, ses_id);

        // obtain return value with generated random
        const auto finish_event = get_events(events_id::game_finished);
        const auto numbers = fc::raw::unpack<std::vector<uint64_t>>(finish_event.value()[0]["msg"].as<bytes>());

        return numbers;
    }

    // generate raw 256-bit random number
    mp::uint256_t raw() {
        auto player_bet = STRSYM("1.0000"); ///< player's bet amount, that stuff doesn't affect to random result

        // create new game session
        auto ses_id = new_game_session(contract_name, player_name, casino_id, player_bet);

        // request new random
        game_action(contract_name, ses_id, 0, { 0, 0 });

        // process signidice protocol
        signidice(contract_name, ses_id);

        // obtain return value with generated random
        const auto message_event = get_events(events_id::game_message);
        const auto raw_sha256 = fc::raw::unpack<sha256>(message_event.value()[0]["msg"].as<bytes>());

        // sha256 to bmp number convertion
        mp::uint256_t number = 0u;
        number |= raw_sha256._hash[0];
        number = number << 64;
        number |= raw_sha256._hash[1];
        number = number << 64;
        number |= raw_sha256._hash[2];
        number = number << 64;
        number |= raw_sha256._hash[3];

        return number;
    }
};

const name prng_tester::contract_name = N(prng);
const name prng_tester::player_name = N(player);


void print_greeting() {
    std::cout << colors::green
        << "=================================================\n"
        << "============== STARTING PRNG TEST ===============\n"
        << "=================================================\n"
        << colors::reset;
}

// cli options parser
po::variables_map parse_cli_args() {
    po::options_description desc("PRNG tester options");
    desc.add_options()
        ("help,h", "produce help message")
        ("seed,s", po::value<uint64_t>(), "initial seed value")
        ("count,c", po::value<uint32_t>(), "iterations amount")
        ("range,r", po::value<uint64_t>(), "random range")
        ("columns,c", po::value<uint64_t>(), "columns count")
        ("out,o", po::value<std::string>(), "output file")
        ("raw", po::value<bool>(), "generate raw numbers")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(framework::master_test_suite().argc, framework::master_test_suite().argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << "\n";
        exit(0);
    }

    return vm;
}


// main test scenario
BOOST_AUTO_TEST_CASE(prng_test) {
    print_greeting();

    auto opts = parse_cli_args();

    if (opts.count("seed")) {
        auto seed = opts["seed"].as<uint64_t>();
        std::cout << "Random number generator "
            << colors::green << "RESEEDED" << colors::reset << " to "
            << colors::red << seed << colors::reset << "\n\n";
        srand(seed);
    }

    uint32_t iters = 10u; ///< default iterations amount
    uint64_t range = UINT64_MAX; ///< default random range
    uint64_t columns = 1u; //< default columns count

    std::ostream* output = &std::cout;
    std::ofstream file_out;

    if (opts.count("count")) {
        iters = opts["count"].as<uint32_t>();
    }

    if (opts.count("range")) {
        range = opts["range"].as<uint64_t>();
    }

    if (opts.count("columns")) {
        columns = opts["columns"].as<uint64_t>();
    }

    if (opts.count("out")) {
        file_out.open(opts["out"].as<std::string>());
        output = &file_out;
        std::cout << "Results will be saved to '" << opts["out"].as<std::string>() << "' file\n";
    }

    auto start_time = std::chrono::system_clock::now();

    auto tester = std::make_shared<prng_tester>();

    for (auto i = 1; i <= iters; ++i) {
        if (!opts.count("raw")) {
            auto new_line = tester->line(range, columns);
            for (auto && item: new_line) {
                *output << item << "\t";
            }

            *output << "\n";
        }
        else {
            auto raw_number = tester->raw();
            *output << raw_number << "\n";
        }

        // recreate new tester to release memory
        if (i % 1000 == 0) {
            tester = std::make_shared<prng_tester>();
        }

        // if out to file print status
        if (opts.count("out") && (i % 100 == 0 || i == iters)) {
            auto pct = (double)i / iters * 100;
            std::cout
                << "Current status: " << i << "/" << iters
                << ", processed " << std::fixed << std::setprecision(2) << pct << "%"
                << "\n" << std::flush;
        }
    }

    auto end_time = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end_time - start_time;

    std::cout << colors::green
        << "\nTest succesfully completed, "
        << "elapsed time: " << elapsed_seconds.count() << "s"
        << colors::reset << "\n";
}

