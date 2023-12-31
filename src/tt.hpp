#pragma once

// clang-format off

#include <cstring> // for memset()

namespace tt { // Transposition table

const u16 DEFAULT_SIZE_MB = 32;
const u8 INVALID_BOUND = 0, EXACT_BOUND = 1, LOWER_BOUND = 2, UPPER_BOUND = 3;
u8 age = 0;

struct Entry
{
    u64 zobristHash = 0;
    i16 score = 0;
    Move bestMove = MOVE_NONE;
    u8 depth = 0; 
    u8 boundAndAge = 0; // lowest 2 bits for bound, highest 6 bits for age

    inline i16 adjustedScore(int plyFromRoot)
    {
        if (score >= MIN_MATE_SCORE)
            return score - plyFromRoot;
        if (score <= -MIN_MATE_SCORE)
            return score + plyFromRoot;
        return score;
    }

    inline u8 getBound() {
         return boundAndAge & 0b0000'0011; 
    }

    inline void setBound(u8 newBound) { 
        boundAndAge = (boundAndAge & 0b1111'1100) | newBound; 
    }

    inline u8 getAge() { 
        return boundAndAge & 0b1111'1100;
    }

    inline void setAge(u8 newAge) { 
        boundAndAge &= 0b0000'0011;
        boundAndAge |= (newAge << 2);
    }

    inline void setBoundAndAge(u8 bound, u8 age) {
        boundAndAge = (age << 2) | bound;
    }

} __attribute__((packed)); 

std::vector<Entry> tt;

inline void resize(u16 sizeMB)
{
    tt.clear();
    u32 numEntries = sizeMB * 1024 * 1024 / sizeof(Entry);
    tt.resize(numEntries);
    std::cout << "TT size: " << sizeMB << " MB (" << numEntries << " entries)" << std::endl;
}

inline void reset()
{
    memset(tt.data(), 0, sizeof(Entry) * tt.size());
    age = 0;
}

inline std::pair<Entry*, bool> probe(u64 zobristHash, int depth, int plyFromRoot, i16 alpha, i16 beta)
{
    Entry *ttEntry = &(tt[zobristHash % tt.size()]);
    u8 ttEntryBound = ttEntry->getBound();

    bool shouldCutoff = plyFromRoot > 0 
                        && ttEntry->zobristHash == zobristHash
                        && ttEntry->depth >= depth 
                        && (ttEntryBound == EXACT_BOUND 
                        || (ttEntryBound == LOWER_BOUND && ttEntry->score >= beta) 
                        || (ttEntryBound == UPPER_BOUND && ttEntry->score <= alpha));

    return { ttEntry, shouldCutoff };
}

inline void store(Entry *ttEntry, u64 zobristHash, int depth, i16 score, Move bestMove, int plyFromRoot, i16 originalAlpha, i16 beta)
{
    u8 bound = EXACT_BOUND;
    if (score <= originalAlpha) 
        bound = UPPER_BOUND;
    else if (score >= beta) 
        bound = LOWER_BOUND;

    // replacement scheme
    if (ttEntry->zobristHash != 0  // always replace empty entries
    && bound != EXACT_BOUND        // always replace if new bound is exact
    && ttEntry->depth >= depth + 3 // keep entry if its depth is much higher
    && ttEntry->getAge() == age)   // always replace entries from previous searches ('go' commands)
        return;

    ttEntry->zobristHash = zobristHash;
    ttEntry->depth = depth;
    ttEntry->score = score;
    ttEntry->setBoundAndAge(bound, age);
    if (bestMove != MOVE_NONE) ttEntry->bestMove = bestMove;

    // Adjust mate scores based on ply
    if (ttEntry->score >= MIN_MATE_SCORE)
        ttEntry->score += plyFromRoot;
    else if (ttEntry->score <= -MIN_MATE_SCORE)
        ttEntry->score -= plyFromRoot;

}

}
