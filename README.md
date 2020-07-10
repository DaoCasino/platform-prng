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
 - Xoshiro RPNG range - ```[0, 2^64 - 1] (uint64_t size)```

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
    virtual uint64_t next() = 0;
};

/**
   Xoshiro256++ PRNG algo implementation
   details: http://prng.di.unimi.it/
*/
class Xoshiro : public PRNG {
  public:
    explicit Xoshiro(const checksum256& seed) : _s(split(seed)) {}
    explicit Xoshiro(checksum256&& seed) : _s(split(std::move(seed))) {}
    explicit Xoshiro(std::array<uint64_t, 4>&& seed) : _s(std::move(seed)) {}
    explicit Xoshiro(const std::array<uint64_t, 4>& seed) : _s(seed) {}

    uint64_t next() override {
        const uint64_t result = roll_up(_s[0] + _s[3], 23) + _s[0];

        const uint64_t t = _s[1] << 17;

        _s[2] ^= _s[0];
        _s[3] ^= _s[1];
        _s[1] ^= _s[2];
        _s[0] ^= _s[3];

        _s[2] ^= t;

        _s[3] = roll_up(_s[3], 45);

        return result;
    }

  private:
    static inline uint64_t roll_up(const uint64_t value, const int count) {
        return (value << count) | (value >> (64 - count));
    }

  private:
    std::array<uint64_t, 4> _s;
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
 - Xoshiro: [xoshiro](http://prng.di.unimi.it/)


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
  ./cicd/run test --columns 3 --count 9
  ```

- write result to file:

  ```bash
  # will write numbers to `result.txt` file
  ./cicd/run test --out result.txt
  ```

- You can also combine all arguments to get a more preferable setup:
  ```bash
  # will generate 999 numbers with seed 42 in the range [0,1000) and write results to `results.txt` file
  ./cicd/run test --count 999 --seed 42 --range 100 --columns 3 --out result.txt
  ```




