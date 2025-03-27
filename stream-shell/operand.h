#pragma once

#include "closure.h"
#include "stream_parser.h"
#include "variant_ext.h"

using ClosureValue = std::function<Result<Value>(const Closure &)>;
using Operand = variant_ext_t<Value, Stream, StreamRef, Word, ClosureValue>;
