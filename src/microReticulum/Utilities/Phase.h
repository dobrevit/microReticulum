/*
 * Copyright (c) 2026 Dobrev IT Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 */

// ============================================================================
//  Phase.h — which call the stack is inside, and which one has ever been slow
//
//  A node was found with its Reticulum task alive but not getting round the
//  loop: the radio kept receiving and the ring filled, 2603 frames were
//  dropped against 39 taken, announces stopped, and the host's own
//  instrumentation could say only that the task was stuck *somewhere after the
//  interface loops*. Reticulum::loop() has four candidates after that point
//  and two of them touch flash, so "somewhere after" is not an answer.
//
//  This names the call. Every span records how many times it ran and the
//  longest it ever took, so a stall does not have to be caught in the act:
//  a node that has recovered still carries the worst pause it ever had, which
//  is what turns "it hung last night" into a line in a table.
//
//  It is always on. A span costs two stores and a compare — nothing against a
//  loop that does filesystem work — and a diagnostic that has to be switched
//  on is one that is off when the thing you needed it for happened.
//
//  Not re-entrant on purpose. These mark the top-level steps of one loop on
//  one task; nesting them would need a stack, and the question they answer
//  ("which of these four calls is the slow one") does not.
// ============================================================================
#pragma once

#include "OS.h"

#include <stddef.h>
#include <stdint.h>

namespace RNS { namespace Utilities {

struct PhaseStat {
	const char* _name = nullptr;
	uint32_t    _count = 0;         // times this span completed
	uint32_t    _max_ms = 0;        // the longest it ever took
	uint64_t    _total_ms = 0;      // and the sum, so an average is available
};

class Phase {

public:
	// More than the loop has steps, so a caller adding one does not silently
	// lose it. Spans are matched by pointer, so every name must be a string
	// literal — which they are, being written at the call site.
	static const size_t MAX_PHASES = 20;

	static void enter(const char* name) {
		_name = name;
		_since = OS::ltime();
	}

	static void leave() {
		if (_name == nullptr) return;
		const uint64_t now = OS::ltime();
		const uint64_t held = now > _since ? now - _since : 0;
		PhaseStat* stat = find_or_add(_name);
		if (stat != nullptr) {
			stat->_count++;
			stat->_total_ms += held;
			if (held > stat->_max_ms) stat->_max_ms = (uint32_t)held;
		}
		_name = nullptr;
		_since = now;
	}

	// What is running now, and for how long. A caller polling from another
	// task reads these to see a stall while it is happening; the table below
	// is what it reads afterwards.
	static const char* current() { return _name != nullptr ? _name : "idle"; }
	static uint32_t current_ms() {
		const uint64_t now = OS::ltime();
		return (uint32_t)(now > _since ? now - _since : 0);
	}

	static size_t count() { return _used; }
	static const PhaseStat& at(size_t i) { return _stats[i < _used ? i : 0]; }

	// Forget the maxima, so a run can be measured from a known point without
	// restarting the node.
	static void reset() {
		for (size_t i = 0; i < _used; i++) {
			_stats[i]._count = 0;
			_stats[i]._max_ms = 0;
			_stats[i]._total_ms = 0;
		}
	}

private:
	static PhaseStat* find_or_add(const char* name) {
		for (size_t i = 0; i < _used; i++)
			if (_stats[i]._name == name) return &_stats[i];
		if (_used >= MAX_PHASES) return nullptr;
		_stats[_used]._name = name;
		return &_stats[_used++];
	}

	static const char* _name;
	static uint64_t    _since;
	static PhaseStat   _stats[MAX_PHASES];
	static size_t      _used;

};

// Marks a span for as long as it is in scope, so an early return or a throw
// cannot leave the tracker claiming the node is still inside a call it left.
class PhaseScope {
public:
	explicit PhaseScope(const char* name) { Phase::enter(name); }
	~PhaseScope() { Phase::leave(); }
	PhaseScope(const PhaseScope&) = delete;
	PhaseScope& operator = (const PhaseScope&) = delete;
};

} }

#define RNS_PHASE(name) RNS::Utilities::PhaseScope _rns_phase_scope_(name)
