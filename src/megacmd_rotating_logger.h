/**
 * (c) 2013 by Mega Limited, Auckland, New Zealand
 *
 * This file is part of MEGAcmd.
 *
 * MEGAcmd is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * @copyright Simplified (2-clause) BSD License.
 *
 * You should have received a copy of the license along with this
 * program.
 */

#pragma once

#include <memory>
#include <vector>
#include <fstream>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <thread>

#include "megacmdlogger.h"

namespace megacmd {

template<size_t BlockSize>
class MessageBuffer final
{
public:
    class MemoryBlock final
    {
        size_t mSize;
        bool mMemoryGap;
        char mBuffer[BlockSize];
    public:
        MemoryBlock();

        bool canAppendData(size_t dataSize) const;
        bool isNearCapacity() const;
        bool hasMemoryGap() const;
        const char *getBuffer() const;

        void markMemoryGap();
        void appendData(const char *data, size_t size);

    };
    using MemoryBlockList = std::vector<MemoryBlock>;

public:
    MessageBuffer(size_t failSafeSize);

    void append(const char *data, size_t size);
    MemoryBlockList popMemoryBlockList(bool &initialMemoryGap);

    bool isEmpty() const;
    bool isNearLastBlockCapacity() const;
    bool reachedFailSafeSize() const;

private:
    const size_t mFailSafeSize;
    mutable std::mutex mListMutex;
    MemoryBlockList mList;
    bool mInitialMemoryGap;
};

class RotationEngine;
class CompressionEngine;

class RotatingFileManager final
{
public:
    enum class RotationType
    {
        Numbered,
        Timestamp
    };

    enum class CompressionType
    {
        None,
        Gzip
    };

    struct Config
    {
        size_t maxBaseFileSize;

        RotationType rotationType;
        std::chrono::seconds maxFileAge;
        int maxFilesToKeep;

        CompressionType compressionType;

        Config();
    };

public:
    RotatingFileManager(const mega::LocalPath &filePath, const Config &config = {});

    bool shouldRotateFiles(size_t fileSize) const;

    void cleanupFiles();
    void rotateFiles();

    std::string popErrors();

private:
    void initializeCompressionEngine();
    void initializeRotationEngine();

private:
    const Config mConfig;
    const mega::LocalPath mDirectory;
    const mega::LocalPath mBaseFilename;

    std::unique_ptr<CompressionEngine> mCompressionEngine;
    std::unique_ptr<RotationEngine> mRotationEngine;
};

class FileRotatingLoggedStream final : public LoggedStream
{
    mutable MessageBuffer<1024> mMessageBuffer;

    std::string mOutputFilePath;
    OUTFSTREAMTYPE mOutputFile;
    RotatingFileManager mFileManager;

    mutable std::mutex mWriteMutex;
    mutable std::condition_variable mWriteCV;
    bool mForceRenew;
    bool mExit;
    bool mFlush;
    std::chrono::seconds mFlushPeriod;
    std::chrono::steady_clock::time_point mNextFlushTime;

    std::thread mWriteThread;

private:
    bool shouldRenew() const;
    bool shouldExit() const;
    bool shouldFlush() const;
    void setForceRenew(bool forceRenew);

    void writeToBuffer(const char* msg, size_t size) const;

    void writeMessagesToFile();
    void flushToFile();
    bool waitForOutputFile();

    void mainLoop();

public:
    FileRotatingLoggedStream(const OUTSTRING &outputFilePath);
    ~FileRotatingLoggedStream();

    const LoggedStream &operator<<(const char &c) const override;
    const LoggedStream &operator<<(const char *str) const override;
    const LoggedStream &operator<<(std::string str) const override;
    const LoggedStream &operator<<(int v) const override;
    const LoggedStream &operator<<(unsigned int v) const override;
    const LoggedStream &operator<<(long long v) const override;
    const LoggedStream &operator<<(unsigned long v) const override;

