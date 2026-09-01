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

#include "Phase.h"

using namespace RNS::Utilities;

/*static*/ const char* Phase::_name = nullptr;
/*static*/ uint32_t    Phase::_since = 0;
/*static*/ PhaseStat   Phase::_stats[Phase::MAX_PHASES];
/*static*/ size_t      Phase::_used = 0;
