#pragma once

#include "stream_parser.h"

using Prompt = std::function<const char *()>;

void printStream(PrintableStream &&, Prompt);
