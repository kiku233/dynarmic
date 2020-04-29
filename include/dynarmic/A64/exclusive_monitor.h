/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <vector>

namespace Dynarmic {
namespace A64 {

using VAddr = std::uint64_t;
using Vector = std::array<std::uint64_t, 2>;

class ExclusiveMonitor {
public:
    /// @param processor_count Maximum number of processors using this global
    ///                        exclusive monitor. Each processor must have a
    ///                        unique id.
    explicit ExclusiveMonitor(size_t processor_count);

    size_t GetProcessorCount() const;

    /// Marks a region containing [address, address+size) to be exclusive to
    /// processor processor_id.
    template <typename T, typename Function>
    T ReadAndMark(size_t processor_id, VAddr address, Function op) {
        static_assert(std::is_trivially_copyable_v<T>);

        Lock();

        const T value = op();
        State& s = state[processor_id];
        s.size = sizeof(T);
        s.address = address;
        s.value = {};
        std::memcpy(s.value.data(), &value, sizeof(T));

        Unlock();
        return value;
    }

    /// Checks to see if processor processor_id has exclusive access to the
    /// specified region. If it does, executes the operation then clears
    /// the exclusive state for processors if their exclusive region(s)
    /// contain [address, address+size).
    template <typename T, typename Function>
    bool DoExclusiveOperation(size_t processor_id, VAddr address, Function op) {
        static_assert(std::is_trivially_copyable_v<T>);

        Lock();
        if (!CheckAndClear(processor_id, address, sizeof(T))) {
            return false;
        }

        T value;
        std::memcpy(&value, state[processor_id].value.data(), sizeof(T));
        const bool result = op(value);

        Unlock();
        return result;
    }

    /// Unmark everything.
    void Clear();
    /// Unmark specific processor.
    void ClearProcessor(size_t processor_id);

private:
    bool CheckAndClear(size_t processor_id, VAddr address, size_t size);

    void Lock();
    void Unlock();

    struct State {
        size_t size = 0;
        VAddr address;
        Vector value;
    };

    std::atomic_flag is_locked;
    std::vector<State> state;
};

} // namespace A64
} // namespace Dynarmic
