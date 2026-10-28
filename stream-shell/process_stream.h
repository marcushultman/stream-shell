#pragma once

#include "process.h"
#include "stream_parser.h"

auto sink(Stream stream, int out, int err) -> std::unique_ptr<Process>;
auto unsink(Env &env, std::unique_ptr<Process> process, int in, int err) -> Stream;
