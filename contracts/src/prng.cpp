#include <prng/prng.hpp>

namespace prng {

void prng::on_new_game(uint64_t ses_id) {
    // always require `prng` action
    require_action(prng_game_action_type);
}

void prng::on_action(uint64_t ses_id, uint16_t type, std::vector<game_sdk::param_t> params) {
    eosio::check(params.size() == 1, "invalid params count");
    state.rnd_range = params[0];

    // always require random
    require_random();
}

void prng::on_random(uint64_t ses_id, checksum256 rand_seed) {
    // player always return deposit
    auto new_random = get_prng(std::move(rand_seed))->next() % state.rnd_range;
    finish_game(get_session(ses_id).deposit, {{ new_random }});
}

void prng::on_finish(uint64_t ses_id) {
    // do nothing
}

} // namespace prng

GAME_CONTRACT(prng::prng)
