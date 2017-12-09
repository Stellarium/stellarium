/*
 * Copyright (c) 2009, Interactive Data Corporation
 */

#pragma once

#include <vector>
#include <string>

std::vector<std::string> tokenize_str(const std::string &str, const std::string &delims = ", \t");
