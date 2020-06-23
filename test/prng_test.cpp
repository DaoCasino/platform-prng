#include <game_tester/game_tester.hpp>

#include <boost/program_options.hpp>

#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include "contracts.hpp"

using namespace testing;
using namespace boost::unit_test;
namespace po = boost::program_options;


struct colors {
    inline static const std::string red   = "\033[1;31m";
    inline static const std::string green = "\033[1;32m";
    inline static const std::string reset = "\033[0m";
};

class prng_tester : public game_tester {
  public:
    static const name contract_name;
    static const name player_name;

    static constexpr uint16_t prng_action_type{0u};

  public:
    prng_tester() {
        create_account(contract_name);

        deploy_game<prng_stub>(contract_name, {});

        create_player(player_name);

        transfer(N(eosio), player_name, STRSYM("1000.0000"));
        transfer(N(eosio), casino_name, STRSYM("1000.0000"));
    }

    uint64_t next(uint64_t range) {
        auto player_bet = STRSYM("1.0000");
        auto ses_id = new_game_session(contract_name, player_name, casino_id, player_bet);

        game_action(contract_name, ses_id, 0, { range });

        signidice(contract_name, ses_id);

        const auto finish_event = get_events(events_id::game_finished);
        const auto values = fc::raw::unpack<std::vector<uint64_t>>(finish_event.value()[0]["msg"].as<bytes>());

        return values[0];
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

po::variables_map parse_cli_args() {
    po::options_description desc("Test options");
    desc.add_options()
        ("help,h", "produce help message")
        ("seed,s", po::value<uint64_t>(), "initial seed value")
        ("count,c", po::value<uint32_t>(), "iterations amount")
        ("range,r", po::value<uint64_t>(), "random range")
        ("out,o", po::value<std::string>(), "output file")
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



BOOST_AUTO_TEST_CASE(prng_test) {
    print_greeting();

    auto opts = parse_cli_args();

    if (opts.count("seed")) {
        auto seed = opts["seed"].as<uint64_t>();
        std::cout << "Random number generator "
            << colors::green << "RESEEDED" << colors::reset << " to "
            << seed << "\n";
        srand(seed);
    }

    uint32_t iters = 10u;
    uint64_t range = UINT64_MAX;
    std::ofstream file_out;

    if (opts.count("count")) {
        iters = opts["count"].as<uint32_t>();
    }

    if (opts.count("range")) {
        range = opts["range"].as<uint64_t>();
    }

    if (opts.count("out")) {
        file_out.open(opts["out"].as<std::string>());
        std::cout << "Results will be saved to '" << opts["out"].as<std::string>() << "' file\n";
    }

    auto tester = std::make_shared<prng_tester>();

    for (auto i = 0; i < iters; ++i) {
        if (file_out.is_open()) {
            file_out << tester->next(range) << "\n";
        } else {
            std::cout << tester->next(range) << "\n";
        }

        // recreate new tester
        if (i % 1000 == 0) {
            tester = std::make_shared<prng_tester>();
        }
    }
}
