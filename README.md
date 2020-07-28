# PRNG

Original idea: [signidice](https://github.com/gluk256/misc/blob/master/rng4ethereum/signidice.md)

## Repo structure

Main components:
- Contract SDK ([link](https://github.com/DaoCasino/game-contract-sdk/tree/491a2e552842687cfda2fa963390b4f5f361c2a0)) - SDK for writing, building and testing game contracts.
- RNG contract ([link](./contracts)) - stub game contract to test RNG generator.
- RNG test ([link](./test)) - RNG test runner.

## Q&A

### What is the number and type of games that will rely on this RNG?
 - Requirements for games:
   - Two parties only (e.g. a casino vs. a player);
   - The game logic may require random number generation;
   - When either party makes a move, a certain end state can be traced to choose the winner and/or distribute funds among participants.
   - For example Dice, Roulette, Slots

### What operating system does it run on?

 - That example by default run in docker, so that can be run on any docker supported OS

### Programming languages used (Java, C++, C# etc...)?

 - We use C++ for Smart Contracts and Test runner
 - And a little bit of bash only for running the scripts

### Is it a hardware or software-based RNG or a combination of both? Please provide any relevant details

 - software-based

### Type of mathematical algorithm used;

 - 2048-bit RSA Signature + SHA-256 hash function

### Mathematical details of the RNG’s period;

 - Our solution has no RNG period, as we use a new unique seed each time a random number is generated. This seed is obtained by combining the user's game data (such as session number, game ID, etc.).

### Mathematical details of the RNG’s range (e.g., minimum value, maximum value);

 - original signidice range - ```[0, 2^256 - 1] (hash size)```
 - RPNG range - ```[0, 2^64 - 1] (uint64_t size)```

### Mathematical details of the methods for seeding and reseeding (e.g., seed entropy source, frequency of reseeding, size of a seed, etc.);

 - seed type: *uint64_t*
 - default test seeding approach:
 ```c++
    const auto seed = time(NULL);
    std::srand(seed);

    // our code uses c++ std::rand() function for all further random generation
 ```
 - you can provide a custom `uint64_t` seed to test (see running examples below)


### Details of all RNG / game implementation, including methods of scaling and mapping from raw RNG output to a game outcome.

 - RNG related code(more details: [link](https://github.com/DaoCasino/game-contract-sdk/tree/491a2e552842687cfda2fa963390b4f5f361c2a0/sdk/include/game-contract-sdk/service.hpp)):

```c++
// ===================================================================
// Utility functions for operations with different types of PRNG seed
// ===================================================================
template <typename T,
          class = std::enable_if_t<std::is_unsigned<T>::value>>
T cut_to(const checksum256& input) {
    return cut_to<uint128_t>(input) % std::numeric_limits<T>::max();
}

template <> uint128_t cut_to(const checksum256& input) {
    const auto& parts = input.get_array();
    const uint128_t left = parts[0] % std::numeric_limits<uint64_t>::max();
    const uint128_t right = parts[1] % std::numeric_limits<uint64_t>::max();
    return (left << (sizeof(uint64_t) * 8)) | right;
}

std::array<uint64_t, 4> split(const checksum256& raw) {
    const auto& parts = raw.get_array();
    return std::array<uint64_t, 4>{
        uint64_t(parts[0] >> 64),
        uint64_t(parts[0]),
        uint64_t(parts[1] >> 64),
        uint64_t(parts[1]),
    };
}


// ===================================================================
// Different PRNG implementations
// ===================================================================

/* Generic PRNG interface */
struct PRNG {
    using Ptr = std::shared_ptr<PRNG>;

    virtual ~PRNG() {};
    // generates numbers in range [from, to)
    virtual uint64_t next(uint64_t from, uint64_t to) = 0;
};

/**
   Implementation of generator based on sha256 mixing with rejection scheme
   details: https://github.com/DaoCasino/PRNG/blob/master/PRNG.pdf
*/
class ShaMixWithRejection : public PRNG {
  public:
    constexpr static intx::uint256 UINT256_MAX = ~intx::uint256(0);

  public:
    explicit ShaMixWithRejection(const checksum256& seed) : _s(to_intx(seed)) {}
    explicit ShaMixWithRejection(checksum256&& seed) : _s(to_intx(seed)) {}

    uint64_t next(uint64_t from, uint64_t to) override {
        eosio::check(to > from, "invalid random range");
        eosio::check(_cur_iter < UINT32_MAX, "too many next() calls");

        const intx::uint256 delta(to - from);
        const intx::uint256 cut_threshold = UINT256_MAX / delta * delta;

        auto lucky_as_hash = mix_bytes();
        auto lucky = to_intx(lucky_as_hash);

        while (lucky >= cut_threshold) {
            auto lucky_bytes = lucky_as_hash.extract_as_byte_array();
            lucky_as_hash = eosio::sha256(reinterpret_cast<const char*>(lucky_bytes.data()), 32);
            lucky = to_intx(lucky_as_hash);
        }

        return uint64_t(lucky % delta + from);
    }

  private:
    // 32bytes of seed and 4 of counter
    checksum256 mix_bytes() {
        static_assert(sizeof(_s) == 32, "invalid `_s` size, should be 32bytes");
        std::array<uint8_t, 36> arr;
        std::memcpy(arr.data(), &_s, sizeof(_s));
        std::memcpy(arr.data() + 32, &_cur_iter, sizeof(_cur_iter));

        _cur_iter++;

        return eosio::sha256(reinterpret_cast<const char*>(arr.data()), arr.size());
    }

    static intx::uint256 to_intx(const checksum256& hash) {
        auto parts = hash.get_array();
        return intx::uint256(
            intx::uint128(uint64_t(parts[0] >> 64), uint64_t(parts[0])),
            intx::uint128(uint64_t(parts[1] >> 64), uint64_t(parts[1]))
        );
    }


  private:
    const intx::uint256 _s;
    uint32_t _cur_iter { 0u };
};


// ===================================================================
// Shuffler functions
// ===================================================================

/* Shuffler overload for r-value prng */
template <class RandomIt>
void shuffle(RandomIt first, RandomIt last, PRNG::Ptr&& prng) { shuffle(first, last, prng); }

/* Standard shuffler implementation proposed by https://en.cppreference.com/w/cpp/algorithm/random_shuffle */
template <class RandomIt>
void shuffle(RandomIt first, RandomIt last, PRNG::Ptr& prng) {
    typename std::iterator_traits<RandomIt>::difference_type i, n;
    n = last - first;
    for (i = n - 1; i > 0; --i) {
        std::swap(first[i], first[prng->next() % (i + 1)]);
    }
}

// ===================================================================
// SIGNIDICE functions
// ===================================================================

/**
   Signidice sign check and calculation of a new 256-bit digest.
   Params:
    - prev_digest - signing digest
    - sign - rsa sign
    - pub_key - base64 encoded 2048-bit RSA public key
   Returns - new 256-bit digest calculated from sign
*/
checksum256 signidice(const checksum256& prev_digest, const std::string& sign, const std::string& rsa_key) {
    eosio::check(daobet::rsa_verify(prev_digest, sign, rsa_key), "invalid rsa signature");

    return eosio::sha256(sign.data(), sign.size());
}
```

 - Usage example:

```c++
// get new random from generator
auto new_random = get_prng(std::move(rand_seed))->next();
```


### Does the RNG process make use of a database? If so, please provide details.

 - RNG process doesn’t use any database. But game-protocols do. [DAOBet](https://github.com/DaoCasino/DAOBet).

### (Optional) If there is any additional documentation on your RNG available that would aid us in understanding its implementation to the game etc., it would be helpful.

 - Original: [signidice](https://github.com/gluk256/misc/blob/master/rng4ethereum/signidice.md)


## Code

### description

You can launch our tests to understand the processes generating a random number between the clients with signature verification and generation in a smart-contract.

### dependencies

 - docker  > 18.0

### clone repo and submodules
```bash
git clone https://github.com/DaoCasino/platform-prng
cd platform-prng
git submodule init
git submodule update --init --recursive
```

### build
```bash
./cicd/run build
```

### test
```bash
./cicd/run test
```

#### test setup&run

You can find available test parameters by calling:
```bash
./cicd/run test --help
```

Some examples:
- pass a particular initial seed:

  ```bash
  ./cicd/run test --seed 42
  ```

- set an upper bound of the generation range:

  ```bash
  # will generate numbers in range [0,1000)
  ./cicd/run test --range 1000
  ```

- define a number of iterations:

  ```bash
  # will generate 100 random numbers
  ./cicd/run test --count 100
  ```

- define columns amount:

  ```bash
  # will generate 3x3 numbers matrix
  ./cicd/run test --columns 3 --count 3
  ```

- generate raw 256-bit numbers:

  ```bash
  # will generate 10 256-bet numbers and print in dec format
  ./cicd/run test --count 10 --raw true
  ```

- write result to file:

  ```bash
  # will write numbers to `result.txt` file
  ./cicd/run test --out result.txt
  ```

- You can also combine all arguments to get a more preferable setup:
  ```bash
  # will generate 999 lines with seed 42 in the range [0,1000) and write results to `results.txt` file
  ./cicd/run test --count 999 --seed 42 --range 100 --columns 3 --out result.txt
  ```




