// Copyright 2017 Yahoo Holdings. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

#include "analyzer.h"

namespace vbench {

/**
 * Dumps the textual representation of a request to standard
 * output. Intended for debugging purposes.
 **/
class RequestDumper : public Analyzer
{
private:
    [[maybe_unused]] Handler<Request> &_next;

public:
    RequestDumper(Handler<Request> &_next);
    void handle(Request::UP request) override;
    void report() override;
};

} // namespace vbench
