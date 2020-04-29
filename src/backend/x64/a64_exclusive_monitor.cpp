/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include <algorithm>

#include <dynarmic/A64/exclusive_monitor.h>
#include "common/assert.h"

namespace Dynarmic {
namespace A64 {

ExclusiveMonitor::ExclusiveMonitor(size_t processor_count) : state(processor_count) {
    Unlock();
}

size_t ExclusiveMonitor::GetProcessorCount() const {
    return state.size();
}

bool ExclusiveMonitor::CheckAndClear(size_t processor_id, VAddr address, size_t size) {
    State& s = state[processor_id];
    if (s.address != address || s.size != size) {
        Unlock();
        return false;
    }

    for (State& other_s : state) {
        if (other_s.address == address) {
            other_s = {};
        }
    }
    return true;
}

void ExclusiveMonitor::Clear() {
    Lock();
    for (State& s : state) {
        s = {};
    }
    Unlock();
}

void ExclusiveMonitor::ClearProcessor(size_t processor_id) {
    Lock();
    state[processor_id] = {};
    Unlock();
}

void ExclusiveMonitor::Lock() {
    while (is_locked.test_and_set(std::memory_order_acquire)) {}
}

void ExclusiveMonitor::Unlock() {
    is_locked.clear(std::memory_order_release);
}

} // namespace A64
} // namespace Dynarmic
