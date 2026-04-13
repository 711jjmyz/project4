#include "repl_policy.h"

// =========================================================
// TODO: Task 1 / Task 3 replacement policies
// Implement LRU first, then extend with SRRIP / BIP.
// =========================================================

void LRUPolicy::onHit(std::vector<CacheLine>& set, int way, uint64_t cycle) {
    // TODO: mark the hit line as most recently used.
    set[way].last_access = cycle;
}

void LRUPolicy::onMiss(std::vector<CacheLine>& set, int way, uint64_t cycle) {
    // TODO: initialize a newly inserted line as MRU.
    set[way].last_access = cycle;
}

int LRUPolicy::getVictim(std::vector<CacheLine>& set) {
    int victim = 0;
    for (int i = 1; i < (int)set.size(); i++) {
        if (set[i].last_access < set[victim].last_access) {
            victim = i;
        }
    }
    return victim;

}

void SRRIPPolicy::onHit(std::vector<CacheLine>& set, int way, uint64_t cycle) {
    set[way].rrpv = 0;
    (void)cycle;
    // we don't use cycle in SRRIP, because it don't need to check the newest line,
    // TODO: typically promote the line to RRPV=0.
}

void SRRIPPolicy::onMiss(std::vector<CacheLine>& set, int way, uint64_t cycle) {
    set[way].rrpv = 2;
    (void)cycle;
    // TODO: insert with a long re-reference interval, e.g. RRPV=2.
}

int SRRIPPolicy::getVictim(std::vector<CacheLine>& set) {
    while (true) {
        // find rrpv == 3 
        for (int i = 0; i < (int)set.size(); i++) {
            if (set[i].rrpv == 3) {
                return i;
            }
        }
        // if not find, all lines rppv++ and retry
        for (int i = 0; i < (int)set.size(); i++) {
            set[i].rrpv++;
        }
    }
    // TODO: search for RRPV==3, otherwise age all lines and retry.   
}

void BIPPolicy::onHit(std::vector<CacheLine>& set, int way, uint64_t cycle) {
     set[way].last_access = cycle; 
    // TODO: hits still become MRU.
}

void BIPPolicy::onMiss(std::vector<CacheLine>& set, int way, uint64_t cycle) {
    if (rand() % 32 == 0) {
        set[way].last_access = 0;      // ramdomly insert at oldest position 
    } else {
        set[way].last_access = cycle;  
    }
    // TODO: mostly insert at LRU position, but occasionally insert at MRU.
    // Hint: use insertion_counter and throttle.
}

int BIPPolicy::getVictim(std::vector<CacheLine>& set) {
        int victim = 0;
    for (int i = 1; i < (int)set.size(); i++) {
        if (set[i].last_access < set[victim].last_access) {
            victim = i;
        }
    }
    return victim;
}

ReplacementPolicy* createReplacementPolicy(std::string name) {
    if (name == "SRRIP") return new SRRIPPolicy();
    if (name == "BIP") return new BIPPolicy();
    return new LRUPolicy();
}
