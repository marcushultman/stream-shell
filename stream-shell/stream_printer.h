#pragma once

#include <expected>
#include <string>
#include "stream_parser.h"

void printStream(std::expected<PrintableStream, std::string> &&);
