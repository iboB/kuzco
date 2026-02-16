# Copyright (c) Borislav Stanimirov
# SPDX-License-Identifier: MIT
#
add_executable(kuzco-example-SharedState e-SharedState.cpp)
target_link_libraries(kuzco-example-SharedState kuzco::kuzco)
