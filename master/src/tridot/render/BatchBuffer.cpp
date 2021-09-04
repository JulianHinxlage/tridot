//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "BatchBuffer.h"

namespace tridot {

    BatchBuffer::BatchBuffer() {
        elementSize = 0;
        elementIndex = 0;
        updateIndex = 0;
    }

    void BatchBuffer::init(uint32_t elementSize, BufferType type) {
        this->elementSize = elementSize;
        elementIndex = 0;
        updateIndex = 0;
        buffer = Ref<Buffer>::make();
        buffer->init(nullptr, 0, elementSize, type, true);
    }

    void *BatchBuffer::next() {
        uint32_t target = (elementIndex + 1) * elementSize;
        if(data.size() < target){
            data.insert(data.end(), target - data.size(), 0);
        }
        return data.data() + elementIndex++ * elementSize;
    }

    void BatchBuffer::update() {
        if(elementIndex > updateIndex){
            buffer->bind();
            buffer->setData(data.data() + updateIndex * elementSize, (elementIndex - updateIndex) * elementSize, updateIndex * elementSize);
            updateIndex = elementIndex;
        }
    }

    void BatchBuffer::reset() {
        elementIndex = 0;
        updateIndex = 0;
    }

    void BatchBuffer::reserve(uint32_t elementCount) {
        data.reserve(elementCount * elementSize);
    }

    uint32_t BatchBuffer::size() {
        return elementIndex;
    }

}