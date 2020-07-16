#pragma once

#include <game-contract-sdk/game_base.hpp>

namespace prng {

using eosio::checksum256;
using eosio::name;

// stub game contract to test prng
class [[eosio::contract]] prng : public game_sdk::game {
  public:
    static constexpr uint16_t prng_game_action_type{0u};

    struct [[eosio::table("state")]] state_row {
        uint64_t rnd_range;
        uint64_t positions;
    };
    using state_singleton = eosio::singleton<"state"_n, state_row>;

  public:
    prng(name receiver, name code, eosio::datastream<const char*> ds) : game(receiver, code, ds) {
        state = state_singleton(_self, _self.value).get_or_default();
    }

    ~prng() {
        state_singleton(_self, _self.value).set(state, _self);
    }

    virtual void on_new_game(uint64_t ses_id) final;

    virtual void on_action(uint64_t ses_id, uint16_t type, std::vector<game_sdk::param_t> params) final;

    virtual void on_random(uint64_t ses_id, checksum256 rand) final;

    virtual void on_finish(uint64_t ses_id) final;

  private:
    state_row state;
};

} // namespace prng
