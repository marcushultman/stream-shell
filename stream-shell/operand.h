#pragma once

#include "closure.h"
#include "stream_parser.h"
#include "variant_ext.h"

using Operand = variant_ext_t<Value, Stream, StreamRef, Word>;

template <typename T>
concept IsValue = InVariant<Value, T>;
