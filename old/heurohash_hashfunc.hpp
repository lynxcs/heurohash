// Shamelessly stolen from https://thenumb.at/Hashtables/
// static constexpr uint64_t squirrel3 (uint64_t at) {
//     constexpr uint64_t BIT_NOISE1 = 0x9E3779B185EBCA87ULL;
//     constexpr uint64_t BIT_NOISE2 = 0xC2B2AE3D27D4EB4FULL;
//     constexpr uint64_t BIT_NOISE3 = 0x27D4EB2F165667C5ULL;
//     at *= BIT_NOISE1;
//     at ^= (at >> 8);
//     at += BIT_NOISE2;
//     at ^= (at << 8);
//     at *= BIT_NOISE3;
//     at ^= (at >> 8);
//     return at;
// }