    const LoggedStream &operator<<(std::ios_base)                      const override { return *this; }
    const LoggedStream &operator<<(std::ios_base*)                     const override { return *this; }
    const LoggedStream &operator<<(OUTSTREAMTYPE& (*)(OUTSTREAMTYPE&)) const override { return *this; }

#ifdef _WIN32
    const LoggedStream &operator<<(std::wstring v) const override;
#endif

    virtual void flush() override;
};

template<size_t BlockSize>
MessageBuffer<BlockSize>::MemoryBlock::MemoryBlock() :
    mSize(0),
    mMemoryGap(false)
{
}

template<size_t BlockSize>
bool MessageBuffer<BlockSize>::MemoryBlock::canAppendData(size_t dataSize) const
{
    // The last byte of buffer is reserved for '\0'
    return !mMemoryGap && mSize + dataSize < BlockSize;
}

template<size_t BlockSize>
bool MessageBuffer<BlockSize>::MemoryBlock::isNearCapacity() const
{
    return mSize + BlockSize/8 > BlockSize;
}

template<size_t BlockSize>
bool MessageBuffer<BlockSize>::MemoryBlock::hasMemoryGap() const
{
    return mMemoryGap;
}

template<size_t BlockSize>
const char *MessageBuffer<BlockSize>::MemoryBlock::getBuffer() const
{
    return mBuffer;
}

template<size_t BlockSize>
void MessageBuffer<BlockSize>::MemoryBlock::markMemoryGap()
{
    mMemoryGap = true;
}

template<size_t BlockSize>
void MessageBuffer<BlockSize>::MemoryBlock::appendData(const char *data, size_t size)
{
    assert(canAppendData(size));
    assert(data && size > 0);

    std::memcpy(&mBuffer[mSize], data, size);
    mSize += size;

    // Append null after the last character so the buffer
    // is properly treated as a C-string
    mBuffer[mSize] = '\0';
}

template<size_t BlockSize>
MessageBuffer<BlockSize>::MessageBuffer(size_t failSafeSize) :
    mFailSafeSize(failSafeSize),
    mInitialMemoryGap(false)
{
    assert(mFailSafeSize > 0);
    assert(mFailSafeSize > BlockSize);
}

template<size_t BlockSize>
void MessageBuffer<BlockSize>::append(const char *data, size_t size)
{
    std::lock_guard<std::mutex> lock(mListMutex);

    auto* lastBlock = mList.empty() ? nullptr : &mList.back();
    if (!lastBlock || !lastBlock->canAppendData(size))
    {
        try
        {
            mList.emplace_back();
        }
        catch (const std::bad_alloc&)
        {
            if (lastBlock)
            {
                // If there's a bad_alloc exception when adding to a vector, it remains unchanged
                // So we note that the existing last block has a gap in memory
                lastBlock->markMemoryGap();
            }
            else
            {
                // If there's not even a last block, then there is a memory gap at the start
                mInitialMemoryGap = true;
            }
            return;
        }

        lastBlock = &mList.back();
    }

    if (lastBlock->canAppendData(size))
    {
        lastBlock->appendData(data, size);
    }
}

template<size_t BlockSize>
typename MessageBuffer<BlockSize>::MemoryBlockList MessageBuffer<BlockSize>::popMemoryBlockList(bool &initialMemoryGap)
{
    std::lock_guard<std::mutex> lock(mListMutex);
    initialMemoryGap = mInitialMemoryGap;
    mInitialMemoryGap = false;
    return std::move(mList);
}

template<size_t BlockSize>
bool MessageBuffer<BlockSize>::isEmpty() const
{
    std::lock_guard<std::mutex> lock(mListMutex);
    return mList.empty();
}

template<size_t BlockSize>
bool MessageBuffer<BlockSize>::isNearLastBlockCapacity() const
{
    std::lock_guard<std::mutex> lock(mListMutex);
    return !mList.empty() && mList.back().isNearCapacity();
}

template<size_t BlockSize>
bool MessageBuffer<BlockSize>::reachedFailSafeSize() const
{
    std::lock_guard<std::mutex> lock(mListMutex);
    return mList.size() > mFailSafeSize / BlockSize;
}
}
