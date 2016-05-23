//
//  Query.cpp
//  interface/src/gpu
//
//  Created by Niraj Venkat on 7/7/2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "Query.h"

#include "GPULogging.h"
#include "Batch.h"

using namespace gpu;

Query::Query(const Handler& returnHandler) :
    _returnHandler(returnHandler)
{
}

Query::~Query()
{
}

double Query::getElapsedTime() const {
    return ((double) _queryResult) * 0.000001;
}

void Query::triggerReturnHandler(uint64_t queryResult) {
    _queryResult = queryResult;
    if (_returnHandler) {
        _returnHandler(*this);
    }
}


RangeTimer::RangeTimer() {
    for (int i = 0; i < QUERY_QUEUE_SIZE; i++) {
        _timerQueries.push_back(std::make_shared<gpu::Query>([&, i] (const Query& query) {
            _tailIndex ++;
            auto elapsedTime = query.getElapsedTime();
        }));
    }
}

void RangeTimer::begin(gpu::Batch& batch) {
    _headIndex++;
    batch.beginQuery(_timerQueries[rangeIndex(_headIndex)]);
}
void RangeTimer::end(gpu::Batch& batch) {
    if (_headIndex < 0) {
        return;
    }
    batch.endQuery(_timerQueries[rangeIndex(_headIndex)]);
    
    if (_tailIndex < 0) {
        _tailIndex = _headIndex;
    }
    
    // Pull the previous tail query hopping to see it return
    if (_tailIndex != _headIndex) {
        batch.getQuery(_timerQueries[rangeIndex(_tailIndex)]);
    }
}

double RangeTimer::getAverage() const {
    return 0;
}