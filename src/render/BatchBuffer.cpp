//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "BatchBuffer.h"

namespace tri {

    BatchBuffer::BatchBuffer() {
        elementSize = 0;
        elementIndex = 0;
        updateIndex = 0;

        backElementIndex = 0;
        backUpdateIndex = 0;
        useBackBuffer = false;
        swapOnUpdate = false;
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
        if (useBackBuffer) {
            if (backElementIndex > backUpdateIndex) {
                buffer->setData(backData.data() + backUpdateIndex * elementSize, (backElementIndex - backUpdateIndex) * elementSize, backUpdateIndex * elementSize);
                backUpdateIndex = backElementIndex;
            }
            useBackBuffer = false;
            if (swapOnUpdate) {
                swapOnUpdate = false;
                swapBuffers();
            }
        }
        else {
            if (elementIndex > updateIndex) {
                buffer->setData(data.data() + updateIndex * elementSize, (elementIndex - updateIndex) * elementSize, updateIndex * elementSize);
                updateIndex = elementIndex;
            }
        }
    }

    void BatchBuffer::reset() {
        elementIndex = 0;
        updateIndex = 0;
    }

    void BatchBuffer::swapBuffers(){
        if (useBackBuffer) {
            swapOnUpdate = true;
        }
        else {
            data.swap(backData);
            std::swap(elementIndex, backElementIndex);
            std::swap(updateIndex, backUpdateIndex);
            elementIndex = 0;
            updateIndex = 0;
            useBackBuffer = true;
        }
    }

    void BatchBuffer::reserve(uint32_t elementCount) {
        data.reserve(elementCount * elementSize);
    }

    uint32_t BatchBuffer::size() {
        return elementIndex;
    }

}