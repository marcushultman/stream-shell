#pragma once

#include "stream_parser.h"

using Prompt = std::function<const char *(const char *prompt)>;

void printStream(PrintableStream &&, const Prompt &);
