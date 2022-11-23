//
// Copyright (c) 2022 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "pch.h"
#include "core/Reflection.h"

namespace tri {

	class Archive {
	public:
		virtual void writeBytes(const void* ptr, int bytes) = 0;
		virtual void readBytes(void* ptr, int bytes) = 0;

		virtual void writeClass(const void* ptr, int classId);
		virtual void readClass(void* ptr, int classId);

		virtual void writeStr(const std::string& str);
		virtual void readStr(std::string &str);
		virtual std::string readStr();

		virtual bool hasDataLeft() { return false; }

		template<typename T>
		void write(const T& value) {
			writeClass(&value, Reflection::getClassId<T>());
		}

		template<typename T>
		void read(T& value) {
			readClass(&value, Reflection::getClassId<T>());
		}

		template<typename T>
		T read() {
			T value;
			read<T>(value);
			return value;
		}

		template<typename T>
		void writeBin(const T& value) {
			writeBytes(&value, sizeof(value));
		}

		template<typename T>
		void readBin(T& value) {
			readBytes(&value, sizeof(value));
		}

		template<typename T>
		T readBin() {
			T value;
			readBin<T>(value);
			return value;
		}
	};

	class MemoryArchive : public Archive {
	public:
		MemoryArchive();
		MemoryArchive(void *data, int bytes);

		void writeBytes(const void* ptr, int bytes) override;
		void readBytes(void* ptr, int bytes) override;
		bool hasDataLeft() override {
			return readIndex < dataSize;
		}


		void skip(int bytes);
		void unskip(int bytes);
		void reset();
		uint8_t* data();
		int size();
		void reserve(int bytes);
		void setData(void* data, int bytes);
		void clear();
		int getReadIndex();
		int getWriteIndex();

	private:
		std::vector<uint8_t> buffer;
		uint8_t* dataPtr;
		int dataSize;
		int readIndex;
		int writeIndex;
	};

	class FileArchive : public Archive {
	public:
		std::fstream stream;

		~FileArchive() {
			stream.close();
		}

		void writeBytes(const void* ptr, int bytes) override {
			stream.write((char*)ptr, bytes);
		}

		void readBytes(void* ptr, int bytes) override {
			stream.read((char*)ptr, bytes);
		}

		bool hasDataLeft() override {
			return !stream.eof();
		}

		bool openFileForRead(const std::string& file) {
			stream.open(file, std::ios_base::in);
			return stream.is_open();
		}

		bool openFileForWrite(const std::string& file) {
			stream.open(file, std::ios_base::out);
			return stream.is_open();
		}
	};

	class BinaryArchive : public Archive {
	public:
		Archive *base;

		BinaryArchive(Archive* base)
			: base(base) {}

		void writeBytes(const void* ptr, int bytes) override {
			base->writeBytes(ptr, bytes);
		}

		void readBytes(void* ptr, int bytes) override {
			base->readBytes(ptr, bytes);
		}

		bool hasDataLeft() override { 
			return base->hasDataLeft();
		}


		void writeClass(const void* ptr, int classId) override;
		void readClass(void* ptr, int classId) override;
	};

}
